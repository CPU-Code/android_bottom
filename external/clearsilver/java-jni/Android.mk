 #
 # @Author: cpu_code
 # @Date: 2020-07-22 20:54:53
 # @LastEditTime: 2020-07-22 21:29:46
 # @FilePath: \android_bottom\external\clearsilver\java-jni\Android.mk
 # @Gitee: https://gitee.com/cpu_code
 # @Github: https://github.com/CPU-Code
 # @CSDN: https://blog.csdn.net/qq_44226094
 # @Gitbook: https://923992029.gitbook.io/cpucode/
 #

LOCAL_PATH:= $(call my-dir)


# clearsilver java library
# ============================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	CS.java \
	CSFileLoader.java \
	JNI.java \
	HDF.java

LOCAL_MODULE:= clearsilver

include $(BUILD_HOST_JAVA_LIBRARY)

our_java_lib := $(LOCAL_BUILT_MODULE)


# libclearsilver-jni.so
# ============================================================
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
	j_neo_util.c \
	j_neo_cs.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/..

LOCAL_CFLAGS += -fPIC

# This forces a 64-bit build for Java6
# Change the following two lines for a 32-bit systems. shyluo@gmail.com, 2011-05-29.
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

ifeq ($(HOST_OS),darwin)
	LOCAL_C_INCLUDES += /System/Library/Frameworks/JavaVM.framework/Headers
	LOCAL_LDLIBS := -framework JavaVM
else
	LOCAL_C_INCLUDES += $(JNI_H_INCLUDE)
endif

LOCAL_MODULE:= libclearsilver-jni

LOCAL_MODULE_SUFFIX := $(HOST_JNILIB_SUFFIX)

LOCAL_SHARED_LIBRARIES := libneo_util libneo_cs libneo_cgi

include $(BUILD_HOST_SHARED_LIBRARY)

# Use -force with javah to make sure that the output file
# gets updated.  If javah decides not to update the file,
# make gets confused.

GEN := $(intermediates)/org_clearsilver_HDF.h
$(GEN): PRIVATE_OUR_JAVA_LIB := $(our_java_lib)
$(GEN): PRIVATE_CUSTOM_TOOL = javah -classpath $(PRIVATE_OUR_JAVA_LIB) -force -o $@ -jni org.clearsilver.HDF 
$(GEN): PRIVATE_MODULE := $(LOCAL_MODULE)
$(GEN): $(our_java_lib)
	$(transform-generated-source)
$(intermediates)/j_neo_util.o : $(GEN)

GEN := $(intermediates)/org_clearsilver_CS.h
$(GEN): PRIVATE_OUR_JAVA_LIB := $(our_java_lib)
$(GEN): PRIVATE_CUSTOM_TOOL = javah -classpath $(PRIVATE_OUR_JAVA_LIB) -force -o $@ -jni org.clearsilver.CS
$(GEN): PRIVATE_MODULE := $(LOCAL_MODULE)
$(GEN): $(our_java_lib)
	$(transform-generated-source)
$(intermediates)/j_neo_cs.o : $(GEN)
