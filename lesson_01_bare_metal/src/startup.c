/*
 * =============================================================================
 * 启动代码 -- ARM Cortex-M0 最小启动程序
 * =============================================================================
 *
 * 本文件负责:
 * 1. 定义中断向量表 (vector table)
 * 2. 实现复位处理函数 (reset_handler)
 * 3. 将 .data 段从 Flash 复制到 RAM
 * 4. 清零 .bss 段
 * 5. 调用 main() 函数
 *
 * Cortex-M0 启动流程:
 *   - 硬件复位后, CPU 从地址 0x00000000 读取 SP (向量表[0])
 *   - 从地址 0x00000004 读取 PC (向量表[1], 即 reset_handler)
 *   - CPU 自动初始化 SP, 然后跳转到 reset_handler 执行
 * =============================================================================
 */

#include <stdint.h>

/* --------------------------------------------------------------------------
 * 链接脚本中定义的符号
 * 注意: 这些是符号本身, 不是指针变量!
 * &__xxx 获取符号的地址值 (即实际的内存地址)
 * -------------------------------------------------------------------------- */
extern uint8_t __stack_top;       /* 栈顶地址 (RAM 末尾) */
extern uint8_t __data_start;      /* .data 段 RAM 起始 */
extern uint8_t __data_end;        /* .data 段 RAM 结束 */
extern uint8_t __data_load_start; /* .data 段 Flash 加载地址 */
extern uint8_t __bss_start;       /* .bss 段起始 */
extern uint8_t __bss_end;         /* .bss 段结束 */

/* --------------------------------------------------------------------------
 * 前置声明
 * -------------------------------------------------------------------------- */
int main(void);                      /* 主程序 (在 main.c 中定义) */

/* --------------------------------------------------------------------------
 * 弱符号声明: 用户可以在自己的代码中覆盖这些处理函数
 * 使用 __attribute__((weak)) 使链接器优先使用非弱符号
 * -------------------------------------------------------------------------- */
void reset_handler(void);           /* 复位处理函数 */
void default_handler(void);         /* 默认异常处理函数 (死循环) */
void nmi_handler(void) __attribute__((weak, alias("default_handler")));
void hardfault_handler(void) __attribute__((weak, alias("default_handler")));
void svcall_handler(void) __attribute__((weak, alias("default_handler")));
void pendsv_handler(void) __attribute__((weak, alias("default_handler")));
void systick_handler(void) __attribute__((weak, alias("default_handler")));

/* --------------------------------------------------------------------------
 * 中断向量表
 * 必须放置在 .vector_table 段, 确保地址 0x00000000
 *
 * 向量表格式 (Cortex-M0):
 *   偏移     内容                        异常编号
 *   0x00     SP_main (初始栈指针)        -
 *   0x04     Reset                         -3
 *   0x08     NMI                           -2
 *   0x0C     HardFault                     -1
 *   0x10-0x28 保留 (M0 不支持 MemManage/BusFault/UsageFault)
 *   0x2C     SVCall                        11
 *   0x30-0x34 保留 (M0 不支持 DebugMonitor)
 *   0x38     PendSV                        14
 *   0x3C     SysTick                       15
 *   0x40+    外设中断 (IRQ0-IRQ31)         16-47
 *
 * M0 的特点是:
 *   - 没有 MemManage/BusFault/UsageFault (只有 HardFault)
 *   - 没有 DebugMonitor
 *   - 保留条目必须填写 0 或 default_handler
 * -------------------------------------------------------------------------- */
