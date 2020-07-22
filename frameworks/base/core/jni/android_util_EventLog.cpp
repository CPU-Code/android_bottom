/*
 * @Author: cpu_code
 * @Date: 2020-07-12 19:24:49
 * @LastEditTime: 2020-07-20 14:25:55
 * @FilePath: \Android系统源代码情景分析（第三版）程序文件\chapter-4\src\frameworks\base\core\jni\android_util_EventLog.cpp
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

#include <fcntl.h>

#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include "jni.h"
#include "cutils/logger.h"

// The size of the tag number comes out of the payload size.
#define MAX_EVENT_PAYLOAD (LOGGER_ENTRY_MAX_PAYLOAD - sizeof(int32_t))

namespace android {

static jclass gCollectionClass;
static jmethodID gCollectionAddID;

static jclass gEventClass;
static jmethodID gEventInitID;

static jclass gIntegerClass;
static jfieldID gIntegerValueID;

static jclass gListClass;
static jfieldID gListItemsID;

static jclass gLongClass;
static jfieldID gLongValueID;

static jclass gStringClass;

/*
 * In class android.util.EventLog:
 *  static native int writeEvent(int tag, int value)
 */
static jint android_util_EventLog_writeEvent_Integer(JNIEnv* env, jobject clazz,
                                                     jint tag, jint value)
{
    // 向Logger日志驱动程序中写入日志记录
    // EVENT_TYPE_INT : 要写入的日志记录的内容为一个整数
    return android_btWriteLog(tag, EVENT_TYPE_INT, &value, sizeof(value));
}

/*
 * In class android.util.EventLog:
 *  static native int writeEvent(long tag, long value)
 */
static jint android_util_EventLog_writeEvent_Long(JNIEnv* env, jobject clazz,
                                                  jint tag, jlong value)
{
    // 向Logger日志驱动程序中写入日志记录
    // EVENT_TYPE_LONG :  要写入的日志记录的内容为一个长整数
    return android_btWriteLog(tag, EVENT_TYPE_LONG, &value, sizeof(value));
}

/*
 * In class android.util.EventLog:
 *  static native int writeEvent(int tag, String value)
 */
static jint android_util_EventLog_writeEvent_String(JNIEnv* env, jobject clazz,
                                                    jint tag, jstring value) {
    uint8_t buf[MAX_EVENT_PAYLOAD];

    // Don't throw NPE -- I feel like it's sort of mean for a logging function
    // to be all crashy if you pass in NULL -- but make the NULL value explicit.
    const char *str = value != NULL ? env->GetStringUTFChars(value, NULL) : "NULL";
    jint len = strlen(str);
    const int max = sizeof(buf) - sizeof(len) - 2;  // Type byte, final newline
    if (len > max) 
    {
        len = max;
    }


    // 日志记录内容的类型为字符串
    buf[0] = EVENT_TYPE_STRING;
    // 字符串的长度
    memcpy(&buf[1], &len, sizeof(len));
    // 字符串内容
    memcpy(&buf[1 + sizeof(len)], str, len);
    // ‘\n'来结束该日志记录
    buf[1 + sizeof(len) + len] = '\n';

    if (value != NULL) env->ReleaseStringUTFChars(value, str);

    // 将日志记录写入到Logger日志驱动程序
    return android_bWriteLog(tag, buf, 2 + sizeof(len) + len);
}

/*
 * In class android.util.EventLog:
 *  static native int writeEvent(long tag, Object... value)
 */
