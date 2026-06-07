# ==============================================================================
# Lesson 11: Error Handling & Defensive Programming
# ==============================================================================
cmake_minimum_required(VERSION 3.20)

project(lesson_11_error_handling
    LANGUAGES C ASM
    DESCRIPTION "Lesson 11: Error Handling & Defensive Programming"
)

set(SRC_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LINKER_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/linker)

set(SOURCES
    ${SRC_DIR}/startup.S
    ${SRC_DIR}/main.c
    ${SRC_DIR}/semihosting.c
    ${SRC_DIR}/assert_impl.c
    ${SRC_DIR}/error_handler.c
    ${SRC_DIR}/soft_crc.c
    ${SRC_DIR}/stack_guard.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -ffreestanding)
target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR})

# 注意: 不使用 -nostdlib (CRCs 和断言都是纯自实现)
target_link_options(${PROJECT_NAME} PRIVATE
    -T${LINKER_DIR}/microbit.ld
    -nostdlib
    -nostartfiles
)
target_link_libraries(${PROJECT_NAME} PRIVATE gcc)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

# Build analysis
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
