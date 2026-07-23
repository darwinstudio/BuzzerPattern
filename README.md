# BuzzerPattern

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Language](https://img.shields.io/badge/language-C-orange.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform](https://img.shields.io/badge/platform-STM32-green.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html)
[![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-lightgrey.svg)](https://www.freertos.org/)

面向 STM32 + FreeRTOS 的轻量级蜂鸣器提示模式框架。

纯 C 实现，纯静态分配，无动态内存。内部通过 HAL 库直接控制 GPIO，10ms 软件定时器自动推进状态机，支持优先级覆盖。

## 特性

- **零配置回调** — 用户只需初始化 GPIO 端口和引脚，内部直接调用 HAL_GPIO_WritePin
- **非阻塞** — 10ms 软件定时器自动推进状态机，无需手动调用
- **纯静态** — 定时器由 xTimerCreateStatic 创建，零 malloc
- **低开销** — 无独立任务栈，仅一个 StaticTimer_t
- **即时响应** — 高优先级直接覆盖低优先级，最新事件即时播放
- **可配置** — Pattern 通过数据表描述，新增提示音无需改代码

## 快速开始

### 1. 初始化硬件配置

```c
buzzer_hw_t buzzer_hw = {
    .port = GPIOA,
    .pin  = GPIO_PIN_5,
};
```

### 2. 定义 Pattern

```c
static const buzzer_step_t beep_short_steps[] = {
    { 1, 100 },   /* 响 100ms */
    { 0, 100 },   /* 停 100ms */
};
static const buzzer_pattern_t beep_short = {
    .steps  = beep_short_steps,
    .length = 2,
    .repeat = 1,      /* 播放 1 次 (0 = 无限循环) */
};
```

### 3. 初始化并使用

```c
Buzzer_Init();

/* 从任意任务调用 */
Buzzer_Play(&beep_short, BUZZER_PRIORITY_LOW);
Buzzer_Play(&beep_alarm, BUZZER_PRIORITY_HIGH);

/* 立即关闭蜂鸣器 */
Buzzer_Stop();
```

## 优先级

三级优先级，高优先级可打断低优先级，低优先级不能打断高优先级：

| 优先级 | 场景 |
|--------|------|
| `BUZZER_PRIORITY_LOW` | 按键反馈 |
| `BUZZER_PRIORITY_NORMAL` | 操作提示、测量完成 |
| `BUZZER_PRIORITY_HIGH` | 故障报警 |

## API

```c
/* 硬件配置 (用户定义并初始化) */
extern buzzer_hw_t buzzer_hw;

/* 初始化模块并启动 10ms 软件定时器 */
void Buzzer_Init(void);

/* 停止播放，蜂鸣器立即关闭；之后调用 Buzzer_Play() 可直接恢复 */
void Buzzer_Stop(void);

/* 从任意任务调用，启动或切换 pattern */
void Buzzer_Play(const buzzer_pattern_t *pattern, buzzer_priority_t priority);

/* 查询是否空闲 */
uint8_t Buzzer_IsIdle(void);
```

## 文件结构

```
inc/buzzer_pattern.h        头文件 (类型定义 + API 声明)
src/buzzer_pattern.c        核心实现 (状态机 + 优先级 + 软件定时器)
examples/main_example.c     使用示例
```

## 集成

### 方式一：Git Submodule

```bash
cd your-project
git submodule add https://github.com/darwinstudio/BuzzerPattern.git drivers/BuzzerPattern
```

将 `drivers/BuzzerPattern/inc` 添加到头文件搜索路径，将 `drivers/BuzzerPattern/src/buzzer_pattern.c` 添加到编译源文件。

### 方式二：手动复制

将 `inc/` 和 `src/` 目录复制到你的 STM32 工程（Keil / IAR / STM32CubeIDE），确保 `inc/` 在头文件搜索路径中。

### 前置依赖

- STM32 HAL 库 (`main.h` 需包含 GPIO 相关定义)
- FreeRTOS (需启用 `configUSE_TIMERS` 和 `configSUPPORT_STATIC_ALLOCATION`)

## 线程安全

本模块使用单个静态上下文，**非线程安全**。

- `Buzzer_Play()` / `Buzzer_Stop()` / `Buzzer_IsIdle()` 可从任意任务调用
- 内部状态机由 10ms 软件定时器自动调度
- `Buzzer_Stop()` 立即关闭蜂鸣器并置空闲状态

## License

MIT
