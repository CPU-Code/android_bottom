/*
 * @Author: cpu_code
 * @Date: 2020-07-22 20:56:35
 * @LastEditTime: 2020-07-24 10:52:57
 * @FilePath: \android_bottom\hardware\libhardware\include\hardware\hardware.h
 * @Gitee: https://gitee.com/cpu_code
 * @Github: https://github.com/CPU-Code
 * @CSDN: https://blog.csdn.net/qq_44226094
 * @Gitbook: https://923992029.gitbook.io/cpucode/
 */ 
/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_INCLUDE_HARDWARE_HARDWARE_H
#define ANDROID_INCLUDE_HARDWARE_HARDWARE_H

#include <stdint.h>
#include <sys/cdefs.h>

#include <cutils/native_handle.h>

__BEGIN_DECLS

/*
 * Value for the hw_module_t.tag field
 */

#define MAKE_TAG_CONSTANT(A,B,C,D) (((A) << 24) | ((B) << 16) | ((C) << 8) | (D))

#define HARDWARE_MODULE_TAG MAKE_TAG_CONSTANT('H', 'W', 'M', 'T')
#define HARDWARE_DEVICE_TAG MAKE_TAG_CONSTANT('H', 'W', 'D', 'T')

struct hw_module_t;
struct hw_module_methods_t;
struct hw_device_t;

/**
 * Every hardware module must have a data structure named HAL_MODULE_INFO_SYM
 * and the fields of this data structure must begin with hw_module_t
 * followed by module specific information.
 */
/* 
 * 硬件抽象层中的每一个模块都必须自定义一个硬件抽象层模块结构体， 
 8 而且它的第一个成员变量的类型必须为 hw_module_t
*/
typedef struct hw_module_t {
    /** tag must be initialized to HARDWARE_MODULE_TAG */
    /* 
     * 成员变量 tag 的值必须设置为 HARDWARE_MODULE_TAG， 即设置为一个常量值（'H'＜＜24|'W'＜＜16|'M'＜＜8|'T'）,
     * 用来标志这是一个硬件抽象层模块结构体 
     */
    uint32_t tag;

    /** major version number for the module */
    uint16_t version_major;

    /** minor version number of the module */
    uint16_t version_minor;

    /** Identifier of module */
    const char *id;

    /** Name of this module */
    const char *name;

    /** Author/owner/implementor of the module */
    const char *author;

    /** Modules methods */
    /* 定义了一个硬件抽象层模块的操作方法列表 */
    struct hw_module_methods_t* methods;

    /** module's dso */
    /* 保存加载硬件抽象层模块后得到的句柄值 */
    /*
     * 加载硬件抽象层模块的过程实际上就是调用 dlopen 函数来加载与其对应的动态链接库文件的过程。 
     * 在调用 dlclose 函数来卸载这个硬件抽象层模块时， 要用到这个句柄值
     */
    void* dso;

    /** padding to 128 bytes, reserved for future use */
    uint32_t reserved[32-7];

} hw_module_t;

typedef struct hw_module_methods_t {
    /** Open a specific device */
    /**
     * @function: 打开硬件抽象层模块中的硬件设备
     * @parameter: 
     *        module : 要打开的硬件设备所在的模块
     *        id : 要打开的硬件设备的ID
     *        device : 一个输出参数，描述一个已经打开的硬件设备
     * @return: 
     *     success: 
     *     error: 
     * @note: 
     */
    int (*open)(const struct hw_module_t* module, const char* id,
            struct hw_device_t** device);

} hw_module_methods_t;

/**
 * Every device data structure must begin with hw_device_t
 * followed by module specific public methods and attributes.
 */
/*
 * 硬件抽象层模块中的每一个硬件设备都必须自定义一个硬件设备结构体，
 * 而且它的第一个成员变量的类型必须为hw_device_t
 */
typedef struct hw_device_t {
    /** tag must be initialized to HARDWARE_DEVICE_TAG */
    /*
     * tag 必须 == HARDWARE_DEVICE_TAG，即设置为一个常量值（'H'＜＜24|'W'＜＜16|'D'＜＜8|'T'）,
     * 用来标志这是一个硬件抽象层中的硬件设备结构体
     */
    uint32_t tag;

    /** version number for hw_device_t */
    uint32_t version;

    /** reference to the module this device belongs to */
    struct hw_module_t* module;

    /** padding reserved for future use */
    uint32_t reserved[12];

    /** Close this device */
    /* 关闭一个硬件设备 */
    int (*close)(struct hw_device_t* device);

} hw_device_t;

