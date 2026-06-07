# ==============================================================================
# Phase 7: FreeRTOS on Cortex-M0+ (microbit)
# ==============================================================================
cmake_minimum_required(VERSION 3.20)

project(lesson_07_freertos
    LANGUAGES C ASM
    DESCRIPTION "Lesson 7: FreeRTOS on Cortex-M0"
)

set(SRC_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INC_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(LINKER_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/linker)
set(FREERTOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel)

# ---------------------------------------------------------------------------
# FreeRTOS 内核源文件
# ---------------------------------------------------------------------------
set(FREERTOS_SOURCES
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/list.c
    ${FREERTOS_DIR}/timers.c
    ${FREERTOS_DIR}/event_groups.c
    ${FREERTOS_DIR}/stream_buffer.c
    ${FREERTOS_DIR}/portable/GCC/ARM_CM0/port.c
    ${FREERTOS_DIR}/portable/GCC/ARM_CM0/portasm.c
    ${FREERTOS_DIR}/portable/MemMang/heap_4.c
)

# ---------------------------------------------------------------------------
# 应用源文件
# ---------------------------------------------------------------------------
set(APP_SOURCES
    ${SRC_DIR}/startup.S
    ${SRC_DIR}/main.c
    ${SRC_DIR}/semihosting.c
    ${SRC_DIR}/syscalls.c
    ${SRC_DIR}/task_blinky.c
    ${SRC_DIR}/task_sensor.c
    ${SRC_DIR}/task_monitor.c
)

# ---------------------------------------------------------------------------
# 可执行目标
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${FREERTOS_SOURCES} ${APP_SOURCES})

# 编译选项
target_compile_options(${PROJECT_NAME} PRIVATE -ffreestanding)

# 包含路径: FreeRTOS include, port, and our config
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRC_DIR}
    ${INC_DIR}
    ${FREERTOS_DIR}/include
    ${FREERTOS_DIR}/portable/GCC/ARM_CM0
)

# 链接选项
target_link_options(${PROJECT_NAME} PRIVATE
    -T${LINKER_DIR}/microbit.ld
    -nostdlib
    -nostartfiles
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--print-memory-usage
)
target_link_libraries(${PROJECT_NAME} PRIVATE gcc)

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

# 构建后
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${PROJECT_NAME}>
        > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.disasm
    COMMENT "Generating disassembly")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Size analysis")

# QEMU run targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/qemu_run.cmake)
add_qemu_targets(${PROJECT_NAME})
