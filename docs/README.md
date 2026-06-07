# Documentation Hub

## 入门指南（按推荐阅读顺序）

| 序号 | 文档 | 内容 | 预计时间 |
|------|------|------|----------|
| 1 | [GNU 工具链指南](00_toolchain.md) | arm-none-eabi-gcc, as, ld, objdump, gdb... | 30 min |
| 2 | [QEMU 指南](06_qemu.md) | QEMU 安装、运行、调试、快捷键 | 20 min |
| 3 | [ELF 格式指南](01_elf_format.md) | ELF 结构、section、VMA/LMA、内存布局 | 20 min |
| 4 | [链接脚本指南](03_linker_script.md) | MEMORY、SECTIONS、栈/堆定义 | 25 min |
| 5 | [汇编指南](02_assembly.md) | ARMv6-M 指令集、寄存器、AAPCS、Thumb-1 约束 | 45 min |
| 6 | [newlib-nano 指南](04_newlib_nano.md) | syscalls、printf、malloc 调用链、空间优化 | 25 min |
| 7 | [FreeRTOS 指南](05_freertos.md) | 任务、调度器、PendSV 上下文切换、内存预算 | 40 min |

---

## 各阶段文档

| 阶段 | README | 主题 |
|------|--------|------|
| Lesson 1 | [README](../lesson_01_bare_metal/README.md) | 裸机 Hello World |
| Lesson 2 | [README](../lesson_02_assembly/README.md) | ARMv6-M 汇编语言 |
| Lesson 3 | [README](../lesson_03_c_asm_integration/README.md) | C 与汇编混合编程 |
| Lesson 4 | [README](../lesson_04_exceptions_interrupts/README.md) | 异常与中断处理 |
| Lesson 5 | [README](../lesson_05_peripherals/README.md) | 外设与硬件编程 |
| Lesson 6 | [README](../lesson_06_newlib/README.md) | C 标准库集成 |
| Lesson 7 | [README](../lesson_07_freertos/README.md) | FreeRTOS 多任务系统 |
| Lesson 8 | [README](../lesson_08_advanced/README.md) | 高级主题与总结 |

---

## 项目相关

| 文档 | 内容 |
|------|------|
| [开源发布计划](OPEN_SOURCE_PLAN.md) | 仓库名、发布策略、每课 commit 模板、Release 模板 |

## 外部参考资源

| 资源 | 链接 |
|------|------|
| ARMv6-M Architecture Reference Manual | https://developer.arm.com/documentation/ddi0419/ |
| Cortex-M0 Technical Reference Manual | https://developer.arm.com/documentation/ddi0484/ |
| nRF51822 Product Specification | https://infocenter.nordicsemi.com/pdf/nRF51822_PS_v3.3.pdf |
| ARM Semihosting Specification | https://developer.arm.com/documentation/100863/ |
| GNU Linker Manual | https://sourceware.org/binutils/docs/ld/Scripts.html |
| FreeRTOS Documentation | https://www.freertos.org/Documentation/RTOS_book.html |
| newlib Documentation | https://sourceware.org/newlib/ |
| QEMU ARM System Emulation | https://www.qemu.org/docs/master/system/arm/microbit.html |
| GCC ARM Options | https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html |
