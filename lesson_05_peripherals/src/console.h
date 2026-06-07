/*
 * =============================================================================
 * console.h -- 简易串口控制台
 * =============================================================================
 */
#ifndef CONSOLE_H
#define CONSOLE_H

/* 初始化控制台 (初始化 UART) */
void console_init(void);

/* 输出提示符 */
void console_prompt(void);

/* 处理输入的命令 (非阻塞)
 * @return: 1=有命令被处理, 0=无输入
 */
int console_poll(void);

/* 控制台输出 (printf 风格, 简化版) */
void console_write(const char *str);

#endif /* CONSOLE_H */
