# Lesson 8: Advanced Topics & Production Deployment

## 学习目标

- 掌握 **GDB + QEMU 远程调试**的完整工作流（断点/监视点/栈回溯/寄存器检查）
- 理解 **编译器优化**对 ARMv6-M (Cortex-M0) 代码的实际影响
- 学习 **Cortex-M0 低功耗模式**（WFI/WFE/Sleep-on-Exit）及实际功耗数据
- 掌握**生产部署**关键技术（版本嵌入、内存分析、栈水印、固件格式）

## 文件结构

```
lesson_08_advanced/
├── CMakeLists.txt                 # 构建配置 (含 git hash 嵌入)
├── README.md
├── linker/
│   └── microbit.ld                # 链接脚本 (256KB Flash, 16KB RAM)
└── src/
    ├── startup.S                  # 向量表 + 启动代码
    ├── main.c                     # 主协调器
    ├── semihosting.c / .h         # semihosting 辅助模块
    ├── gdb_demo.c                 # Module 1: GDB 调试实战
    ├── optimization_demo.c        # Module 2: 编译器优化分析
    ├── low_power_demo.c           # Module 3: 低功耗模式
    └── production_demo.c          # Module 4: 生产部署
```

## 构建与运行

```bash
# 从根目录构建
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake -DLESSON=lesson_08_advanced -G Ninja
cmake --build build

# 正常运行
cmake --build build --target run_lesson_08_advanced

# GDB 调试模式 (暂停在第一条指令)
cmake --build build --target debug_lesson_08_advanced

# GDB 调试模式 (程序运行中, 随时 attach)
cmake --build build --target debug_continue_lesson_08_advanced

# 构建 Release 版本 (看优化效果)
cmake -B build_rel -S . -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
    -DLESSON=lesson_08_advanced -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build_rel
```

---

## Module 1: GDB 远程调试实战 (`gdb_demo.c`)

### 启动调试会话

```bash
# Terminal 1 — QEMU (GDB server, 端口 1234, CPU 暂停)
qemu-system-arm -M microbit \
    -kernel build/lesson_08_advanced/lesson_08_advanced.elf \
    -semihosting -nographic -s -S

# Terminal 2 — GDB 客户端
arm-none-eabi-gdb build/lesson_08_advanced/lesson_08_advanced.elf \
    -ex "target remote :1234"
```

### 5 个子练习

| 子练习 | 内容 | 推荐 GDB 命令 |
|--------|------|--------------|
| **1. 断点与单步** | 跟踪状态机 IDLE→ARMED→TRIGGERED→COMPLETE | `break gdb_state_machine_step`, `continue`, `print g_fsm_state` |
| **2. 监视点与内存** | 观察变量变化, 检查全局数组 | `watch g_watch_target`, `x/8x &g_pattern` |
| **3. 寄存器与回溯** | 查看寄存器、fibonacci 递归调用栈 | `info registers`, `backtrace`, `info frame` |
| **4. 反汇编探索** | 对比除法/移位的汇编实现 | `disas /m disassembly_demo`, `x/10i $pc` |
| **5. Bug 定位练习** | 寻找 `sum_array_buggy()` 中的 off-by-one 错误 | `break sum_array_buggy`, `step`, `print i` |

### GDB 命令速查

```
断点:
  break func              # 在函数入口设断点
  hbreak *0xNNNN          # 硬件断点 (Flash 中也可用)
  break func if x > 100   # 条件断点
  watch var               # 监视点 (变量被写入时停下)
  delete 1 / disable 2    # 删除/禁用断点

步进:
  step / s                # 单步 (进入函数)
  next / n                # 单步 (跨过函数)
  stepi                   # 单步一条机器指令
  finish                  # 运行到当前函数返回
  continue / c            # 继续运行

检查:
  info registers          # 所有 CPU 寄存器 (R0-R12, SP, LR, PC, xPSR)
  print/x $sp             # 栈指针 (十六进制)
  print sizeof(var)       # 变量大小
  x/16x $sp               # 查看栈顶 16 个字
  x/s (char*)ptr          # 以字符串形式查看内存
  disas /m                # 混合源码的反汇编
  backtrace / bt          # 调用栈回溯

修改:
  set var = value         # 在调试中修改变量值
  monitor system_reset    # 通过 QEMU monitor 复位 CPU
```

---

## Module 2: 编译器优化分析 (`optimization_demo.c`)

### 构建不同优化级别进行对比

