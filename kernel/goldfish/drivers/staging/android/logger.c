/*
 * @Author: cpu_code
 * @Date: 2020-07-12 19:24:49
 * @LastEditTime: 2020-07-19 14:03:27
 * @FilePath: \Android系统源代码情景分析（第三版）程序文件\chapter-4\src\kernel\goldfish\drivers\staging\android\logger.c
 * @Gitee: https://gitee.com/cpu_code
 * @Github: https://github.com/CPU-Code
 * @CSDN: https://blog.csdn.net/qq_44226094
 * @Gitbook: https://923992029.gitbook.io/cpucode/
 */ 
/*
 * drivers/misc/logger.c
 *
 * A Logging Subsystem
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * Robert Love <rlove@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/time.h>
#include "logger.h"

#include <asm/ioctls.h>

/*
 * struct logger_log - represents a specific log, such as 'main' or 'radio'
 *
 * This structure lives from module insertion until module removal, so it does
 * not need additional reference counting. The structure is protected by the
 * mutex 'mutex'.
 */
// 描述一个日志缓冲区
struct logger_log 
{
	//指向一个内核缓冲区， 用来保存日志记录， 它是循环使用的
	unsigned char *		buffer;	/* the ring buffer itself */
	// 描述一个日志设备
	struct miscdevice	misc;	/* misc device representing the log */
	// 等待队列，用来记录那些正在等待读取新的日志记录的进程, 每一个进程都使用一个结构体logger_reader来描述
	wait_queue_head_t	wq;	/* wait queue for readers */
	// 列表， 用来记录那些正在读取日志记录的进程
	struct list_head	readers; /* this log's readers */
	// 互斥量， 用来保护日志缓冲区的并发访问
	struct mutex		mutex;	/* mutex protecting buffer */
	// 下一条要写入的日志记录在日志缓冲区中的位置
	size_t			w_off;	/* current write head offset */
	// 当一个新的进程读取日志时，会从日志缓冲区中的什么位置开始读取
	size_t			head;	/* new readers start here */
	// 大小
	size_t			size;	/* size of the log */
};

/*
 * struct logger_reader - a logging device open for reading
 *
 * This object lives from open to release, so we don't need additional
 * reference counting. The structure is protected by log->mutex.
 */
// 描述一个正在读取某一个日志缓冲区的日志记录的进程
struct logger_reader 
{
	// 指向要读取的日志缓冲区结构体
	struct logger_log *	log;	/* associated log */
	// 一个列表项， 用来连接所有读取同一种类型日志记录的进程
	struct list_head	list;	/* entry in logger_log's list */
	// 当前进程要读取的下一条日志记录在日 志缓冲区中的位置
	size_t			r_off;	/* current read head offset */
};

/* logger_offset - returns index 'n' into the log via (optimized) modulus */
// 日志缓冲区的长度是2的N 次方， 因此，只要它的值减1之后，
// 再与参数n执行按位与运算，就可以得到参数n在日志缓冲区中的正确位置
#define logger_offset(n)	((n) & (log->size - 1))

/*
 * file_get_log - Given a file structure, return the associated log
 *
 * This isn't aesthetic. We have several goals:
 *
 * 	1) Need to quickly obtain the associated log during an I/O operation
 * 	2) Readers need to maintain state (logger_reader)
 * 	3) Writers need to be very fast (open() should be a near no-op)
 *
 * In the reader case, we can trivially go file->logger_reader->logger_log.
 * For a writer, we don't want to maintain a logger_reader, so we just go
 * file->logger_log. Thus what file->private_data points at depends on whether
 * or not the file was opened for reading. This function hides that dirtiness.
 */
static inline struct logger_log * file_get_log(struct file *file)
{
	if (file->f_mode & FMODE_READ) {
		struct logger_reader *reader = file->private_data;
		return reader->log;
	} else
		return file->private_data;
}

