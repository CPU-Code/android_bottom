 #
 # @Author: cpu_code
 # @Date: 2020-07-22 20:55:46
 # @LastEditTime: 2020-07-24 10:21:57
 # @FilePath: \android_bottom\external\freg\Android.mk
 # @Gitee: https://gitee.com/cpu_code
 # @Github: https://github.com/CPU-Code
 # @CSDN: https://blog.csdn.net/qq_44226094
 # @Gitbook: https://923992029.gitbook.io/cpucode/
 #
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := freg
LOCAL_SRC_FILES := $(call all-subdir-c-files)
# 当前要编译的是一个可执行应用程序模块
# 编译结果在out/target/product/gerneric/system/bin
include $(BUILD_EXECUTABLE)
