/*
 * @Author: cpu_code
 * @Date: 2020-07-12 19:24:49
 * @LastEditTime: 2020-07-20 09:31:52
 * @FilePath: \Android系统源代码情景分析（第三版）程序文件\chapter-4\src\system\core\liblog\logd_write.c
 * @Gitee: https://gitee.com/cpu_code
 * @Github: https://github.com/CPU-Code
 * @CSDN: https://blog.csdn.net/qq_44226094
 * @Gitbook: https://923992029.gitbook.io/cpucode/
 */ 
/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <time.h>
#include <stdio.h>
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <cutils/logger.h>
#include <cutils/logd.h>
#include <cutils/log.h>

#define LOG_BUF_SIZE	1024

#if FAKE_LOG_DEVICE
// This will be defined when building for the host.
#define log_open(pathname, flags) fakeLogOpen(pathname, flags)
#define log_writev(filedes, vector, count) fakeLogWritev(filedes, vector, count)
#define log_close(filedes) fakeLogClose(filedes)
#else
#define log_open(pathname, flags) open(pathname, flags)
#define log_writev(filedes, vector, count) writev(filedes, vector, count)
#define log_close(filedes) close(filedes)
#endif

// 初始化日志库liblog
static int __write_to_log_init(log_id_t, struct iovec *vec, size_t nr);
// 开始的时候被设置为函数__write_to_log_init
static int (*write_to_log)(log_id_t, struct iovec *vec, size_t nr) = __write_to_log_init;
#ifdef HAVE_PTHREADS
static pthread_mutex_t log_init_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static int log_fds[(int)LOG_ID_MAX] = { -1, -1, -1, -1 };

/*
 * This is used by the C++ code to decide if it should write logs through
 * the C code.  Basically, if /dev/log/... is available, we're running in
 * the simulator rather than a desktop tool and want to use the device.
 */
static enum {
    kLogUninitialized, kLogNotAvailable, kLogAvailable
} g_log_status = kLogUninitialized;
int __android_log_dev_available(void)
{
    if (g_log_status == kLogUninitialized) {
        if (access("/dev/"LOGGER_LOG_MAIN, W_OK) == 0)
            g_log_status = kLogAvailable;
        else
            g_log_status = kLogNotAvailable;
    }

    return (g_log_status == kLogAvailable);
}


static int __write_to_log_null(log_id_t log_fd, struct iovec *vec, size_t nr)
{
    // 一个空实现 
    //日志设备文件打开失败的情况下， 函数指针write_to_log才会指向该函数
    return -1;
}

static int __write_to_log_kernel(log_id_t log_id, struct iovec *vec, size_t nr)
{
    ssize_t ret;
    int log_fd;

    if (/*(int)log_id >= 0 &&*/ (int)log_id < (int)LOG_ID_MAX) 
    {
        // 根据log_id在全局数组log_fds中找到对应的日志设备文件描述符
        log_fd = log_fds[(int)log_id];
    } 
    else 
    {
        return EBADF;
    }

    do 
    {
        // 把日志记录写入到Logger日志驱动程序中
        ret = log_writev(log_fd, vec, nr);

        // 返回值小于0， 并且错误码等于EINTR， 那就重新执行写入日志记录的操作
        
        // 这种情况一般出现在当前进程等待写入日志记录的过程中，刚好有新的信号需要处理，
        //内核就会返回一个EINTR错误码给调用者， 表示调用者需要再次执行相同的操作
    } while (ret < 0 && errno == EINTR);

    return ret;
}

