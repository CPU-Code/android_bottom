 #
 # @Author: cpu_code
 # @Date: 2020-07-22 20:56:57
 # @LastEditTime: 2020-07-23 21:50:35
 # @FilePath: \android_bottom\kernel\goldfish\drivers\freg\Makefile
 # @Gitee: https://gitee.com/cpu_code
 # @Github: https://github.com/CPU-Code
 # @CSDN: https://blog.csdn.net/qq_44226094
 # @Gitbook: https://923992029.gitbook.io/cpucode/
 #
# 编译脚本文件
# $（CONFIG_FREG） 是一个变量

# 将驱动程序freg内建到内核中， 那么变量$（CONFIG_FREG） 的值为y
# 以模块的方式来编译驱动程序freg， 那么变量$（CONFIG_FREG） 的值为m
# 变量$（CONFIG_FREG） 的值不为y， 也不为m，那么驱动程序freg就不会被编译

obj-$(CONFIG_FREG) += freg.o
