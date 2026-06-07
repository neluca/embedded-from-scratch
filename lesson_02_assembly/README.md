# Lesson 2: ARM Cortex-M0 Assembly Language

## 学习目标

- 掌握 ARMv6-M（Thumb-1）指令集
- 理解寄存器组和程序状态寄存器（xPSR）
- 掌握内存访问、分支、栈操作
- 实现常见算法（memcpy, strlen, fibonacci, bubble sort）

## 文件结构

```
lesson_02_assembly/
├── CMakeLists.txt               # 每个 .S 文件构建为独立 .elf
├── linker/
│   └── microbit.ld
└── src/
    ├── 01_registers_and_mov.S   # 寄存器 + 数据处理指令
    ├── 02_memory_access.S       # LDR/STR, LDM/STM, PUSH/POP
    ├── 03_branch_and_loop.S     # B/BL/BX, 条件分支, 循环
    ├── 04_stack_and_call.S      # 栈帧, AAPCS, 递归
    └── 05_exercises.S           # 综合练习（memcpy, strlen, 排序等）
```

## 构建与运行

```bash
cmake -B build/Debug -S . -DCMAKE_TOOLCHAIN_FILE=../../cmake/arm-none-eabi-gcc.cmake
cmake --build build/Debug

# 运行单个练习
qemu-system-arm -M microbit -kernel build/Debug/01_registers_and_mov.elf \
    -semihosting -nographic
```

## 练习内容

| 练习 | 内容 | 关键指令 |
|------|------|----------|
| 01 | 寄存器与数据处理 | MOVS, ADDS, SUBS, ANDS, ORRS, LSLS, LSRS, CMP |
| 02 | 内存访问 | LDR/STR, LDRB/STRB, LDRH/STRH, LDMIA/STMIA, PUSH/POP |
| 03 | 分支与循环 | B, BL, BX, BLX, B<cc>, 循环模式 |
| 04 | 栈与子程序 | PUSH {lr}/POP {pc}, 叶函数, 递归, AAPCS 传参 |
| 05 | 综合练习 | memcpy, strlen, fibonacci, bubble_sort 的汇编实现 |

## 关键知识点

### Thumb-1 约束

M0 仅支持 Thumb-1（ARMv6-M），与 Thumb-2（ARMv7-M）有重要区别：

| 特性 | Thumb-1 (M0) | Thumb-2 (M3/M4) |
|------|---------------|------------------|
| ALU 操作数 | **2 操作数** (Rd,Rm) | 3 操作数 (Rd,Rn,Rm) |
| 寄存器 | **仅低寄存器 R0-R7** | 所有寄存器 R0-R12 |
| 硬件除法 | **无** (软件模拟) | 有 (UDIV/SDIV) |
| IT 块 | 部分 GAS 版本不支持 | 完全支持 |
| CBZ/CBNZ | 部分 GAS 版本不支持 | 完全支持 |

### 汇编文件扩展名

- `.S`（大写）→ 自动经过 C 预处理器，支持 `#define`, `#include`, `/* 注释 */`
- `.s`（小写）→ 纯汇编，不经过预处理器

## 相关文档

- [ARM Cortex-M0 汇编指南](../docs/02_assembly.md)
- [QEMU 指南](../docs/06_qemu.md)
