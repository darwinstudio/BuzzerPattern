/**
 * @file buzzer_pattern.c
 * @brief BuzzerPattern 核心实现: 状态机 + 优先级 + FreeRTOS 软件定时器
 */

#include "buzzer_pattern.h"

#define BUZZER_TICK_MS  10

/** @brief 内部状态机状态 */
typedef enum {
    STATE_IDLE,     /**< 空闲，无播放 */
    STATE_PLAYING,  /**< 正在播放 */
} buzzer_state_t;

/** @brief 模块运行时上下文 */
typedef struct {
    const buzzer_pattern_t *current_pattern;    /**< 当前播放的 pattern */
    uint8_t                 step_index;         /**< 当前 step 索引 */
    uint16_t                tick_counter;       /**< 当前 step 已消耗的 tick 计数 */
    uint8_t                 repeat_counter;     /**< 剩余播放轮次 (0 = 无限循环) */
    buzzer_state_t          state;              /**< 当前状态 */
    buzzer_priority_t       current_priority;   /**< 当前播放优先级 */
} buzzer_context_t;

/* ---- 模块静态变量 ---- */

static buzzer_context_t ctx;
static TimerHandle_t    buzzer_timer;
static StaticTimer_t    buzzer_timer_buf;

/* ---- 内部 GPIO 操作 ---- */

/**
 * @brief 控制蜂鸣器 GPIO 输出
 *
 * @param level 1 = 高电平 (蜂鸣器开), 0 = 低电平 (蜂鸣器关)
 */
static void buzzer_gpio_write(uint8_t level)
{
    HAL_GPIO_WritePin(buzzer_hw.port, buzzer_hw.pin,
                      level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief 推进状态机一个 tick
 */
static void buzzer_pattern_process(void)
{
    const buzzer_step_t *step;

    if (ctx.state == STATE_IDLE) {
        return;
    }

    step = &ctx.current_pattern->steps[ctx.step_index];

    /* duration_ms 为 0 时跳过该 step，避免 tick_counter 无限累加 */
    if (step->duration_ms == 0) {
        ctx.step_index++;
        if (ctx.step_index >= ctx.current_pattern->length) {
            buzzer_gpio_write(0);
            ctx.state = STATE_IDLE;
        } else {
            buzzer_gpio_write(ctx.current_pattern->steps[ctx.step_index].on);
        }
        return;
    }

    ctx.tick_counter += BUZZER_TICK_MS;

    if (ctx.tick_counter < step->duration_ms) {
        return;
    }

    /* 当前 Step 结束，移动到下一个 */
    ctx.tick_counter = 0;
    ctx.step_index++;

    if (ctx.step_index < ctx.current_pattern->length) {
        buzzer_gpio_write(ctx.current_pattern->steps[ctx.step_index].on);
        return;
    }

    /* 当前轮次结束，检查 repeat
     * repeat=0: 无限循环; repeat=N: 播放 N 轮后停止 */
    if (ctx.repeat_counter == 0) {
        ctx.step_index = 0;
        buzzer_gpio_write(ctx.current_pattern->steps[0].on);
    } else if (--ctx.repeat_counter > 0) {
        ctx.step_index = 0;
        buzzer_gpio_write(ctx.current_pattern->steps[0].on);
    } else {
        buzzer_gpio_write(0);
        ctx.state = STATE_IDLE;
    }
}

/**
 * @brief 软件定时器回调
 *
 * @param xTimer 定时器句柄 (未使用)
 */
static void buzzer_timer_callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    buzzer_pattern_process();
}

/**
 * @brief 初始化蜂鸣器模块并启动软件定时器
 *
 * 用户需先初始化 buzzer_hw 结构体 (GPIO 端口和引脚)。
 * 首次调用时创建 auto-reload 定时器，重复调用仅重置状态。
 */
void Buzzer_Init(void)
{
    ctx.state            = STATE_IDLE;
    ctx.current_pattern  = NULL;
    ctx.step_index       = 0;
    ctx.tick_counter     = 0;
    ctx.repeat_counter   = 0;
    ctx.current_priority = BUZZER_PRIORITY_LOW;

    buzzer_gpio_write(0);

    /* 首次调用时创建定时器，之后复用 */
    if (buzzer_timer == NULL) {
        buzzer_timer = xTimerCreateStatic(
            "buzzer",
            pdMS_TO_TICKS(BUZZER_TICK_MS),
            pdTRUE,
            NULL,
            buzzer_timer_callback,
            &buzzer_timer_buf
        );
    }

    xTimerStart(buzzer_timer, 0);
}

/**
 * @brief 停止当前播放
 *
 * 立即关闭蜂鸣器并将状态置为空闲。
 * 定时器保留不删除，之后调用 Buzzer_Play() 可直接恢复播放。
 */
void Buzzer_Stop(void)
{
    buzzer_gpio_write(0);
    ctx.state            = STATE_IDLE;
    ctx.current_priority = BUZZER_PRIORITY_LOW;
}

/**
 * @brief 启动一个 pattern 播放
 *
 * 高优先级可打断低优先级，低优先级不能打断高优先级。
 * 重复调用同一优先级时，新 pattern 替换旧 pattern。
 *
 * @param pattern    要播放的 pattern，不可为 NULL
 * @param priority   播放优先级
 */
void Buzzer_Play(const buzzer_pattern_t *pattern, buzzer_priority_t priority)
{
    if (pattern == NULL || pattern->steps == NULL || pattern->length == 0) {
        return;
    }

    if (ctx.state == STATE_IDLE || priority > ctx.current_priority) {
        ctx.current_pattern  = pattern;
        ctx.step_index       = 0;
        ctx.tick_counter     = 0;
        ctx.repeat_counter   = pattern->repeat;
        ctx.current_priority = priority;
        ctx.state            = STATE_PLAYING;
        buzzer_gpio_write(pattern->steps[0].on);
    }
}

/**
 * @brief 查询是否处于空闲状态
 *
 * @retval 1 空闲 (未播放)
 * @retval 0 正在播放
 */
uint8_t Buzzer_IsIdle(void)
{
    return ctx.state == STATE_IDLE;
}
