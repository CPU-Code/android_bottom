 #
 # @Author: cpu_code
 # @Date: 2020-07-22 20:54:53
 # @LastEditTime: 2020-07-22 21:28:53
 # @FilePath: \android_bottom\external\clearsilver\cgi\Android.mk
 # @Gitee: https://gitee.com/cpu_code
 # @Github: https://github.com/CPU-Code
 # @CSDN: https://blog.csdn.net/qq_44226094
 # @Gitbook: https://923992029.gitbook.io/cpucode/
 #


LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	cgiwrap.c \
	cgi.c \
	html.c \
	date.c \
	rfc2388.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/..

LOCAL_CFLAGS := -fPIC

# This forces a 64-bit build for Java6
# Change the following two lines for a 32-bit system. shyluo@gmail.com, 2011-05-29.
#LOCAL_CFLAGS += -m64
#LOCAL_LDFLAGS += -m64
# 在32位的机器上编译Android源代码
LOCAL_CFLAGS += -m32
LOCAL_LDFLAGS += -m32
# We use the host compilers because the Linux SDK build
# uses a 32-bit toolchain that can't handle -m64
LOCAL_CC := $(CC)
LOCAL_CXX := $(CXX)

LOCAL_NO_DEFAULT_COMPILER_FLAGS := true

LOCAL_MODULE:= libneo_cgi

LOCAL_SHARED_LIBRARIES := libneo_util libneo_cs

LOCAL_LDLIBS += -lz

include $(BUILD_HOST_SHARED_LIBRARY)
