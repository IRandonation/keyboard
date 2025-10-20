#ifndef __MATRIX_KEYBOARD_H
#define __MATRIX_KEYBOARD_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
// --- 可配置参数 ---

// 1. 定义行列数量
#define ROW_NUM 5
#define COL_NUM 4

// 2. 定义按键处理时间间隔 (ms)
#define KEY_SCAN_INTERVAL   10  // 每10ms处理一次按键逻辑，替代HAL_Delay
#define KEY_DEBOUNCE_TIME   20  // 消抖时间 (ms)
#define KEY_LONG_PRESS_TIME 700 // 长按初始触发时间 (ms)
#define KEY_REPEAT_INTERVAL 100 // 长按连发间隔 (ms)

// 3. 定义最大同时按下的按键数
#define MAX_PRESSED_KEYS    6

// --- 公共类型和函数声明 ---

/**
 * @brief 按键事件类型
 */
typedef enum {
    KEY_EVENT_PRESS,      // 单击按下
    KEY_EVENT_LONG_PRESS, // 长按首次触发
    KEY_EVENT_REPEAT,     // 长按重复触发
    KEY_EVENT_RELEASE     // 按键释放 (可选功能)
} KeyEventType;

/**
 * @brief 按键事件结构体
 */
typedef struct {
    uint8_t      key_code; // 按键的键码 (来自Key_Map)
    uint8_t      row;      // 行索引
    uint8_t      col;      // 列索引
    KeyEventType event;    // 按键的事件类型
} KeyEvent;

/**
 * @brief 初始化矩阵键盘所需的GPIO
 */
void MatrixKeyboard_Init(void);

/**
 * @brief 处理矩阵键盘逻辑（应在主循环中被周期性调用）
 * @param p_key_events - 指向用于存储按键事件的缓冲区
 * @param buffer_size  - 缓冲区的最大长度
 * @return uint8_t     - 本次调用检测到的有效按键事件数量
 */
uint8_t MatrixKeyboard_Process(KeyEvent* p_key_events, uint8_t buffer_size);

/**
 * @brief 在TIM中断里调用的扫描步进函数（1kHz）。
 */
void MatrixKeyboard_ScanStep_ISR(void);

/**
 * @brief 从事件队列取出一个事件，主循环调用。
 * @return true 表示取到事件；false 表示队列为空。
 */
bool MatrixKeyboard_PopEvent(KeyEvent* out_event);

#endif // __MATRIX_KEYBOARD_H