 #
 # @Author: cpu_code
 # @Date: 2020-07-22 20:54:53
 # @LastEditTime: 2020-07-22 21:59:01
 # @FilePath: \android_bottom\packages\experimental\HelloAndroid\Android.mk
 # @Gitee: https://gitee.com/cpu_code
 # @Github: https://github.com/CPU-Code
 # @CSDN: https://blog.csdn.net/qq_44226094
 # @Gitbook: https://923992029.gitbook.io/cpucode
 #
 # 编译脚本文件
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME := HelloAndroid

include $(BUILD_PACKAGE)
