/*
 * @Author: cpu_code
 * @Date: 2020-07-22 20:56:57
 * @LastEditTime: 2020-07-23 21:42:33
 * @FilePath: \android_bottom\kernel\goldfish\drivers\freg\freg.c
 * @Gitee: https://gitee.com/cpu_code
 * @Github: https://github.com/CPU-Code
 * @CSDN: https://blog.csdn.net/qq_44226094
 * @Gitbook: https://923992029.gitbook.io/cpucode/
 */ 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>

#include "freg.h"

// 主设备号
static int freg_major = 0;
// 次设备号
static int freg_minor = 0;

// 设备类别
static struct class* freg_class = NULL;
// 设备变量
static struct fake_reg_dev* freg_dev = NULL;

// 传统的设备文件操作方法

// 打开虚拟硬件设备freg
static int freg_open(struct inode* inode, struct file* filp);
// 关闭虚拟硬件设备freg
static int freg_release(struct inode* inode, struct file* filp);
// 读取虚拟硬件设备freg中的寄存器val
static ssize_t freg_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos);
// 写入虚拟硬件设备freg中的寄存器val
static ssize_t freg_write(struct file* filp, const char __user *buf, size_t count, loff_t* f_pos);

// 传统的设备文件操作方法表
static struct file_operations freg_fops = {
        .owner = THIS_MODULE,
        .open = freg_open,
        .release = freg_release,
        .read = freg_read,
        .write = freg_write,
};

// devfs文件系统的设备属性操作方法
static ssize_t freg_val_show(struct device* dev, struct device_attribute* attr,  char* buf);
static ssize_t freg_val_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);

// devfs文件系统的设备属性
static DEVICE_ATTR(val, S_IRUGO | S_IWUSR, freg_val_show, freg_val_store);

// 打开设备方法
static int freg_open(struct inode* inode, struct file* filp) 
{
	struct fake_reg_dev* dev;
	
	//将自定义设备结构体保存在文件指针的私有数据中, 并访问设备时可直接使用
	dev = container_of(inode->i_cdev, struct fake_reg_dev, dev);
	filp->private_data = dev;

	return 0;
}

//设备文件释放, 空调用
static int freg_release(struct inode* inode, struct file* filp) 
{
	return 0;
}

// 读取设备的寄存器val的值
static ssize_t freg_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos) 
{
	ssize_t err = 0;
	struct fake_reg_dev* dev = filp->private_data;

	//同步访问
	if(down_interruptible(&(dev->sem))) 
	{	
		return -ERESTARTSYS;
	}

	if(count < sizeof(dev->val)) 
	{
		goto out;
	}

	//将寄存器val复制到用户提供的缓冲区buf中
	if(copy_to_user(buf, &(dev->val), sizeof(dev->val))) 
	{
		err = -EFAULT;
		goto out;
	}

	err = sizeof(dev->val);

out:
	up(&(dev->sem));
	return err;
}

// 写设备的寄存器val的值
static ssize_t freg_write(struct file* filp, const char __user *buf, size_t count, loff_t* f_pos) 
{
	struct fake_reg_dev* dev = filp->private_data;
	ssize_t err = 0;

	//同步访问
	if(down_interruptible(&(dev->sem))) 
	{
        return -ERESTARTSYS;
    }

    if(count != sizeof(dev->val)) 
	{
        goto out;
    }

	// 将用户提供的缓冲区buf的值写到设备寄存器val中
	if(copy_from_user(&(dev->val), buf, count)) 
	{
		err = -EFAULT;
		goto out;
	}

	err = sizeof(dev->val);

out:
	up(&(dev->sem));
	return err;
}

//将寄存器val的值读取到缓冲区buf中
static ssize_t __freg_get_val(struct fake_reg_dev* dev, char* buf) 
{
	int val = 0;

	//同步访问
	if(down_interruptible(&(dev->sem))) 
	{
        return -ERESTARTSYS;
    }

    val = dev->val;
    up(&(dev->sem));

    return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

// 把缓冲区buf的值写到设备寄存器val中
static ssize_t __freg_set_val(struct fake_reg_dev* dev, const char* buf, size_t count) 
{
	int val = 0;

	//将字符串转换成数字
    val = simple_strtol(buf, NULL, 10);

	//同步访问
    if(down_interruptible(&(dev->sem))) 
	{
        return -ERESTARTSYS;
    }

    dev->val = val;
    up(&(dev->sem));

	return count;
}

// 读取设备属性val的值
static ssize_t freg_val_show(struct device* dev, struct device_attribute* attr, char* buf) 
{
	struct fake_reg_dev* hdev = (struct fake_reg_dev*)dev_get_drvdata(dev);
	
    return __freg_get_val(hdev, buf);
}

// 写设备属性val的值
static ssize_t freg_val_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count) 
{
	 struct fake_reg_dev* hdev = (struct fake_reg_dev*)dev_get_drvdata(dev);

    return __freg_set_val(hdev, buf, count);
}

// 读取设备寄存器val的值,保存在page缓冲区中
static ssize_t freg_proc_read(char* page, char** start, off_t off, int count, int* eof, void* data) 
{
	if(off > 0) 
	{
		*eof = 1;

		return 0;
	}

	return __freg_get_val(freg_dev, page);
}

