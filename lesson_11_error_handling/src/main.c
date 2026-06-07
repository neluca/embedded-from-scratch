/*
 * =============================================================================
 * Lesson 11 主程序 -- 错误处理与防御性编程
 * =============================================================================
 *
 * 学习目标:
 * 1. 掌握嵌入式断言系统的设计与使用
 * 2. 理解栈溢出检测的原理 (栈哨兵/栈绘画)
 * 3. 学习错误码传递模式和统一错误处理
 * 4. 了解 CRC 校验在数据完整性保护中的应用
 * 5. 构建可保留的故障日志 (跨复位诊断)
 *
 * 为什么嵌入式系统需要这些?
 *   - 资源受限 → 没有 OS 的错误处理机制
 *   - 无法"退出" → 必须优雅降级或复位
 *   - 调试困难 → 需要可靠的故障诊断信息
 *   - 安全关键 → 错误必须被检测和记录
 *
 * 本课程全部为纯软件实现, 无需特定硬件, QEMU 完美支持.
 * =============================================================================
 */

#include "assert_impl.h"
#include "error_handler.h"
#include "semihosting.h"
#include "soft_crc.h"
#include "stack_guard.h"
#include <stddef.h>
#include <stdint.h>

/* --------------------------------------------------------------------------
 * 演示 1: 断言系统
 * --------------------------------------------------------------------------
 * ASSERT 和 ASSERT_ALWAYS 宏的使用.
 *
 * 关键设计:
 *   - ASSERT(expr):    Debug 构建检查, Release 编译为空 (定义 NDEBUG)
 *   - ASSERT_ALWAYS:   永远检查, 用于关键安全条件
 *   - STATIC_ASSERT:   编译期检查, 零运行时开销!
 *
 * 常见用法:
 *   ASSERT_NOT_NULL(ptr)      — 空指针检查
 *   ASSERT_IN_RANGE(val,0,9)  — 范围检查
 *   ASSERT(len <= MAX_SIZE)   — 自定义条件
 */
