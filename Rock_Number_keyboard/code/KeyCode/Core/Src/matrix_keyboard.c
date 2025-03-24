#include "matrix_keyboard.h"

// 行配置（默认低电平）
static GPIO_TypeDef* ROW_Port[ROW_NUM] = {GPIOE, GPIOE, GPIOE, GPIOE, GPIOE};
static uint16_t ROW_Pin[ROW_NUM] = {GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_13, GPIO_PIN_12, GPIO_PIN_11};

// 列配置（下拉输入）
static GPIO_TypeDef* COL_Port[COL_NUM] = {GPIOA, GPIOA, GPIOA, GPIOA};
static uint16_t COL_Pin[COL_NUM] = {GPIO_PIN_7, GPIO_PIN_6, GPIO_PIN_5, GPIO_PIN_4};

static const uint8_t Key_Map[ROW_NUM][COL_NUM] = {
    // 小键盘布局示例（根据实际硬件调整行列顺序）
    /* ROW0 */ {0x53, 0x54, 0x55, 0x56},  // num,/,*,-
    /* ROW1 */ {0x59, 0x5A, 0x5B, 0x00},  // 1,2,3
    /* ROW2 */ {0x5C, 0x5D, 0x5E, 0x57},  // 4,5,6,+
    /* ROW3 */ {0x5F, 0x60, 0x61, 0x00},  // 7,8,9
    /* ROW4 */ {0x62, 0x63, 0x58, 0x00}   // 0,.,enter
};

void MatrixKeyboard_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 使能所有用到的GPIO端口时钟
    __HAL_RCC_GPIOE_CLK_ENABLE();  // 行线使用GPIOE
    __HAL_RCC_GPIOA_CLK_ENABLE();  // 列线使用GPIOA

    // 初始化行线为推挽输出，默认低电平
    for (uint8_t i = 0; i < ROW_NUM; i++) {
        HAL_GPIO_WritePin(ROW_Port[i], ROW_Pin[i], GPIO_PIN_RESET); // 确保初始化为低
        GPIO_InitStruct.Pin = ROW_Pin[i];
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // 推挽输出
        GPIO_InitStruct.Pull = GPIO_NOPULL;          // 无需上/下拉
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(ROW_Port[i], &GPIO_InitStruct);
    }

    // 初始化列线为下拉输入
    for (uint8_t i = 0; i < COL_NUM; i++) {
        GPIO_InitStruct.Pin = COL_Pin[i];
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;     // 输入模式
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;       // 下拉电阻
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(COL_Port[i], &GPIO_InitStruct);
    }
}

uint8_t MatrixKeyboard_Scan(void) {
    uint8_t current_key = 0;

    // 逐行扫描
    for (uint8_t i = 0; i < ROW_NUM; i++) {
        // 拉高当前行，其他行保持低电平
        for (uint8_t r = 0; r < ROW_NUM; r++) {
            HAL_GPIO_WritePin(ROW_Port[r], ROW_Pin[r], 
                            (r == i) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }

        // 延时确保电平稳定（1ms）
        HAL_Delay(1);

        // 检测列线高电平
        for (uint8_t j = 0; j < COL_NUM; j++) {
            if (HAL_GPIO_ReadPin(COL_Port[j], COL_Pin[j]) == GPIO_PIN_SET) {
                current_key = Key_Map[i][j];
                goto exit_scan;
            }
        }
    }

exit_scan:
    // 恢复所有行线为低电平
    for (uint8_t r = 0; r < ROW_NUM; r++) {
        HAL_GPIO_WritePin(ROW_Port[r], ROW_Pin[r], GPIO_PIN_RESET);
    }

    // 状态机处理（修改如下）
    switch (s_key_state) {
        case KEY_STATE_UP:
            if (current_key != 0) {
                s_last_key = current_key;
                s_last_tick = HAL_GetTick();
                s_key_state = KEY_STATE_DEBOUNCE;
            }
            break;

        case KEY_STATE_DEBOUNCE:
            if (current_key == s_last_key) {
                if (HAL_GetTick() - s_last_tick >= 20) {
                    s_key_state = KEY_STATE_DOWN;
                    return s_last_key; // 首次触发
                }
            } else {
                s_key_state = KEY_STATE_UP;
            }
            break;

        case KEY_STATE_DOWN:
            if (current_key == s_last_key) {
                // 达到初始延迟后进入重复状态
                if (HAL_GetTick() - s_last_tick >= INITIAL_DELAY) {
                    s_key_state = KEY_STATE_REPEAT;
                    s_last_tick = HAL_GetTick(); // 重置计时
                    return s_last_key; // 触发第一次重复
                }
            } else {
                s_key_state = KEY_STATE_UP;
            }
            break;

        case KEY_STATE_REPEAT:
            if (current_key == s_last_key) {
                // 按固定间隔重复触发
                if (HAL_GetTick() - s_last_tick >= REPEAT_INTERVAL) {
                    s_last_tick = HAL_GetTick();
                    return s_last_key;
                }
            } else {
                s_key_state = KEY_STATE_UP;
            }
            break;
    }

    return 0;
}