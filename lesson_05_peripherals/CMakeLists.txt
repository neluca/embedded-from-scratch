# ==============================================================================
# Phase 5: 外设与硬件编程
# ==============================================================================
cmake_minimum_required(VERSION 3.20)

project(lesson_05_peripherals
    LANGUAGES C ASM
    DESCRIPTION "Phase 5: Peripherals & Hardware Programming"
)

set(SRC_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INC_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(LINKER_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/linker)

set(SOURCES
    ${SRC_DIR}/startup.S
    ${SRC_DIR}/main.c
    ${SRC_DIR}/semihosting.c
    ${SRC_DIR}/uart_driver.c
    ${SRC_DIR}/gpio_driver.c
    ${SRC_DIR}/timer_driver.c
    ${SRC_DIR}/console.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -ffreestanding)
target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR} ${INC_DIR})

target_link_options(${PROJECT_NAME} PRIVATE
    -T${LINKER_DIR}/microbit.ld
    -nostdlib
    -nostartfiles
)
target_link_libraries(${PROJECT_NAME} PRIVATE gcc)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${PROJECT_NAME}>
        > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.disasm
    COMMENT "Generating disassembly")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -h $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Section headers")

# QEMU run targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/qemu_run.cmake)
add_qemu_targets(${PROJECT_NAME})
