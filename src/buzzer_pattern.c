#include "buzzer_pattern.h"

/* ---- 内部状态 ---- */

typedef enum {
    STATE_IDLE,
    STATE_PLAYING,
} buzzer_state_t;

typedef struct {
    const buzzer_pattern_t *current_pattern;
    uint8_t                 step_index;
    uint16_t                tick_counter;
    uint8_t                 repeat_counter;
    buzzer_state_t          state;
    buzzer_priority_t       current_priority;
} buzzer_context_t;

static buzzer_context_t ctx;
static buzzer_write_fn   buzzer_write;

/* ---- 内部辅助函数 ---- */

static void buzzer_pattern_start(const buzzer_pattern_t *pattern, buzzer_priority_t priority)
{
    ctx.current_pattern  = pattern;
    ctx.step_index       = 0;
    ctx.tick_counter     = 0;
    ctx.repeat_counter   = pattern->repeat;
    ctx.current_priority = priority;
    ctx.state            = STATE_PLAYING;
    buzzer_write(pattern->steps[0].on);
}

/* ---- 公共 API ---- */

void buzzer_pattern_init(buzzer_write_fn write_fn)
{
    buzzer_write         = write_fn;
    ctx.current_pattern  = NULL;
    ctx.step_index       = 0;
    ctx.tick_counter     = 0;
    ctx.repeat_counter   = 0;
    ctx.state            = STATE_IDLE;
    ctx.current_priority = BUZZER_PRIORITY_LOW;
    buzzer_write(0);
}

void buzzer_pattern_play(const buzzer_pattern_t *pattern, buzzer_priority_t priority)
{
    if (ctx.state == STATE_IDLE || priority > ctx.current_priority) {
        buzzer_pattern_start(pattern, priority);
    }
    /* 否则丢弃，当前播放不受影响 */
}

void buzzer_pattern_stop(void)
{
    buzzer_write(0);
    ctx.state            = STATE_IDLE;
    ctx.current_priority = BUZZER_PRIORITY_LOW;
}

void buzzer_pattern_process(void)
{
    const buzzer_step_t *step;

    if (ctx.state == STATE_IDLE) {
        return;
    }

    step = &ctx.current_pattern->steps[ctx.step_index];
    ctx.tick_counter += 10; /* 推荐 10ms 调用周期 */

    if (ctx.tick_counter < step->duration_ms) {
        return;
    }

    /* 当前 Step 结束，移动到下一个 */
    ctx.tick_counter = 0;
    ctx.step_index++;

    if (ctx.step_index < ctx.current_pattern->length) {
        /* 还有下一个 Step */
        buzzer_write(ctx.current_pattern->steps[ctx.step_index].on);
        return;
    }

    /* 当前轮次结束，检查 repeat */
    if (ctx.repeat_counter == 0) {
        /* 无限循环 */
        ctx.step_index = 0;
        buzzer_write(ctx.current_pattern->steps[0].on);
    } else {
        ctx.repeat_counter--;
        if (ctx.repeat_counter > 0) {
            /* 继续下一轮 */
            ctx.step_index = 0;
            buzzer_write(ctx.current_pattern->steps[0].on);
        } else {
            /* 播放完毕 */
            buzzer_write(0);
            ctx.state = STATE_IDLE;
        }
    }
}

uint8_t buzzer_pattern_is_idle(void)
{
    return ctx.state == STATE_IDLE;
}
