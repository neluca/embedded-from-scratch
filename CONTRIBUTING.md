# Contributing to embedded-from-scratch

Thank you for your interest in contributing! This project aims to be the best resource for learning ARM Cortex-M0 embedded development from scratch.

## Ways to Contribute

- **Report bugs**: Open an issue describing the problem, what you expected, and your environment (OS, ARM GCC version, QEMU version)
- **Fix code issues**: Submit a PR with the fix, ensuring it builds with the ARM GCC toolchain
- **Improve documentation**: Fix typos, clarify explanations, add diagrams
- **Add lessons**: Propose new lessons covering advanced topics (DMA, RTOS, bootloaders, power management)
- **Test on real hardware**: Verify lessons work on actual BBC micro:bit or other Cortex-M0 boards

## Development Setup

### Prerequisites

- ARM GCC toolchain (`arm-none-eabi-gcc`)
- QEMU with ARM system emulation (`qemu-system-arm`)
- CMake 3.20+ and Ninja
- Git (with Bash on Windows)

### Build All Lessons

```bash
git clone --recurse-submodules https://github.com/neluca/embedded-from-scratch.git
cd embedded-from-scratch

# Configure and build all lessons
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake -G Ninja
cmake --build build
```

### Build a Single Lesson

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
    -DLESSON=lesson_01_bare_metal -G Ninja
cmake --build build
```

### Run in QEMU

```bash
# Run a specific lesson
cmake --build build --target run_lesson_01_bare_metal
```

## Code Style

This project uses clang-format (LLVM-based style) to maintain consistency:

```bash
clang-format -i src/*.c src/*.h
```

Key conventions:
- **4 spaces** for indentation (no tabs)
- **LF** line endings (handled by `.clang-format` `LineEnding: DeriveLF`)
- Include `<stdint.h>` for fixed-width types (`uint32_t`, etc.)
- Use `volatile` for memory-mapped hardware registers only
- Semihosting functions should use the project's `sh_write0` / `sh_write_hex` pattern
- Assembly files: `.S` extension with `.syntax unified`, `.cpu cortex-m0`, `.thumb`

## ARM Cortex-M0 Constraints

When contributing code, remember these M0 limitations:
- **ARMv6-M (Thumb-1) only**: No Thumb-2 instructions
- **No hardware divider**: `a / b` calls `__aeabi_uidiv` (~100 cycles)
- **No CLZ, REV, BFI/UBFX instructions**: Software-implemented alternatives recommended
- **No VTOR**: Vector table is fixed at `0x00000000`
- **2-operand ALU**: `ADDS Rd, Rm` not `ADD Rd, Rn, Rm`
- **Low registers (R0-R7)** only for most data-processing instructions
- **No DSB/ISB**: Only simple memory barriers needed (M0's 3-stage pipeline)

## Project Structure

```
embedded-from-scratch/
├── cmake/                       # Shared CMake modules
│   ├── arm-none-eabi-gcc.cmake  # Toolchain file
│   └── qemu_run.cmake           # QEMU run/debug targets
├── docs/                        # Reference documentation
├── scripts/                     # Build & debug helper scripts
├── .clang-format                # Code formatting rules
├── CMakeLists.txt               # Root build file
└── lesson_XX_name/              # Independent lesson
    ├── CMakeLists.txt
    ├── README.md
    ├── linker/                  # Linker script (if needed)
    └── src/                     # Source code
```

## Pull Request Process

1. Fork the repository and create a feature branch from `main`
2. Make your changes following the code style above
3. Build and test your changes:
   ```bash
   cmake --build build
   ```
4. Update documentation if needed
5. Submit a PR with a clear description of the changes

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