static void demo_assert(void)
{
    sh_write0("[1] Assertion System\n\n");

    /* --- 正常 assert: 通过 --- */
    sh_write0("    Normal asserts (passing):\n");
    int x = 42;
    ASSERT(x == 42); /* 通过, 无输出 */
    sh_write0("      ASSERT(x == 42)     -> passed (silent)\n");

    ASSERT_NOT_NULL(&x); /* 通过 */
    sh_write0("      ASSERT_NOT_NULL(&x) -> passed (silent)\n");

    ASSERT_IN_RANGE(x, 0, 100); /* 通过 */
    sh_write0("      ASSERT_IN_RANGE(x,0,100) -> passed (silent)\n\n");

    /* --- 编译期断言 --- */
    sh_write0("    Compile-time check:\n");
    STATIC_ASSERT(sizeof(uint32_t) == 4, uint32_must_be_4_bytes);
    sh_write0("      STATIC_ASSERT(sizeof(uint32_t)==4) -> OK (compile time)\n\n");

    sh_write0(
        "    NOTE: ASSERT failures are NOT demonstrated here\n"
        "    because they halt the CPU. Uncomment the failing\n"
        "    assert below to see the error output:\n"
        "      // ASSERT(0); // <-- uncomment to test\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 2: 错误码与统一错误处理
 * --------------------------------------------------------------------------
 * 使用枚举错误码替代魔法数字.
 * 统一的错误记录入口, 便于集中监控和日志.
 */
static void demo_error_codes(void)
{
    sh_write0("[2] Error Code System\n\n");

    /* 模拟一些错误 */
    sh_write0("    Simulating errors:\n");

    error_record(ERR_OK);
    sh_write0("      error_record(ERR_OK)          -> ");
    sh_write0(error_to_string(error_get_last()));
    sh_write0("\n");

    error_record(ERR_NULL_POINTER);
    sh_write0("      error_record(ERR_NULL_POINTER) -> ");
    sh_write0(error_to_string(error_get_last()));
    sh_write0("\n");

    error_record(ERR_TIMEOUT);
    sh_write0("      error_record(ERR_TIMEOUT)      -> ");
    sh_write0(error_to_string(error_get_last()));
    sh_write0("\n");

    error_record(ERR_FATAL_STACK_OVERFLOW);
    sh_write0("      error_record(ERR_FATAL_STACK)   -> ");
    sh_write0(error_to_string(error_get_last()));
    sh_write0("\n\n");

    /* 错误统计 */
    sh_write0("    Error statistics:\n");
    sh_write0("      Total errors recorded: ");
    sh_write_dec(error_get_count());
    sh_write0("\n\n");

    sh_write0("    Best practice pattern:\n");
    sh_write0("      error_code_t err = do_something();\n");
    sh_write0("      if (err != ERR_OK) {\n");
    sh_write0("          error_record(err);\n");
    sh_write0("          return err;  // propagate\n");
    sh_write0("      }\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 3: 栈溢出检测
 * --------------------------------------------------------------------------
 * 演示栈哨兵初始化、定期检查和栈使用量估算.
 */
static void demo_stack_guard(void)
{
    sh_write0("[3] Stack Overflow Detection\n\n");

    sh_write0("    microbit RAM: 16KB (0x20000000-0x20004000)\n\n");

    /* 初始化栈哨兵 */
    stack_sentinel_init();

    sh_write0("    Stack sentinel initialized:\n");
    sh_write0("      Sentinel addr: 0x");
    sh_write_hex(stack_sentinel_addr());
    sh_write0("\n");
    sh_write0("      Pattern:       0xDEADBEEF\n");
    sh_write0("      Size:          ");
    sh_write_dec(STACK_SENTINEL_WORDS * 4);
    sh_write0(" bytes (");
    sh_write_dec(STACK_SENTINEL_WORDS);
    sh_write0(" words)\n\n");

    /* 检查哨兵完整性 */
    uint32_t corrupted = stack_sentinel_check();
    sh_write0("    Sentinel check: ");
    sh_write_dec(corrupted);
    sh_write0(" words corrupted\n");

    /* 当前 SP 位置 */
    uint32_t sp = stack_guard_get_sp();
    sh_write0("    Current SP:    0x");
    sh_write_hex(sp);
    sh_write0("\n");

    /* SP 安全检查 */
    int safe = stack_guard_check();
    sh_write0("    SP in range:   ");
    sh_write0(safe ? "YES (safe)" : "NO (DANGER!)");
    sh_write0("\n\n");

    /* 估算栈使用量 */
    uint32_t usage = stack_usage_estimate();
    sh_write0("    Stack usage estimate: ");
    sh_write_dec(usage);
    sh_write0(" bytes\n");

    sh_write0("    (0 = stack has not reached sentinel area)\n\n");
}

/* --------------------------------------------------------------------------
 * 故障日志 — 跨复位诊断
 * --------------------------------------------------------------------------
 * 故障日志存储在 .noinit section 中:
 *   - 上电复位: 值为不确定 (magic 检查失败), 视为干净启动
 *   - 软复位/WDT复位: RAM 内容保留, magic 匹配, 日志有效
 */
static void demo_fault_log(void)
{
    sh_write0("[4] Fault Log (Cross-Reset Diagnostics)\n\n");

    sh_write0("    Fault log stored in .noinit RAM section\n");
    sh_write0("    (preserved across soft/WDT reset).\n\n");

    /* 检查是否有上次的故障日志 */
    sh_write0("    Checking for previous fault log...\n");
    const fault_log_t *log = fault_log_get();

    if (log != NULL)
    {
        sh_write0("    Found! Previous system fault:\n");
        fault_log_print();

        /* 清除旧日志 */
        fault_log_clear();
        sh_write0("    (old fault log cleared)\n\n");
    }
    else
    {
        sh_write0("    No previous fault (clean boot).\n\n");
    }

    /* 演示写入故障日志 */
    sh_write0("    Writing a test fault log...\n");
    fault_log_write(ERR_TIMEOUT, 0x00001234, 42);

    log = fault_log_get();
    if (log)
    {
        sh_write0("    Test log written and verified (CRC check passed):\n");
        fault_log_print();
    }
    sh_write0("\n");
}

/* --------------------------------------------------------------------------
 * 演示 5: CRC 数据完整性
 * --------------------------------------------------------------------------
 * CRC-32 用于验证数据在存储/传输中未被损坏.
 *
 * 典型场景:
 *   - 配置结构体写入 Flash 前计算 CRC
 *   - 读取配置时验证 CRC — 不匹配则使用默认值
 *   - 固件镜像验证 (Bootloader)
 */
static void demo_crc(void)
{
    sh_write0("[5] CRC-32 Data Integrity\n\n");

    /* --- 基本 CRC 计算 --- */
    const char *test_str = "Hello, embedded world!";
    uint32_t crc = crc32(test_str, 22); /* 22 = length of string */

    sh_write0("    CRC32('Hello, embedded world!') = 0x");
    sh_write_hex(crc);
    sh_write0("\n\n");

    /* --- CRC 校验示例 --- */
    sh_write0("    Data integrity check pattern:\n\n");

    /* 构造一个配置结构体 */
    typedef struct
    {
        uint32_t device_id;
        uint32_t baud_rate;
        uint8_t flags;
        uint32_t crc; /* CRC 放在最后 */
    } config_t;

    config_t cfg;
    cfg.device_id = 0x12345678;
    cfg.baud_rate = 115200;
    cfg.flags = 0xAA;

    /* 计算 CRC (不含 crc 字段) 并附加 */
    cfg.crc = crc32(&cfg, sizeof(config_t) - 4);

    sh_write0("    Config struct:\n");
    sh_write0("      device_id = 0x");
    sh_write_hex(cfg.device_id);
    sh_write0("\n");
    sh_write0("      baud_rate = ");
    sh_write_dec(cfg.baud_rate);
    sh_write0("\n");
    sh_write0("      flags     = 0x");
    sh_write_hex(cfg.flags);
    sh_write0("\n");
    sh_write0("      CRC       = 0x");
    sh_write_hex(cfg.crc);
    sh_write0("\n\n");

    /* 验证: 对完整结构体计算 CRC 应为 0x2144DF1C */
    uint32_t verify = crc32(&cfg, sizeof(config_t));
    sh_write0("    Verify: CRC32(entire struct) = 0x");
    sh_write_hex(verify);
    sh_write0("\n");
    sh_write0("    Expected check value:         0x2144DF1C\n");
    sh_write0("    (match = data is intact)\n\n");

    /* --- 演示损坏检测 --- */
    sh_write0("    Simulating data corruption:\n");
    uint32_t saved_crc = cfg.crc;
    cfg.flags = 0x00; /* 模拟数据损坏 */
    cfg.crc = crc32(&cfg, sizeof(config_t) - 4);

    sh_write0("      Original CRC: 0x");
    sh_write_hex(saved_crc);
    sh_write0("\n");
    sh_write0("      After change: 0x");
    sh_write_hex(cfg.crc);
    sh_write0("\n");
    sh_write0("      CRC changed → data corruption detected!\n\n");
}

/* --------------------------------------------------------------------------
 * 被调用函数: 模拟深层调用栈 (用于栈使用量演示)
 * --------------------------------------------------------------------------
 * 故意用几层调用来消耗栈空间, 展示栈哨兵的工作原理.
 */
static void deep_call_level3(void)
{
    volatile uint32_t local_arr[16]; /* 64 字节局部变量 */
    for (int i = 0; i < 16; i++)
    {
        local_arr[i] = i;
    }
    sh_write0("      deep_call_level3() called, SP = 0x");
    sh_write_hex(stack_guard_get_sp());
    sh_write0("\n");
    (void)local_arr;
}

static void deep_call_level2(void)
{
    volatile uint32_t local_arr[16];
    for (int i = 0; i < 16; i++)
    {
        local_arr[i] = i;
    }
    deep_call_level3();
    (void)local_arr;
}

static void deep_call_level1(void)
{
    volatile uint32_t local_arr[16];
    for (int i = 0; i < 16; i++)
    {
        local_arr[i] = i;
    }
    deep_call_level2();
    (void)local_arr;
}

/* --------------------------------------------------------------------------
 * 演示 6: 防御性编程实践
 * --------------------------------------------------------------------------
 * 真实代码中防御性编程的几个关键模式.
 */
static void demo_defensive(void)
{
    sh_write0("[6] Defensive Programming Patterns\n\n");

    /* --- 模式 1: 输入验证 --- */
    sh_write0("    Pattern 1: Input validation\n");
    sh_write0("      void set_pwm_duty(uint8_t percent) {\n");
    sh_write0("          if (percent > 100) {\n");
    sh_write0("              error_record(ERR_INVALID_PARAM);\n");
    sh_write0("              return;  // clamp or reject\n");
    sh_write0("          }\n");
    sh_write0("          // ... set duty cycle\n");
    sh_write0("      }\n\n");

    /* --- 模式 2: 配置 CRC 保护 --- */
    sh_write0("    Pattern 2: Config with CRC\n");
    sh_write0("      config_t load_config(void) {\n");
    sh_write0("          config_t cfg;\n");
    sh_write0("          flash_read(CONFIG_ADDR, &cfg, sizeof(cfg));\n");
    sh_write0("          uint32_t calc = crc32(&cfg, sizeof(cfg)-4);\n");
    sh_write0("          if (calc != cfg.crc) {\n");
    sh_write0("              return config_defaults;  // fallback!\n");
    sh_write0("          }\n");
    sh_write0("          return cfg;\n");
    sh_write0("      }\n\n");

    /* --- 模式 3: 超时保护 --- */
    sh_write0("    Pattern 3: Timeout guard\n");
    sh_write0("      uint32_t deadline = now() + TIMEOUT_MS;\n");
    sh_write0("      while (!uart_tx_ready()) {\n");
    sh_write0("          if (now() > deadline) {\n");
    sh_write0("              error_record(ERR_TIMEOUT);\n");
    sh_write0("              return ERR_TIMEOUT;\n");
    sh_write0("          }\n");
    sh_write0("      }\n\n");

    /* --- 模式 4: 栈监控 --- */
    sh_write0("    Pattern 4: Periodic stack check\n");
    sh_write0("      void main_loop(void) {\n");
    sh_write0("          while (1) {\n");
    sh_write0("              do_work();\n");
    sh_write0("              if (stack_sentinel_check() != 0) {\n");
    sh_write0("                  fault_log_write(ERR_FATAL_STACK,...);\n");
    sh_write0("                  system_reset();  // safe restart\n");
    sh_write0("              }\n");
    sh_write0("          }\n");
    sh_write0("      }\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 7: 深层调用与栈哨兵验证
 * --------------------------------------------------------------------------
 */
static void demo_deep_call(void)
{
    sh_write0("[7] Deep Call Chain & Stack Sentinel\n\n");

    sh_write0("    Calling deep nested functions to exercise stack...\n");
    sh_write0("    (each level allocates 64 bytes of locals)\n");

    deep_call_level1();

    sh_write0("\n    After deep calls — sentinel check: ");
    uint32_t corr = stack_sentinel_check();
    sh_write_dec(corr);
    sh_write0(" words corrupted\n");
    sh_write0("    Stack usage estimate: ");
    sh_write_dec(stack_usage_estimate());
    sh_write0(" bytes\n\n");
}

/* --------------------------------------------------------------------------
 * main — 主程序入口
 * -------------------------------------------------------------------------- */
int main(void)
{
    sh_write0("\n==============================================\n");
    sh_write0("  Lesson 11: Error Handling\n");
    sh_write0("  Defensive Programming for Embedded Systems\n");
    sh_write0("==============================================\n\n");

    sh_write0(
        "In production embedded systems, errors are inevitable.\n"
        "The key is: detect them early, handle them gracefully,\n"
        "and leave enough diagnostic info to fix them.\n\n");

    /* 0. 首先初始化栈哨兵 */
    stack_sentinel_init();

    /* 1. 断言系统 */
    demo_assert();

    /* 2. 错误码系统 */
    demo_error_codes();

    /* 3. 栈溢出检测 */
    demo_stack_guard();

    /* 4. 故障日志 (检查上次是否有故障) */
    demo_fault_log();

    /* 5. CRC 数据完整性 */
    demo_crc();

    /* 6. 防御性编程模式 */
    demo_defensive();

    /* 7. 深层调用演示 */
    demo_deep_call();

    /* --- 总结 --- */
    sh_write0("==============================================\n");
    sh_write0("  Key Takeaways:\n\n");
    sh_write0("  1. ASSERT catches bugs early (debug builds).\n");
    sh_write0("  2. ASSERT_ALWAYS for safety-critical conditions.\n");
    sh_write0("  3. STATIC_ASSERT catches bugs at compile time.\n");
    sh_write0("  4. Error codes enable structured error handling.\n");
    sh_write0("  5. Stack sentinel detects overflow risks.\n");
    sh_write0("  6. Fault logs survive reset for diagnostics.\n");
    sh_write0("  7. CRC protects critical data from corruption.\n");
    sh_write0("  8. Defensive code validates inputs and timeouts.\n");
    sh_write0("==============================================\n\n");

    sh_write0("=== Lesson 11 complete! ===\n\n");

    /* 正常退出 (semihosting) */
    register int r0 __asm__("r0") = 0x18;
    register int r1 __asm__("r1") = 0;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");

    while (1)
    {
    }
    return 0;
}