```bash
# Debug: -O0 (无优化, 代码最大但调试最方便)
cmake -B build_dbg -S . -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake \
    -DCMAKE_BUILD_TYPE=Debug

# Release: -O2 (速度优先)
cmake -B build_rel -S . -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake \
    -DCMAKE_BUILD_TYPE=Release

# MinSizeRel: -Os (体积优先, 嵌入式常用)
cmake -B build_min -S . -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake \
    -DCMAKE_BUILD_TYPE=MinSizeRel

# 对比反汇编输出
diff <(arm-none-eabi-objdump -d build_dbg/lesson_08_advanced.elf) \
     <(arm-none-eabi-objdump -d build_min/lesson_08_advanced.elf)
```

### 关键发现

| 表达式 | -O0 实现 | -Os/-O2 实现 | 性能差距 |
|--------|----------|-------------|----------|
| `x / 10` | `BL __aeabi_uidiv` (~100 cycles) | 乘法逆元 `(x*0xCCCCCCCD)>>35` (~30 cycles) | **3x** |
| `x / 8` | `BL __aeabi_uidiv` (~100 cycles) | `LSRS r0, r0, #3` (1 cycle) | **100x** |
| `x % 8` | 调用除法 | `ANDS r0, r0, #7` (1 cycle) | **100x** |
| `x * 2` | `LSLS r0, r0, #1` (1 cycle) | 同上 (编译器始终优化) | 1x |
| volatile 循环 | 每次从内存重新读取 | 提升到寄存器 | **3-5x** |

### M0 优化速查表

```
✅ DO:                                ❌ AVOID:
  uint32_t (原生字长)                  uint64_t (软件模拟, ~10x 慢)
  x >> 3 (移位, 1 cycle)               x / 8 (调用除法库)
  x & 0x7 (按位与, 1 cycle)            x % 8 (调用除法库)
  ringbuf & (size-1) (AND 取模)        ringbuf % size
  定点数 Q16.16                        浮点数 float/double (无 FPU!)
  static inline 小函数                 大量微型函数调用
  __attribute__((aligned(4)))          未对齐数据访问
  -Os -flto -ffunction-sections        默认编译选项
```

---

## Module 3: 低功耗模式 (`low_power_demo.c`)

### WFI vs WFE

```
                          WFI                     WFE
唤醒源:                   任何中断                 中断 OR 事件
SEV 指令影响:             无                      唤醒
事件寄存器:               N/A                     有 (避免 miss-wake)
M0 适用性:                ✅ 通用空闲              ⚠️ 仅单 CPU (无 TXEV/RXEV)
M0+ 适用性:               ✅ 通用空闲              ✅ 外设事件唤醒
```

### Sleep-on-Exit 模式

```
main() → WFI → [CPU 睡眠]
                  ↓ (中断到来)
                ISR 执行
                  ↓ (ISR 返回)
                [自动回到睡眠]  ← main() 的 while(1) 永不执行!
                  ↓ (下一个中断)
                ISR 执行 → ...

适用: 纯中断驱动的传感器节点
功耗: 平均 ~3μA (纽扣电池可用数年)
```

### nRF51822 功耗数据

| 模式 | 电流 | 备注 |
|------|------|------|
| CPU 运行 @ 16MHz | ~2.4 mA | 正常运行 |
| CPU 运行 @ 4MHz | ~1.0 mA | 降频 |
| WFI 普通睡眠 | ~2.0 μA | RAM 保持 |
| WFI 深度睡眠 | ~0.6 µA | RAM 丢失 (需 .noinit) |
| System OFF | ~0.3 µA | GPIO 唤醒 |
| BLE TX @ 0dBm | ~5.3 mA | 无线电功耗最高! |

> ⚠️ **QEMU 限制**: WFI/WFE 在 QEMU 中立即返回 (不模拟功耗)。代码逻辑对真实硬件正确。

---

## Module 4: 生产部署 (`production_demo.c`)

### 固件版本嵌入

CMake 在构建时自动注入 `GIT_HASH` (通过 `execute_process(git rev-parse)`)。代码中通过 `#ifndef` 提供回退值:

```c
#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif
```

### 内存使用分析

链接符号直接报告各段大小：

```c
extern uint8_t __data_start, __data_end, __bss_start, __bss_end;
uint32_t ram_used = &__bss_end - &__data_start;
```

### 栈水印技术

