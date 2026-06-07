# Lesson 3: C & Assembly Integration

## 学习目标

- 理解 AAPCS（ARM Architecture Procedure Call Standard）调用约定
- 掌握 C 调用汇编函数的参数传递机制
- 掌握 GCC 内联汇编（inline assembly）的使用
- 理解编译器优化输出（objdump 分析）

## 文件结构

```
lesson_03_c_asm_integration/
├── CMakeLists.txt
├── linker/microbit.ld
└── src/
    ├── startup.S           # 汇编启动代码
    ├── asm_funcs.S          # 可从 C 调用的汇编函数库
    ├── main.c              # 主程序
    ├── call_asm_from_c.c   # 演示 C → 汇编调用
    ├── inline_asm_demo.c   # 内联汇编演示
    └── optimization_demo.c  # 编译器优化分析
```

## 演示内容

| 文件 | 内容 |
|------|------|
| `asm_funcs.S` | asm_add, asm_mul, asm_abs, asm_clz, asm_swap_bytes, asm_memcpy32 |
| `call_asm_from_c.c` | C 声明汇编函数，AAPCS 参数传递验证 |
| `inline_asm_demo.c` | PRIMASK 读写、WFI、CPSID/CPSIE、临界区、饱和加法 |
| `optimization_demo.c` | volatile 用法、inline vs 非 inline、除法优化、位操作 |

## 关键知识点

### AAPCS 参数传递

```
C: uint32_t func(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
                  R0             R1             R2             R3            [SP+0]
```

- 前 4 个参数通过 R0-R3 传递
- 第 5+ 个参数通过栈传递
- 返回值在 R0（64 位在 R0-R1）

### 内联汇编约束

```c
__asm__ volatile (
    "mrs %0, PRIMASK\n"    // 汇编模板
    : "=r"(result)          // 输出操作数：%0
    :                        // 输入操作数
    : "memory"               // clobber 列表
);
```

| 约束 | 含义 |
|------|------|
| `"r"` | 任意寄存器 |
| `"l"` | 低寄存器 R0-R7（Thumb-1 约束） |
| `"=r"` | 只写寄存器 |
| `"+r"` | 读写寄存器 |
| `"memory"` | 可能修改内存 |

### `.syntax divided` 问题

GCC 在内联汇编前生成 `.syntax divided`，导致 `adds r0, r1` 被 GAS 拒绝。解决方案：在模板开头加 `.syntax unified`。

## 相关文档

- [ARM Cortex-M0 汇编指南](../docs/02_assembly.md)
- [GNU 工具链指南](../docs/00_toolchain.md)