/*
 * get_entry_len - Grabs the length of the payload of the next entry starting
 * from 'off'.
 *
 * Caller needs to hold log->mutex.
 */
static __u32 get_entry_len(struct logger_log *log, size_t off)
{
	__u16 val;

	// 将日志缓冲区结构体log的总长度size - 下一条要读取的日志记录的位置off
	switch (log->size - off) 
	{
		case 1:
			// 该日志记录的长度值分别保存在日志缓冲区的首尾字节中

			// 将它的前1个字节从日志缓冲区的首字节中读取出来，并将它的内容保存在变量vals
			memcpy(&val, log->buffer + off, 1);
			// 将它的后1个字节从日志缓冲区的尾字节中读取出来，并将它的内容保存在变量vals + 1
			memcpy(((char *) &val) + 1, log->buffer, 1);

			break;

		default:
			// 该日志记录的长度值保存在两个连在一起的字节中， 即保存在地址log-＞buffer+off中

			// 将日志记录的长度值读取出来， 并且保存在变量val中
			memcpy(&val, log->buffer + off, 2);
	}

	// 将变量val的值 + 结构体logger_entry的大小， 就得到下一条要读取的日志记录的总长度	
	return sizeof(struct logger_entry) + val;
}

/*
 * do_read_log_to_user - reads exactly 'count' bytes from 'log' into the
 * user-space buffer 'buf'. Returns 'count' on success.
 *
 * Caller must hold log->mutex.
 */
static ssize_t do_read_log_to_user(struct logger_log *log,
				   struct logger_reader *reader,
				   char __user *buf,
				   size_t count)
{
	size_t len;

	/*
	 * We read from the log in two disjoint operations. First, we read from
	 * the current read head offset up to 'count' bytes or to the end of
	 * the log, whichever comes first.
	 */
	// 计算第一次要读取的日志记录的长度
	len = min(count, log->size - reader->r_off);
	// 将这一部分的日志记录复制到用户空间缓冲区buf中
	if (copy_to_user(buf, log->buffer + reader->r_off, len))
		return -EFAULT;

	/*
	 * Second, we read any remaining bytes, starting back at the head of
	 * the log.
	 */
	// 检查上一次是否已经将日志记录的内容读取完毕
	if (count != len)
	{
		// 未读取完毕

		// 继续将第二部分的日志记录复制到用户空间缓冲区buf中，就完成了一条日志记录的读取
		if (copy_to_user(buf + len, log->buffer, count - len))
			return -EFAULT;
	}

	// 当前进程从日志缓冲区结构体log中读取了一条日志记录之后，就要修改它的成员变量r_off的值，
	// 表示下次要读取的是日志缓冲区结构体log中的下一条日志记录
	
	// 将日志读取进程结构体reader的成员变量r_off的值 + 前面已经读取的日志记录的长度count ，
	// 然后再使用宏logger_offset来对计算结果进行调整
	reader->r_off = logger_offset(reader->r_off + count);

	return count;
}

/*
 * logger_read - our log's read() method
 *
 * Behavior:
 *
 * 	- O_NONBLOCK works
 * 	- If there are no log entries to read, blocks until log is written to
 * 	- Atomically reads exactly one log entry
 *
 * Optimal read size is LOGGER_ENTRY_MAX_LEN. Will set errno to EINVAL if read
 * buffer is insufficient to hold next entry.
 */
