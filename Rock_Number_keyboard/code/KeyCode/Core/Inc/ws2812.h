#ifndef __WS2812_H
#define __WS2812_H

#include "stm32f4xx_hal.h"

#define WS2812_LED_NUM 20  // 根据实际LED数量调整

// 背光模式枚举
typedef enum {
    WS2812_MODE_OFF = 0,        // 关闭
    WS2812_MODE_STATIC,         // 静态单色
    WS2812_MODE_BREATHING,      // 呼吸灯效果
    WS2812_MODE_RAINBOW,        // 彩虹循环
    WS2812_MODE_KEY_REACTIVE,   // 按键响应
    WS2812_MODE_WAVE,           // 波浪效果
    WS2812_MODE_COUNT           // 模式总数
} WS2812_Mode;

// 颜色结构体
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} WS2812_Color;

// 预定义颜色
extern const WS2812_Color WS2812_COLOR_RED;
extern const WS2812_Color WS2812_COLOR_GREEN;
extern const WS2812_Color WS2812_COLOR_BLUE;
extern const WS2812_Color WS2812_COLOR_WHITE;
extern const WS2812_Color WS2812_COLOR_YELLOW;
extern const WS2812_Color WS2812_COLOR_CYAN;
extern const WS2812_Color WS2812_COLOR_MAGENTA;
extern const WS2812_Color WS2812_COLOR_OFF;

// 基础函数
void WS2812_Init(void);
void WS2812_SetColor(uint8_t led_index, uint8_t red, uint8_t green, uint8_t blue);
void WS2812_SetColorStruct(uint8_t led_index, WS2812_Color color);
void WS2812_Update(void);
void WS2812_DMAComplete(void);

// 模式管理函数
void WS2812_SetMode(WS2812_Mode mode);
WS2812_Mode WS2812_GetMode(void);
void WS2812_NextMode(void);
void WS2812_SetBrightness(uint8_t brightness); // 0-100
uint8_t WS2812_GetBrightness(void);

// 效果函数
void WS2812_ClearAll(void);
void WS2812_SetAll(WS2812_Color color);
void WS2812_ProcessEffects(void);  // 在主循环中调用以更新动态效果

// 按键响应函数
void WS2812_OnKeyPress(uint8_t row, uint8_t col);
void WS2812_OnKeyRelease(uint8_t row, uint8_t col);

#endif