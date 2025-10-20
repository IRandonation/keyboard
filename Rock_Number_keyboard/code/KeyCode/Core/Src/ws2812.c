/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ws2812.c
  * @brief   WS2812 LED driver using TIM4 PWM + DMA
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "ws2812.h"
#include "tim.h"
#include <math.h>

// WS2812 时序参数 (基于84MHz APB1时钟，预分频器=0，周期=104)
// 0码: 高电平约0.4us，低电平约0.85us
// 1码: 高电平约0.8us，低电平约0.45us
#define WS2812_0_CODE 33   // ~0.4us high (33/105 * 1.25us)
#define WS2812_1_CODE 66   // ~0.8us high (66/105 * 1.25us)

// 每个LED需要24位数据 (GRB格式)
#define WS2812_BITS_PER_LED 24
#define WS2812_RESET_BITS 50  // 复位信号需要的低电平位数

// DMA缓冲区大小
#define WS2812_DMA_BUFFER_SIZE (WS2812_LED_NUM * WS2812_BITS_PER_LED + WS2812_RESET_BITS)

/* Private variables ---------------------------------------------------------*/
extern TIM_HandleTypeDef htim4;
extern DMA_HandleTypeDef hdma_tim4_ch1;

// DMA缓冲区
static uint16_t ws2812_dma_buffer[WS2812_DMA_BUFFER_SIZE];

// LED颜色缓冲区 (GRB格式)
static uint8_t ws2812_led_buffer[WS2812_LED_NUM * 3];

// 更新标志
static volatile uint8_t ws2812_updating = 0;

// 模式管理变量
static WS2812_Mode current_mode = WS2812_MODE_STATIC;
static uint8_t brightness = 50;  // 0-100
static uint32_t effect_timer = 0;
static uint8_t effect_step = 0;

// 按键状态跟踪
static uint8_t key_press_time[5][4] = {0};  // 5行4列键盘

// 预定义颜色
const WS2812_Color WS2812_COLOR_RED = {255, 0, 0};
const WS2812_Color WS2812_COLOR_GREEN = {0, 255, 0};
const WS2812_Color WS2812_COLOR_BLUE = {0, 0, 255};
const WS2812_Color WS2812_COLOR_WHITE = {255, 255, 255};
const WS2812_Color WS2812_COLOR_YELLOW = {255, 255, 0};
const WS2812_Color WS2812_COLOR_CYAN = {0, 255, 255};
const WS2812_Color WS2812_COLOR_MAGENTA = {255, 0, 255};
const WS2812_Color WS2812_COLOR_OFF = {0, 0, 0};

// 内部函数声明
static void apply_brightness(uint8_t *red, uint8_t *green, uint8_t *blue);
static WS2812_Color hsv_to_rgb(uint16_t hue, uint8_t sat, uint8_t val);
static uint8_t get_led_index_from_key(uint8_t row, uint8_t col);

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void WS2812_Init(void)
{
    // 清空LED缓冲区
    for (int i = 0; i < WS2812_LED_NUM * 3; i++) {
        ws2812_led_buffer[i] = 0;
    }
    
    // 清空DMA缓冲区
    for (int i = 0; i < WS2812_DMA_BUFFER_SIZE; i++) {
        ws2812_dma_buffer[i] = 0;
    }
    
    // 清空按键状态
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 4; j++) {
            key_press_time[i][j] = 0;
        }
    }
    
    ws2812_updating = 0;
    current_mode = WS2812_MODE_STATIC;
    brightness = 50;
    effect_timer = 0;
    effect_step = 0;
}

void WS2812_SetColor(uint8_t led_index, uint8_t red, uint8_t green, uint8_t blue)
{
    if (led_index >= WS2812_LED_NUM) return;
    
    // 应用亮度调节
    apply_brightness(&red, &green, &blue);
    
    // WS2812使用GRB格式
    ws2812_led_buffer[led_index * 3 + 0] = green;
    ws2812_led_buffer[led_index * 3 + 1] = red;
    ws2812_led_buffer[led_index * 3 + 2] = blue;
}

void WS2812_SetColorStruct(uint8_t led_index, WS2812_Color color)
{
    WS2812_SetColor(led_index, color.red, color.green, color.blue);
}

