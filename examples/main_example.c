/**
 * BuzzerPattern 使用示例
 *
 * 本文件展示如何在裸机和 FreeRTOS 环境中使用 BuzzerPattern。
 * Pattern 定义放在独立的头文件中，按项目需要组织。
 */

#include "buzzer_pattern.h"

/* ---- Pattern 定义 ---- */

/* 单次短响 (按键反馈) */
static const buzzer_step_t beep_short_steps[] = {
    { 1, 100 },
    { 0, 100 },
};
static const buzzer_pattern_t beep_short = {
    .steps  = beep_short_steps,
    .length = 2,
    .repeat = 1,
};

/* 三短响 (测量完成) */
static const buzzer_step_t beep_done_steps[] = {
    { 1, 60 },
    { 0, 60 },
};
static const buzzer_pattern_t beep_done = {
    .steps  = beep_done_steps,
    .length = 2,
    .repeat = 3,
};

/* 错误报警 */
static const buzzer_step_t beep_error_steps[] = {
    { 1, 500 },
    { 0, 500 },
};
static const buzzer_pattern_t beep_error = {
    .steps  = beep_error_steps,
    .length = 2,
    .repeat = 3,
};

/* 长鸣报警 (无限循环) */
static const buzzer_step_t beep_alarm_steps[] = {
    { 1, 500 },
};
static const buzzer_pattern_t beep_alarm = {
    .steps  = beep_alarm_steps,
    .length = 1,
    .repeat = 0,
};

/* ---- GPIO 回调 (用户实现) ---- */

/*
 * void buzzer_write(uint8_t level)
 * {
 *     HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, level);
 * }
 */

/* ============================================================
 * 裸机示例
 * ============================================================ */

/*
 * int main(void)
 * {
 *     HAL_Init();
 *     SystemClock_Config();
 *     MX_GPIO_Init();
 *
 *     buzzer_pattern_init(buzzer_write);
 *
 *     while (1) {
 *         buzzer_pattern_process();
 *
 *         if (key_pressed) {
 *             buzzer_pattern_play(&beep_short, BUZZER_PRIORITY_LOW);
 *         }
 *         if (measurement_done) {
 *             buzzer_pattern_play(&beep_done, BUZZER_PRIORITY_NORMAL);
 *         }
 *         if (error_occurred) {
 *             buzzer_pattern_play(&beep_error, BUZZER_PRIORITY_HIGH);
 *         }
 *     }
 * }
 */

/* ============================================================
 * FreeRTOS 示例
 * ============================================================ */

/*
 * // 蜂鸣器任务: 10ms 周期推进状态机
 * void buzzer_task(void *argument)
 * {
 *     buzzer_pattern_init(buzzer_write);
 *
 *     for (;;) {
 *         buzzer_pattern_process();
 *         vTaskDelay(pdMS_TO_TICKS(10));
 *     }
 * }
 *
 * // 传感器任务: 从其他任务调用 play
 * void sensor_task(void *argument)
 * {
 *     for (;;) {
 *         if (measurement_done) {
 *             buzzer_pattern_play(&beep_done, BUZZER_PRIORITY_NORMAL);
 *         }
 *         if (error_occurred) {
 *             buzzer_pattern_play(&beep_error, BUZZER_PRIORITY_HIGH);
 *         }
 *         vTaskDelay(pdMS_TO_TICKS(100));
 *     }
 * }
 */