__attribute__((section(".vector_table"), used, aligned(4)))
const void *vector_table[] = {
    /* 0x00: 初始 SP 值 */
    &__stack_top,

    /* 0x04-0x0C: 系统异常 */
    reset_handler,           /* 0x04: Reset     (-3) */
    nmi_handler,             /* 0x08: NMI       (-2) */
    hardfault_handler,       /* 0x0C: HardFault (-1) */

    /* 0x10-0x28: 保留槽位 -- M0 无 MemManage/BusFault/UsageFault */
    0,
    0,
    0,
    0,
    0,
    0,

    /* 0x2C: SVCall (11) -- FreeRTOS 用它启动第一个任务 */
    svcall_handler,

    /* 0x30: 保留 -- M0 无 DebugMonitor */
    0,

    /* 0x34: 保留 */
    0,

    /* 0x38: PendSV (14) -- FreeRTOS 用它做上下文切换 */
    pendsv_handler,

    /* 0x3C: SysTick (15) -- 系统节拍定时器 */
    systick_handler,

    /* 0x40+: 外设中断 (IRQ0-IRQ31) -- microbit 使用 nRF51 外设 */
    /* 为简洁起见, 这里只列出 nRF51 关键外设中断, 其余用 0 填充 */
    /* UART0_IRQ (IRQ2) 在偏移 0x48 */
    0,                          /* 0x40: IRQ0  - POWER_CLOCK */
    0,                          /* 0x44: IRQ1  - RADIO */
    0,                          /* 0x48: IRQ2  - UART0 */
    0,                          /* 0x4C: IRQ3  - SPI0/TWI0 */
    0,                          /* 0x50: IRQ4  - SPI1/TWI1 */
    0,                          /* 0x54: IRQ5  - (保留) */
    0,                          /* 0x58: IRQ6  - GPIOTE */
    0,                          /* 0x5C: IRQ7  - ADC */
    0,                          /* 0x60: IRQ8  - TIMER0 */
    0,                          /* 0x64: IRQ9  - TIMER1 */
    0,                          /* 0x68: IRQ10 - TIMER2 */
    0,                          /* 0x6C: IRQ11 - RTC0 */
    0,                          /* 0x70: IRQ12 - TEMP */
    0,                          /* 0x74: IRQ13 - RNG */
    0,                          /* 0x78: IRQ14 - ECB */
    0,                          /* 0x7C: IRQ15 - CCM/AAR */
    0,                          /* 0x80: IRQ16 - WDT */
    0,                          /* 0x84: IRQ17 - RTC1 */
    0,                          /* 0x88: IRQ18 - QDEC */
    0,                          /* 0x8C: IRQ19 - LPCOMP */
    0,                          /* 0x90: IRQ20 - SWI0 */
    0,                          /* 0x94: IRQ21 - SWI1 */
    0,                          /* 0x98: IRQ22 - SWI2 */
    0,                          /* 0x9C: IRQ23 - SWI3 */
    0,                          /* 0xA0: IRQ24 - SWI4 */
    0,                          /* 0xA4: IRQ25 - SWI5 */
    0,                          /* 0xA8: IRQ26 - (保留) */
    0,                          /* 0xAC: IRQ27 - (保留) */
    0,                          /* 0xB0: IRQ28 - (保留) */
    0,                          /* 0xB4: IRQ29 - (保留) */
    0,                          /* 0xB8: IRQ30 - (保留) */
    0,                          /* 0xBC: IRQ31 - (保留) */
};

/* --------------------------------------------------------------------------
 * reset_handler -- 系统上电/复位后执行的第一个函数
 * --------------------------------------------------------------------------
 * 职责:
 * 1. 复制 .data 段 (已初始化变量) 从 Flash 到 RAM
 * 2. 清零 .bss 段 (未初始化变量)
 * 3. 调用 main() 函数
 * 4. 如果 main() 返回, 进入安全死循环
 * -------------------------------------------------------------------------- */
void reset_handler(void)
{
    uint8_t *src;
    uint8_t *dst;
    uint32_t len;

    /* 1. 将 .data 段从 Flash (LMA) 复制到 RAM (VMA)
     * __data_load_start: Flash 中的源地址
     * __data_start:      RAM 中的目标地址
     * __data_end:        RAM 中结束地址
     */
    src = &__data_load_start;
    dst = &__data_start;
    len = (uint32_t)(&__data_end - &__data_start);
    for (uint32_t i = 0; i < len; i++)
    {
        dst[i] = src[i];
    }

    /* 2. 清零 .bss 段
     * C 标准规定未初始化的静态/全局变量必须为 0
     */
    dst = &__bss_start;
    len = (uint32_t)(&__bss_end - &__bss_start);
    for (uint32_t i = 0; i < len; i++)
    {
        dst[i] = 0;
    }

    /* 3. 跳转到主程序
     * 编译时已确保 main 返回类型为 int
     */
    main();

    /* 4. main() 不应该返回. 如果返回, 进入死循环.
     * 在嵌入式系统中, main() 通常包含一个无限循环.
     */
    while (1)
    {
        /* 等待看门狗复位或调试器介入 */
    }
}

/* --------------------------------------------------------------------------
 * default_handler -- 默认异常/中断处理函数
 * --------------------------------------------------------------------------
 * 对于未配置的异常, 进入死循环.
 * 在调试阶段, 可以通过检查此函数来判断是否意外进入了未配置的中断.
 * -------------------------------------------------------------------------- */
void default_handler(void)
{
    while (1)
    {
        /* 死循环 -- 等待调试器介入
         * 在真实硬件上, 看门狗定时器会复位系统
         */
    }
}
