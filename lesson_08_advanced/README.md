# Lesson 8: Advanced Topics & Final Demo

## 学习目标

- 掌握 GDB + QEMU 远程调试
- 理解编译器优化级别对代码的影响
- 了解 M0 低功耗设计（WFI/WFE/Sleep-on-Exit）
- 规划从仿真到真实硬件的迁移路径
- 回顾完整学习路线

## 文件结构

```
lesson_08_advanced/
├── CMakeLists.txt
├── linker/microbit.ld
└── src/
    ├── startup.S
    └── main.c                  # 综合演示
```

## 演示内容

| 演示 | 内容 |
|------|------|
| GDB 调试 | QEMU GDB server 连接方法、常用调试命令 |
| 优化分析 | -O0 vs -Os vs -O2 对 ARMv6-M 代码的影响 |
| 低功耗 | WFI/WFE 指令、Sleep-on-Exit、FreeRTOS Tickless Idle |
| 迁移指南 | QEMU → 真实 microbit 的步骤和差异 |
| 学习总结 | 8 个阶段的核心技能回顾 |

## 调试流程

### Terminal 1 — QEMU

```bash
qemu-system-arm -M microbit -kernel lesson_08.elf \
    -semihosting -nographic -s -S
```

### Terminal 2 — GDB

```bash
arm-none-eabi-gdb lesson_08.elf -x ../../scripts/debug.gdb
```

## 优化级别对比

| 级别 | Flash 用量 | 速度 | 调试体验 |
|------|-----------|------|----------|
| `-O0` | 100% | 慢 | 优秀（变量可观察） |
| `-Og` | ~80% | 中 | 良好（调试优化） |
| `-Os` | ~40% | 快 | 差（变量被优化掉） |
| `-O2` | ~50% | 最快 | 差 |

## 迁移到真实硬件 checklist

```
□ 硬件准备
  □ BBC micro:bit 开发板
  □ USB 数据线
  □ 串口终端软件（PuTTY / minicom）

□ 固件准备
  □ arm-none-eabi-objcopy -O ihex firmware.elf firmware.hex
  □ 将 firmware.hex 复制到 MICROBIT USB 驱动器

□ 调试准备
  □ 安装 OpenOCD
  □ SWD 调试器（或使用 microbit 的 DAPLink）
  □ 串口终端：115200 baud, 8N1

□ 代码适配
  □ UART 驱动无需修改（nRF51 原生驱动）
  □ GPIO/LED 引脚映射确认
  □ 移除 semihosting 调用（或保留，通过调试器使用）
```

## 相关文档

- [QEMU 指南](../docs/06_qemu.md)（GDB 调试章节）
- [GNU 工具链指南](../docs/00_toolchain.md)（优化选项）
- [ARM Cortex-M0 汇编指南](../docs/02_assembly.md)（WFI/WFE 指令）
