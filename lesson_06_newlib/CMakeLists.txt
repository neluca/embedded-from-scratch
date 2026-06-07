# ==============================================================================
# Phase 6: C 标准库集成 (newlib-nano)
# ==============================================================================
# 使用 newlib-nano (ARM GCC 工具链自带的精简版 C 标准库)
#
# 关键区别:
#   之前的阶段: -nostdlib (完全没有标准库)
#   本阶段:     -specs=nano.specs (链接精简版 C 标准库)
#
# -specs=nano.specs 自动:
#   - 链接 libc_nano.a (精简版 C 库)
#   - 链接 libg_nano.a (精简版 GCC 支持库)
#   - 链接 libm.a (数学库, 使用 -lm)
#   - 设置适当的库搜索路径
# ==============================================================================
cmake_minimum_required(VERSION 3.20)

project(lesson_06_newlib
    LANGUAGES C ASM
    DESCRIPTION "Lesson 6: C Standard Library (newlib-nano)"
)

set(SRC_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LINKER_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/linker)

set(SOURCES
    ${SRC_DIR}/startup.S
    ${SRC_DIR}/main.c
    ${SRC_DIR}/syscalls.c
    ${SRC_DIR}/semihosting.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

# 编译选项: 不使用 -nostdlib, 保留标准头文件
target_compile_options(${PROJECT_NAME} PRIVATE
    -ffreestanding
)

# 包含路径
target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR})

# 链接选项:
#   -specs=nano.specs: 使用 newlib-nano (精简版 C 标准库)
#   -lm:               链接数学库 (sin/cos/sqrt 等)
#   -nostartfiles:     使用我们自己的启动代码
#   -u _printf_float:  启用 printf 的浮点格式化支持 (需要的话)
target_link_options(${PROJECT_NAME} PRIVATE
    -T${LINKER_DIR}/microbit.ld
    -specs=nano.specs
    -nostartfiles
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--print-memory-usage
)

# 链接库: libm (数学), libc 由 nano.specs 自动处理
target_link_libraries(${PROJECT_NAME} PRIVATE m)

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

# 构建后分析
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${PROJECT_NAME}>
        > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.disasm
    COMMENT "Generating disassembly")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -h $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Section headers")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Size analysis")

# QEMU run targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/qemu_run.cmake)
add_qemu_targets(${PROJECT_NAME})
