# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BuzzerPattern is a lightweight buzzer pattern framework for STM32 + FreeRTOS. It abstracts buzzer behaviors into configurable Patterns (step sequences) executed non-blockingly via a state machine driven by a 10ms FreeRTOS software timer. Supports three priority levels (LOW/NORMAL/HIGH) where higher priority can override lower. GPIO control is done internally via HAL_GPIO_WritePin.

## Language & Platform

- Language: C (embedded target)
- Platform: STM32 HAL (direct GPIO control via HAL_GPIO_WritePin)
- Environment: FreeRTOS (configUSE_TIMERS + configSUPPORT_STATIC_ALLOCATION required)

## Build

No build system configured yet. Integrate source files (`inc/buzzer_pattern.h`, `src/buzzer_pattern.c`) into your STM32 project (Keil/IAR/STM32CubeIDE).

## Architecture

```
inc/buzzer_pattern.h    — Public API: types, Buzzer_Init/Stop/Play/IsIdle
src/buzzer_pattern.c    — State machine + priority logic + 10ms software timer
examples/main_example.c — Hardware config + pattern definitions + usage
```

Core flow: User initializes `buzzer_hw` with GPIO port/pin, then calls `Buzzer_Init()`. A 10ms auto-reload software timer drives the state machine. `Buzzer_Play()` starts a pattern if idle or higher priority. `Buzzer_Stop()` immediately turns off the buzzer and sets idle state.

## Key Design Decisions

- STM32 only, direct HAL_GPIO_WritePin, no callback abstraction
- FreeRTOS only, no bare-metal fallback
- Fixed 10ms timer period, not user-configurable
- Pure static allocation: timer created with xTimerCreateStatic (StaticTimer_t is module-internal)
- Hardware config via extern `buzzer_hw_t buzzer_hw` struct
- No dedicated task: uses FreeRTOS software timer service task (zero extra stack)
- No queue: Buzzer_Play() is immediate (replace or discard), prioritizing responsiveness
- No dynamic memory: all static allocation
- Pattern data is `const` (ROM), only runtime state is the single `buzzer_context_t`
- `repeat=0` means infinite loop, `repeat=N` means play N times total
- `process()` is internal (static), called only by the timer callback
