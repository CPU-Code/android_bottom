/*
 * @Author: cpu_code
 * @Date: 2020-07-12 19:24:49
 * @LastEditTime: 2020-07-18 13:37:48
 * @FilePath: \Android系统源代码情景分析（第三版）程序文件\chapter-4\src\kernel\goldfish\drivers\staging\android\logger.h
 * @Gitee: https://gitee.com/cpu_code
 * @Github: https://github.com/CPU-Code
 * @CSDN: https://blog.csdn.net/qq_44226094
 * @Gitbook: https://923992029.gitbook.io/cpucode/
 */ 
/* include/linux/logger.h
 *
 * Copyright (C) 2007-2008 Google, Inc.
 * Author: Robert Love <rlove@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_LOGGER_H
#define _LINUX_LOGGER_H

#include <linux/types.h>
#include <linux/ioctl.h>


// 描述一个日志记录
struct logger_entry 
{
/* 实际的日志记录的有效负载长度 */
	__u16		len;	/* length of the payload */
	/* 将成员变量pid的地址值对齐到4字节边界的 */
	__u16		__pad;	/* no matter what, we get 2 bytes of padding */
	/* 写入该日志记录的进程的TGID  线程组ID */
	__s32		pid;	/* generating process's pid */
	/* 写入该日志记录的进程的PID 进程ID */
	__s32		tid;	/* generating process's tid */
	/* 写入该日志记录的时间 用标准基准时间（Epoch）来描述*/
	__s32		sec;	/* seconds since Epoch */
	/* 写入该日志记录的时间 */
	__s32		nsec;	/* nanoseconds */
	/* 实际写入的日志记录内容，长度是可变的，len决定 */
	char		msg[0];	/* the entry's payload */
};

#define LOGGER_LOG_RADIO	"log_radio"	/* radio-related messages */
#define LOGGER_LOG_EVENTS	"log_events"	/* system/hardware events */
#define LOGGER_LOG_MAIN		"log_main"	/* everything else */

// 每一个日志记录的最大长度 == 4K
#define LOGGER_ENTRY_MAX_LEN		(4*1024)
// 有效负载长度最大 == 4K - 结构体logger_entry的大小
#define LOGGER_ENTRY_MAX_PAYLOAD	\
	(LOGGER_ENTRY_MAX_LEN - sizeof(struct logger_entry))

#define __LOGGERIO	0xAE

#define LOGGER_GET_LOG_BUF_SIZE		_IO(__LOGGERIO, 1) /* size of log */
#define LOGGER_GET_LOG_LEN		_IO(__LOGGERIO, 2) /* used log len */
#define LOGGER_GET_NEXT_ENTRY_LEN	_IO(__LOGGERIO, 3) /* next entry len */
#define LOGGER_FLUSH_LOG		_IO(__LOGGERIO, 4) /* flush log */

#endif /* _LINUX_LOGGER_H */