static ssize_t logger_read(struct file *file, char __user *buf,
			   size_t count, loff_t *pos)
{
	// 得到日志记录读取进程结构体reader
	struct logger_reader *reader = file->private_data;
	// 得到日志缓冲区结构体 log
	struct logger_log *log = reader->log;
	ssize_t ret;
	DEFINE_WAIT(wait);

start:
	while (1) 
	{
		// 循环 检查日志缓冲区结构体 log 中是否有日志记录可读取

		// 将等待队列项wait加入日志记录读取进程的等待队列log-＞wq中
		prepare_to_wait(&log->wq, &wait, TASK_INTERRUPTIBLE);

		mutex_lock(&log->mutex);

		// w_off用来描述下一条新写入的日志记录在日志缓冲区中的位置
		// r_off用来描述当前进程下一条要读取的日志记录在日志缓冲区中的位置

		// 两者相等时，说明日志缓冲区结构体log中没有新的日志记录可读
		ret = (log->w_off == reader->r_off);

		mutex_unlock(&log->mutex);
		
		// ret == false，日志缓冲区结构体log中有新的日志记录可读
		if (!ret)
		{
			// 跳出循环
			break;
		}
			
		// 当前进程是否 以非阻塞模式来打开日志设备文件
		if (file->f_flags & O_NONBLOCK) 
		{
			// 当前进程就不会因为日志缓冲区结构体log中没有日志记录可读而进入睡眠状态，
			//它直接返回到用户空间中
			ret = -EAGAIN;
			
			break;
		}

		// 检查当前进程是否有信号正在等待处理
		if (signal_pending(current)) 
		{
			// 不能使当前进程进入睡眠状态， 而必须要让它结束当前系统调用， 
			//以便它可以立即返回去处理那些待处理信号
			ret = -EINTR;

			break;
		}

		// 请求内核进行一次进程调度， 从而进入睡眠状态的
		schedule();
	}

	// 将等待队列项wait从日志记录读取进程的等待队列log-＞wq中移除
	finish_wait(&log->wq, &wait);
	if (ret)
		return ret;

	// 获取互斥量log-＞mutex时， 可能会失败。 
	// 在这种情况下， 当前进程就会进入睡眠状态， 直到成功获取互斥量log-＞mutex为止
	// 在当前进程的睡眠过程中， 日志缓冲区结构体log中的日志记录可能已经被其他进程访问过了，
	// 所以 log-＞w_off的值可能已经发生了变化
	mutex_lock(&log->mutex);

	/* is there still something to read or did we race? */
	// 重新判断日志缓冲区结构体 w_off 和日志读取进程结构体 r_off 是否相等
	if (unlikely(log->w_off == reader->r_off)) 
	{
		mutex_unlock(&log->mutex);
		goto start;
	}

	/* get the size of the next entry */
	// 得到下一条要读取的日志记录的长度
	ret = get_entry_len(log, reader->r_off);
	if (count < ret) 
	{
		ret = -EINVAL;
		goto out;
	}

	/* get exactly one entry from the log */
	// 执行真正的日志记录读取操作
	ret = do_read_log_to_user(log, reader, buf, ret);

out:
	mutex_unlock(&log->mutex);

	return ret;
}

/*
 * get_next_entry - return the offset of the first valid entry at least 'len'
 * bytes after 'off'.
 *
 * Caller must hold log->mutex.
 */
static size_t get_next_entry(struct logger_log *log, size_t off, size_t len)
{
	size_t count = 0;

	do {
		size_t nr = get_entry_len(log, off);
		off = logger_offset(off + nr);
		count += nr;
	} while (count < len);

	return off;
}

/*
 * clock_interval - is a < c < b in mod-space? Put another way, does the line
 * from a to b cross c?
 */
static inline int clock_interval(size_t a, size_t b, size_t c)
{
	if (b < a) {
		if (a < c || b >= c)
			return 1;
	} else {
		if (a < c && b >= c)
			return 1;
	}

	return 0;
}

/*
 * fix_up_readers - walk the list of all readers and "fix up" any who were
 * lapped by the writer; also do the same for the default "start head".
 * We do this by "pulling forward" the readers and start head to the first
 * entry after the new write head.
 *
 * The caller needs to hold log->mutex.
 */
static void fix_up_readers(struct logger_log *log, size_t len)
{
	size_t old = log->w_off;
	size_t new = logger_offset(old + len);
	struct logger_reader *reader;

	if (clock_interval(old, new, log->head))
		log->head = get_next_entry(log, log->head, len);

	list_for_each_entry(reader, &log->readers, list)
		if (clock_interval(old, new, reader->r_off))
			reader->r_off = get_next_entry(log, reader->r_off, len);
}

