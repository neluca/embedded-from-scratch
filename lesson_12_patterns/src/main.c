/*
 * =============================================================================
 * Lesson 12 主程序 — 嵌入式通用数据结构与设计模式
 * =============================================================================
 *
 * 学习目标:
 * 1. 掌握环形缓冲区 (所有流式数据处理的基础)
 * 2. 理解状态机两种实现 (枚举 vs 表驱动)
 * 3. 精通位操作 (跨架构通用的寄存器操作基本功)
 * 4. 了解 COBS 数据帧编码 (跨平台通信协议设计)
 *
 * 为什么这些是"举一反三"的核心?
 *   这些模式不依赖于任何特定芯片.
 *   ARM、RISC-V、MIPS、x86 — 环形缓冲区、状态机、位操作的
 *   实现逻辑完全一致. 学会一次, 终身受用.
 * =============================================================================
 */

#include "bit_utils.h"
#include "cobs_framer.h"
#include "ring_buffer.h"
#include "state_machine.h"
#include <stddef.h>
#include <stdint.h>

/* Semihosting 输出 */
static void sh_write0(const char *s)
{
    register int r0 __asm__("r0") = 0x04;
    register const char *r1 __asm__("r1") = s;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
}
static void sh_write_hex(uint32_t v)
{
    char b[] = "00000000 ";
    for (int i = 7; i >= 0; i--)
    {
        uint32_t n = v & 0xF;
        b[i] = n < 10 ? '0' + n : 'A' + n - 10;
        v >>= 4;
    }
    sh_write0(b);
}
static void sh_write_dec(uint32_t v)
{
    char buf[12];
    int pos = sizeof(buf) - 1;
    buf[pos] = '\0';
    if (v == 0)
    {
        buf[--pos] = '0';
    }
    else
    {
        while (v > 0)
        {
            buf[--pos] = '0' + (v % 10);
            v /= 10;
        }
    }
    sh_write0(&buf[pos]);
}

/* --------------------------------------------------------------------------
 * 演示 1: 环形缓冲区
 * --------------------------------------------------------------------------
 * 展示: 初始化 → 写入 → 读取 → 环绕 → 满/空检测 → 批量操作
 */
