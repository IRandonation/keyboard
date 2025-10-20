/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"
#include "ws2812.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_hid.h"
#include "matrix_keyboard.h"  // 包含矩阵键盘头文件
#include <stdbool.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceFS;
// HID报告结构体（兼容标准键盘）
#pragma pack(push, 1)
typedef struct {
    uint8_t modifier;   // 修饰键
    uint8_t reserved;   // 保留
    uint8_t keycode[6]; // 最多6键
} HID_KeyboardReport;
#pragma pack(pop)

// 添加键盘报告变量
static HID_KeyboardReport keyboard_report;

// 发送HID报告
void Send_HID_Report(uint8_t keycode) {
    HID_KeyboardReport report = {0};

    if (keycode != 0) {
        report.keycode[0] = keycode; // 单键按下
    }

    // 通过USB发送报告
    USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&report, sizeof(report));
    HAL_Delay(20); // 等待数据发送完成
}

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  WS2812_Init();
  HAL_TIM_Base_Start_IT(&htim3);
  MatrixKeyboard_Init();
  /* 定义一个缓冲区来接收按键事件 */
   KeyEvent key_events_buffer[MAX_PRESSED_KEYS];
   
  // 用于标记HID报告是否需要更新并发送
  bool report_needs_update = false;

  // 清空初始的键盘报告
  memset(&keyboard_report, 0, sizeof(keyboard_report));
  
  // 模式切换相关变量
  static uint32_t num_lock_press_time = 0;
  static bool num_lock_long_press_handled = false;
  
  // 设置默认背光模式
  WS2812_SetMode(WS2812_MODE_STATIC);
  WS2812_SetBrightness(50);
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 1. 从事件队列获取按键事件（由TIM3中断产生）
        uint8_t event_count = 0;
        for (uint8_t i = 0; i < MAX_PRESSED_KEYS; i++) {
            if (MatrixKeyboard_PopEvent(&key_events_buffer[i])) {
                event_count++;
            } else {
                break;
            }
        }

        // 2. 如果有任何事件发生，则处理它们
        if (event_count > 0) {
            // 标记报告需要更新
            report_needs_update = true;

            for (uint8_t i = 0; i < event_count; i++) {
                KeyEvent event = key_events_buffer[i];

                // 检查是否是Num Lock键 (ROW0, COL0, 键码0x53)
                bool is_num_lock = (event.row == 0 && event.col == 0 && event.key_code == 0x53);

                if (event.event == KEY_EVENT_PRESS) 
                {
                    if (is_num_lock) {
                        num_lock_press_time = HAL_GetTick();
                        num_lock_long_press_handled = false;
                    }
                    
                    // 按下事件: 将键码添加到HID报告中
                    // 查找一个空位来存放新的键码
                    for (int j = 0; j < 6; j++) {
                        if (keyboard_report.keycode[j] == 0x00) {
                            keyboard_report.keycode[j] = event.key_code;
                            break;
                        }
                    }
                    
                    // 通知WS2812按键按下事件
                    WS2812_OnKeyPress(event.row, event.col);
                }
                else if (event.event == KEY_EVENT_LONG_PRESS) 
                {
                    if (is_num_lock && !num_lock_long_press_handled) {
                        // Num Lock长按切换背光模式
                        WS2812_NextMode();
                        num_lock_long_press_handled = true;
                        
                        // 不发送Num Lock的HID报告，避免切换系统Num Lock状态
                        continue;
                    }
                    
                    // 其他键的长按处理
                    for (int j = 0; j < 6; j++) {
                        if (keyboard_report.keycode[j] == 0x00) {
                            keyboard_report.keycode[j] = event.key_code;
                            break;
                        }
                    }
                    
                    WS2812_OnKeyPress(event.row, event.col);
                }
                else if (event.event == KEY_EVENT_REPEAT) 
                {
                    // 连发事件处理
                    if (!is_num_lock) {  // Num Lock不处理连发
                        for (int j = 0; j < 6; j++) {
                            if (keyboard_report.keycode[j] == 0x00) {
                                keyboard_report.keycode[j] = event.key_code;
                                break;
                            }
                        }
                    }
                }
                else if (event.event == KEY_EVENT_RELEASE) 
                {
                    if (is_num_lock) {
                        uint32_t press_duration = HAL_GetTick() - num_lock_press_time;
                        
                        // 如果是短按且没有处理过长按，则正常发送Num Lock
                        if (press_duration < 1000 && !num_lock_long_press_handled) {
                            // 正常的Num Lock短按，发送HID报告
                        } else {
                            // 长按释放，不发送HID报告
                            continue;
                        }
                    }
                    
                    // 释放事件: 从HID报告中移除该键码
                    for (int j = 0; j < 6; j++) {
                        if (keyboard_report.keycode[j] == event.key_code) {
                            keyboard_report.keycode[j] = 0x00; // 清除
                        }
                    }
                    
                    // 通知WS2812按键释放事件
                    WS2812_OnKeyRelease(event.row, event.col);
                }
            }
        }

        // 3. 检查是否需要发送HID报告
        // 只有在按键状态变化时才发送，这比每次循环都发送更高效
        if (report_needs_update) {
            // 清除更新标记
            report_needs_update = false;

            // 发送HID报告（仅在状态变化时发送）
            USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&keyboard_report, sizeof(keyboard_report));
            
            // 为了处理按键释放，我们需要在发送完有效报告后，
            // 立即发送一个全零的 "释放" 报告。
            // 但更好的做法是让PC自己处理按键释放。
            // 只有当所有按键都释放时，我们才需要发送一个全零报告。
            // 我们可以通过检查 keyboard_report 是否全为0来判断。
            bool all_keys_released = true;
            for(int j=0; j<6; j++) {
                if (keyboard_report.keycode[j] != 0) {
                    all_keys_released = false;
                    break;
                }
            }

            // 如果所有按键都释放了，并且上一次不是全零报告，则发送一次全零报告
            if(all_keys_released){
                 // 稍作延时，确保上一个报告被PC接收
                 // HAL_Delay(1); 
                 // 发送全零报告
                 memset(&keyboard_report, 0, sizeof(keyboard_report));
                 USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&keyboard_report, sizeof(keyboard_report));
            }
        }

        // 处理WS2812动态效果
        WS2812_ProcessEffects();
        
        // 更新 WS2812 LED 如果有变化
        if (report_needs_update) {
            WS2812_Update();
        }
        
        // 可以在这里加一个非常短的延时，以降低CPU使用率，但不是必须的
        // HAL_Delay(1); 
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3) {
    MatrixKeyboard_ScanStep_ISR();
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
