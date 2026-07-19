# BuzzerPattern

一个面向嵌入式系统的轻量级蜂鸣器提示模式框架。

```
# BuzzerPattern一个面向嵌入式系统的轻量级蜂鸣器提示模式框架。BuzzerPattern 用于管理设备中的蜂鸣器提示行为，通过 Pattern（模式）描述蜂鸣器动作序列，实现非阻塞、多样化的声音提示。---# 1. 项目简介在嵌入式设备中，蜂鸣器通常用于：- 按键反馈- 操作提示- 测量完成提示- 错误报警- 系统状态提示简单项目通常直接控制 GPIO：```cHAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);HAL_Delay(100);HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
```

但是随着项目复杂度增加，会出现：

- 蜂鸣器控制代码散落

- 延时阻塞任务

- 多种提示逻辑重复实现

- FreeRTOS 环境下阻塞任务

- 提示效果难以维护

BuzzerPattern 的目标：

> 将蜂鸣器提示行为抽象为可配置的 Pattern，通过状态机非阻塞执行。





## 9.1 定义短鸣

```
static const buzzer_step_t beep_short_steps[] ={    {1,100},    {0,100},};static const buzzer_pattern_t beep_short ={    .steps = beep_short_steps,    .length = 2,    .repeat = 1,};
```

效果：

```
响100ms停止100ms结束
```

---

## 9.2 定义错误报警

```
static const buzzer_step_t beep_error_steps[] ={    {1,500},    {0,500},};static const buzzer_pattern_t beep_error ={    .steps = beep_error_steps,    .length = 2,    .repeat = 3,};
```

效果：

```
响500ms停500ms重复3次
```

## 9.3 播放

```
buzzer_pattern_play(    &beep_error);
```

# 10. 调度方式

## 裸机

```
while(1){    buzzer_pattern_process();}
```

---

## FreeRTOS

推荐独立任务：

```
void buzzer_task(void *argument){    while(1)    {        buzzer_pattern_process();        vTaskDelay(            pdMS_TO_TICKS(10)        );    }}
```

推荐周期：

```
10ms
```

# 11. GPIO抽象

BuzzerPattern 不直接依赖 HAL。

不要：

```
HAL_GPIO_WritePin();
```

使用接口：

```
buzzer_write(level);
```

用户实现：

```
void buzzer_write(uint8_t level){    HAL_GPIO_WritePin(        BEEP_GPIO_Port,        BEEP_Pin,        level    );}
```

# 12. 优先级设计

设备中可能同时存在多个提示：

低优先级：

```
按键反馈
```

高优先级：

```
故障报警
```

因此支持优先级。

```
typedef enum{    BUZZER_PRIORITY_LOW,    BUZZER_PRIORITY_NORMAL,    BUZZER_PRIORITY_HIGH,}buzzer_priority_t;
```

规则：

```
HIGH覆盖NORMAL覆盖LOW
```
