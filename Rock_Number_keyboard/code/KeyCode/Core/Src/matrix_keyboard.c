#include "matrix_keyboard.h"

// 行对应的GPIO
static GPIO_TypeDef* ROW_Port[ROW_NUM] = {GPIOC, GPIOC, GPIOC, GPIOC, GPIOC};
static uint16_t ROW_Pin[ROW_NUM] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12};

// 列对应的GPIO
static GPIO_TypeDef* COL_Port[COL_NUM] = {GPIOC, GPIOC, GPIOC, GPIOC};
static uint16_t COL_Pin[COL_NUM] = {GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4};

// 按键映射到HID键值
const uint8_t Key_Map[ROW_NUM][COL_NUM] = {
    {0x53, 0x54, 0x55, 0x56},  // ROW0: K1=KEYPAD_1, K2=KEYPAD_2, K3=KEYPAD_3, K4=KEYPAD_4
    {0x59, 0x5A, 0x5B, 0x00},  // ROW1: K5=KEYPAD_5, K6=KEYPAD_6, K7=KEYPAD_7, K8=KEYPAD_8
    {0x5C, 0x5D, 0x5E, 0x57},  // ROW2: K9=KEYPAD_9, K10=KEYPAD_0, K11=KEYPAD_ENTER, K12=KEYPAD_PLUS
    {0x5F, 0x60, 0x61, 0x00},  // ROW3: K13=KEYPAD_MINUS, K14=KEYPAD_ASTERISK, K15=KEYPAD_SLASH, K16=KEYPAD_DOT
    {0x62, 0x63, 0x58, 0x00}   // ROW4: K17=KEYPAD_NUMLOCK, K18=未使用, K19=未使用, K20=未使用
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