static void demo_ring_buffer(void)
{
    sh_write0("[1] Ring Buffer (Circular Buffer)\n\n");

    /* 创建 16 字节的环形缓冲区 (大小 = 2^4 = 16) */
    uint8_t buf_mem[16];
    ring_buf_t rb;
    ring_buf_init(&rb, buf_mem, 16);

    sh_write0("    Buffer created: 16 bytes, mask = 0x0F\n");
    sh_write0("    (power-of-2 size → fast bitwise AND instead of modulo)\n\n");

    /* --- 基本操作 --- */
    sh_write0("    Basic put/get:\n");
    ring_buf_put(&rb, 'A');
    ring_buf_put(&rb, 'B');
    ring_buf_put(&rb, 'C');
    sh_write0("      Put: 'A', 'B', 'C'\n");
    sh_write0("      Available: ");
    sh_write_dec(ring_buf_available(&rb));
    sh_write0(" (expect 3)\n");
    sh_write0("      Free:      ");
    sh_write_dec(ring_buf_free(&rb));
    sh_write0(" (expect 12, 1 reserved)\n");

    uint8_t ch;
    char tmp[2];
    ring_buf_get(&rb, &ch);
    tmp[0] = (char)ch;
    tmp[1] = '\0';
    sh_write0("      Get: '");
    sh_write0(tmp);
    sh_write0("'\n");
    ring_buf_get(&rb, &ch);
    tmp[0] = (char)ch;
    sh_write0("      Get: '");
    sh_write0(tmp);
    sh_write0("'\n");
    sh_write0("      Available: ");
    sh_write_dec(ring_buf_available(&rb));
    sh_write0(" (expect 1)\n\n");

    /* --- 批量操作 --- */
    sh_write0("    Bulk write/read:\n");
    ring_buf_reset(&rb);
    const char *msg = "Hello!";
    uint32_t n = ring_buf_write(&rb, (const uint8_t *)msg, 6);
    sh_write0("      Wrote: ");
    sh_write_dec(n);
    sh_write0(" bytes\n");

    char readback[16];
    for (int i = 0; i < 16; i++)
        readback[i] = 0;
    n = ring_buf_read(&rb, (uint8_t *)readback, 16);
    sh_write0("      Read:  \"");
    sh_write0(readback);
    sh_write0("\" (");
    sh_write_dec(n);
    sh_write0(" bytes)\n\n");

    /* --- 环绕演示 --- */
    sh_write0("    Wrap-around demonstration:\n");
    ring_buf_reset(&rb);
    /* 先写 14 个字节 (留 1 个空槽, 所以最多写 15-已读 = 15) */
    for (int i = 0; i < 14; i++)
        ring_buf_put(&rb, 'A' + (uint8_t)i);
    sh_write0("      After 14 puts: avail=");
    sh_write_dec(ring_buf_available(&rb));
    sh_write0(", free=");
    sh_write_dec(ring_buf_free(&rb));
    sh_write0("\n");

    /* 读 10 个, head 前移, 空间释放 */
    for (int i = 0; i < 10; i++)
        ring_buf_get(&rb, &ch);
    sh_write0("      After 10 gets: avail=");
    sh_write_dec(ring_buf_available(&rb));
    sh_write0(", free=");
    sh_write_dec(ring_buf_free(&rb));
    sh_write0("\n");

    /* 再写 10 个 → 环绕! 新数据从 buffer 开头继续写 */
    for (int i = 0; i < 10; i++)
        ring_buf_put(&rb, '0' + (uint8_t)i);
    sh_write0("      After 10 more puts (wrapped!): avail=");
    sh_write_dec(ring_buf_available(&rb));
    sh_write0(", free=");
    sh_write_dec(ring_buf_free(&rb));
    sh_write0("\n\n");

    sh_write0("    Key insight: head/tail always move forward (modulo size).\n");
    sh_write0("    No memory copy needed for wrap-around!\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 2: 状态机 — 包解析器
 * --------------------------------------------------------------------------
 */
static void demo_state_machine(void)
{
    sh_write0("[2] State Machine — Packet Parser\n\n");

    sh_write0("    Packet format: [SYNC 0xAA] [LEN N] [DATA N bytes] [CHKSUM]\n");
    sh_write0("    States: IDLE → HEADER → PAYLOAD → CHECKSUM\n\n");

    /* 模拟接收一个有效包: 0xAA, 0x03, 0x10, 0x20, 0x30, 0x0D
     * Checksum = (0xAA + 0x03 + 0x10 + 0x20 + 0x30) & 0xFF = 0x0D */
    pkt_parser_t parser;
    pkt_parser_reset(&parser);

    uint8_t test_pkt[] = { 0xAA, 0x03, 0x10, 0x20, 0x30, 0x0D };
    int result = 0;

    sh_write0("    Feeding bytes: ");
    for (uint32_t i = 0; i < sizeof(test_pkt); i++)
    {
        sh_write_hex(test_pkt[i]);
        sh_write0(" ");

        result = pkt_parser_feed(&parser, test_pkt[i]);

        /* 每个字节之后显示当前状态 */
        sh_write0("[");
        sh_write0(pkt_state_name(parser.state));
        sh_write0("] ");
    }
    sh_write0("\n");

    if (result)
    {
        sh_write0("    Packet received! Payload: ");
        for (uint8_t i = 0; i < parser.length; i++)
        {
            sh_write_hex(parser.payload[i]);
            sh_write0(" ");
        }
        sh_write0("\n");
    }
    sh_write0("    Total packets received: ");
    sh_write_dec(parser.packets_rx);
    sh_write0("\n\n");

    /* --- 错误恢复: 在 PAYLOAD 中间发送无效字节 --- */
    sh_write0("    Error recovery test:\n");
    pkt_parser_reset(&parser);

    /* 喂入: 0xAA, 0x02, 0x11, [错误: 0xAA 不是 checksum 但出现], 重新同步... */
    /* 实际: 0xAA → HEADER, 0x02 → PAYLOAD (等2字节), 0x11 + any → CHECKSUM, wrong → IDLE */
    uint8_t bad_seq[] = { 0xAA, 0x02, 0x11, 0xBB };
    for (uint32_t i = 0; i < sizeof(bad_seq); i++)
    {
        pkt_parser_feed(&parser, bad_seq[i]);
    }
    sh_write0("    After invalid packet: state = ");
    sh_write0(pkt_state_name(parser.state));
    sh_write0(", packets_rx = ");
    sh_write_dec(parser.packets_rx);
    sh_write0(" (no increment for bad packet)\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 3: 表驱动状态机 — 交通灯
 * --------------------------------------------------------------------------
 */
/* 外部数据 (from state_machine.c) */
extern const int traffic_transitions[3][1];

static void demo_table_fsm(void)
{
    sh_write0("[3] Table-Driven State Machine\n\n");

    sh_write0("    Transition table stored in const array.\n");
    sh_write0("    Adding new states = adding rows to array.\n");
    sh_write0("    Core engine code never changes!\n\n");

    /* Traffic light: 状态 0=RED, 1=YELLOW, 2=GREEN, 事件 0=TIMER */
    tfsm_t fsm;
    tfsm_init(&fsm, 3, 1, NULL, (const int *)traffic_transitions);

    const char *names[] = { "RED", "YELLOW", "GREEN" };
    int current = 0;

    sh_write0("    Traffic light FSM (8 timer ticks):\n");
    for (int i = 0; i < 8; i++)
    {
        current = tfsm_feed(&fsm, 0, NULL);
        sh_write0("      Tick ");
        sh_write_dec(i);
        sh_write0(": ");
        sh_write0(names[current]);
        sh_write0("\n");
    }

    sh_write0("\n");
    sh_write0("    Key insight:\n");
    sh_write0("      enum FSM:  simple, debuggable, best for < 8 states.\n");
    sh_write0("      table FSM: scalable, data-driven, best for > 8 states.\n");
    sh_write0("      Both patterns are architecture-independent.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 4: 位操作
 * --------------------------------------------------------------------------
 */
static void demo_bit_manipulation(void)
{
    sh_write0("[4] Bit Manipulation\n\n");

    /* --- 单比特操作 --- */
    sh_write0("    Single-bit operations:\n");
    uint32_t reg = 0;

    BIT_SET(reg, 3); /* reg = 0x08 */
    sh_write0("      BIT_SET(reg, 3)             → 0x");
    sh_write_hex(reg);

    BIT_SET(reg, 0); /* reg = 0x09 */
    sh_write0("      BIT_SET(reg, 0)             → 0x");
    sh_write_hex(reg);

    BIT_CLEAR(reg, 3); /* reg = 0x01 */
    sh_write0("      BIT_CLEAR(reg, 3)           → 0x");
    sh_write_hex(reg);

    sh_write0("      BIT_TEST(reg, 0)            → ");
    sh_write_dec(BIT_TEST(reg, 0));
    sh_write0("\n");
    sh_write0("      BIT_TEST(reg, 3)            → ");
    sh_write_dec(BIT_TEST(reg, 3));
    sh_write0("\n\n");

    /* --- 位域提取/插入 --- */
    sh_write0("    Bit-field extract/insert:\n");
    reg = 0xAB; /* 1010 1011 */
    sh_write0("      reg = 0xAB (1010 1011)\n");
    sh_write0("      BF_EXTRACT(reg, 0, 3)       → 0x");
    sh_write_hex(BF_EXTRACT(reg, 0, 3)); /* 0x0B */
    sh_write0("      BF_EXTRACT(reg, 4, 7)       → 0x");
    sh_write_hex(BF_EXTRACT(reg, 4, 7)); /* 0x0A */
    sh_write0("\n\n");

    /* --- 端序 --- */
    sh_write0("    Endianness:\n");
    sh_write0("      System is: ");
    sh_write0(is_little_endian() ? "LITTLE-ENDIAN" : "BIG-ENDIAN");
    sh_write0("\n");
    sh_write0("      swap16(0x1234)              = 0x");
    sh_write_hex(swap16(0x1234));
    sh_write0("      swap32(0x12345678)          = 0x");
    sh_write_hex(swap32(0x12345678));
    sh_write0("\n\n");

    /* --- 结构体打包 --- */
    sh_write0("    Struct packing:\n");
    sh_write0("      sizeof(aligned_struct_t)    = ");
    sh_write_dec(sizeof(aligned_struct_t));
    sh_write0(" (with padding)\n");
    sh_write0("      sizeof(packed_struct_t)     = ");
    sh_write_dec(sizeof(packed_struct_t));
    sh_write0(" (no padding, but unaligned!)\n");
    sh_write0("      M0 cannot access unaligned uint32_t! Use memcpy.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 5: COBS 数据帧
 * --------------------------------------------------------------------------
 */
static void demo_cobs_framing(void)
{
    sh_write0("[5] COBS Data Framing\n\n");

    /* 原始数据 (包含 0x00) */
    uint8_t original[] = { 0x45, 0x00, 0x00, 0x55, 0x00, 0x00, 0x04, 0x00 };
    sh_write0("    Original data (8 bytes):\n      ");
    for (uint32_t i = 0; i < 8; i++)
    {
        sh_write_hex(original[i]);
    }
    sh_write0("\n\n");

    /* COBS 编码 */
    uint8_t encoded[32];
    uint32_t enc_len = cobs_encode(original, 8, encoded);

    sh_write0("    Encoded (");
    sh_write_dec(enc_len);
    sh_write0(" bytes, overhead = ");
    sh_write_dec(enc_len - 8);
    sh_write0("):\n      ");
    for (uint32_t i = 0; i < enc_len; i++)
    {
        sh_write_hex(encoded[i]);
    }
    sh_write0("\n\n");

    /* COBS 解码 */
    uint8_t decoded[32];
    uint32_t dec_len = cobs_decode(encoded, enc_len, decoded);

    sh_write0("    Decoded (");
    sh_write_dec(dec_len);
    sh_write0(" bytes):\n      ");
    for (uint32_t i = 0; i < dec_len; i++)
    {
        sh_write_hex(decoded[i]);
    }
    sh_write0("\n");

    /* 验证 */
    int match = (dec_len == 8);
    for (uint32_t i = 0; i < dec_len && match; i++)
    {
        if (decoded[i] != original[i])
            match = 0;
    }
    sh_write0("    Round-trip: ");
    sh_write0(match ? "MATCH (data intact)" : "MISMATCH!");
    sh_write0("\n\n");

    sh_write0("    Key insight: COBS guarantees deterministic overhead.\n");
    sh_write0("    Worst case: +0.4% (vs SLIP's +100%).\n");
    sh_write0("    This algorithm works identically on any CPU.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 6: 综合 — 用环形缓冲 + 状态机 + COBS 构建协议栈
 * --------------------------------------------------------------------------
 */
static void demo_integration(void)
{
    sh_write0("[6] Integration: Ring Buffer + FSM + COBS\n\n");

    sh_write0("    Real UART protocol stack:\n\n");
    sh_write0("    +----------------------------------------+\n");
    sh_write0("    | Application                            |\n");
    sh_write0("    |   ↓  decoded packets                   |\n");
    sh_write0("    | COBS Decoder                           |\n");
    sh_write0("    |   ↓  raw bytes (no 0x00)              |\n");
    sh_write0("    | Ring Buffer (ISR→main thread bridge)   |\n");
    sh_write0("    |   ↓  bytes from UART RX ISR           |\n");
    sh_write0("    | Packet Parser FSM (IDLE→HDR→PAY→CHK)  |\n");
    sh_write0("    |   ↓  validated packets                |\n");
    sh_write0("    | Application Logic                      |\n");
    sh_write0("    +----------------------------------------+\n\n");

    sh_write0("    Every layer is architecture-independent:\n");
    sh_write0("    - Ring buffer: works on ARM/RISC-V/x86\n");
    sh_write0("    - FSM: works in C/C++/Rust/Python\n");
    sh_write0("    - COBS: deterministic, portable framing\n\n");
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */
int main(void)
{
    sh_write0("\n==============================================\n");
    sh_write0("  Lesson 12: Embedded Data Structures\n");
    sh_write0("  Universal Patterns for Any Architecture\n");
    sh_write0("==============================================\n\n");

    sh_write0(
        "These patterns are architecture-independent.\n"
        "Learn them once, apply them everywhere — ARM,\n"
        "RISC-V, MIPS, x86 — the logic never changes.\n\n");

    demo_ring_buffer();
    demo_state_machine();
    demo_table_fsm();
    demo_bit_manipulation();
    demo_cobs_framing();
    demo_integration();

    sh_write0("==============================================\n");
    sh_write0("  Key Takeaways:\n\n");
    sh_write0("  1. Ring buffer: the universal streaming data structure.\n");
    sh_write0("  2. FSM (enum): simple, debuggable, best for < 8 states.\n");
    sh_write0("  3. FSM (table): scalable, data-driven, best for > 8 states.\n");
    sh_write0("  4. Bit ops: the daily bread of embedded programming.\n");
    sh_write0("  5. COBS: deterministic framing for serial protocols.\n");
    sh_write0("  6. ALL of the above work identically on any CPU.\n");
    sh_write0("==============================================\n\n");

    sh_write0("=== Lesson 12 complete! ===\n\n");

    register int r0 __asm__("r0") = 0x18;
    register int r1 __asm__("r1") = 0;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
    while (1)
    {
    }
    return 0;
}
