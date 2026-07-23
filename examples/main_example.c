/**
 * @file main_example.c
 * @brief BuzzerPattern FreeRTOS 使用示例
 *
 * 用户只需初始化 buzzer_hw 结构体，无需实现 GPIO 回调。
 */

#include "buzzer_pattern.h"

/* ---- 硬件配置 (用户初始化) ---- */

buzzer_hw_t buzzer_hw = {
    .port = GPIOA,
    .pin  = GPIO_PIN_5,
};

/* ---- Pattern 定义 ----
 *
 * steps 数组为 static (文件内部引用)，buzzer_pattern_t 为非 static (外部链接)。
 * 其他文件通过 extern 声明即可访问，例如:
 *   extern const buzzer_pattern_t beep_short;
 */

/** @brief 单次短响 (按键反馈) */
static const buzzer_step_t beep_short_steps[] = {
    { 1, 100 },
    { 0, 100 },
};
const buzzer_pattern_t beep_short = {
    .steps  = beep_short_steps,
    .length = 2,
    .repeat = 1,
};

/** @brief 三短响 (测量完成) */
static const buzzer_step_t beep_done_steps[] = {
    { 1, 60 },
    { 0, 60 },
};
const buzzer_pattern_t beep_done = {
    .steps  = beep_done_steps,
    .length = 2,
    .repeat = 3,
};

/** @brief 错误报警 */
static const buzzer_step_t beep_error_steps[] = {
    { 1, 500 },
    { 0, 500 },
};
const buzzer_pattern_t beep_error = {
    .steps  = beep_error_steps,
    .length = 2,
    .repeat = 3,
};

/** @brief 长鸣报警 (无限循环) */
static const buzzer_step_t beep_alarm_steps[] = {
    { 1, 500 },
};
const buzzer_pattern_t beep_alarm = {
    .steps  = beep_alarm_steps,
    .length = 1,
    .repeat = 0,
};

/* ---- FreeRTOS 示例 ---- */

/*
 * void app_init(void)
 * {
 *     Buzzer_Init();
 * }
 *
 * void sensor_task(void *argument)
 * {
 *     for (;;) {
 *         if (key_pressed) {
 *             Buzzer_Play(&beep_short, BUZZER_PRIORITY_LOW);
 *         }
 *         if (measurement_done) {
 *             Buzzer_Play(&beep_done, BUZZER_PRIORITY_NORMAL);
 *         }
 *         if (error_occurred) {
 *             Buzzer_Play(&beep_error, BUZZER_PRIORITY_HIGH);
 *         }
 *         if (critical_alarm) {
 *             Buzzer_Play(&beep_alarm, BUZZER_PRIORITY_HIGH);
 *         }
 *         if (cancel_alarm) {
 *             Buzzer_Stop();
 *         }
 *         vTaskDelay(pdMS_TO_TICKS(100));
 *     }
 * }
 */
