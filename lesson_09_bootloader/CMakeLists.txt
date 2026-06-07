# ==============================================================================
# Phase 9: Bootloader + Application
# ==============================================================================
# 构建两个独立的固件:
#   bootloader.elf/bin — 位于 Flash 0x00000000 (8KB)
#   app.elf/bin        — 位于 Flash 0x00002000
#
# 合并为 firmware.bin 加载到 QEMU 中运行
# ==============================================================================
cmake_minimum_required(VERSION 3.20)

project(lesson_09_bootloader
    LANGUAGES C ASM
    DESCRIPTION "Lesson 9: Bootloader & Application"
)

set(BOOT_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/bootloader)
set(APP_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/app)
set(SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)

# ============================================================================
# Bootloader (0x00000000)
# ============================================================================
add_executable(bootloader
    ${BOOT_DIR}/src/startup.S
    ${BOOT_DIR}/src/main.c
)
target_compile_options(bootloader PRIVATE -ffreestanding)
target_include_directories(bootloader PRIVATE ${BOOT_DIR}/src)
target_link_options(bootloader PRIVATE
    -T${BOOT_DIR}/linker/bootloader.ld -nostdlib -nostartfiles
)
target_link_libraries(bootloader PRIVATE gcc)
set_target_properties(bootloader PROPERTIES SUFFIX ".elf")

# ============================================================================
# Application (0x00002000)
# ============================================================================
add_executable(app
    ${APP_DIR}/src/startup_app.S
    ${APP_DIR}/src/main.c
)
target_compile_options(app PRIVATE -ffreestanding)
target_include_directories(app PRIVATE ${APP_DIR}/src)
target_link_options(app PRIVATE
    -T${APP_DIR}/linker/app.ld -nostdlib -nostartfiles
)
target_link_libraries(app PRIVATE gcc)
set_target_properties(app PROPERTIES SUFFIX ".elf")

# ============================================================================
# 生成 .bin 文件
# ============================================================================
add_custom_command(TARGET bootloader POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:bootloader>
            ${CMAKE_CURRENT_BINARY_DIR}/bootloader.bin
    COMMENT "Generating bootloader.bin"
)
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:app>
            ${CMAKE_CURRENT_BINARY_DIR}/app.bin
    COMMENT "Generating app.bin"
)

# ============================================================================
# 合并固件: bootloader.bin (pad to 8KB) + app.bin
# Bootloader 预留 8KB, App 从 0x2000 开始
# ============================================================================
set(BOOT_SIZE 8192)  # 8KB reserved for bootloader

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/firmware.bin
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_CURRENT_BINARY_DIR}/firmware.bin
    COMMAND ${CMAKE_COMMAND}
            -DOUTFILE=${CMAKE_CURRENT_BINARY_DIR}/firmware.bin
            -DBOOTBIN=${CMAKE_CURRENT_BINARY_DIR}/bootloader.bin
            -DAPPBIN=${CMAKE_CURRENT_BINARY_DIR}/app.bin
            -DBOOTSIZE=${BOOT_SIZE}
            -P ${SCRIPTS_DIR}/merge_firmware.cmake
    DEPENDS bootloader app
    COMMENT "Merging bootloader.bin + app.bin -> firmware.bin"
)
add_custom_target(firmware ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/firmware.bin)

# ============================================================================
# QEMU 运行目标
# ============================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/qemu_run.cmake)

# 单独运行 bootloader (无 app)
add_qemu_targets(bootloader)

# 运行合并的 firmware (bootloader ELF + app BIN)
add_custom_target(run_firmware
    COMMAND ${QEMU_PATH} -M microbit
            -kernel $<TARGET_FILE:bootloader>
            -device loader,file=${CMAKE_CURRENT_BINARY_DIR}/app.bin,addr=0x00002000
            -semihosting -nographic -serial null -monitor none
    DEPENDS bootloader app
    COMMENT "Running bootloader -> app in QEMU"
    USES_TERMINAL
)

# 构建后分析
add_custom_command(TARGET bootloader POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:bootloader>
            > ${CMAKE_CURRENT_BINARY_DIR}/bootloader.disasm
    COMMENT "Disasm: bootloader"
)
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:app>
            > ${CMAKE_CURRENT_BINARY_DIR}/app.disasm
    COMMENT "Disasm: app"
)

message(STATUS "QEMU targets: run_bootloader run_firmware")
