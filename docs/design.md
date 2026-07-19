# BuzzerPattern 设计文档

## 1. 项目概述

BuzzerPattern 是一个面向嵌入式系统的轻量级蜂鸣器提示模式框架。通过将蜂鸣器提示行为抽象为可配置的 Pattern，使用状态机非阻塞执行，解决以下问题：

- 蜂鸣器控制代码散落在各处
- `HAL_Delay` 阻塞主循环或 RTOS 任务
- 多种提示逻辑重复实现
- 提示效果难以统一维护

## 2. 核心设计目标

| 目标 | 说明 |
|------|------|
| 非阻塞 | 基于状态机 process 驱动，不使用任何 delay |
| 轻量 | 纯 C 实现，无动态内存分配，无队列，ROM/RAM 占用极小 |
| 可配置 | Pattern 通过数据表描述，新增提示音无需改代码 |
| 即时响应 | 最新事件即时播放，高优先级直接覆盖低优先级 |
| 平台无关 | 核心逻辑与 HAL 解耦，通过单一回调操作 GPIO |

## 3. 架构总览

```
┌─────────────────────────────────────────────┐
│              Application Layer              │
│   (定义 Pattern 数据, 调用 buzzer API)       │
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│            BuzzerPattern Core               │
│  ┌──────────┐  ┌──────────┐  ┌───────────┐  │
│  │ Pattern  │  │  State   │  │ Priority  │  │
│  │  Table   │  │ Machine  │  │  Check    │  │
│  └──────────┘  └──────────┘  └───────────┘  │
└──────────────────┬──────────────────────────┘
                   │  buzzer_write(level)
┌──────────────────▼──────────────────────────┐
│            用户实现的 GPIO 回调              │
│    void buzzer_write(uint8_t level)         │
└─────────────────────────────────────────────┘
```

## 4. GPIO 抽象

BuzzerPattern 不直接依赖 HAL，通过单一回调 `buzzer_write(level)` 控制蜂鸣器：

```c
// 用户实现
void buzzer_write(uint8_t level)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, level);
}
```

初始化时注册：

```c
void buzzer_pattern_init(buzzer_write_fn write_fn);
```

## 5. Pattern 数据结构

### 5.1 基本单元：Step

每个 Step 描述一次蜂鸣器状态和持续时间，字段顺序为 `{on, duration_ms}`：

```c
typedef struct {
    uint8_t  on;            // 1 = 蜂鸣器开, 0 = 蜂鸣器关
    uint16_t duration_ms;   // 持续时间 (毫秒)
} buzzer_step_t;
```

### 5.2 Pattern 定义

Pattern 包含 Step 数组指针、长度和重复次数：

```c
typedef struct {
    const buzzer_step_t *steps;     // Step 数组指针
    uint8_t              length;    // Step 数组长度
    uint8_t              repeat;    // 总播放次数 (0 = 无限循环)
} buzzer_pattern_t;
```

**repeat 语义**: `repeat` 表示完整播放 steps 的总次数。例如 repeat=3 表示完整播放 3 次后停止，repeat=0 表示无限循环。

### 5.3 Pattern 示例

```c
// 单次短响 (按键反馈)：响100ms，停100ms，结束
static const buzzer_step_t beep_short_steps[] = {
    { 1, 100 },
    { 0, 100 },
};
static const buzzer_pattern_t beep_short = {
    .steps  = beep_short_steps,
    .length = 2,
    .repeat = 1,
};

// 错误报警：响500ms停500ms，重复3次
static const buzzer_step_t beep_error_steps[] = {
    { 1, 500 },
    { 0, 500 },
};
static const buzzer_pattern_t beep_error = {
    .steps  = beep_error_steps,
    .length = 2,
    .repeat = 3,
};

// 三短响 (测量完成)：响60ms停60ms，重复3次
static const buzzer_step_t beep_done_steps[] = {
    { 1, 60 },
    { 0, 60 },
};
static const buzzer_pattern_t beep_done = {
    .steps  = beep_done_steps,
    .length = 2,
    .repeat = 3,
};

// 长鸣报警 (无限循环)
static const buzzer_step_t beep_alarm_steps[] = {
    { 1, 500 },
};
static const buzzer_pattern_t beep_alarm = {
    .steps  = beep_alarm_steps,
    .length = 1,
    .repeat = 0,    // 无限循环
};
```

## 6. 优先级设计

设备中可能同时存在多个提示需求，高优先级直接覆盖低优先级，低优先级不能打断高优先级：

```c
typedef enum {
    BUZZER_PRIORITY_LOW,        // 按键反馈等
    BUZZER_PRIORITY_NORMAL,     // 操作提示、测量完成
    BUZZER_PRIORITY_HIGH,       // 故障报警
} buzzer_priority_t;
```

### 6.1 优先级规则

| 条件 | 行为 |
|------|------|
| 新优先级 > 当前优先级 | 直接覆盖当前播放，立即生效 |
| 新优先级 <= 当前优先级 | 丢弃，当前播放不受影响 |
| 当前空闲 | 无条件播放 |

### 6.2 调度示例

```
当前播放: NORMAL (测量完成)
│
├─ 收到 LOW (按键) → 丢弃
├─ 收到 HIGH (故障) → 直接覆盖，立即播放报警
└─ 收到 NORMAL (操作提示) → 丢弃
```

## 7. 播放控制

### 7.1 内部辅助函数: buzzer_pattern_start

启动一个 Pattern，初始化上下文并开始播放第一个 Step：

```c
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
```

### 7.2 Play

```c
void buzzer_pattern_play(const buzzer_pattern_t *pattern, buzzer_priority_t priority)
{
    if (ctx.state == STATE_IDLE || priority > ctx.current_priority) {
        buzzer_pattern_start(pattern, priority);
    }
    // 否则丢弃，当前播放不受影响
}
```

