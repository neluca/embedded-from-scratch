# ==============================================================================
# Lesson 1: Bare-Metal Hello World
# ==============================================================================
# 构建一个最小的 ARM Cortex-M0 裸机程序, 通过 semihosting 输出 "Hello World"
#
# 构建:
#   cmake -B build/Debug -S . -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake
#   cmake --build build/Debug
#
# 运行:
#   qemu-system-arm -M microbit -kernel build/Debug/phase_01.elf -semihosting -nographic
# ==============================================================================
cmake_minimum_required(VERSION 3.20)

# 项目名称
project(lesson_01_bare_metal
    LANGUAGES C ASM
    DESCRIPTION "Lesson 1: ARM Cortex-M0 Bare-Metal Hello World"
)

# ---------------------------------------------------------------------------
# 源文件
# ---------------------------------------------------------------------------
set(SRC_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LINKER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/linker)

set(SOURCES
    ${SRC_DIR}/startup.c
    ${SRC_DIR}/main.c
)

# ---------------------------------------------------------------------------
# 可执行文件
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES})

# 设置输出后缀为 .elf (裸机程序的标准扩展名)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

# 链接脚本和库
# -nostdlib: 不链接标准 C 库
# -nostartfiles: 不使用标准 crt0 启动文件
target_link_options(${PROJECT_NAME} PRIVATE
    -T${LINKER_DIR}/microbit.ld
    -nostdlib
    -nostartfiles
)

# 链接 GCC 运行时库 (提供软件除法等辅助函数)
# 使用 target_link_libraries 确保 -lgcc 放在 object 文件之后
# M0 无硬件除法器, 使用 / 或 % 运算符需要 __aeabi_uidiv 系列函数
target_link_libraries(${PROJECT_NAME} PRIVATE gcc)

# 编译选项
# -ffreestanding: 独立环境 (无宿主操作系统)，不假定标准库函数存在
# 注意: -nostdlib 是链接选项，不应放在编译选项中
target_compile_options(${PROJECT_NAME} PRIVATE
    -ffreestanding
)

# 包含路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRC_DIR}
)

# ---------------------------------------------------------------------------
# 构建后分析: 生成反汇编和内存使用报告
# 使用 generator expression $<TARGET_FILE:...> 获取正确的目标文件路径
# ---------------------------------------------------------------------------

# 生成反汇编 (带源码混合)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${PROJECT_NAME}>
        > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.disasm
    COMMENT "Generating disassembly with source"
)

# 生成详细的反汇编 (所有 section)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -D $<TARGET_FILE:${PROJECT_NAME}>
        > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.disasm_full
    COMMENT "Generating full disassembly"
)

# 打印 section 头信息
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -h $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Displaying section headers"
)

# 打印符号表
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -t $<TARGET_FILE:${PROJECT_NAME}>
        > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.symbols
    COMMENT "Generating symbol table"
)

# 生成 Intel HEX 文件 (用于烧录, 这里仅作演示)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}>
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMENT "Generating Intel HEX file"
)

# 生成 raw binary (用于其他工具)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}>
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMENT "Generating raw binary"
)

# QEMU run targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/qemu_run.cmake)
add_qemu_targets(${PROJECT_NAME})