/*
 * do_write_log - writes 'len' bytes from 'buf' to 'log'
 *
 * The caller needs to hold log->mutex.
 */
static void do_write_log(struct logger_log *log, const void *buf, size_t count)
{
	size_t len;

	len = min(count, log->size - log->w_off);
	memcpy(log->buffer + log->w_off, buf, len);

	if (count != len)
		memcpy(log->buffer, buf + len, count - len);

	log->w_off = logger_offset(log->w_off + count);

}

/*
 * do_write_log_user - writes 'len' bytes from the user-space buffer 'buf' to
 * the log 'log'
 *
 * The caller needs to hold log->mutex.
 *
 * Returns 'count' on success, negative error code on failure.
 */
static ssize_t do_write_log_from_user(struct logger_log *log,
				      const void __user *buf, size_t count)
{
	size_t len;

	len = min(count, log->size - log->w_off);
	if (len && copy_from_user(log->buffer + log->w_off, buf, len))
		return -EFAULT;

	if (count != len)
		if (copy_from_user(log->buffer, buf + len, count - len))
			return -EFAULT;

	log->w_off = logger_offset(log->w_off + count);

	return count;
}

/*
 * logger_aio_write - our write method, implementing support for write(),
 * writev(), and aio_write(). Writes are our fast path, and we try to optimize
 * them above all else.
 */
/**
 * @function: 
 * @parameter: 
 * 		iocb : 一个I/O上下文
 * 		iov : 保存了要写入的日志记录的内容，数组向量
 * 		nr_segs : 长度
 * 		ppos : 指定日志记录的写入位置
 * @return: 
 *     success: 
 *     error: 
 * @note: 
 */
ssize_t logger_aio_write(struct kiocb *iocb, const struct iovec *iov,
			 unsigned long nr_segs, loff_t ppos)
{
	// 将对应的日志缓冲区结构体log获取回来
	struct logger_log *log = file_get_log(iocb->ki_filp);
	size_t orig = log->w_off;

	// 定义一个日志记录结构体header
	struct logger_entry header;
	struct timespec now;
	ssize_t ret = 0;

	now = current_kernel_time();

	// 记录写入进程的TGID
	header.pid = current->tgid;
	// 写入进程的PID
	header.tid = current->pid;
	// 写入时间
	header.sec = now.tv_sec;
	header.nsec = now.tv_nsec;
	// 初始化日志记录的长度
	// 最大长度为LOGGER_ENTRY_MAX_PAYLOAD
	header.len = min_t(size_t, iocb->ki_left, LOGGER_ENTRY_MAX_PAYLOAD);


	/* null writes succeed, return zero */
	if (unlikely(!header.len))
		return 0;

	mutex_lock(&log->mutex);

	/*
	 * Fix up any readers, pulling them forward to the first readable
	 * entry after (what will be) the new write offset. We do this now
	 * because if we partially fail, we can end up with clobbered log
	 * entries that encroach on readable buffer.
	 */
	// 修正日志记录读取进程的读取位置的工作
	fix_up_readers(log, sizeof(struct logger_entry) + header.len);

	// 将它的内容写入到日志缓冲区结构体log中
	do_write_log(log, &header, sizeof(struct logger_entry));

	while (nr_segs-- > 0) 
	{
		size_t len;
		ssize_t nr;

		/* figure out how much of this vector we can keep */
		len = min_t(size_t, iov->iov_len, header.len - ret);

		/* write out this segment's payload */
		nr = do_write_log_from_user(log, iov->iov_base, len);
		if (unlikely(nr < 0)) {
			log->w_off = orig;
			mutex_unlock(&log->mutex);
			return nr;
		}

		iov++;
		ret += nr;
	}

	mutex_unlock(&log->mutex);

	/* wake up any blocked readers */
	wake_up_interruptible(&log->wq);

	return ret;
}

