#ifndef MATRIX_KEYBOARD_H
#define MATRIX_KEYBOARD_H

#include "stm32f4xx_hal.h"

// 矩阵键盘配置
#define ROW_NUM  5  // 行数
#define COL_NUM  4  // 列数

// 按键映射到HID键值（根据实际需求修改）
extern const uint8_t Key_Map[ROW_NUM][COL_NUM];

// 函数声明
void Matrix_Keyboard_Init(void);
void Matrix_Keyboard_Scan(uint8_t *report);

#endif