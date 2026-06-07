# ==============================================================================
# Phase 3: C 与 ARM 汇编混合编程
# ==============================================================================
cmake_minimum_required(VERSION 3.20)

project(lesson_03_c_asm_integration
    LANGUAGES C ASM
    DESCRIPTION "Phase 3: C and Assembly Integration"
)

set(SRC_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LINKER_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/linker)

# ---------------------------------------------------------------------------
# 可执行目标
# ---------------------------------------------------------------------------
set(SOURCES
    ${SRC_DIR}/startup.S         # 汇编启动代码
    ${SRC_DIR}/main.c            # C 主程序
    ${SRC_DIR}/asm_funcs.S       # 汇编函数库
    ${SRC_DIR}/call_asm_from_c.c # 从 C 调用汇编
    ${SRC_DIR}/inline_asm_demo.c # 内联汇编
    ${SRC_DIR}/optimization_demo.c # 编译器输出分析
)

add_executable(${PROJECT_NAME} ${SOURCES})

# 编译选项
# -ffreestanding: 独立环境 (无宿主 OS), GCC 不假定标准库函数存在
# 注意: 不使用 -nostdlib (它会影响头文件搜索路径)
target_compile_options(${PROJECT_NAME} PRIVATE
    -ffreestanding
)

# 链接选项
# -nostdlib: 不链接标准库, -nostartfiles: 不链接 crt0
target_link_options(${PROJECT_NAME} PRIVATE
    -T${LINKER_DIR}/microbit.ld
    -nostdlib
    -nostartfiles
)

target_link_libraries(${PROJECT_NAME} PRIVATE gcc)

# 包含路径
target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR})

# .elf 后缀
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

# ---------------------------------------------------------------------------
# 构建后分析
# ---------------------------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${PROJECT_NAME}>
        > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.disasm
    COMMENT "Generating disassembly with source"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -h $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Displaying section headers"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -t $<TARGET_FILE:${PROJECT_NAME}>
        > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.symbols
    COMMENT "Generating symbol table"
)

# QEMU run targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/qemu_run.cmake)
add_qemu_targets(${PROJECT_NAME})