static int __write_to_log_init(log_id_t log_id, struct iovec *vec, size_t nr)
{
#ifdef HAVE_PTHREADS
    pthread_mutex_lock(&log_init_lock);
#endif

    // write_to_log指向的是自己
    if (write_to_log == __write_to_log_init) {

        // 打开系统中的日志设备文件，并把得到的文件描述符保存在全局数组log_fds中
        // 实际打开/dev/log/main /dev/log/radio  /dev/log/events  /dev/log/system 四个日志设备文件
        log_fds[LOG_ID_MAIN] = log_open("/dev/"LOGGER_LOG_MAIN, O_WRONLY);
        log_fds[LOG_ID_RADIO] = log_open("/dev/"LOGGER_LOG_RADIO, O_WRONLY);
        log_fds[LOG_ID_EVENTS] = log_open("/dev/"LOGGER_LOG_EVENTS, O_WRONLY);
        log_fds[LOG_ID_SYSTEM] = log_open("/dev/"LOGGER_LOG_SYSTEM, O_WRONLY);

        // 将函数指针write_to_log指向函数__write_to_log_kernel
        write_to_log = __write_to_log_kernel;

        // 判断/dev/log/main  /dev/log/radio  /dev/log/events三个日志设备文件是否都打开成功
        if (log_fds[LOG_ID_MAIN] < 0 || log_fds[LOG_ID_RADIO] < 0 ||
                log_fds[LOG_ID_EVENTS] < 0) 
        {
            log_close(log_fds[LOG_ID_MAIN]);
            log_close(log_fds[LOG_ID_RADIO]);
            log_close(log_fds[LOG_ID_EVENTS]);
            log_fds[LOG_ID_MAIN] = -1;
            log_fds[LOG_ID_RADIO] = -1;
            log_fds[LOG_ID_EVENTS] = -1;

            // 将函数指针write_to_log 指向 函数 __write_to_log_null
            write_to_log = __write_to_log_null;
        }

        // 判断日志设备文件/dev/log/system是否打开成功
        if (log_fds[LOG_ID_SYSTEM] < 0) 
        {
            // 不成功

            // 将类型为system和main的日志记录都写入到日志设备文件/dev/log/main中
            log_fds[LOG_ID_SYSTEM] = log_fds[LOG_ID_MAIN];
        }
    }

#ifdef HAVE_PTHREADS
    pthread_mutex_unlock(&log_init_lock);
#endif

    return write_to_log(log_id, vec, nr);
}

int __android_log_write(int prio, const char *tag, const char *msg)
{
    struct iovec vec[3];

    // 在默认情况下，写入的日志记录的类型是 main
    log_id_t log_id = LOG_ID_MAIN;

    if (!tag)
        tag = "";

    /* XXX: This needs to go! */
    if (!strcmp(tag, "HTC_RIL") ||
        !strncmp(tag, "RIL", 3) || /* Any log tag with "RIL" as the prefix */
        !strcmp(tag, "AT") ||
        !strcmp(tag, "GSM") ||
        !strcmp(tag, "STK") ||
        !strcmp(tag, "CDMA") ||
        !strcmp(tag, "PHONE") ||
        !strcmp(tag, "SMS"))
            // 类型为radio的日志记录
            log_id = LOG_ID_RADIO;

    // 日志记录的优先级
    vec[0].iov_base   = (unsigned char *) &prio;
    vec[0].iov_len    = 1;

    // 标签
    vec[1].iov_base   = (void *) tag;
    vec[1].iov_len    = strlen(tag) + 1;

    //内容
    vec[2].iov_base   = (void *) msg;
    vec[2].iov_len    = strlen(msg) + 1;

    // 写入到Logger日志驱动程序中
    return write_to_log(log_id, vec, 3);
}

