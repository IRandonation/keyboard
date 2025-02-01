#include "matrix_keyboard.h"

// 行对应的GPIO
static GPIO_TypeDef* ROW_Port[ROW_NUM] = {GPIOC, GPIOC, GPIOC, GPIOC, GPIOC};
static uint16_t ROW_Pin[ROW_NUM] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12};

// 列对应的GPIO
static GPIO_TypeDef* COL_Port[COL_NUM] = {GPIOC, GPIOC, GPIOC, GPIOC};
static uint16_t COL_Pin[COL_NUM] = {GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4};

// 按键映射到HID键值
const uint8_t Key_Map[ROW_NUM][COL_NUM] = {
    {0x04, 0x05, 0x06, 0x07},  // ROW0: K1=KEY_A, K2=KEY_B, K3=KEY_C, K4=KEY_D
    {0x08, 0x09, 0x0A, 0x0B},  // ROW1: K5=KEY_E, K6=KEY_F, K7=KEY_G, K8=KEY_H
    {0x0C, 0x0D, 0x0E, 0x0F},  // ROW2: K9=KEY_I, K10=KEY_J, K11=KEY_K, K12=KEY_L
    {0x10, 0x11, 0x12, 0x13},  // ROW3: K13=KEY_M, K14=KEY_N, K15=KEY_O, K16=KEY_P
    {0x14, 0x15, 0x16, 0x17}   // ROW4: K17=KEY_Q, K18=KEY_R, K19=KEY_S, K20=KEY_T
};

// 初始化矩阵键盘
void Matrix_Keyboard_Init(void) {
    // GPIO初始化已在MX_GPIO_Init中完成
}

// 扫描矩阵键盘
void Matrix_Keyboard_Scan(uint8_t *report) {
    static uint8_t last_report[8] = {0};
    uint8_t key_pressed = 0;

    // 清空当前报告
    memset(report, 0, 8);

    // 遍历每一行
    for (int row = 0; row < ROW_NUM; row++) {
        // 拉低当前行
        HAL_GPIO_WritePin(ROW_Port[row], ROW_Pin[row], GPIO_PIN_RESET);
        
        // 短暂延时（等待电平稳定）
        HAL_Delay(1);
        
        // 读取所有列的状态
        for (int col = 0; col < COL_NUM; col++) {
            if (HAL_GPIO_ReadPin(COL_Port[col], COL_Pin[col]) == GPIO_PIN_RESET) {
                // 按键按下，获取键值并填充HID报告
                uint8_t keycode = Key_Map[row][col];
                if (keycode != 0) {
                    // 将键值填入HID报告的按键数组（最多支持6个键）
                    for (int i = 2; i < 8; i++) {
                        if (report[i] == 0) {
                            report[i] = keycode;
                            break;
                        }
                    }
                }
            }
        }
        
        // 恢复当前行为高电平
        HAL_GPIO_WritePin(ROW_Port[row], ROW_Pin[row], GPIO_PIN_SET);
    }

    // 消抖：仅当报告变化时发送
    if (memcmp(report, last_report, 8) != 0) {
        memcpy(last_report, report, 8);
    } else {
        memset(report, 0, 8);
    }
}