```
1. 启动后, 用已知 pattern (0xEEEEEEEE) 填充 heap_start → SP 之间的空间
2. 执行程序的最深调用路径
3. 检查 pattern 从栈顶方向被覆写了多少 → 最大栈深度

公式: 栈安全性 = 总 RAM - (data + bss + max_stack + ISR_nesting_margin)
      建议 ISR 嵌套余量: 256 bytes (M0 每层嵌套 ~64 bytes)
```

### 固件输出格式

| 格式 | 用途 | 创建命令 |
|------|------|---------|
| `.elf` | QEMU, GDB 调试 | 构建直接输出 |
| `.bin` | 烧录器 (raw binary) | `objcopy -O binary` |
| `.hex` | Bootloader, 真实硬件 | `objcopy -O ihex` |
| `.disasm` | 学习编译器输出 | `objdump -d -S` |
| `.map` | 内存布局分析 | 添加 `-Wl,-Map=output.map` |

### 生产检查清单

```
[ ] 检查 .map 文件, 确认无意外代码膨胀
[ ] 用栈水印验证最大栈深度 + 余量
[ ] 启用 WDT, 设置合理超时
[ ] 嵌入构建版本号 + Git hash
[ ] 添加固件 CRC 校验 (见 Lesson 11)
[ ] 在真实硬件上测试 (不仅是 QEMU!)
[ ] 用 -Os -flto 编译最终镜像
[ ] 剥离调试符号 (strip)
[ ] 测量所有运行模式的实际功耗
```

---

## 练习题

### GDB 调试
1. 在 `gdb_state_machine_step` 设置断点, 单步跟踪状态转换, 验证每个转换条件
2. 对 `recursive_fibonacci` 设置断点, 观察 n=8 时递归栈深度, 用 `backtrace` 查看调用链
3. 找到 `sum_array_buggy` 的 off-by-one 错误, 用 `print` 展示越界读取的值, 修复后重新测试

### 优化分析
4. 对比 `sum_loop_no_volatile` 和 `sum_loop_volatile` 在 -O0 和 -Os 下的反汇编, 解释差异
5. 写一个除 7 的函数, 检查 -Os 下编译器是否使用了乘法逆元优化 (提示: 逆元是 `0x92492493`)
6. 修改 `struct unpacked` 的字段顺序, 最小化 padding, 验证 `sizeof()` 从 12 降到 8

### 低功耗
7. 在 L8 的代码中找到所有调用 `__WFI()` 的位置 (提示: 在 low_power_demo.c 和 FreeRTOS 的 port.c)
8. 计算: nRF51 用 CR2032 (225mAh) 供电, WFI 深度睡眠模式 (0.6µA), 理论运行时间是多少?

### 生产部署
9. 运行 `arm-none-eabi-size lesson_08_advanced.elf`, 解读 text/data/bss 各段含义
10. 实现 `fill_stack_watermark()` 和 `measure_stack_usage()`, 在不同优化级别下对比栈使用量

---

## QEMU 限制

| 限制 | 影响 |
|------|------|
| WFI/WFE 不睡眠 | QEMU 中立即返回, 功耗分析仅作理论参考 |
| SysTick 中断不触发 | 使用 COUNTFLAG 轮询代替 (见 low_power_demo) |
| 无实际功耗模拟 | 功耗数据来自 nRF51822 产品规格, 需要在真实硬件上实测 |
| GDB 监视点无限制 | QEMU 使用软件模拟, 硬件监视点数量取决于调试器 |

所有代码在真实 BBC micro:bit (nRF51822) 上完全正确.

## 相关文档

- [QEMU 详解](../docs/06_qemu.md) — GDB 调试章节
- [GNU 工具链指南](../docs/00_toolchain.md) — 优化选项、objdump 详解
- [Cortex-M0 架构详解](../docs/07_cortex_m0_architecture.md) — WFI/WFE 指令、SCR 寄存器
- [QEMU 限制详解](../docs/11_qemu_limitations.md) — SysTick/PendSV 根因分析
- [Lesson 11: 错误处理](../lesson_11_error_handling/) — CRC 校验、栈溢出检测
- [FreeRTOS 指南](../docs/05_freertos.md) — Tickless Idle 详解

## 从 QEMU 到真实硬件

所有 4 个模块的代码在迁移到真实 BBC micro:bit 时:
- **GDB**: 通过 OpenOCD + SWD 连接, 命令与 QEMU 相同
- **优化**: 反汇编输出因编译器版本可能略有差异, 优化模式完全一致
- **低功耗**: WFI/WFE 在真实硬件上真正进入睡眠, 可用万用表验证电流
- **生产**: .hex 文件可直接拖放至 MICROBIT USB 驱动器烧录
