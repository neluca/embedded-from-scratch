# Lesson 1: Bare-Metal Hello World

## 学习目标

- 搭建 ARM Cortex-M0 交叉编译环境
- 理解 CMake 工具链文件的作用
- 编写第一个链接脚本（linker script）
- 实现最小启动代码（中断向量表 + 复位处理）
- 通过 semihosting 输出 "Hello World"

## 文件结构

```
lesson_01_bare_metal/
├── CMakeLists.txt          # CMake 构建文件
├── linker/
│   └── microbit.ld         # 链接脚本
└── src/
    ├── startup.c           # 启动代码（向量表 + .data/.bss 初始化）
    └── main.c              # 主程序（semihosting 输出）
```

## 构建与运行

```bash
# 构建
cmake -B build/Debug -S . -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake
cmake --build build/Debug

# 运行
qemu-system-arm -M microbit -kernel build/Debug/lesson_01_bare_metal.elf \
    -semihosting -nographic
```

## 关键知识点

### 1. CMake 工具链文件

`cmake/arm-none-eabi-gcc.cmake` 告诉 CMake 使用 ARM 交叉编译器而非本地 gcc：

```cmake
set(CMAKE_SYSTEM_NAME  Generic)          # 裸机环境
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)  # 交叉编译器
```

### 2. 链接脚本

定义内存布局（Flash 256KB, RAM 16KB）和 section 映射：
- `.vector_table` → Flash 0x00000000（Cortex-M 硬件要求）
- `.text`, `.rodata` → Flash
- `.data` → RAM（初始值从 Flash 复制）
- `.bss` → RAM（启动时清零）

### 3. 中断向量表

Cortex-M0 上电后，硬件自动从地址 0x00000000 读取两个值：
- `[0]` → 初始栈指针（SP）
- `[1]` → 复位向量（PC，即 reset_handler 的地址）

### 4. Semihosting

使用 `BKPT 0xAB` 指令调用 QEMU/调试器的 I/O 功能：
- R0 = 操作码（0x04 = SYS_WRITE0）
- R1 = 参数指针（字符串地址）

## 构建产物

| 文件 | 说明 |
|------|------|
| `lesson_01_bare_metal.elf` | ELF 可执行文件 |
| `lesson_01_bare_metal.disasm` | 反汇编（带源码） |
| `lesson_01_bare_metal.map` | 内存映射文件 |
| `lesson_01_bare_metal.hex` | Intel HEX 格式 |

## 相关文档

- [GNU 工具链指南](../docs/00_toolchain.md)
- [ELF 格式指南](../docs/01_elf_format.md)
- [链接脚本指南](../docs/03_linker_script.md)
- [QEMU 指南](../docs/06_qemu.md)
