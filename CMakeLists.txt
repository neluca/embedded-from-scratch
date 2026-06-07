# ==============================================================================
# Root CMakeLists.txt — ARM Cortex-M0 Embedded Learning Project
# ==============================================================================
# Build all lessons:
#   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake -G Ninja
#   cmake --build build
#
# Build a single lesson:
#   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
#       -DLESSON=lesson_01_bare_metal -G Ninja
#   cmake --build build
#
# Run a lesson in QEMU:
#   cmake --build build --target run_lesson_01_bare_metal
# ==============================================================================
cmake_minimum_required(VERSION 3.20)

project(embedded-from-scratch
    LANGUAGES C ASM
    DESCRIPTION "ARM Cortex-M0 Embedded Development from Scratch"
)

# --- Single-lesson mode ---
# Enables building just one lesson for faster iteration.
if(DEFINED LESSON)
    message(STATUS "Building single lesson: ${LESSON}")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${LESSON}/CMakeLists.txt")
        message(FATAL_ERROR "Lesson not found: ${LESSON}")
    endif()
    add_subdirectory("${LESSON}" "${LESSON}_build")
    return()
endif()

# --- Build all lessons ---
# Lesson 7 requires FreeRTOS submodule
set(FREERTOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lesson_07_freertos/FreeRTOS-Kernel")
if(NOT EXISTS "${FREERTOS_DIR}/tasks.c")
    message(WARNING "FreeRTOS kernel not found at ${FREERTOS_DIR}")
    message(WARNING "  To clone: git submodule update --init --recursive")
    message(WARNING "  Lesson 7 will be skipped.")
    set(SKIP_LESSON_07 TRUE)
endif()

add_subdirectory(lesson_01_bare_metal)
add_subdirectory(lesson_02_assembly)
add_subdirectory(lesson_03_c_asm_integration)
add_subdirectory(lesson_04_exceptions_interrupts)
add_subdirectory(lesson_05_peripherals)
add_subdirectory(lesson_06_newlib)
if(NOT SKIP_LESSON_07)
    add_subdirectory(lesson_07_freertos)
endif()
add_subdirectory(lesson_08_advanced)
add_subdirectory(lesson_09_bootloader)
add_subdirectory(lesson_10_watchdog)
add_subdirectory(lesson_11_error_handling)
add_subdirectory(lesson_12_patterns)
add_subdirectory(lesson_13_interrupt_priority)

message(STATUS "All lessons configured. Build with: cmake --build ${CMAKE_BINARY_DIR}")