// 把缓冲区的值buff保存到设备寄存器val中
static ssize_t freg_proc_write(struct file* filp, const char __user *buff, unsigned long len, void* data) 
{	
	int err = 0;
	char* page = NULL;

	if(len > PAGE_SIZE) 
	{
		printk(KERN_ALERT"The buff is too large: %lu.\n", len);

		return -EFAULT;
	}

	page = (char*)__get_free_page(GFP_KERNEL);
	if(!page) 
	{
        printk(KERN_ALERT"Failed to alloc page.\n");
		
		return -ENOMEM;
	}
	
	//把用户提供的缓冲区的值复制到内核缓冲区中
	if(copy_from_user(page, buff, len)) 
	{
		printk(KERN_ALERT"Failed to copy buff from user.\n");

        err = -EFAULT;

		goto out;
	}

	err = __freg_set_val(freg_dev, page, len);

out:
	free_page((unsigned long)page);
	
	return err;	
}

//创建 /proc/freg文件
static void freg_create_proc(void) 
{
	struct proc_dir_entry* entry;
	
	entry = create_proc_entry(FREG_DEVICE_PROC_NAME, 0, NULL);
	if(entry) 
	{
		entry->owner = THIS_MODULE;
		entry->read_proc = freg_proc_read;
		entry->write_proc = freg_proc_write;
	}
}

// 删除 /proc/freg文件
static void freg_remove_proc(void) 
{
	remove_proc_entry(FREG_DEVICE_PROC_NAME, NULL);
}

// 初始化设备
static int  __freg_setup_dev(struct fake_reg_dev* dev) 
{
	int err;
	dev_t devno = MKDEV(freg_major, freg_minor);

	memset(dev, 0, sizeof(struct fake_reg_dev));

	//初始化字符设备
	cdev_init(&(dev->dev), &freg_fops);
	dev->dev.owner = THIS_MODULE;
	dev->dev.ops = &freg_fops;

	//注册字符设备
	err = cdev_add(&(dev->dev),devno, 1);
	if(err) 
	{
		return err;
	}	

	//初始化信号量和寄存器val的值
	init_MUTEX(&(dev->sem));
	dev->val = 0;

	return 0;
}

//模块加载方法
static int __init freg_init(void) 
{ 
	int err = -1;
	dev_t dev = 0;
	struct device* temp = NULL;

	printk(KERN_ALERT"Initializing freg device.\n");

	//动态分配主设备号 和 从设备号
	err = alloc_chrdev_region(&dev, 0, 1, FREG_DEVICE_NODE_NAME);
	if(err < 0) 
	{
		printk(KERN_ALERT"Failed to alloc char dev region.\n");
		goto fail;
	}

	freg_major = MAJOR(dev);
	freg_minor = MINOR(dev);

	//分配freg设备结构体
	freg_dev = kmalloc(sizeof(struct fake_reg_dev), GFP_KERNEL);
	if(!freg_dev) 
	{
		err = -ENOMEM;
		printk(KERN_ALERT"Failed to alloc freg device.\n");
		goto unregister;
	}

	//初始化设备
	err = __freg_setup_dev(freg_dev);
	if(err) 
	{
		printk(KERN_ALERT"Failed to setup freg device: %d.\n", err);
		goto cleanup;
	}

	//在 /sys/class/ 目录下创建设备级别目录ferg
	freg_class = class_create(THIS_MODULE, FREG_DEVICE_CLASS_NAME);
	if(IS_ERR(freg_class)) 
	{
		err = PTR_ERR(freg_class);
		printk(KERN_ALERT"Failed to create freg device class.\n");
		
		goto destroy_cdev;
	}

	//在 /dev目录 和 /sys/class/freg 目录下分别创建设备文件freg
	temp = device_create(freg_class, NULL, dev, "%s", FREG_DEVICE_FILE_NAME);
	if(IS_ERR(temp)) 
	{
		err = PTR_ERR(temp);
		printk(KERN_ALERT"Failed to create freg device.\n");

		goto destroy_class;
	}

	// 在/sys/class/freg/freg目录下创建属性文件val
	err = device_create_file(temp, &dev_attr_val);
	if(err < 0) 
	{
		printk(KERN_ALERT"Failed to create attribute val of freg device.\n");

        goto destroy_device;
	}

	dev_set_drvdata(temp, freg_dev);

	//创建/proc/freg文件
	freg_create_proc();

	printk(KERN_ALERT"Succedded to initialize freg device.\n");

	return 0;

destroy_device:
	device_destroy(freg_class, dev);
destroy_class:
	class_destroy(freg_class);
destroy_cdev:
	cdev_del(&(freg_dev->dev));	
cleanup:
	kfree(freg_dev);
unregister:
	unregister_chrdev_region(MKDEV(freg_major, freg_minor), 1);	
fail:
	return err;
}

//模块卸载方法
static void __exit freg_exit(void) 
{
	dev_t devno = MKDEV(freg_major, freg_minor);

	printk(KERN_ALERT"Destroy freg device.\n");
	
	// 删除/proc/freg文件
	freg_remove_proc();

	if(freg_class) 
	{
		//注销设备类别
		device_destroy(freg_class, MKDEV(freg_major, freg_minor));
		//注销设备
		class_destroy(freg_class);
	}

	if(freg_dev) 
	{
		//删除字符设备
		cdev_del(&(freg_dev->dev));
		//释放设备内存
		kfree(freg_dev);
	}

	//释放设备号资源
	unregister_chrdev_region(devno, 1);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Fake Register Driver");

module_init(freg_init);
module_exit(freg_exit);