static jint android_util_EventLog_writeEvent_Array(JNIEnv* env, jobject clazz,
                                                   jint tag, jobjectArray value) 
{
    if (value == NULL) 
    {
        return android_util_EventLog_writeEvent_String(env, clazz, tag, NULL);
    }

    uint8_t buf[MAX_EVENT_PAYLOAD];
    const size_t max = sizeof(buf) - 1;  // leave room for final newline
    size_t pos = 2;  // Save room for type tag & array count

    // 一个列表中最多可以包含255个元素
    jsize copied = 0, num = env->GetArrayLength(value);
    for (; copied < num && copied < 255; ++copied) 
    {
        // 依次取出列表中的元素， 且根据它们的值类型来组织缓冲区buf的内存布局
        jobject item = env->GetObjectArrayElement(value, copied);
        
        if (item == NULL || env->IsInstanceOf(item, gStringClass)) 
        {
            // 值类型为字符串
            if (pos + 1 + sizeof(jint) > max) 
                break;
            const char *str = item != NULL ? env->GetStringUTFChars((jstring) item, NULL) : "NULL";
            jint len = strlen(str);
            if (pos + 1 + sizeof(len) + len > max) 
                len = max - pos - 1 - sizeof(len);

            // EVENT_TYPE_STRING值
            buf[pos++] = EVENT_TYPE_STRING;
            // 字符串长度值
            memcpy(&buf[pos], &len, sizeof(len));
            // 字符串的内容
            memcpy(&buf[pos + sizeof(len)], str, len);
            
            pos += sizeof(len) + len;
            if (item != NULL) 
                env->ReleaseStringUTFChars((jstring) item, str);
        } 
        else if (env->IsInstanceOf(item, gIntegerClass)) 
        {
            // 值类型为整数
            
            jint intVal = env->GetIntField(item, gIntegerValueID);
            if (pos + 1 + sizeof(intVal) > max) 
                break;
                
            // 第一个字节设置为一个EVENT_TYPE_INT值
            buf[pos++] = EVENT_TYPE_INT;
            // 整数值
            memcpy(&buf[pos], &intVal, sizeof(intVal));
            pos += sizeof(intVal);
        } 
        else if (env->IsInstanceOf(item, gLongClass)) 
        {
            // 值类型为长整数
            
            jlong longVal = env->GetLongField(item, gLongValueID);
            if (pos + 1 + sizeof(longVal) > max) 
                break;
            
            // buf的内容就为一个EVENT_TYPE_LONG值
            buf[pos++] = EVENT_TYPE_LONG;
            // 长整数值
            memcpy(&buf[pos], &longVal, sizeof(longVal));
            pos += sizeof(longVal);
        } 
        else 
        {
            // 抛出一个异常
            
            jniThrowException(env,
                    "java/lang/IllegalArgumentException",
                    "Invalid payload item type");
            return -1;
        }
        env->DeleteLocalRef(item);
    }

    // 列表类型的日志记录
    buf[0] = EVENT_TYPE_LIST;
    // 列表元素个数
    buf[1] = copied;
    // 日志记录的结束标志
    buf[pos++] = '\n';
    // 将日志记录写入到Logger日志驱动程序中
    return android_bWriteLog(tag, buf, pos);
}

/*
 * In class android.util.EventLog:
 *  static native void readEvents(int[] tags, Collection<Event> output)
 *
 *  Reads events from the event log, typically /dev/log/events
 */
static void android_util_EventLog_readEvents(JNIEnv* env, jobject clazz,
                                             jintArray tags,
                                             jobject out) {
    if (tags == NULL || out == NULL) {
        jniThrowException(env, "java/lang/NullPointerException", NULL);
        return;
    }

    int fd = open("/dev/" LOGGER_LOG_EVENTS, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        jniThrowIOException(env, errno);
        return;
    }

    jsize tagLength = env->GetArrayLength(tags);
    jint *tagValues = env->GetIntArrayElements(tags, NULL);

    uint8_t buf[LOGGER_ENTRY_MAX_LEN];
    struct timeval timeout = {0, 0};
    fd_set readset;
    FD_ZERO(&readset);

    for (;;) {
        // Use a short select() to try to avoid problems hanging on read().
        // This means we block for 5ms at the end of the log -- oh well.
        timeout.tv_usec = 5000;
        FD_SET(fd, &readset);
        int r = select(fd + 1, &readset, NULL, NULL, &timeout);
        if (r == 0) {
            break;  // no more events
        } else if (r < 0 && errno == EINTR) {
            continue;  // interrupted by signal, try again
        } else if (r < 0) {
            jniThrowIOException(env, errno);  // Will throw on return
            break;
        }

        int len = read(fd, buf, sizeof(buf));
        if (len == 0 || (len < 0 && errno == EAGAIN)) {
            break;  // no more events
        } else if (len < 0 && errno == EINTR) {
            continue;  // interrupted by signal, try again
        } else if (len < 0) {
            jniThrowIOException(env, errno);  // Will throw on return
            break;
        } else if ((size_t) len < sizeof(logger_entry) + sizeof(int32_t)) {
            jniThrowException(env, "java/io/IOException", "Event too short");
            break;
        }

        logger_entry* entry = (logger_entry*) buf;
        int32_t tag = * (int32_t*) (buf + sizeof(*entry));

        int found = 0;
        for (int i = 0; !found && i < tagLength; ++i) {
            found = (tag == tagValues[i]);
        }

        if (found) {
            jsize len = sizeof(*entry) + entry->len;
            jbyteArray array = env->NewByteArray(len);
            if (array == NULL) break;

            jbyte *bytes = env->GetByteArrayElements(array, NULL);
            memcpy(bytes, buf, len);
            env->ReleaseByteArrayElements(array, bytes, 0);

            jobject event = env->NewObject(gEventClass, gEventInitID, array);
            if (event == NULL) break;

            env->CallBooleanMethod(out, gCollectionAddID, event);
            env->DeleteLocalRef(event);
            env->DeleteLocalRef(array);
        }
    }

    close(fd);
    env->ReleaseIntArrayElements(tags, tagValues, 0);
}

