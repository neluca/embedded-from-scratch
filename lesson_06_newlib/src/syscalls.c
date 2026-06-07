/*
 * =============================================================================
 * syscalls.c -- 系统调用实现 (C 运行时与硬件的桥梁)
 * =============================================================================
 *
 * C 标准库函数 (printf, malloc, fopen 等) 最终依赖一组底层系统调用.
 * 在嵌入式系统中, 我们必须自己实现这些调用.
 *
 * 实现方式有两种:
 *   1. Semihosting 后端 -- 通过 BKPT 0xAB 调用 QEMU/调试器
 *   2. 硬件后端 -- 直接操作 UART, 文件系统等外设
 *
 * 本文件实现了两套后端, 通过宏 SEMIHOSTING_BACKEND 切换.
 *
 * 关键系统调用:
 *   _sbrk:   堆内存分配 (malloc/free/realloc 依赖)
 *   _write:  文件写 (printf/fprintf 最终调用)
 *   _read:   文件读 (scanf/fgets 依赖)
 *   _exit:   程序退出
 *   _close:  关闭文件
 *   _fstat:  文件状态
 *   _isatty: 判断是否为终端
 *   _lseek:  文件定位
 *   _open:   打开文件
 *
 * 堆管理 (_sbrk):
 *   堆从 __heap_start (链接脚本定义) 开始, 向高地址增长.
 *   栈从 __stack_top 向低地址增长.
 *   _sbrk(incr) 分配 incr 字节的堆空间, 返回新空间的起始地址.
 *   malloc 内部调用 _sbrk 来获取内存块.
 * =============================================================================
 */

#include "semihosting.h"
#include <stdint.h>
#include <sys/stat.h>

/* 使用 Semihosting 后端 (通过 QEMU) */
#define SEMIHOSTING_BACKEND 1

/* =========================================================================
 * 链接脚本定义的符号
 * ========================================================================= */
extern uint8_t __heap_start; /* 堆起始地址 */
extern uint8_t __heap_end;   /* 堆结束地址 (预留栈空间后) */

/* 堆管理: 当前堆顶指针 (由 _sbrk 维护) */
static uint8_t *heap_top = &__heap_start;

/*
 * =========================================================================
 * _sbrk -- 增加堆空间 (malloc 的核心依赖)
 * =========================================================================
 *
 * @param incr: 请求的字节数
 * @return: 新分配空间的起始地址 (之前堆顶的值)
 *          -1 表示堆空间不足
 *
 * 嵌入式堆是简单的线性增长模型:
 *   heap_top 从 __heap_start 开始, 每次 _sbrk(n) 增加 n 字节.
 *   malloc 在堆空间内管理已分配/空闲的块.
 *   当 heap_top + incr > __heap_end 时, 堆溢出.
 *
 * 注意: 嵌入式堆通常不缩小 (没有 brk 的减少操作).
 *       一般嵌入式程序在启动时 malloc, 不再 free.
 */
void *_sbrk(int incr)
{
    uint8_t *prev_top = heap_top;

    /* 检查堆溢出: 新的 heap_top 不能超过 __heap_end */
    if (heap_top + incr > &__heap_end)
    {
        /* 堆空间不足!
         * 在真实系统中应该:
         *   1. 记录错误
         *   2. 尝试垃圾回收 (如果有)
         *   3. 返回 -1 (ENOMEM)
         */
        return (void *)-1;
    }

    heap_top += incr;

    /* 注意: 新分配的内存不会自动清零 (malloc 自己处理) */
    return (void *)prev_top;
}

/*
 * =========================================================================
 * _write -- 写数据到文件描述符
 * =========================================================================
 *
 * @param fd:   文件描述符 (1=stdout, 2=stderr)
 * @param buf:  数据缓冲区
 * @param count: 字节数
 * @return: 实际写入的字节数, -1 表示错误
 *
 * printf/fprintf 最终调用 _write(1, ...) 输出到 stdout.
 * 在此实现中, _write(1) 通过 semihosting 输出到 QEMU 控制台.
 * 在真实硬件上, 这里会调用 UART 驱动.
 */
