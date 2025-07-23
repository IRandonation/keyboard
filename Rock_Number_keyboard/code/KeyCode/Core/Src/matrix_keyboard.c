#include "matrix_keyboard.h"

// --- GPIO 配置 (与您原来的代码一致) ---
static GPIO_TypeDef* ROW_Port[ROW_NUM] = {GPIOE, GPIOE, GPIOE, GPIOE, GPIOE};
static uint16_t ROW_Pin[ROW_NUM] = {GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_13, GPIO_PIN_12, GPIO_PIN_11};

static GPIO_TypeDef* COL_Port[COL_NUM] = {GPIOA, GPIOA, GPIOA, GPIOA};
static uint16_t COL_Pin[COL_NUM] = {GPIO_PIN_7, GPIO_PIN_6, GPIO_PIN_5, GPIO_PIN_4};

// --- 按键映射表 (与您原来的代码一致) ---
static const uint8_t Key_Map[ROW_NUM][COL_NUM] = {
    /* ROW0 */ {0x53, 0x54, 0x55, 0x56}, // num,/,*,-
    /* ROW1 */ {0x59, 0x5A, 0x5B, 0x00}, // 7,8,9 (注意，我根据小键盘布局调整了这里，原代码可能是1,2,3)
    /* ROW2 */ {0x5C, 0x5D, 0x5E, 0x57}, // 4,5,6,+
    /* ROW3 */ {0x5F, 0x60, 0x61, 0x00}, // 1,2,3 (原代码可能是7,8,9)
    /* ROW4 */ {0x62, 0x63, 0x58, 0x00}  // 0,.,enter
};

// --- 内部状态定义 ---
typedef enum {
    STATE_IDLE,       // 0. 空闲（已释放）
    STATE_DEBOUNCE,   // 1. 消抖
    STATE_PRESSED,    // 2. 已按下
    STATE_LONG_PRESS  // 3. 长按
} KeyState;

// --- 静态变量 ---
// 存储每个按键的独立状态
static struct {
    KeyState state;       // 当前状态
    uint32_t timer;       // 计时器，用于消抖和长按判断
} s_key_fsm[ROW_NUM][COL_NUM]; // FSM: Finite State Machine

// 上次处理的时间戳
static uint32_t s_last_process_tick = 0;

// --- 函数实现 ---

/**
 * @brief 初始化函数 (与您的版本基本相同，只是更整洁)
 */
void MatrixKeyboard_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. 使能时钟
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // 2. 初始化行线 (推挽输出)
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    for (uint8_t i = 0; i < ROW_NUM; i++) {
        HAL_GPIO_WritePin(ROW_Port[i], ROW_Pin[i], GPIO_PIN_RESET);
        GPIO_InitStruct.Pin = ROW_Pin[i];
        HAL_GPIO_Init(ROW_Port[i], &GPIO_InitStruct);
    }

    // 3. 初始化列线 (下拉输入)
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    for (uint8_t i = 0; i < COL_NUM; i++) {
        GPIO_InitStruct.Pin = COL_Pin[i];
        HAL_GPIO_Init(COL_Port[i], &GPIO_InitStruct);
    }
    
    // 4. 初始化所有按键状态机
    for (uint8_t r = 0; r < ROW_NUM; r++) {
        for (uint8_t c = 0; c < COL_NUM; c++) {
            s_key_fsm[r][c].state = STATE_IDLE;
            s_key_fsm[r][c].timer = 0;
        }
    }
}


/**
 * @brief 核心处理函数 (非阻塞)
 */
