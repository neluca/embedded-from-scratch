# ==============================================================================
# Lesson 8: Advanced Topics & Production Deployment
# ==============================================================================
cmake_minimum_required(VERSION 3.20)
project(lesson_08_advanced LANGUAGES C ASM)

set(SRC_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LINKER_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/linker)

add_executable(${PROJECT_NAME}
    ${SRC_DIR}/startup.S
    ${SRC_DIR}/main.c
    ${SRC_DIR}/semihosting.c
    ${SRC_DIR}/gdb_demo.c
    ${SRC_DIR}/optimization_demo.c
    ${SRC_DIR}/low_power_demo.c
    ${SRC_DIR}/production_demo.c
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -ffreestanding
    -fno-builtin
    -fno-tree-loop-distribute-patterns
    -Wall -Wextra
)

target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR})

target_link_options(${PROJECT_NAME} PRIVATE
    -T${LINKER_DIR}/microbit.ld
    -nostdlib -nostartfiles
)
target_link_libraries(${PROJECT_NAME} PRIVATE gcc)

# Set ELF suffix
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

# Post-build: generate disassembly and size report
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${PROJECT_NAME}>
        > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.disasm
    COMMENT "Generating annotated disassembly: ${PROJECT_NAME}.disasm")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Size report:")

# Try to embed git hash (if in a git repo)
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(GIT_HASH)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        -DGIT_HASH="${GIT_HASH}")
endif()

# QEMU run targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/qemu_run.cmake)
add_qemu_targets(${PROJECT_NAME})