int _write(int fd, const char *buf, int count)
{
#if SEMIHOSTING_BACKEND
    if (fd == 1 || fd == 2)
    {
        /* stdout/stderr: 通过 semihosting 逐字符输出
         * 注意: SYS_WRITE0 输出 NUL 结尾字符串, 但我们的 buf 可能包含 NUL.
         * 所以不能直接使用 SYS_WRITE0. 使用 SYS_WRITEC 逐字符输出. */
        for (int i = 0; i < count; i++)
        {
            /* 使用 SYS_WRITEC 输出单个字符 */
            register int r0 __asm__("r0") = 0x03; /* SYS_WRITEC */
            register char *r1 __asm__("r1") = (char *)&buf[i];
            __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
        }
        return count;
    }
#endif

    /* 真实硬件: 这里调用 UART 驱动输出 */
    /* for (int i = 0; i < count; i++) uart_putc(buf[i]); */

    return -1; /* 不支持的文件描述符 */
}

/*
 * =========================================================================
 * _read -- 从文件描述符读取数据
 * =========================================================================
 *
 * @param fd:   文件描述符 (0=stdin)
 * @param buf:  数据缓冲区
 * @param count: 最大读取字节数
 * @return: 实际读取的字节数, 0=EOF, -1=错误
 *
 * scanf/fgets 调用 _read(0, ...) 从 stdin 读取.
 * semihosting SYS_READ 从 QEMU 控制台读取.
 */
int _read(int fd, char *buf, int count)
{
#if SEMIHOSTING_BACKEND
    if (fd == 0)
    {
        /* SYS_READ 参数块: [fd, buf, count] */
        uint32_t params[3] = { 0, (uint32_t)buf, (uint32_t)count };
        register int r0 __asm__("r0") = 0x06; /* SYS_READ */
        register void *r1 __asm__("r1") = params;
        __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
        /* 返回值: 0=EOF, >0=读取字节数, <0=错误 */
        return r0;
    }
#endif
    return -1;
}

/*
 * =========================================================================
 * _exit -- 程序退出
 * =========================================================================
 *
 * exit() / abort() 最终调用 _exit().
 * 在嵌入式系统中, 不能真正 "退出".
 * 常见做法:
 *   1. 进入死循环 (等待看门狗复位)
 *   2. 执行软件复位
 *   3. 通过 semihosting 通知调试器
 */
void _exit(int status)
{
#if SEMIHOSTING_BACKEND
    /* SYS_EXIT: 通知 QEMU 程序已退出 */
    register int r0 __asm__("r0") = 0x18; /* SYS_EXIT */
    register int *r1 __asm__("r1") = &status;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
#endif

    /* 如果 semihosting 不可用, 进入死循环 */
    while (1)
    {
    }
}

/*
 * =========================================================================
 * _close -- 关闭文件
 * =========================================================================
 */
int _close(int fd)
{
#if SEMIHOSTING_BACKEND
    register int r0 __asm__("r0") = 0x02; /* SYS_CLOSE */
    register int *r1 __asm__("r1") = &fd;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
    return r0;
#endif
    return -1;
}

/*
 * =========================================================================
 * _lseek -- 文件定位
 * =========================================================================
 */
int _lseek(int fd, int offset, int whence)
{
#if SEMIHOSTING_BACKEND
    uint32_t params[3] = { (uint32_t)fd, (uint32_t)offset, (uint32_t)whence };
    register int r0 __asm__("r0") = 0x0A; /* SYS_SEEK (semihosting) */
    register void *r1 __asm__("r1") = params;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
    return r0;
#endif
    return -1;
}

/*
 * =========================================================================
 * _fstat -- 获取文件状态
 * =========================================================================
 */
int _fstat(int fd, struct stat *st)
{
    st->st_mode = S_IFCHR; /* 字符设备 (控制台) */
    return 0;
}

/*
 * =========================================================================
 * _isatty -- 判断文件描述符是否为终端
 * =========================================================================
 */
int _isatty(int fd)
{
    return (fd == 0 || fd == 1 || fd == 2); /* stdin/stdout/stderr */
}

/*
 * =========================================================================
 * _open -- 打开文件
 * =========================================================================
 * semihosting SYS_OPEN 参数: [filename_ptr, mode, name_length]
 */
int _open(const char *path, int flags, ...)
{
#if SEMIHOSTING_BACKEND
    /* 计算文件名字符串长度 */
    int name_len = 0;
    while (path[name_len])
    {
        name_len++;
    }

    uint32_t params[3] = { (uint32_t)path, (uint32_t)flags, (uint32_t)name_len };
    register int r0 __asm__("r0") = 0x01; /* SYS_OPEN */
    register void *r1 __asm__("r1") = params;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
    return r0;
#endif
    return -1;
}
