# ==============================================================================
# GDB 初始化脚本 - ARM Cortex-M0 调试
# 用法:
#   arm-none-eabi-gdb -x scripts/debug.gdb <elf_file>
#
# 前提: QEMU 已启动并等待 GDB 连接 (使用 run_qemu.sh --gdb-pause)
# ==============================================================================

# 连接到 QEMU GDB server (默认端口 1234)
target remote localhost:1234

# 在 Reset_Handler 设置硬件断点
# mon reset init
# hbreak Reset_Handler

# 常用命令速查:
#   info registers      - 显示所有寄存器
#   info reg r0 r1 r2   - 显示指定寄存器
#   x/10x $sp           - 以 hex 格式显示栈顶 10 个字
#   x/s <addr>          - 显示地址处的字符串
#   disas               - 反汇编当前 PC 附近
#   disas /m            - 混合源码和汇编
#   stepi               - 单步执行一条指令 (进入函数)
#   nexti               - 单步执行一条指令 (跳过函数调用)
#   step                - 单步执行一行 C 代码 (进入函数)
#   next                - 单步执行一行 C 代码 (跳过函数调用)
#   bt                  - 显示调用栈
#   info frame          - 显示当前栈帧
#   print/x $xpsr       - 显示程序状态寄存器
#   p (int)variable     - 打印变量值

echo ===== GDB connected to QEMU Cortex-M0 =====\n
