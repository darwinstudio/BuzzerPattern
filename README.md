# BuzzerPattern

面向嵌入式系统的轻量级蜂鸣器提示模式框架。

纯 C 实现，无动态内存，无队列依赖。通过状态机非阻塞执行，支持优先级覆盖。适用于裸机和 FreeRTOS 环境。

## 特性

- **非阻塞** — 状态机驱动，不使用任何 delay
- **轻量** — 单文件实现，~100 行代码，全部静态分配
- **即时响应** — 高优先级直接覆盖低优先级，最新事件即时播放
- **可配置** — Pattern 通过数据表描述，新增提示音无需改代码
- **平台无关** — 通过回调操作 GPIO，不依赖具体 HAL

## 快速开始

### 1. 定义 Pattern

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

### 2. 实现 GPIO 回调

```c
void buzzer_write(uint8_t level)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, level);
}
```

### 3. 初始化并使用

```c
buzzer_pattern_init(buzzer_write);

/* 裸机主循环 */
while (1) {
    buzzer_pattern_process();   /* 每 10ms 调用一次 */

    if (key_pressed) {
        buzzer_pattern_play(&beep_short, BUZZER_PRIORITY_LOW);
    }
    if (error_occurred) {
        buzzer_pattern_play(&beep_alarm, BUZZER_PRIORITY_HIGH);
    }
}
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
void    buzzer_pattern_init(buzzer_write_fn write_fn);
void    buzzer_pattern_play(const buzzer_pattern_t *pattern, buzzer_priority_t priority);
void    buzzer_pattern_stop(void);
void    buzzer_pattern_process(void);       /* 推荐 10ms 周期调用 */
uint8_t buzzer_pattern_is_idle(void);
```

## 文件结构

```
inc/buzzer_pattern.h        头文件 (类型定义 + API 声明)
src/buzzer_pattern.c        核心实现 (状态机 + 优先级)
examples/main_example.c     使用示例 (裸机 + FreeRTOS)
docs/design.md              设计文档
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

## 开发工具

本项目使用 [Claude Code](https://claude.ai/code) (Claude CLI) + mimo 2.5 pro 进行设计与编码。

## License

MIT
