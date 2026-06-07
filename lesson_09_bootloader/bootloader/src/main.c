/*
 * =============================================================================
 * Bootloader — 验证并跳转到应用程序
 * =============================================================================
 * 流程:
 *   1. 输出启动信息
 *   2. 检查 App 区域 (0x00002000) 是否有有效的向量表
 *   3. 读取 App 的初始 SP 和 PC (向量表[0] 和 [1])
 *   4. 设置 SP → App 的栈指针
 *   5. 跳转到 App 的复位处理函数
 *
 * Cortex-M0 注意事项:
 *   - 无 VTOR 寄存器，硬件异常始终通过 0x00000000 的向量表
 *   - 应用程序的向量表在 0x00002000，仅在 bootloader 跳转时手动读取
 *   - App 的中断会回到 bootloader 的向量表 (这是 M0 的硬件限制)
 */

#include <stdint.h>

/* Semihosting helpers */
static void sh_write0(const char *s)
{
    register int         r0 __asm__("r0") = 0x04;
    register const char *r1 __asm__("r1") = s;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
}
static void sh_write_hex(uint32_t v)
{
    char b[] = "0x00000000\n";
    for (int i = 9; i >= 2; i--) { uint32_t n = v & 0xF; b[i] = n < 10 ? '0' + n : 'A' + n - 10; v >>= 4; }
    sh_write0(b);
}

/* App 在 Flash 中的位置 */
#define APP_START_ADDR  0x00002000
#define APP_MAGIC_ADDR  (APP_START_ADDR + 4)  /* 向量表[1] = reset_handler 地址 */
#define APP_MAGIC_MIN   0x00002000            /* PC 至少应该在 App 区域内 */
#define APP_MAGIC_MAX   0x00040000            /* Flash 结束 */

/* App 向量表结构: [0]=SP, [1]=PC, [2]=NMI, ... */
typedef struct {
    uint32_t sp;
    uint32_t pc;
} app_vector_t;

/*
 * bootloader_main — Bootloader 入口
 */
int bootloader_main(void)
{
    sh_write0("\n=== Bootloader ===\n");
    sh_write0("Addr: 0x00000000 (Flash start)\n");

    /* 读取 App 区域的向量表 */
    const app_vector_t *app = (const app_vector_t *)APP_START_ADDR;

    sh_write0("App vector table @ ");
    sh_write_hex(APP_START_ADDR);
    sh_write0("App SP = ");
    sh_write_hex(app->sp);
    sh_write0("App PC = ");
    sh_write_hex(app->pc);

    /* 验证 App 是否有效 */
    if (app->sp < 0x20000000 || app->sp > 0x20004000) {
        sh_write0("ERROR: App SP out of RAM range!\n");
        sh_write0("Halted.\n");
        while (1) {}
    }
    if (app->pc < APP_MAGIC_MIN || app->pc > APP_MAGIC_MAX) {
        sh_write0("ERROR: App PC out of Flash range!\n");
        sh_write0("Halted.\n");
        while (1) {}
    }
    if ((app->pc & 1) == 0) {
        sh_write0("ERROR: App PC bit[0]=0 (not Thumb)!\n");
        sh_write0("Halted.\n");
        while (1) {}
    }

    sh_write0("App validated OK.\n");
    sh_write0("Jumping to App...\n\n");

    /*
     * 跳转到 App:
     *   1. 设置 SP = App 的栈指针
     *   2. 跳转到 App 的复位处理函数
     *
     * 使用内联汇编确保精确控制:
     *   msr msp, r0     — 设置主栈指针
     *   bx  r1          — 跳转到 App (bit[0]=1 表示 Thumb)
     */
    uint32_t app_sp = app->sp;
    uint32_t app_pc = app->pc;

    __asm__ volatile (
        "msr MSP, %0\n"     /* SP = App stack pointer */
        "bx  %1\n"          /* PC = App reset handler */
        :
        : "r"(app_sp), "r"(app_pc)
        : "memory"
    );

    /* 不应该到达这里 */
    while (1) {}
    return 0;
}