/**
 * Name of the hal_module_info
 */
/*
硬件抽象层中的每一个模块都必须存在一个导出符号 HAL_MODULE_IFNO_SYM， 即“HMI”， 
它指向一个自定义的硬件抽象层模块结构体
*/
#define HAL_MODULE_INFO_SYM         HMI

/**
 * Name of the hal_module_info as a string
 */
#define HAL_MODULE_INFO_SYM_AS_STR  "HMI"

/**
 * Get the module info associated with a module by id.
 * @return: 0 == success, <0 == error and *pHmi == NULL
 */
int hw_get_module(const char *id, const struct hw_module_t **module);


/**
 * pixel format definitions
 */

enum {
    HAL_PIXEL_FORMAT_RGBA_8888          = 1,
    HAL_PIXEL_FORMAT_RGBX_8888          = 2,
    HAL_PIXEL_FORMAT_RGB_888            = 3,
    HAL_PIXEL_FORMAT_RGB_565            = 4,
    HAL_PIXEL_FORMAT_BGRA_8888          = 5,
    HAL_PIXEL_FORMAT_RGBA_5551          = 6,
    HAL_PIXEL_FORMAT_RGBA_4444          = 7,

    /* 0x8 - 0xFF range unavailable */

    /*
     * 0x100 - 0x1FF
     *
     * This range is reserved for pixel formats that are specific to the HAL
     * implementation.  Implementations can use any value in this range to
     * communicate video pixel formats between their HAL modules.  These formats
     * must not have an alpha channel.  Additionally, an EGLimage created from a
     * gralloc buffer of one of these formats must be supported for use with the
     * GL_OES_EGL_image_external OpenGL ES extension.
     */

    /*
     * Android YUV format:
     *
     * This format is exposed outside of the HAL to software
     * decoders and applications.
     * EGLImageKHR must support it in conjunction with the
     * OES_EGL_image_external extension.
     *
     * YV12 is 4:2:0 YCrCb planar format comprised of a WxH Y plane followed
     * by (W/2) x (H/2) Cr and Cb planes.
     *
     * This format assumes
     * - an even width
     * - an even height
     * - a horizontal stride multiple of 16 pixels
     * - a vertical stride equal to the height
     *
     *   y_size = stride * height
     *   c_size = ALIGN(stride/2, 16) * height/2
     *   size = y_size + c_size * 2
     *   cr_offset = y_size
     *   cb_offset = y_size + c_size
     *
     */
    HAL_PIXEL_FORMAT_YV12   = 0x32315659, // YCrCb 4:2:0 Planar



    /* Legacy formats (deprecated), used by ImageFormat.java */
    HAL_PIXEL_FORMAT_YCbCr_422_SP       = 0x10, // NV16
    HAL_PIXEL_FORMAT_YCrCb_420_SP       = 0x11, // NV21
    HAL_PIXEL_FORMAT_YCbCr_422_I        = 0x14, // YUY2
};


/**
 * Transformation definitions
 *
 * IMPORTANT NOTE:
 * HAL_TRANSFORM_ROT_90 is applied CLOCKWISE and AFTER HAL_TRANSFORM_FLIP_{H|V}.
 *
 */

enum {
    /* flip source image horizontally (around the vertical axis) */
    HAL_TRANSFORM_FLIP_H    = 0x01,
    /* flip source image vertically (around the horizontal axis)*/
    HAL_TRANSFORM_FLIP_V    = 0x02,
    /* rotate source image 90 degrees clockwise */
    HAL_TRANSFORM_ROT_90    = 0x04,
    /* rotate source image 180 degrees */
    HAL_TRANSFORM_ROT_180   = 0x03,
    /* rotate source image 270 degrees clockwise */
    HAL_TRANSFORM_ROT_270   = 0x07,
};

__END_DECLS

#endif  /* ANDROID_INCLUDE_HARDWARE_HARDWARE_H */
