# Lesson 13: Interrupt Priority & Preemption

## 学习目标

- 掌握 NVIC 优先级机制的完整原理
- 理解中断抢占（Preemption）、尾链（Tail-Chaining）、迟到（Late-Arriving）
- 学会配置外设中断和系统异常的优先级
- 掌握 PRIMASK 临界区保护的正确模式
- 理解优先级反转及优先级继承解决方案

## 文件结构

```
lesson_13_interrupt_priority/
├── CMakeLists.txt
├── linker/microbit.ld
└── src/
    ├── startup.S    # 带 SWI0-SWI3 中断处理函数
    └── main.c       # 6 个演示模块
```

## 演示内容

| 模块 | 内容 |
|------|------|
| 1 | NVIC 寄存器探索 — ISER/ICER/ISPR/ICPR/IPR/SHPR/PRIMASK |
| 2 | 优先级配置 — SWI0-3 设置为不同优先级 (3→0) |
| 3 | 软件中断触发 — NVIC_ISPR 触发悬起中断 |
| 4 | 临界区保护 — PRIMASK/CPSID/CPSIE 的正确模式 |
| 5 | 优先级反转 — 场景分析 + 优先级继承解决方案 |
| 6 | ISR 设计最佳实践 — 短 ISR / 优先级分配 / 延时预算 |

## 关键知识点

### M0 优先级系统

```
4 级优先级 (2 bits):
  0x00 = 最高优先级 (0)
  0x40 = 高优先级   (1)
  0x80 = 中优先级   (2)
  0xC0 = 最低优先级 (3)

抢占规则:
  - 数值小的可以抢占数值大的
  - 同优先级按到达顺序处理
  - NMI 和 HardFault 可以抢占一切 (负优先级)
```

### 中断延迟分析

```
最坏情况中断延迟 =
  硬件固定延迟 (16 cycles)
  + 最长临界区 (关中断时间)
  + 内存访问等待状态
```

### 临界区正确模式

```c
/* 正确: 保存并恢复之前的状态 */
uint32_t saved = __get_PRIMASK();
__disable_irq();
// ... critical work ...
__set_PRIMASK(saved);

/* 错误: 盲目开中断! */
__disable_irq();
// ... critical work ...
__enable_irq();  // ← 如果进入临界区前中断本来就是关的?
                  //   这会意外打开中断!
```

## QEMU 限制

- NVIC 寄存器读写正常
- ISPR 写入可能不触发实际中断（QEMU microbit 限制）
- 优先级配置和寄存器操作逻辑完全正确

## 相关文档

- [Cortex-M0 架构详解](../docs/07_cortex_m0_architecture.md) — NVIC/抢占/尾链/迟到
- [FreeRTOS 指南](../docs/05_freertos.md) — PendSV 优先级设计
- [Lesson 4: 异常与中断](../lesson_04_exceptions_interrupts/) — SysTick/PendSV 基础
