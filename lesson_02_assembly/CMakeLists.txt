# ==============================================================================
# Phase 2: ARM Cortex-M0+ 汇编语言
# ==============================================================================
# 每个 .s 文件是一个独立程序, 构建为单独的 ELF 可执行文件
#
# 构建:
#   cmake -B build/Debug -S . -DCMAKE_TOOLCHAIN_FILE=../../cmake/arm-none-eabi-gcc.cmake
#   cmake --build build/Debug
# ==============================================================================
cmake_minimum_required(VERSION 3.20)

project(lesson_02_assembly
    LANGUAGES C ASM
    DESCRIPTION "Lesson 2: ARM Cortex-M0 Assembly Language"
)

set(LINKER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/linker)

# ---------------------------------------------------------------------------
# 为每个 .S (大写, 自动预处理) 文件创建独立的可执行目标
# 每个文件是一个自包含的汇编程序 (包含向量表 + 完整逻辑)
# ---------------------------------------------------------------------------
file(GLOB ASM_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.S")
list(SORT ASM_FILES)

foreach(ASM_FILE ${ASM_FILES})
    get_filename_component(EXE_NAME ${ASM_FILE} NAME_WE)

    add_executable(${EXE_NAME} ${ASM_FILE})

    # 汇编编译选项 (.S 文件自动启用预处理)
    target_compile_options(${EXE_NAME} PRIVATE
        -ffreestanding
    )

    # 链接选项
    target_link_options(${EXE_NAME} PRIVATE
        -T${LINKER_DIR}/microbit.ld
        -nostdlib
        -nostartfiles
        -Wl,--gc-sections
        -Wl,-Map=${EXE_NAME}.map
        -Wl,--print-memory-usage
    )

    # 设置 .elf 后缀
    set_target_properties(${EXE_NAME} PROPERTIES SUFFIX ".elf")

    # 构建后: 反汇编 + section 信息
    add_custom_command(TARGET ${EXE_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${EXE_NAME}>
            > ${CMAKE_CURRENT_BINARY_DIR}/${EXE_NAME}.disasm
        COMMENT "Disasm: ${EXE_NAME}"
    )

    add_custom_command(TARGET ${EXE_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJDUMP} -h $<TARGET_FILE:${EXE_NAME}>
        COMMENT "Sections: ${EXE_NAME}"
    )

    message(STATUS "  Added target: ${EXE_NAME}")
endforeach()

# QEMU run targets (one per exercise)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/qemu_run.cmake)
foreach(ASM_FILE ${ASM_FILES})
    get_filename_component(EXE_NAME ${ASM_FILE} NAME_WE)
    add_qemu_targets(${EXE_NAME})
endforeach()

message(STATUS "Phase 2: ${ASM_FILES} assembly programs ready")
