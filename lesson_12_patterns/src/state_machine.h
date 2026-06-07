/*
 * =============================================================================
 * state_machine.h -- 有限状态机 (Finite State Machine)
 * =============================================================================
 *
 * 状态机是嵌入式系统中最通用的控制流模式.
 * 跨架构、跨语言 — ARM/RISC-V/x86 完全一致, C/Python/Rust 均可实现.
 *
 * 两种实现方式:
 *
 * 1. 枚举 + switch (简单 FSM)
 *    优点: 直观, 易调试, 无额外内存
 *    缺点: 状态多时 switch 变长, 添加状态需要修改函数
 *
 * 2. 表驱动 (复杂 FSM)
 *    优点: 添加状态/事件不修改核心逻辑, 数据与代码分离
 *    缺点: 额外的转换表内存, 调试稍复杂
 *
 * 本模块展示两种方式, 以 UART 数据包解析器为例:
 *
 *   数据包格式: [SYNC 0xAA] [LENGTH N] [PAYLOAD N bytes] [CHECKSUM]
 *
 *   状态转换:
 *     IDLE  --收到0xAA--> HEADER
 *     HEADER --收到长度--> PAYLOAD
 *     PAYLOAD --收完N字节--> CHECKSUM
 *     CHECKSUM --校验通过--> IDLE (输出完整包)
 */
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>

/* =========================================================================
 * 方式 1: 枚举 + switch FSM — UART 包解析器
 * ========================================================================= */

typedef enum
{
    PKT_STATE_IDLE,     /* 等待同步字 0xAA */
    PKT_STATE_HEADER,   /* 等待长度字节 */
    PKT_STATE_PAYLOAD,  /* 接收负载数据 */
    PKT_STATE_CHECKSUM, /* 验证校验和 */
} pkt_state_t;

/* 包解析器上下文 */
typedef struct
{
    pkt_state_t state;       /* 当前状态 */
    uint8_t payload[32]; /* 负载缓冲区 */
    uint8_t length;      /* 负载长度 */
    uint8_t index;       /* 当前接收位置 */
    uint8_t checksum;    /* 累加校验和 */
    uint32_t packets_rx;  /* 成功接收的包计数 */
} pkt_parser_t;

/* 喂入一个字节, 返回 1=接收到完整数据包, 0=未完成 */
int pkt_parser_feed(pkt_parser_t *p, uint8_t byte);

/* 获取解析器状态名 */
const char *pkt_state_name(pkt_state_t s);

/* 重置解析器 */
void pkt_parser_reset(pkt_parser_t *p);

/* =========================================================================
 * 方式 2: 表驱动 FSM — 通用框架
 * =========================================================================
 * 将状态转换规则存储在 const 表中, 引擎代码只有几行.
 *
 * 适用场景:
 *   - 状态数 > 8 的复杂协议
 *   - 需要在运行时动态加载转换表
 *   - 需要在不修改核心代码的情况下扩展功能
 */

/* 前向声明 */
typedef struct tfsm_t tfsm_t;

/* 状态处理函数: 返回下一个状态 ID */
typedef int (*tfsm_handler_t)(tfsm_t *fsm, int event, void *ctx);

struct tfsm_t
{
    int state;       /* 当前状态 */
    int num_states;  /* 状态总数 */
    int num_events;  /* 事件总数 */
    tfsm_handler_t *handlers;    /* 处理函数表 [state] */
    const int *transitions; /* 转换表 [state * num_events + event] */
};

/* 初始化表驱动 FSM */
void tfsm_init(tfsm_t *fsm, int num_states, int num_events,
               tfsm_handler_t *handlers, const int *transitions);

/* 喂入一个事件. 返回新的状态 ID */
int tfsm_feed(tfsm_t *fsm, int event, void *ctx);

/* 交通灯 FSM 转换表 (0=RED, 1=YELLOW, 2=GREEN; 事件=TIMER) */
extern const int traffic_transitions[3][1];

#endif /* STATE_MACHINE_H */
