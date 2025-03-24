#ifndef __MATRIX_KEYBOARD_H
#define __MATRIX_KEYBOARD_H

#include "stm32f4xx_hal.h"

// 定义行列数量
#define ROW_NUM 5
#define COL_NUM 4

// 长按参数配置（单位：ms）
#define INITIAL_DELAY   500  // 首次触发后的延迟时间
#define REPEAT_INTERVAL 100   // 重复触发间隔

// 在全局变量区添加状态跟踪结构体
typedef enum {
    KEY_STATE_UP,
    KEY_STATE_DEBOUNCE,
    KEY_STATE_DOWN,
    KEY_STATE_REPEAT  // 重复触发状态
} KeyState;

static KeyState s_key_state = KEY_STATE_UP;
static uint8_t s_last_key = 0;
static uint32_t s_last_tick = 0;

// 键盘初始化
void MatrixKeyboard_Init(void);

// 键盘扫描（返回HID键码，无按键返回0）
uint8_t MatrixKeyboard_Scan(void);

#endif