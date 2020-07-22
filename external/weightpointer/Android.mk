/*
 * @Author: cpu_code
 * @Date: 2020-07-12 19:24:49
 * @LastEditTime: 2020-07-16 16:55:07
 * @FilePath: \Android系统源代码情景分析（第三版）程序文件\chapter-3\src\external\weightpointer\Android.mk
 * @Gitee: https://gitee.com/cpu_code
 * @Github: https://github.com/CPU-Code
 * @CSDN: https://blog.csdn.net/qq_44226094
 * @Gitbook: https://923992029.gitbook.io/cpucode/
 */ 

#　编译脚本文件
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := weightpointer
LOCAL_SRC_FILES := weightpointer.cpp

# 引用了libcutils和libutils两个库
LOCAL_SHARED_LIBRARIES :=  \
	libcutils \
	libutils
include $(BUILD_EXECUTABLE)