static struct logger_log * get_log_from_minor(int);

/*
 * logger_open - the log's open() file operation
 *
 * Note how near a no-op this is in the write-only case. Keep it that way!
 */
static int logger_open(struct inode *inode, struct file *file)
{
	struct logger_log *log;
	int ret;

	// 将相应的设备文件设置为不可随机访问， 就是 不可以用lseek来设置设备文件的当前读写位置， 
	//因为日志记录的读取或者写入都必须按顺序来进行。
	ret = nonseekable_open(inode, file);
	if (ret)
		return ret;

	// 根据从设备号获得要操作的日志缓冲区结构体。
	log = get_log_from_minor(MINOR(inode->i_rdev));
	if (!log)
		return -ENODEV;

	if (file->f_mode & FMODE_READ) 
	{
		// 以读模式打开日志设备的情况

		// 为当前进程创建一个logger_reader结构体reader
		struct logger_reader *reader;

		// 初始化
		reader = kmalloc(sizeof(struct logger_reader), GFP_KERNEL);
		if (!reader)
			return -ENOMEM;

		// 获得的日志缓冲区结构体log保存在结构体reader的成员变量log中
		reader->log = log;
		INIT_LIST_HEAD(&reader->list);

		mutex_lock(&log->mutex);
		
		// 该进程从log-＞head位置开始读取日志记录
		reader->r_off = log->head;
		list_add_tail(&reader->list, &log->readers);

		mutex_unlock(&log->mutex);

		// 把结构体reader保存在参数file的成员变量private_data中
		file->private_data = reader;
	} 
	else
	{
		// 以写模式打开日志设备的情况

		// 把前面获得的日志缓冲区结构体log保存在参数file的成员变量private_data中
		file->private_data = log;
	}

	return 0;
}

/*
 * logger_release - the log's release file operation
 *
 * Note this is a total no-op in the write-only case. Keep it that way!
 */
static int logger_release(struct inode *ignored, struct file *file)
{
	if (file->f_mode & FMODE_READ) {
		struct logger_reader *reader = file->private_data;
		list_del(&reader->list);
		kfree(reader);
	}

	return 0;
}

/*
 * logger_poll - the log's poll file operation, for poll/select/epoll
 *
 * Note we always return POLLOUT, because you can always write() to the log.
 * Note also that, strictly speaking, a return value of POLLIN does not
 * guarantee that the log is readable without blocking, as there is a small
 * chance that the writer can lap the reader in the interim between poll()
 * returning and the read() request.
 */
static unsigned int logger_poll(struct file *file, poll_table *wait)
{
	struct logger_reader *reader;
	struct logger_log *log;
	unsigned int ret = POLLOUT | POLLWRNORM;

	if (!(file->f_mode & FMODE_READ))
		return ret;

	reader = file->private_data;
	log = reader->log;

	poll_wait(file, &log->wq, wait);

	mutex_lock(&log->mutex);
	if (log->w_off != reader->r_off)
		ret |= POLLIN | POLLRDNORM;
	mutex_unlock(&log->mutex);

	return ret;
}

static long logger_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct logger_log *log = file_get_log(file);
	struct logger_reader *reader;
	long ret = -ENOTTY;

	mutex_lock(&log->mutex);

	switch (cmd) {
	case LOGGER_GET_LOG_BUF_SIZE:
		ret = log->size;
		break;
	case LOGGER_GET_LOG_LEN:
		if (!(file->f_mode & FMODE_READ)) {
			ret = -EBADF;
			break;
		}
		reader = file->private_data;
		if (log->w_off >= reader->r_off)
			ret = log->w_off - reader->r_off;
		else
			ret = (log->size - reader->r_off) + log->w_off;
		break;
	case LOGGER_GET_NEXT_ENTRY_LEN:
		if (!(file->f_mode & FMODE_READ)) {
			ret = -EBADF;
			break;
		}
		reader = file->private_data;
		if (log->w_off != reader->r_off)
			ret = get_entry_len(log, reader->r_off);
		else
			ret = 0;
		break;
	case LOGGER_FLUSH_LOG:
		if (!(file->f_mode & FMODE_WRITE)) {
			ret = -EBADF;
			break;
		}
		list_for_each_entry(reader, &log->readers, list)
			reader->r_off = log->w_off;
		log->head = log->w_off;
		ret = 0;
		break;
	}

	mutex_unlock(&log->mutex);

	return ret;
}

