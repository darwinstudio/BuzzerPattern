/**
 * @file buzzer_pattern.h
 * @brief BuzzerPattern 公共 API 及类型定义
 *
 * 内部使用 FreeRTOS 软件定时器以 10ms 周期推进状态机。
 * 纯静态分配，零 malloc。通过 HAL 库直接控制 GPIO。
 * 线程安全: Play/Stop 内部通过 taskENTER_CRITICAL 保护共享状态。
 */

#ifndef BUZZER_PATTERN_H
#define BUZZER_PATTERN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "FreeRTOS.h"
#include "timers.h"

/** @brief 蜂鸣器硬件配置 (用户初始化此结构体) */
typedef struct {
    GPIO_TypeDef *port;     /**< GPIO 端口, 如 GPIOA */
    uint16_t      pin;      /**< GPIO 引脚, 如 GPIO_PIN_5 */
} buzzer_hw_t;

/** @brief 单个播放步骤 */
typedef struct {
    uint8_t  on;            /**< 1 = 蜂鸣器开, 0 = 蜂鸣器关 */
    uint16_t duration_ms;   /**< 持续时间 (毫秒), 不可为 0 */
} buzzer_step_t;

/** @brief 播放模式: 由多个 Step 组成的序列 */
typedef struct {
    const buzzer_step_t *steps;     /**< Step 数组指针, 不可为 NULL */
    uint8_t              length;    /**< Step 数组长度, 不可为 0 */
    uint8_t              repeat;    /**< 总播放次数: 0 = 无限循环, N = 播放 N 轮后停止 */
} buzzer_pattern_t;

/** @brief 蜂鸣器播放优先级 */
typedef enum {
    BUZZER_PRIORITY_LOW,        /**< 按键反馈等 */
    BUZZER_PRIORITY_NORMAL,     /**< 操作提示、测量完成 */
    BUZZER_PRIORITY_HIGH,       /**< 故障报警 */
} buzzer_priority_t;

extern buzzer_hw_t buzzer_hw;

void    Buzzer_Init(void);
void    Buzzer_Stop(void);
void    Buzzer_Play(const buzzer_pattern_t *pattern, buzzer_priority_t priority);
uint8_t Buzzer_IsIdle(void);

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_PATTERN_H */
