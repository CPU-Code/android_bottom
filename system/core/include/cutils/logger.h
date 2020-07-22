/*
 * @Author: cpu_code
 * @Date: 2020-07-12 19:24:49
 * @LastEditTime: 2020-07-20 15:50:21
 * @FilePath: \Android系统源代码情景分析（第三版）程序文件\chapter-4\src\system\core\include\cutils\logger.h
 * @Gitee: https://gitee.com/cpu_code
 * @Github: https://github.com/CPU-Code
 * @CSDN: https://blog.csdn.net/qq_44226094
 * @Gitbook: https://923992029.gitbook.io/cpucode/
 */ 
/* utils/logger.h
** 
** Copyright 2007, The Android Open Source Project
**
** This file is dual licensed.  It may be redistributed and/or modified
** under the terms of the Apache 2.0 License OR version 2 of the GNU
** General Public License.
*/

#ifndef _UTILS_LOGGER_H
#define _UTILS_LOGGER_H

#include <stdint.h>

// 描述一条日志记录
struct logger_entry {
    uint16_t    len;    /* length of the payload */
    uint16_t    __pad;  /* no matter what, we get 2 bytes of padding */
    int32_t     pid;    /* generating process's pid */
    int32_t     tid;    /* generating process's tid */
    int32_t     sec;    /* seconds since Epoch */
    int32_t     nsec;   /* nanoseconds */
    char        msg[0]; /* the entry's payload */
};

#define LOGGER_LOG_MAIN		"log/main"
#define LOGGER_LOG_RADIO	"log/radio"
#define LOGGER_LOG_EVENTS	"log/events"
#define LOGGER_LOG_SYSTEM	"log/system"

// 描述一条日志记录的最大长度， 包括日志记录头和日志记录有效负载两部分内容的长度
#define LOGGER_ENTRY_MAX_LEN		(4*1024)

// 描述日志记录有效负载的最大长度
#define LOGGER_ENTRY_MAX_PAYLOAD	\
	(LOGGER_ENTRY_MAX_LEN - sizeof(struct logger_entry))

#ifdef HAVE_IOCTL

#include <sys/ioctl.h>

#define __LOGGERIO	0xAE

#define LOGGER_GET_LOG_BUF_SIZE		_IO(__LOGGERIO, 1) /* size of log */
#define LOGGER_GET_LOG_LEN		_IO(__LOGGERIO, 2) /* used log len */
#define LOGGER_GET_NEXT_ENTRY_LEN	_IO(__LOGGERIO, 3) /* next entry len */
#define LOGGER_FLUSH_LOG		_IO(__LOGGERIO, 4) /* flush log */

#endif // HAVE_IOCTL

#endif /* _UTILS_LOGGER_H */
