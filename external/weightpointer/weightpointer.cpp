/*
 * @Author: cpu_code
 * @Date: 2020-07-12 19:24:49
 * @LastEditTime: 2020-07-16 18:01:07
 * @FilePath: \Android系统源代码情景分析（第三版）程序文件\chapter-3\src\external\weightpointer\weightpointer.cpp
 * @Gitee: https://gitee.com/cpu_code
 * @Github: https://github.com/CPU-Code
 * @CSDN: https://blog.csdn.net/qq_44226094
 * @Gitbook: https://923992029.gitbook.io/cpucode/
 */ 
#include <stdio.h>
#include <utils/RefBase.h>

#define INITIAL_STRONG_VALUE (1<<28)

using namespace android;

class WeightClass : public RefBase
{
public:
	void printRefCount()
    {
		int32_t strong = getStrongCount();
        weakref_type* ref = getWeakRefs();

        printf("-----------------------\n");
        printf("Strong Ref Count: %d.\n", (strong  == INITIAL_STRONG_VALUE ? 0 : strong));
        printf("Weak Ref Count: %d.\n", ref->getWeakCount());
        printf("-----------------------\n");
    }
};

class StrongClass : public WeightClass
{
    //生命周期只受强引用计数影响
public:
    // 构造
	StrongClass() 
	{
		printf("Construct StrongClass Object.\n");
	}
    //析构
	virtual ~StrongClass() 
	{
		printf("Destory StrongClass Object.\n");
	}
};

class WeakClass : public WeightClass
{
public:
    // 构造
    WeakClass()
    {
        // 生命周期同时受到强引用计数和弱引用计数的影响
		extendObjectLifetime(OBJECT_LIFETIME_WEAK);
        
        printf("Construct WeakClass Object.\n");
    }
    //析构
    virtual ~WeakClass()
    {
        printf("Destory WeakClass Object.\n");
    }
};

class ForeverClass : public WeightClass
{
    
public:
    // 构造
    ForeverClass()
    {
        // 生命周期完全不受强引用计数和弱引用计数的影响
		extendObjectLifetime(OBJECT_LIFETIME_FOREVER);

        printf("Construct ForeverClass Object.\n");
    }

    //析构
    virtual ~ForeverClass()
    {
        printf("Destory ForeverClass Object.\n");
    }
};

// 测试函数, 测试强指针和弱指针的使用情景
void TestStrongClass(StrongClass* pStrongClass)
{
    // 将 StrongClass 对象赋值给弱指针 wpOut
	wp<StrongClass> wpOut = pStrongClass;

    // 打印出该StrongClass对象的强引用计数值==0 弱引用计数值==1
	pStrongClass->printRefCount();

	{
        // 将该 StrongClass 对象赋值给 强指针 spInner
		sp<StrongClass> spInner = pStrongClass;

        // 打印出该 StrongClass 对象的强引用计数值==1  弱引用计数值== 2
		pStrongClass->printRefCount();
	}
    // 超出了强指针spInner的作用域， 所以该StrongClass对象的强引用计数值==0 弱引用计数值==1
    // 该StrongClass对象的生命周期只受强引用计数的影响，所以 该StrongClass对象会自动被释放
    // 可以看 StrongClass 类的析构函数中的日志输出来确认

    // 将弱指针 wpOut 升级为强指针 
    // 但弱指针 wpOut 所引用的StrongClass对象已经被释放，所以, 弱指针wpOut升级不了为强指针
	sp<StrongClass> spOut = wpOut.promote();

    // 获得的强指针spOut所引用的对象地址 == 0
	printf("spOut: %p.\n", spOut.get());

    // 当TestStrongClass函数返回时，超出了弱指针 wpOut 的作用域
    // 该 StrongClass 对象的弱引用计数值 == 0
}

// 测试函数, 测试强指针和弱指针的使用情景
void TestWeakClass(WeakClass* pWeakClass)
{
    //将 WeakClass 对象赋值给弱指针 wpOut
    wp<WeakClass> wpOut = pWeakClass;

    // 打印出该WeakClass对象的强引用计数值 == 0 弱引用计数值 == 1
    pWeakClass->printRefCount();

    {
        // 将该WeakClass对象赋值给 强指针spInner
        sp<WeakClass> spInner = pWeakClass;

        // 打印出该WeakClass对象的强引用计数值 == 1 弱引用计数值 == 2
        pWeakClass->printRefCount();
    }
    // 该WeakClass对象的生命周期同时受强引用计数和弱引用计数的影响，所以 该WeakClass对象不会被释放

    // 已经超出了强指针spInner的作用域，所以 该WeakClass对象的强引用计数值 == 0 弱引用计数值==1
	pWeakClass->printRefCount();

    // 将弱指针wpOut升级为强指针，
    // 因为弱指针wpOut所引用的WeakClass对象存在，所以 弱指针wpOut成功升级为强指针spOut， 
    sp<WeakClass> spOut = wpOut.promote();

    // 获得的强指针spOut所引用的对象地址 != 0，
    // 并且该WeakClass对象的强引用计数值==1  弱引用计数值 == 2
	printf("spOut: %p.\n", spOut.get());

    // 当TestWeakClass函数返回，因为超出了弱指针wpOut和强指针spOut的作用域, 
    //所以，该WeakClass对象的强引用计数值和弱引用计数值 == 0， 
    // 可以看 WeakClass类的析构函数中的日志输出来确认
}

// 测试函数, 测试强指针和弱指针的使用情景
void TestForeverClass(ForeverClass* pForeverClass)
{
    // 将ForeverClass对象赋值给弱指针wpOut
	wp<ForeverClass> wpOut = pForeverClass;
    // 打印出该ForeverClass对象的强引用计数值==0 弱引用计数值==1
    pForeverClass->printRefCount();

    {
        // 将该ForeverClass对象赋值给 强指针spInner
        sp<ForeverClass> spInner = pForeverClass;
        // 打印出该ForeverClass对象的强引用计数值==1 弱引用计数值== 2
        pForeverClass->printRefCount();
    }

    // 当TestForeverClass函数返回时，因为 超出了弱指针wpOut和强指针spInner的作用域，
    // 所以该ForeverClass对象的强引用计数值和弱引用计数值都 == 0。 
    // 但是该ForeverClass对象的生命周期不受强引用计数和弱引用计数的影响，所以它不会被自动释放，
    // 可以看 WeakClass 类的析构函数有没有日志输出来确认
}

int main(int argc, char** argv) 
{
	printf("Test Strong Class: \n");

    // 受到引用计数的影响, 会被自动释放
	StrongClass* pStrongClass = new StrongClass();
	TestStrongClass(pStrongClass);

	printf("\nTest Weak Class: \n");

    // 受到引用计数的影响, 会被自动释放
	WeakClass* pWeakClass = new WeakClass();
    TestWeakClass(pWeakClass);

	printf("\nTest Froever Class: \n");

    // 不受引用计数的影响
	ForeverClass* pForeverClass = new ForeverClass();
    TestForeverClass(pForeverClass);

	pForeverClass->printRefCount();

    // 手动地释放该对象
	delete pForeverClass;

	return 0;
}
