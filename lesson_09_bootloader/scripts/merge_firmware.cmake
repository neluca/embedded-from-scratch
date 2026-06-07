# ==============================================================================
# merge_firmware.cmake — merge bootloader.bin + app.bin into firmware.bin
# ==============================================================================
cmake_minimum_required(VERSION 3.20)

file(READ "${BOOTBIN}" boot_hex HEX)
file(READ "${APPBIN}"  app_hex  HEX)

string(LENGTH "${boot_hex}" boot_hexlen)
math(EXPR boot_len "${boot_hexlen} / 2")

if(boot_len GREATER BOOTSIZE)
    message(FATAL_ERROR "Bootloader ${boot_len}B exceeds reserved ${BOOTSIZE}B")
endif()

math(EXPR pad_len "${BOOTSIZE} - ${boot_len}")
message(STATUS "Bootloader: ${boot_len}B + ${pad_len}B padding -> ${BOOTSIZE}B")

# Pad with 0xFF (erased flash state)
set(pad_str "")
math(EXPR n "${pad_len}")
foreach(i RANGE 1 ${n})
    string(APPEND pad_str "FF")
endforeach()

string(LENGTH "${app_hex}" app_hexlen)
math(EXPR app_len "${app_hexlen} / 2")
message(STATUS "App: ${app_len}B")

set(fw_hex "${boot_hex}${pad_str}${app_hex}")

file(WRITE "${OUTFILE}" "${fw_hex}")

string(LENGTH "${fw_hex}" fw_hexlen)
math(EXPR fw_len "${fw_hexlen} / 2")
message(STATUS "Firmware total: ${fw_len}B")