/*
 * JNI registration.
 */
static JNINativeMethod gRegisterMethods[] = {
    /* name, signature, funcPtr */
    { "writeEvent", "(II)I", (void*) android_util_EventLog_writeEvent_Integer },
    { "writeEvent", "(IJ)I", (void*) android_util_EventLog_writeEvent_Long },
    { "writeEvent",
      "(ILjava/lang/String;)I",
      (void*) android_util_EventLog_writeEvent_String
    },
    { "writeEvent",
      "(I[Ljava/lang/Object;)I",
      (void*) android_util_EventLog_writeEvent_Array
    },
    { "readEvents",
      "([ILjava/util/Collection;)V",
      (void*) android_util_EventLog_readEvents
    },
};

static struct { const char *name; jclass *clazz; } gClasses[] = {
    { "android/util/EventLog$Event", &gEventClass },
    { "java/lang/Integer", &gIntegerClass },
    { "java/lang/Long", &gLongClass },
    { "java/lang/String", &gStringClass },
    { "java/util/Collection", &gCollectionClass },
};

static struct { jclass *c; const char *name, *ft; jfieldID *id; } gFields[] = {
    { &gIntegerClass, "value", "I", &gIntegerValueID },
    { &gLongClass, "value", "J", &gLongValueID },
};

static struct { jclass *c; const char *name, *mt; jmethodID *id; } gMethods[] = {
    { &gEventClass, "<init>", "([B)V", &gEventInitID },
    { &gCollectionClass, "add", "(Ljava/lang/Object;)Z", &gCollectionAddID },
};

int register_android_util_EventLog(JNIEnv* env) {
    for (int i = 0; i < NELEM(gClasses); ++i) {
        jclass clazz = env->FindClass(gClasses[i].name);
        if (clazz == NULL) {
            LOGE("Can't find class: %s\n", gClasses[i].name);
            return -1;
        }
        *gClasses[i].clazz = (jclass) env->NewGlobalRef(clazz);
    }

    for (int i = 0; i < NELEM(gFields); ++i) {
        *gFields[i].id = env->GetFieldID(
                *gFields[i].c, gFields[i].name, gFields[i].ft);
        if (*gFields[i].id == NULL) {
            LOGE("Can't find field: %s\n", gFields[i].name);
            return -1;
        }
    }

    for (int i = 0; i < NELEM(gMethods); ++i) {
        *gMethods[i].id = env->GetMethodID(
                *gMethods[i].c, gMethods[i].name, gMethods[i].mt);
        if (*gMethods[i].id == NULL) {
            LOGE("Can't find method: %s\n", gMethods[i].name);
            return -1;
        }
    }

    return AndroidRuntime::registerNativeMethods(
            env,
            "android/util/EventLog",
            gRegisterMethods, NELEM(gRegisterMethods));
}

}; // namespace android
