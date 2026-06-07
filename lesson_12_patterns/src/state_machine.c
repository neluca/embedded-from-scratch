/*
 * =============================================================================
 * state_machine.c -- 有限状态机实现
 * =============================================================================
 */
#include "state_machine.h"
#include <stddef.h>

/* =========================================================================
 * 方式 1: 枚举 + switch FSM — UART 包解析器
 * =========================================================================
 *
 * 包格式: [SYNC 0xAA] [LENGTH N] [PAYLOAD N bytes] [CHECKSUM]
 * CHECKSUM = (Sync + Length + sum of Payload) & 0xFF
 *
 * 状态转换:
 *   IDLE:     收到 0xAA → HEADER (checksum = 0xAA)
 *   IDLE:     收到其他 → 保持 IDLE
 *   HEADER:   收到长度 → PAYLOAD (重置 index/checksum)
 *   PAYLOAD:  收到 N 个字节 → CHECKSUM
 *   CHECKSUM: 校验通过 → IDLE (完整包!), 失败 → IDLE
 */

int pkt_parser_feed(pkt_parser_t *p, uint8_t byte)
{
    switch (p->state)
    {
    case PKT_STATE_IDLE:
        if (byte == 0xAA)
        {
            p->state = PKT_STATE_HEADER;
            p->checksum = 0xAA; /* 初始化累加和 */
        }
        /* 否则: 忽略, 保持 IDLE */
        break;

    case PKT_STATE_HEADER:
        if (byte > 0 && byte <= 32)
        { /* 长度 1-32 */
            p->length = byte;
            p->index = 0;
            p->state = PKT_STATE_PAYLOAD;
            p->checksum += byte;
        }
        else
        {
            p->state = PKT_STATE_IDLE; /* 无效长度, 重新同步 */
        }
        break;

    case PKT_STATE_PAYLOAD:
        p->payload[p->index] = byte;
        p->checksum += byte;
        p->index++;
        if (p->index >= p->length)
        {
            p->state = PKT_STATE_CHECKSUM;
        }
        break;

    case PKT_STATE_CHECKSUM:
        if (byte == (p->checksum & 0xFF))
        {
            p->packets_rx++;
            p->state = PKT_STATE_IDLE;
            return 1; /* 完整包接收成功 */
        }
        /* 校验失败: 丢弃, 重新开始 */
        p->state = PKT_STATE_IDLE;
        break;
    }

    return 0; /* 包未完成 */
}

const char *pkt_state_name(pkt_state_t s)
{
    switch (s)
    {
    case PKT_STATE_IDLE:
        return "IDLE";
    case PKT_STATE_HEADER:
        return "HEADER";
    case PKT_STATE_PAYLOAD:
        return "PAYLOAD";
    case PKT_STATE_CHECKSUM:
        return "CHECKSUM";
    default:
        return "UNKNOWN";
    }
}

void pkt_parser_reset(pkt_parser_t *p)
{
    p->state = PKT_STATE_IDLE;
    p->length = 0;
    p->index = 0;
    p->checksum = 0;
    p->packets_rx = 0;
}

/* =========================================================================
 * 方式 2: 表驱动 FSM — 通用框架
 * ========================================================================= */

void tfsm_init(tfsm_t *fsm, int num_states, int num_events,
               tfsm_handler_t *handlers, const int *transitions)
{
    fsm->state = 0; /* 从状态 0 开始 */
    fsm->num_states = num_states;
    fsm->num_events = num_events;
    fsm->handlers = handlers;
    fsm->transitions = transitions;
}

int tfsm_feed(tfsm_t *fsm, int event, void *ctx)
{
    if (event < 0 || event >= fsm->num_events)
    {
        return fsm->state;
    }

    /* 查表: next_state = transitions[current_state * num_events + event] */
    int next_state = fsm->transitions[fsm->state * fsm->num_events + event];

    /* 如果有处理函数, 调用它 (返回值覆盖查表结果) */
    if (fsm->handlers && fsm->handlers[fsm->state])
    {
        int handler_result = fsm->handlers[fsm->state](fsm, event, ctx);
        if (handler_result >= 0)
        {
            next_state = handler_result;
        }
    }

    fsm->state = next_state;
    return fsm->state;
}

/* 交通灯示例数据 (由 main.c 中的 traffic_light_demo 使用) */
const int traffic_transitions[3][1] = {
    { 2 }, /* RED    + TIMER → GREEN */
    { 0 }, /* YELLOW + TIMER → RED */
    { 1 }, /* GREEN  + TIMER → YELLOW */
};