void WS2812_Update(void)
{
    if (ws2812_updating) return;  // 防止并发更新
    
    ws2812_updating = 1;
    
    // 将LED数据转换为PWM数据
    uint16_t dma_index = 0;
    
    for (int led = 0; led < WS2812_LED_NUM; led++) {
        for (int byte = 0; byte < 3; byte++) {
            uint8_t color_byte = ws2812_led_buffer[led * 3 + byte];
            
            // 从最高位开始发送
            for (int bit = 7; bit >= 0; bit--) {
                if (color_byte & (1 << bit)) {
                    ws2812_dma_buffer[dma_index] = WS2812_1_CODE;
                } else {
                    ws2812_dma_buffer[dma_index] = WS2812_0_CODE;
                }
                dma_index++;
            }
        }
    }
    
    // 添加复位信号
    for (int i = 0; i < WS2812_RESET_BITS; i++) {
        ws2812_dma_buffer[dma_index++] = 0;
    }
    
    // 启动DMA传输
    HAL_TIM_PWM_Start_DMA(&htim4, TIM_CHANNEL_1, (uint32_t*)ws2812_dma_buffer, WS2812_DMA_BUFFER_SIZE);
}

void WS2812_DMAComplete(void)
{
    // 停止PWM输出
    HAL_TIM_PWM_Stop_DMA(&htim4, TIM_CHANNEL_1);
    ws2812_updating = 0;
}

// 模式管理函数
void WS2812_SetMode(WS2812_Mode mode)
{
    if (mode < WS2812_MODE_COUNT) {
        current_mode = mode;
        effect_timer = 0;
        effect_step = 0;
        
        // 根据模式初始化
        switch (mode) {
            case WS2812_MODE_OFF:
                WS2812_ClearAll();
                break;
            case WS2812_MODE_STATIC:
                WS2812_SetAll(WS2812_COLOR_WHITE);
                break;
            default:
                break;
        }
    }
}

WS2812_Mode WS2812_GetMode(void)
{
    return current_mode;
}

void WS2812_NextMode(void)
{
    WS2812_Mode next_mode = (current_mode + 1) % WS2812_MODE_COUNT;
    WS2812_SetMode(next_mode);
}

void WS2812_SetBrightness(uint8_t new_brightness)
{
    if (new_brightness <= 100) {
        brightness = new_brightness;
    }
}

uint8_t WS2812_GetBrightness(void)
{
    return brightness;
}

// 效果函数
void WS2812_ClearAll(void)
{
    for (int i = 0; i < WS2812_LED_NUM; i++) {
        WS2812_SetColorStruct(i, WS2812_COLOR_OFF);
    }
}

void WS2812_SetAll(WS2812_Color color)
{
    for (int i = 0; i < WS2812_LED_NUM; i++) {
        WS2812_SetColorStruct(i, color);
    }
}

