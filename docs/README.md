# Documentation Hub

## 入门指南（按推荐阅读顺序）

| 序号 | 文档 | 内容 | 预计时间 |
|------|------|------|----------|
| 1 | [GNU 工具链指南](00_toolchain.md) | arm-none-eabi-gcc, as, ld, objdump, gdb... | 30 min |
| 2 | [QEMU 指南](06_qemu.md) | QEMU 安装、运行、调试、快捷键、semihosting 原理 | 20 min |
| 3 | [ELF 格式指南](01_elf_format.md) | ELF 结构、section、VMA/LMA、内存布局 | 20 min |
| 4 | [链接脚本指南](03_linker_script.md) | MEMORY、SECTIONS、栈/堆定义、调试链接问题 | 25 min |
| 5 | [汇编指南](02_assembly.md) | ARMv6-M 指令集（56 条）、寄存器、AAPCS、Thumb-1 约束 | 45 min |

## 架构与系统设计

| 序号 | 文档 | 内容 | 预计时间 |
|------|------|------|----------|
| 6 | [Cortex-M0 架构详解](07_cortex_m0_architecture.md) | 程序员模型、内存映射、异常模型、NVIC、SCB、SysTick、M0 限制速查 | 35 min |
| 7 | [newlib-nano 指南](04_newlib_nano.md) | syscalls、printf/malloc 调用链、空间优化、堆管理 | 25 min |
| 8 | [FreeRTOS 指南](05_freertos.md) | 任务、调度器、PendSV 上下文切换、内存预算、API 速查 | 40 min |
| 9 | [Bootloader 设计指南](08_bootloader_design.md) | Flash 分区、跳转机制、固件验证、故障安全更新、M0 VTOR 限制 | 30 min |

---

## 各阶段文档

| 阶段 | README | 主题 | 相关参考文档 |
|------|--------|------|-------------|
| Lesson 1 | [README](../lesson_01_bare_metal/README.md) | 裸机 Hello World | 工具链、ELF、链接脚本 |
| Lesson 2 | [README](../lesson_02_assembly/README.md) | ARMv6-M 汇编语言 | 汇编指南、架构详解 |
| Lesson 3 | [README](../lesson_03_c_asm_integration/README.md) | C 与汇编混合编程 | 汇编指南 (AAPCS) |
| Lesson 4 | [README](../lesson_04_exceptions_interrupts/README.md) | 异常与中断处理 | 架构详解 (NVIC/SysTick/SCB) |
| Lesson 5 | [README](../lesson_05_peripherals/README.md) | 外设与硬件编程 | 架构详解 (内存映射) |
| Lesson 6 | [README](../lesson_06_newlib/README.md) | C 标准库集成 | newlib-nano 指南 |
| Lesson 7 | [README](../lesson_07_freertos/README.md) | FreeRTOS 多任务系统 | FreeRTOS 指南、架构详解 (PendSV) |
| Lesson 8 | [README](../lesson_08_advanced/README.md) | 高级主题与总结 | QEMU 指南 (GDB 调试) |
| Lesson 9 | [README](../lesson_09_bootloader/README.md) | Bootloader + Application | Bootloader 设计指南 |
| Lesson 10 | [README](../lesson_10_watchdog/README.md) | WDT 看门狗与系统可靠性 | 架构详解 (WDT 基地址) |
| Lesson 11 | [README](../lesson_11_error_handling/README.md) | 错误处理与防御性编程 | Bootloader 指南 (CRC 验证) |
| Lesson 12 | [README](../lesson_12_patterns/README.md) | 数据结构与通用设计模式 | — |

---

## 外部参考资源

| 资源 | 链接 |
|------|------|
| ARMv6-M Architecture Reference Manual | https://developer.arm.com/documentation/ddi0419/ |
| Cortex-M0 Technical Reference Manual | https://developer.arm.com/documentation/ddi0484/ |
| Cortex-M0 Devices Generic User Guide | https://developer.arm.com/documentation/dui0662/ |
| nRF51822 Product Specification | https://infocenter.nordicsemi.com/pdf/nRF51822_PS_v3.3.pdf |
| ARM Semihosting Specification | https://developer.arm.com/documentation/100863/ |
| GNU Linker Manual | https://sourceware.org/binutils/docs/ld/Scripts.html |
| FreeRTOS Documentation | https://www.freertos.org/Documentation/RTOS_book.html |
| newlib Documentation | https://sourceware.org/newlib/ |
| QEMU ARM System Emulation | https://www.qemu.org/docs/master/system/arm/microbit.html |
| GCC ARM Options | https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html |
| MCUboot (开源安全 Bootloader) | https://www.mcuboot.com/ |