uint8_t MatrixKeyboard_Process(KeyEvent* p_key_events, uint8_t buffer_size) {
    uint8_t event_count = 0;
    uint32_t now = HAL_GetTick();

    // 使用时间片轮询，避免阻塞
    if (now - s_last_process_tick < KEY_SCAN_INTERVAL) {
        return 0;
    }
    s_last_process_tick = now;

    // --- 硬件扫描 ---
    for (uint8_t r = 0; r < ROW_NUM; r++) {
        // 1. 拉高当前行
        HAL_GPIO_WritePin(ROW_Port[r], ROW_Pin[r], GPIO_PIN_SET);

        // 2. 检查所有列 (这里不需要延时，GPIO电平变化很快)
        for (uint8_t c = 0; c < COL_NUM; c++) {
            // 如果按键的键码为0, 则跳过处理
            if (Key_Map[r][c] == 0x00) continue;

            // 读取物理按键状态 (按下为1, 弹起为0)
            uint8_t is_pressed = (HAL_GPIO_ReadPin(COL_Port[c], COL_Pin[c]) == GPIO_PIN_SET);

            // --- 独立状态机逻辑处理 ---
            switch (s_key_fsm[r][c].state) {
                case STATE_IDLE:
                    if (is_pressed) {
                        s_key_fsm[r][c].state = STATE_DEBOUNCE;
                        s_key_fsm[r][c].timer = now; // 启动消抖计时
                    }
                    break;

                case STATE_DEBOUNCE:
                    if (is_pressed) {
                        if (now - s_key_fsm[r][c].timer >= KEY_DEBOUNCE_TIME) {
                            // 消抖成功，确认为按下
                            s_key_fsm[r][c].state = STATE_PRESSED;
                            s_key_fsm[r][c].timer = now; // 启动长按计时
                            
                            // 产生一个 "单击按下" 事件
                            if (event_count < buffer_size) {
                                p_key_events[event_count].key_code = Key_Map[r][c];
                                p_key_events[event_count].event = KEY_EVENT_PRESS;
                                event_count++;
                            }
                        }
                    } else {
                        // 消抖期间抖动或释放，返回空闲状态
                        s_key_fsm[r][c].state = STATE_IDLE;
                    }
                    break;

                case STATE_PRESSED:
                    if (is_pressed) {
                        // 检查是否达到长按时间
                        if (now - s_key_fsm[r][c].timer >= KEY_LONG_PRESS_TIME) {
                            s_key_fsm[r][c].state = STATE_LONG_PRESS;
                            s_key_fsm[r][c].timer = now; // 启动连发计时
                            
                            // 产生一个 "长按" 事件
                            if (event_count < buffer_size) {
                                p_key_events[event_count].key_code = Key_Map[r][c];
                                p_key_events[event_count].event = KEY_EVENT_LONG_PRESS;
                                event_count++;
                            }
                        }
                    } else {
                        // 按键被释放
                        s_key_fsm[r][c].state = STATE_IDLE;
                        // --- 新增代码 开始 ---
                        // 产生 "释放" 事件
                        if (event_count < buffer_size) {
                            p_key_events[event_count].key_code = Key_Map[r][c];
                            p_key_events[event_count].event = KEY_EVENT_RELEASE;
                            event_count++;
                        }
                        // --- 新增代码 结束 ---
                    }
                    break;
                
                case STATE_LONG_PRESS:
                    if (is_pressed) {
                        // 检查是否达到连发间隔
                        if (now - s_key_fsm[r][c].timer >= KEY_REPEAT_INTERVAL) {
                            s_key_fsm[r][c].timer = now; // 重置连发计时
                            
                            // 产生一个 "连发" 事件
                            if (event_count < buffer_size) {
                                p_key_events[event_count].key_code = Key_Map[r][c];
                                p_key_events[event_count].event = KEY_EVENT_REPEAT;
                                event_count++;
                            }
                        }
                    } else {
                        // 长按状态下释放
                        s_key_fsm[r][c].state = STATE_IDLE;
                        // --- 新增代码 开始 ---
                        // 产生 "释放" 事件
                        if (event_count < buffer_size) {
                            p_key_events[event_count].key_code = Key_Map[r][c];
                            p_key_events[event_count].event = KEY_EVENT_RELEASE;
                            event_count++;
                        }
                        // --- 新增代码 结束 ---
                    }
                    break;
                
            }
        } // end of col loop

        // 3. 恢复（拉低）当前行
        HAL_GPIO_WritePin(ROW_Port[r], ROW_Pin[r], GPIO_PIN_RESET);
    } // end of row loop

    return event_count;
}