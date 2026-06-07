# Lesson 7: FreeRTOS on Cortex-M0

## 学习目标

- 理解 FreeRTOS 内核架构（任务、调度器、队列）
- 掌握 Cortex-M0 上 FreeRTOS 的移植（SVC、PendSV、SysTick）
- 创建多任务应用（Blinky、Sensor、Monitor）
- 使用 Queue 进行任务间通信
- 配置 FreeRTOSConfig.h 以适应 16KB RAM 约束

## 文件结构

```
lesson_07_freertos/
├── CMakeLists.txt
├── FreeRTOS-Kernel/                      # FreeRTOS V11.1.0 内核源码（git clone）
│   ├── tasks.c, queue.c, list.c, ...
│   └── portable/GCC/ARM_CM0/            # Cortex-M0 移植层
│       ├── port.c                        # 上下文切换、临界区
│       ├── portasm.c                     # SVC/PendSV 汇编处理函数
│       └── portmacro.h                   # 架构宏定义
├── inc/
│   └── FreeRTOSConfig.h                 # 内核配置
├── linker/microbit.ld
└── src/
    ├── startup.S                         # 向量表 → FreeRTOS 处理函数
    ├── main.c                            # 创建任务和队列
    ├── syscalls.c                        # memcpy/memset（FreeRTOS 依赖）
    ├── semihosting.c / .h
    ├── task_blinky.c                     # 心跳任务（演示 vTaskDelay）
    ├── task_sensor.c                     # 传感器任务（演示 xQueueSend）
    └── task_monitor.c                    # 监控任务（演示 xQueueReceive）
```

## 构建与运行

```bash
# FreeRTOS 内核作为 git submodule 管理
# 克隆项目时使用 --recurse-submodules 自动获取：
#   git clone --recurse-submodules https://github.com/neluca/embedded-from-scratch.git
# 如果已克隆但缺少子模块：
#   git submodule update --init --recursive

# 构建
cmake -B build/Debug -S . -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake
cmake --build build/Debug

# 运行（注意：QEMU microbit 不触发 SysTick 中断，调度器不工作）
qemu-system-arm -M microbit -kernel build/Debug/lesson_07_freertos.elf \
    -semihosting -nographic
```

## 任务设计

```mermaid
flowchart TD
    subgraph PRIO["任务优先级设计"]
        direction TB
        P3["优先级 3 — (空闲，未使用)"]
        P2["优先级 2 — Sensor<br>300ms 周期，生成模拟读数 -> 发送 Queue"]
        P1A["优先级 1 — Blinky<br>500ms 周期，输出心跳"]
        P1B["优先级 1 — Monitor<br>从 Queue 接收数据，计算平均值"]
        P0["优先级 0 — Idle<br>FreeRTOS 自动创建 (WFI 低功耗)"]
        P3 --> P2 --> P1A
        P2 --> P1B
        P1A --> P0
        P1B --> P0
    end
```

## 内存配置（16KB RAM）

```mermaid
flowchart TD
    subgraph MEM["内存配置 (16KB RAM)"]
        direction TB
        A["configTOTAL_HEAP_SIZE = 4096 B<br>(FreeRTOS 堆)"]
        B["Task Blinky  stack = 128 B"]
        C["Task Sensor  stack = 256 B"]
        D["Task Monitor stack = 256 B"]
        E["Idle task    stack = ~64 B"]
        F["System stack (MSP) = 512 B"]
        G[".data + .bss = ~200 B"]
        H["已用总计 = ~5.6 KB / 16 KB (35%)"]
        G --> A --> B --> C --> D --> E --> F --> H
    end
```

## 关键知识点

### FreeRTOS 启动流程

```mermaid
flowchart TD
    MAIN["main()"] --> QUEUE["xQueueCreate()<br>创建同步对象"]
    QUEUE --> TASK["xTaskCreate() x 3<br>创建任务"]
    TASK --> SCHED["vTaskStartScheduler()<br>启动调度器（从不返回）"]
    SCHED --> SELECT["选择最高优先级就绪任务"]
    SELECT --> INIT["初始化任务栈<br>(伪造异常帧)"]
    INIT --> SVC["SVC 指令 -> vPortSVCHandler"]
    SVC --> RESTORE["恢复第一个任务的上下文"]
    RESTORE --> TICK["SysTick -> PendSV 循环"]
    TICK --> SAVE["保存当前任务 R4-R11"]
    SAVE --> NEXT["选择下一个任务"]
    NEXT --> LOAD["恢复新任务 R4-R11"]
    LOAD --> TICK
```

### 上下文切换（PendSV）汇编伪代码

```asm
PendSV_Handler:
    cpsid   i                    @ 关中断
    mrs     r0, psp              @ 获取当前任务栈指针
    stmdb   r0!, {r4-r11}        @ 保存 R4-R11 到任务栈
    str     r0, [r1]             @ 更新 TCB 中的栈指针

    @ 选择下一个任务...

    ldr     r1, =pxCurrentTCB    @ 获取新 TCB
    ldr     r0, [r1]             @ 读取新任务栈指针
    ldmia   r0!, {r4-r11}        @ 恢复 R4-R11
    msr     psp, r0              @ 更新 PSP
    cpsie   i                    @ 开中断
    bx      lr                   @ 异常返回
```

### 为什么 PendSV 优先级最低？

```mermaid
sequenceDiagram
    participant HW as 硬件
    participant SYSTICK as SysTick (优先级 0x80)
    participant PENDSV as PendSV (优先级 0xC0)
    
    HW->>SYSTICK: SysTick 中断触发
    SYSTICK->>SYSTICK: 处理 tick 计数
    SYSTICK->>SYSTICK: 发现需要任务切换
    SYSTICK->>PENDSV: 置 PendSV 悬起
    SYSTICK->>HW: 退出 SysTick
    Note over PENDSV: PendSV 优先级最低<br/>等所有 ISR 完成后才执行
    PENDSV->>PENDSV: 执行上下文切换
    PENDSV->>HW: 返回被抢占的任务（或新任务）
```

> 如果直接在高优先级的 SysTick 中做上下文切换，会延迟其他同等优先级的中断。

### QEMU 限制与解决方案

QEMU microbit 模型有两个已知缺陷影响 FreeRTOS 运行：

| 缺陷 | 影响 | 状态 |
|------|------|------|
| SysTick 中断不触发 | 调度器无时钟源 | COUNTFLAG 轮询可替代 (见 idle hook) |
| PendSV 触发 HardFault | 任务无法切换 | 协作式调度可规避 (`configUSE_PREEMPTION=0`) |

**本课程在 `vApplicationIdleHook()` 中实现了 COUNTFLAG 轮询**
作为教学示例，展示如何在 QEMU 限制下理解 FreeRTOS tick 机制。

详细分析见 [QEMU 限制详解](../docs/11_qemu_limitations.md)。

> 在真实 BBC micro:bit 硬件上，SysTick 和 PendSV 完全正常。
> 代码无需修改即可在真实硬件上运行抢占式 FreeRTOS。

## 相关文档

- [FreeRTOS 指南](../docs/05_freertos.md)
- [ARM Cortex-M0 汇编指南](../docs/02_assembly.md)（PendSV 上下文切换）