### 7.3 Stop

```c
void buzzer_pattern_stop(void)
{
    buzzer_write(0);
    ctx.state = STATE_IDLE;
    ctx.current_priority = BUZZER_PRIORITY_LOW;  // 重置优先级，允许后续任何 Pattern 播放
}
```

## 8. 状态机设计

### 8.1 状态

```c
typedef enum {
    STATE_IDLE,         // 空闲，无 Pattern 执行
    STATE_PLAYING,      // 正在播放当前 Step
} buzzer_state_t;
```

### 8.2 核心运行上下文

```c
typedef struct {
    const buzzer_pattern_t *current_pattern;    // 当前执行的 Pattern
    uint8_t                 step_index;         // 当前 Step 索引
    uint16_t                tick_counter;       // 当前 Step 已经过的 tick 数
    uint8_t                 repeat_counter;     // 剩余重复次数
    buzzer_state_t          state;              // 状态机状态
    buzzer_priority_t       current_priority;   // 当前播放的优先级
} buzzer_context_t;
```

### 8.3 Process 驱动流程

```
buzzer_pattern_process() 周期调用 (推荐 10ms)
│
├─ state == IDLE → 无操作
│
├─ state == PLAYING
│   ├─ tick_counter < current_step.duration_ms → tick_counter += 周期, return
│   └─ tick_counter >= current_step.duration_ms → 移动到下一个 Step
│       ├─ 有下一个 Step → buzzer_write(next.on), reset counter
│       └─ 无下一个 Step → 检查 repeat_counter
│           ├─ repeat_counter == 0 (无限循环) → 回到 steps[0], buzzer_write(steps[0].on)
│           └─ repeat_counter > 0 → repeat_counter--
│               ├─ repeat_counter > 0 → 回到 steps[0], buzzer_write(steps[0].on)
│               └─ repeat_counter == 0 → buzzer_write(0), state = IDLE
│
└─ 无操作
```

**注意**: Pattern 启动时直接调用 `buzzer_write(steps[0].on)`，进入第一个 Step 的播放状态。

## 9. API 接口定义

```c
// ---- 类型 ----
typedef void (*buzzer_write_fn)(uint8_t level);

// ---- 初始化 ----
void buzzer_pattern_init(buzzer_write_fn write_fn);

// ---- 播放控制 ----
void buzzer_pattern_play(const buzzer_pattern_t *pattern, buzzer_priority_t priority);
void buzzer_pattern_stop(void);                     // 停止当前播放，蜂鸣器静音

// ---- 周期调用 ----
void buzzer_pattern_process(void);                  // 周期调用，推荐 10ms

// ---- 状态查询 ----
uint8_t buzzer_pattern_is_idle(void);               // 是否空闲
```

### 9.1 API 行为

| API | 行为 |
|-----|------|
| `buzzer_pattern_play(pattern, priority)` | 高优先级或空闲时立即播放，否则丢弃 |
| `buzzer_pattern_stop()` | 停止当前播放，蜂鸣器静音 |
| `buzzer_pattern_process()` | 状态机推进，必须周期调用 |
| `buzzer_pattern_is_idle()` | 返回当前是否空闲 |

## 10. 文件结构

```
BuzzerPattern/
├── inc/
│   └── buzzer_pattern.h        // 公共 API 头文件
├── src/
│   └── buzzer_pattern.c        // 核心实现 (状态机 + 优先级)
├── docs/
│   └── design.md               // 本文档
└── examples/
    └── main_example.c          // 使用示例
```

### 10.1 头文件与源文件职责

**buzzer_pattern.h** — 对外接口
- 类型定义: `buzzer_step_t`, `buzzer_pattern_t`, `buzzer_priority_t`
- API 声明: init, play, stop, process, is_idle

**buzzer_pattern.c** — 内部实现
- 静态全局 context
- 状态机逻辑
- 优先级判断
- `buzzer_write_fn` 回调存储

## 11. 使用示例

### 11.1 裸机 (主循环)

```c
void buzzer_write(uint8_t level)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, level);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    buzzer_pattern_init(buzzer_write);

    while (1) {
        buzzer_pattern_process();

        if (key_pressed) {
            buzzer_pattern_play(&beep_short, BUZZER_PRIORITY_LOW);
        }
        if (error_occurred) {
            buzzer_pattern_play(&beep_error, BUZZER_PRIORITY_HIGH);
        }
    }
}
```

### 11.2 FreeRTOS

```c
// 独立任务，推荐 10ms 周期
void buzzer_task(void *argument)
{
    buzzer_pattern_init(buzzer_write);

    for (;;) {
        buzzer_pattern_process();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// 其他任务中调用
void sensor_task(void *argument)
{
    for (;;) {
        if (measurement_done) {
            buzzer_pattern_play(&beep_done, BUZZER_PRIORITY_NORMAL);
        }
        if (error_occurred) {
            buzzer_pattern_play(&beep_error, BUZZER_PRIORITY_HIGH);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

## 12. 设计约束

| 约束 | 说明 |
|------|------|
| 运行环境 | 裸机或 FreeRTOS 均可 |
| Tick 周期 | 推荐 10ms，duration_ms 最小粒度等于 process 调用周期 |
| 最大 Step 数 | 单个 Pattern 不超过 255 个 Step (length 为 uint8_t) |
| 无动态内存 | 全部静态分配 |
| 无队列 | play() 即时响应，高优先级覆盖，低优先级丢弃 |
| 线程安全 | 裸机无需考虑；FreeRTOS 环境下若从多个任务调用 play()，需自行加锁 |