void WS2812_ProcessEffects(void)
{
    uint32_t current_time = HAL_GetTick();
    
    switch (current_mode) {
        case WS2812_MODE_OFF:
            // 无需处理
            break;
            
        case WS2812_MODE_STATIC:
            // 静态模式，无需更新
            break;
            
        case WS2812_MODE_BREATHING:
            if (current_time - effect_timer >= 20) {  // 每20ms更新一次
                effect_timer = current_time;
                
                // 使用正弦波实现呼吸效果
                float breath = (sin(effect_step * 0.1f) + 1.0f) * 0.5f;  // 0-1范围
                uint8_t breath_brightness = (uint8_t)(breath * brightness);
                
                WS2812_Color breath_color = {breath_brightness * 255 / 100, 
                                           breath_brightness * 255 / 100, 
                                           breath_brightness * 255 / 100};
                WS2812_SetAll(breath_color);
                
                effect_step++;
                if (effect_step >= 63) effect_step = 0;  // 重置周期
            }
            break;
            
        case WS2812_MODE_RAINBOW:
            if (current_time - effect_timer >= 50) {  // 每50ms更新一次
                effect_timer = current_time;
                
                for (int i = 0; i < WS2812_LED_NUM; i++) {
                    uint16_t hue = (effect_step + i * 360 / WS2812_LED_NUM) % 360;
                    WS2812_Color rainbow_color = hsv_to_rgb(hue, 255, brightness * 255 / 100);
                    WS2812_SetColorStruct(i, rainbow_color);
                }
                
                effect_step += 5;
                if (effect_step >= 360) effect_step = 0;
            }
            break;
            
        case WS2812_MODE_KEY_REACTIVE:
            // 按键响应模式在按键事件中处理
            // 这里处理按键释放后的渐变效果
            if (current_time - effect_timer >= 10) {
                effect_timer = current_time;
                
                for (int r = 0; r < 5; r++) {
                    for (int c = 0; c < 4; c++) {
                        if (key_press_time[r][c] > 0) {
                            key_press_time[r][c]--;
                            uint8_t led_index = get_led_index_from_key(r, c);
                            if (led_index < WS2812_LED_NUM) {
                                uint8_t fade = key_press_time[r][c] * 255 / 50;  // 50步渐变
                                WS2812_SetColor(led_index, fade, fade, fade);
                            }
                        }
                    }
                }
            }
            break;
            
        case WS2812_MODE_WAVE:
            if (current_time - effect_timer >= 100) {  // 每100ms更新一次
                effect_timer = current_time;
                
                for (int i = 0; i < WS2812_LED_NUM; i++) {
                    float wave = sin((effect_step + i * 2) * 0.3f);
                    uint8_t wave_brightness = (uint8_t)((wave + 1.0f) * 0.5f * brightness * 255 / 100);
                    WS2812_SetColor(i, 0, wave_brightness, wave_brightness);
                }
                
                effect_step++;
                if (effect_step >= 100) effect_step = 0;
            }
            break;
            
        default:
            break;
    }
}

// 按键响应函数
void WS2812_OnKeyPress(uint8_t row, uint8_t col)
{
    if (current_mode == WS2812_MODE_KEY_REACTIVE) {
        uint8_t led_index = get_led_index_from_key(row, col);
        if (led_index < WS2812_LED_NUM) {
            WS2812_SetColorStruct(led_index, WS2812_COLOR_WHITE);
            key_press_time[row][col] = 50;  // 设置渐变时间
        }
    }
}

void WS2812_OnKeyRelease(uint8_t row, uint8_t col)
{
    if (current_mode == WS2812_MODE_KEY_REACTIVE) {
        // 开始渐变效果，在ProcessEffects中处理
    }
}

// 内部函数实现
static void apply_brightness(uint8_t *red, uint8_t *green, uint8_t *blue)
{
    *red = (*red * brightness) / 100;
    *green = (*green * brightness) / 100;
    *blue = (*blue * brightness) / 100;
}

static WS2812_Color hsv_to_rgb(uint16_t hue, uint8_t sat, uint8_t val)
{
    WS2812_Color rgb;
    uint8_t region, remainder, p, q, t;

    if (sat == 0) {
        rgb.red = val;
        rgb.green = val;
        rgb.blue = val;
        return rgb;
    }

    region = hue / 43;
    remainder = (hue - (region * 43)) * 6;

    p = (val * (255 - sat)) >> 8;
    q = (val * (255 - ((sat * remainder) >> 8))) >> 8;
    t = (val * (255 - ((sat * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            rgb.red = val; rgb.green = t; rgb.blue = p;
            break;
        case 1:
            rgb.red = q; rgb.green = val; rgb.blue = p;
            break;
        case 2:
            rgb.red = p; rgb.green = val; rgb.blue = t;
            break;
        case 3:
            rgb.red = p; rgb.green = q; rgb.blue = val;
            break;
        case 4:
            rgb.red = t; rgb.green = p; rgb.blue = val;
            break;
        default:
            rgb.red = val; rgb.green = p; rgb.blue = q;
            break;
    }

    return rgb;
}

static uint8_t get_led_index_from_key(uint8_t row, uint8_t col)
{
    // 根据键盘布局映射按键位置到LED索引
    // 这里假设是5x4的数字键盘布局
    if (row < 5 && col < 4) {
        return row * 4 + col;
    }
    return 255;  // 无效索引
}

/* USER CODE BEGIN 1 */
// HAL回调函数
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4) {
        WS2812_DMAComplete();
    }
}

/* USER CODE END 1 */