static struct file_operations logger_fops = {
	.owner = THIS_MODULE,
	.read = logger_read,
	.aio_write = logger_aio_write,
	.poll = logger_poll,
	.unlocked_ioctl = logger_ioctl,
	.compat_ioctl = logger_ioctl,
	.open = logger_open,
	.release = logger_release,
};

/*
 * Defines a log structure with name 'NAME' and a size of 'SIZE' bytes, which
 * must be a power of two, greater than LOGGER_ENTRY_MAX_LEN, and less than
 * LONG_MAX minus LOGGER_ENTRY_MAX_LEN.
 */
#define DEFINE_LOGGER_DEVICE(VAR, NAME, SIZE) \
// 将各自的日志记录保存在一个静态分配的unsigned char数组
// 大小由SIZE决定
static unsigned char _buf_ ## VAR[SIZE]; \
static struct logger_log VAR = { \
	.buffer = _buf_ ## VAR, \
	.misc = { \
		.minor = MISC_DYNAMIC_MINOR, \
		.name = NAME, \
		.fops = &logger_fops, \
		.parent = NULL, \
	}, \
	// 等待队列
	.wq = __WAIT_QUEUE_HEAD_INITIALIZER(VAR .wq), \
	// 队列
	.readers = LIST_HEAD_INIT(VAR .readers), \
	// 互斥量
	.mutex = __MUTEX_INITIALIZER(VAR .mutex), \
	.w_off = 0, \
	.head = 0, \
	.size = SIZE, \
};

// 宏DEFINE_LOGGER_DEVICE来定义的
// 传入参数分别为变量名、 设备文件名，分配的日志缓冲区的大小
// system和 main 的日志记录保存在同一个日志缓冲区 log_main 中 大小为64K
// events的日志记录保存在日志缓冲区 log_events 大小为256K
// radio的日志记录保存在日志缓冲区 log_radio 大小为64K
DEFINE_LOGGER_DEVICE(log_main, LOGGER_LOG_MAIN, 64*1024)
DEFINE_LOGGER_DEVICE(log_events, LOGGER_LOG_EVENTS, 256*1024)
DEFINE_LOGGER_DEVICE(log_radio, LOGGER_LOG_RADIO, 64*1024)

static struct logger_log * get_log_from_minor(int minor)
{
	if (log_main.misc.minor == minor)
		return &log_main;
	if (log_events.misc.minor == minor)
		return &log_events;
	if (log_radio.misc.minor == minor)
		return &log_radio;
	return NULL;
}

static int __init init_log(struct logger_log *log)
{
	int ret;

	//将相应的日志设备注册到系统
	ret = misc_register(&log->misc);
	if (unlikely(ret)) {
		printk(KERN_ERR "logger: failed to register misc "
		       "device for log '%s'!\n", log->misc.name);
		return ret;
	}

	printk(KERN_INFO "logger: created %luK log '%s'\n",
	       (unsigned long) log->size >> 10, log->misc.name);

	return 0;
}

static int __init logger_init(void)
{
	int ret;
	// 将三个日志设备注册到系统

	ret = init_log(&log_main);
	if (unlikely(ret))
		goto out;

	ret = init_log(&log_events);
	if (unlikely(ret))
		goto out;

	ret = init_log(&log_radio);
	if (unlikely(ret))
		goto out;

out:
	return ret;
}
device_initcall(logger_init);