int __android_log_buf_write(int bufID, int prio, const char *tag, const char *msg)
{
    // 可以指定写入的日志记录的类型

    struct iovec vec[3];

    if (!tag)
        tag = "";

    /* XXX: This needs to go! */
    if (!strcmp(tag, "HTC_RIL") ||
        !strncmp(tag, "RIL", 3) || /* Any log tag with "RIL" as the prefix */
        !strcmp(tag, "AT") ||
        !strcmp(tag, "GSM") ||
        !strcmp(tag, "STK") ||
        !strcmp(tag, "CDMA") ||
        !strcmp(tag, "PHONE") ||
        !strcmp(tag, "SMS"))

            // radio的日志记录
            bufID = LOG_ID_RADIO;

    vec[0].iov_base   = (unsigned char *) &prio;
    vec[0].iov_len    = 1;
    vec[1].iov_base   = (void *) tag;
    vec[1].iov_len    = strlen(tag) + 1;
    vec[2].iov_base   = (void *) msg;
    vec[2].iov_len    = strlen(msg) + 1;

    // 把紧跟在日志记录标签和内容后面的字符串结束符号‘\0'也写入到Logger日志驱动程序中， 
    //目的是为了以后从Logger日志驱动程序读取日志时，可以将日志记录的标签字段和内容字段解析出来

    return write_to_log(bufID, vec, 3);
}

int __android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap)
{
    char buf[LOG_BUF_SIZE];

    // 使用格式化字符串来描述要写入的日志记录内容
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);

    // 向Logger日志驱动程序中写入日志记录
    return __android_log_write(prio, tag, buf);
}

int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    va_list ap;

    // // 使用格式化字符串来描述要写入的日志记录内容
    char buf[LOG_BUF_SIZE];

    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);

    // 向Logger日志驱动程序中写入日志记录
    return __android_log_write(prio, tag, buf);
}

int __android_log_buf_print(int bufID, int prio, const char *tag, const char *fmt, ...)
{
    va_list ap;
    char buf[LOG_BUF_SIZE];

    va_start(ap, fmt);

    // 格式化字符串来描述要写入的日志记录内容
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    
    va_end(ap);

    // 向Logger日志驱动程序中写入日志记录 , 指定要写入的日志记录的类型
    return __android_log_buf_write(bufID, prio, tag, buf);
}

void __android_log_assert(const char *cond, const char *tag,
			  const char *fmt, ...)
{
    // 使用格式化字符串来描述要写入的日志记录内容
    char buf[LOG_BUF_SIZE];

    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
        va_end(ap);
    } else {
        /* Msg not provided, log condition.  N.B. Do not use cond directly as
         * format string as it could contain spurious '%' syntax (e.g.
         * "%d" in "blocks%devs == 0").
         */
        if (cond)
            snprintf(buf, LOG_BUF_SIZE, "Assertion failed: %s", cond);
        else
            strcpy(buf, "Unspecified assertion failed");
    }

    // 向Logger日志驱动程序中写入日志记录
    __android_log_write(ANDROID_LOG_FATAL, tag, buf);

    __builtin_trap(); /* trap so we have a chance to debug the situation */
}

int __android_log_bwrite(int32_t tag, const void *payload, size_t len)
{
    struct iovec vec[2];
    
    // 名称
    vec[0].iov_base = &tag;
    vec[0].iov_len = sizeof(tag);
    
    // 单位
    vec[1].iov_base = (void*)payload;
    vec[1].iov_len = len;
    
    // 写入的日志记录的类型为events
    return write_to_log(LOG_ID_EVENTS, vec, 2);
}

/*
 * Like __android_log_bwrite, but takes the type as well.  Doesn't work
 * for the general case where we're generating lists of stuff, but very
 * handy if we just want to dump an integer into the log.
 */
int __android_log_btwrite(int32_t tag, char type, const void *payload,
    size_t len)
{

    struct iovec vec[3];

    // 名称
    vec[0].iov_base = &tag;
    vec[0].iov_len = sizeof(tag);

    // 定要写入的日志记录内容的值类型
    vec[1].iov_base = &type;
    vec[1].iov_len = sizeof(type);

    // 单位
    vec[2].iov_base = (void*)payload;
    vec[2].iov_len = len;

    // 写入的日志记录的类型为events
    return write_to_log(LOG_ID_EVENTS, vec, 3);
}
