# Lesson 4: Exception & Interrupt Handling

## 学习目标

- 理解 Cortex-M0 异常模型（异常入口/退出、硬件压栈/出栈）
- 掌握 NVIC（嵌套向量中断控制器）编程
- 配置 SysTick 系统节拍定时器
- 理解 PendSV 在 RTOS 上下文切换中的作用
- 编写 HardFault 分析器（解码异常栈帧）

## 文件结构

```
lesson_04_exceptions_interrupts/
├── CMakeLists.txt
├── linker/microbit.ld
└── src/
    ├── startup.S              # 向量表 + 真实异常处理函数
    ├── main.c                 # 主程序
    ├── semihosting.c / .h     # semihosting 辅助
    ├── systick_demo.c         # SysTick 配置与演示
    ├── pendsv_demo.c          # PendSV 触发与优先级设置
    └── hardfault_analyzer.c   # HardFault 栈帧解码器
```

## 演示内容

| 模块 | 内容 |
|------|------|
| SysTick | 轮询模式（COUNTFLAG）、中断配置、节拍计数 |
| PendSV | 优先级设置（SHPR3）、ICSR 触发、FreeRTOS 上下文切换理论 |
| HardFault | 异常栈帧结构、寄存器恢复、故障 PC 诊断 |

## 关键知识点

### 异常入口（硬件自动操作）

```
1. 压栈 8 个寄存器（从高到低）：
   xPSR → PC → LR → R12 → R3 → R2 → R1 → R0

2. 从向量表加载异常处理函数地址 → PC

3. 更新 LR = EXC_RETURN 值：
   0xFFFFFFF9 = 返回 Thread 模式，使用 MSP
   0xFFFFFFFD = 返回 Thread 模式，使用 PSP
   0xFFFFFFF1 = 返回 Handler 模式（嵌套异常）
```

### 异常退出

```asm
bx  lr     @ LR = EXC_RETURN → 硬件自动出栈 8 个寄存器
```

### 异常优先级（M0 仅 4 级）

| 优先级值 | 优先级 | 典型用途 |
|----------|--------|----------|
| 0x00 | 最高 | HardFault, NMI |
| 0x40 | 高 | 时间关键中断 |
| 0x80 | 中 | 普通外设中断 |
| 0xC0 | 最低 | PendSV（上下文切换） |

### SysTick 寄存器

| 寄存器 | 地址 | 作用 |
|--------|------|------|
| CSR | 0xE000E010 | bit0=ENABLE, bit1=TICKINT, bit2=CLKSOURCE |
| RVR | 0xE000E014 | 重载值（24 位） |
| CVR | 0xE000E018 | 当前值（写任意值清零） |

### QEMU 限制

- SysTick 计数器运行（COUNTFLAG 可用），但中断不触发
- PendSV 触发导致 HardFault（QEMU microbit 模型问题）
- 代码逻辑对真实硬件完全正确

## 相关文档

- [FreeRTOS 指南](../docs/05_freertos.md)（PendSV 的详细应用）
- [QEMU 指南](../docs/06_qemu.md)（已知限制）
