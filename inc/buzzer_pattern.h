#ifndef BUZZER_PATTERN_H
#define BUZZER_PATTERN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 类型定义 ---- */

typedef void (*buzzer_write_fn)(uint8_t level);

typedef struct {
    uint8_t  on;            /* 1 = 蜂鸣器开, 0 = 蜂鸣器关 */
    uint16_t duration_ms;   /* 持续时间 (毫秒) */
} buzzer_step_t;

typedef struct {
    const buzzer_step_t *steps;     /* Step 数组指针 */
    uint8_t              length;    /* Step 数组长度 */
    uint8_t              repeat;    /* 总播放次数 (0 = 无限循环) */
} buzzer_pattern_t;

typedef enum {
    BUZZER_PRIORITY_LOW,        /* 按键反馈等 */
    BUZZER_PRIORITY_NORMAL,     /* 操作提示、测量完成 */
    BUZZER_PRIORITY_HIGH,       /* 故障报警 */
} buzzer_priority_t;

/* ---- API ---- */

void    buzzer_pattern_init(buzzer_write_fn write_fn);
void    buzzer_pattern_play(const buzzer_pattern_t *pattern, buzzer_priority_t priority);
void    buzzer_pattern_stop(void);
void    buzzer_pattern_process(void);
uint8_t buzzer_pattern_is_idle(void);

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_PATTERN_H */
