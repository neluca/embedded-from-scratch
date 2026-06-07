# ==============================================================================
# QEMU Run Targets for ARM Cortex-M0 (microbit)
# ==============================================================================
# Include this file in any phase's CMakeLists.txt to add:
#   run_<target>              — run ELF in QEMU with semihosting
#   debug_<target>            — run ELF with GDB server, CPU paused
#   debug_continue_<target>   — run ELF with GDB server, CPU running
#   run_bin_<target>          — run raw binary in QEMU (Release mode)
#
# Usage:
#   include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/qemu_run.cmake)
#   add_qemu_targets(<target_name>)            # ELF + BIN targets
# ==============================================================================

if(NOT QEMU_PATH)
    if(DEFINED ENV{QEMU_PATH})
        set(QEMU_PATH "$ENV{QEMU_PATH}")
        message(STATUS "Using QEMU_PATH from environment: ${QEMU_PATH}")
    else()
        # Try PATH lookup first
        find_program(QEMU_FOUND NAMES qemu-system-arm qemu-system-arm.exe)
        if(QEMU_FOUND)
            set(QEMU_PATH "${QEMU_FOUND}")
        else()
            # Fallback: common install locations
            set(QEMU_CANDIDATES
                "C:/Program Files/qemu/qemu-system-arm.exe"
                "/usr/bin/qemu-system-arm"
                "/usr/local/bin/qemu-system-arm"
            )
            foreach(CANDIDATE ${QEMU_CANDIDATES})
                if(EXISTS "${CANDIDATE}")
                    set(QEMU_PATH "${CANDIDATE}")
                    message(STATUS "Found QEMU at: ${CANDIDATE}")
                    break()
                endif()
            endforeach()
            if(NOT QEMU_PATH)
                message(WARNING "qemu-system-arm not found. QEMU run targets will be unavailable.")
            endif()
        endif()
    endif()
    set(QEMU_PATH "${QEMU_PATH}" CACHE FILEPATH "Path to qemu-system-arm")
endif()

message(STATUS "QEMU: ${QEMU_PATH}")

# ==============================================================================
# add_qemu_targets — add ELF run/debug targets + BIN generation + BIN run target
# ==============================================================================
function(add_qemu_targets TARGET)
    # Guard: skip if QEMU is not available
    if(NOT QEMU_PATH)
        message(STATUS "  QEMU targets: SKIPPED (qemu-system-arm not found)")
        return()
    endif()

    set(ELF_FILE "$<TARGET_FILE:${TARGET}>")

    # --- Ensure .bin is generated via objcopy ---
    # The .bin file is needed for run_bin_<target> and for real hardware flashing.
    set(BIN_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bin")
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary "${ELF_FILE}" "${BIN_FILE}"
        COMMENT "Generating ${TARGET}.bin"
        VERBATIM)

    # QEMU base flags for ELF (semihosting via BKPT 0xAB)
    set(QEMU_ELF "${QEMU_PATH}" -M microbit -kernel "${ELF_FILE}"
                 -semihosting -nographic -serial null -monitor none)

    # --- ELF targets ---
    add_custom_target(run_${TARGET}
        COMMAND ${QEMU_ELF}
        DEPENDS ${TARGET}
        COMMENT "Running ${TARGET}.elf in QEMU (semihosting)"
        USES_TERMINAL)

    add_custom_target(debug_${TARGET}
        COMMAND ${QEMU_ELF} -s -S
        DEPENDS ${TARGET}
        COMMENT "QEMU GDB server :1234 (paused). Attach: arm-none-eabi-gdb ${ELF_FILE}"
        USES_TERMINAL)

    add_custom_target(debug_continue_${TARGET}
        COMMAND ${QEMU_ELF} -s
        DEPENDS ${TARGET}
        COMMENT "QEMU GDB server :1234 (running)"
        USES_TERMINAL)

    # --- BIN target (raw binary, no semihosting, UART output to terminal) ---
    # QEMU loads .bin at address 0x00000000 with -device loader
    set(QEMU_BIN "${QEMU_PATH}" -M microbit
                 -nographic -serial stdio -monitor none
                 -device loader,file="${BIN_FILE}",addr=0x00000000)

    add_custom_target(run_bin_${TARGET}
        COMMAND ${QEMU_BIN}
        DEPENDS ${TARGET}
        COMMENT "Running ${TARGET}.bin in QEMU (raw binary, Release mode)"
        USES_TERMINAL)

    message(STATUS "  QEMU: run_${TARGET} debug_${TARGET} debug_continue_${TARGET} run_bin_${TARGET}")
endfunction()
