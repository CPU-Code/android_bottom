/*
 * @Author: cpu_code
 * @Date: 2020-07-22 20:56:57
 * @LastEditTime: 2020-07-23 19:32:06
 * @FilePath: \android_bottom\kernel\goldfish\drivers\freg\freg.h
 * @Gitee: https://gitee.com/cpu_code
 * @Github: https://github.com/CPU-Code
 * @CSDN: https://blog.csdn.net/qq_44226094
 * @Gitbook: https://923992029.gitbook.io/cpucode/
 */ 
#ifndef _FAKE_REG_H_
#define _FAKE_REG_H_

#include <linux/cdev.h>
#include <linux/semaphore.h>

// 虚拟硬件设备freg在设备文件系统中的名称
#define FREG_DEVICE_NODE_NAME  "freg"
#define FREG_DEVICE_FILE_NAME  "freg"
#define FREG_DEVICE_PROC_NAME  "freg"
#define FREG_DEVICE_CLASS_NAME "freg"

// 描述虚拟硬件设备freg
struct fake_reg_dev 
{
	// 描述一个虚拟寄存器
	int val;
	// 一个信号量， 用来同步访问虚拟寄存器val
	struct semaphore sem;
	// 一个标准的Linux字符设备结构体变量， 用来标志该虚拟硬件设备freg的类型为字符设备
	struct cdev dev;
};

#endif

