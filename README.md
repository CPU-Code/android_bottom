<!--
 * @Author: cpu_code
 * @Date: 2020-07-22 20:48:51
 * @LastEditTime: 2020-07-26 19:46:37
 * @FilePath: \android_bottom\README.md
 * @Gitee: https://gitee.com/cpu_code
 * @Github: https://github.com/CPU-Code
 * @CSDN: https://blog.csdn.net/qq_44226094
 * @Gitbook: https://923992029.gitbook.io/cpucode/
--> 

# android_bottom

安卓底层源码注释


## 目录






------------------------



## [build/core](build/core)

在32位的机器上编译Android源代码

* [main.mk](build/core/main.mk)

---------------------------

## [external](external)



### [external/ashmem](external/ashmem)



### [external/binder](external/binder)



### [external/clearsilver](external/clearsilver)

在32位的机器上编译Android源代码

* [cgi/Android.mk](external/clearsilver/cgi/Android.mk)

* [cs/Android.mk](external/clearsilver/cs/Android.mk)

* [java-jni/Android.mk](external/clearsilver/java-jni/Android.mk)

* [util/Android.mk](external/clearsilver/util/Android.mk)

### [external/freg](external/freg)

* [虚拟设备的编译__Android](external/freg/Android.mk)
* [虚拟设备的应用__freg](external/freg/freg.c)

### [external/lightpointer](external/lightpointer)



### [external/weightpointer](external/weightpointer)

------------------------------

## [frameworks/base](frameworks/base)


---------------------------

## [hardware/libhardware](hardware/libhardware)

* [硬件抽象层模块的命名规范__hardware](hardware/libhardware/hardware.c)

### [libhardware/include/hardware](hardware/libhardware/include/hardware)

* [硬件抽象层模块__hardware](hardware/libhardware/include/hardware/hardware.h)

-------------------

## [kernel/goldfish](kernel/goldfish)

### [goldfish/arch/arm](kernel/goldfish/arch/arm)

* [虚拟设备的配置__kconfig](kernel/goldfish/arch/arm/Kconfig)


### [goldfish/drivers](kernel/goldfish/drivers)

* [虚拟设备的驱动__freg](kernel/goldfish/drivers/freg/freg.c)
* [虚拟设备的驱动头文件__freg](kernel/goldfish/drivers/freg/freg.h)
* [虚拟设备的配置__Kconfig](kernel/goldfish/drivers/freg/Kconfig)
* [虚拟设备的编译__Makefile](kernel/goldfish/drivers/freg/Makefile)
* [编译__Makefile](kernel/goldfish/drivers/Makefile)

### [goldfish/include](kernel/goldfish/include)

### [goldfish/mm](kernel/goldfish/mm)

------------------

## [out](out/target/common/obj/JAVA_LIBRARIES/framework_intermediates/src/core/java/android/os)

-----------------------

## [packages](packages)

### [packages/experimental](packages/experimental)

### [packages/apps](packages/apps)

* [安卓应用显示__HelloAndroid](packages/apps/HelloAndroid)

---------------------------

## [system/core](system/core)

