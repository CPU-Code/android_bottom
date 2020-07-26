/*
 * @Author: cpu_code
 * @Date: 2020-07-22 20:55:46
 * @LastEditTime: 2020-07-24 10:04:40
 * @FilePath: \android_bottom\external\freg\freg.c
 * @Gitee: https://gitee.com/cpu_code
 * @Github: https://github.com/CPU-Code
 * @CSDN: https://blog.csdn.net/qq_44226094
 * @Gitbook: https://923992029.gitbook.io/cpucode/
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define FREG_DEVICE_NAME "/dev/freg"

int main(int argc, char** argv)
{
	int fd = -1;
	int val = 0;

	// 以读写方式打开设备文件/dev/freg
	fd = open(FREG_DEVICE_NAME, O_RDWR);
	if(fd == -1)
	{
		printf("Failed to open device %s.\n", FREG_DEVICE_NAME);
		return -1;
	}
	
	printf("Read original value:\n");
	
	// 读取它的内容，即读取虚拟硬件设备freg的寄存器val的内容
	read(fd, &val, sizeof(val));
	// 内容打印
	printf("%d.\n\n", val);

	val = 5;
	printf("Write value %d to %s.\n\n", val, FREG_DEVICE_NAME);

	// 将一个整数5写入到虚拟硬件设备freg的寄存器val中
    write(fd, &val, sizeof(val));
	printf("Read the value again:\n");

	// 将这个整数5读取
    read(fd, &val, sizeof(val));
	//打印出来
    printf("%d.\n\n", val);

	close(fd);

	return 0;
}
