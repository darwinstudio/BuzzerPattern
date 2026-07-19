# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BuzzerPattern is a lightweight buzzer pattern framework for embedded systems. It abstracts buzzer behaviors into configurable Patterns (step sequences) executed non-blockingly via a state machine. Supports three priority levels (LOW/NORMAL/HIGH) where higher priority can override lower.

## Language & Platform

- Language: C (embedded target)
- Platform: STM32 HAL (GPIO-based buzzer control)
- Environment: Bare metal or FreeRTOS

## Build

No build system configured yet. Integrate source files (`inc/buzzer_pattern.h`, `src/buzzer_pattern.c`) into your STM32 project (Keil/IAR/STM32CubeIDE).

## Architecture

```
inc/buzzer_pattern.h    — Public API: types, init, play, stop, process, is_idle
src/buzzer_pattern.c    — State machine + priority logic, single static context
examples/main_example.c — Pattern definitions and usage (bare metal / FreeRTOS)
```

Core flow: `play()` starts a pattern if idle or higher priority. `process()` advances the state machine every 10ms. `buzzer_write(level)` is a user-provided GPIO callback.

## Key Design Decisions

- No queue: play() is immediate (replace or discard), prioritizing responsiveness
- No dynamic memory: all static allocation
- Pattern data is `const` (ROM), only runtime state is the single `buzzer_context_t`
- `repeat=0` means infinite loop, `repeat=N` means play N times total
