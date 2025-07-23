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
#include "usb_device.h"
#include "gpio.h"

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
  // MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  MatrixKeyboard_Init();
  /* 定义一个缓冲区来接收按键事件 */
   KeyEvent key_events_buffer[MAX_PRESSED_KEYS];
   
  // 用于标记HID报告是否需要更新并发送
  bool report_needs_update = false;

  // 清空初始的键盘报告
  memset(&keyboard_report, 0, sizeof(keyboard_report));  
  
  /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 1. 非阻塞地调用键盘处理函数，获取事件列表
        uint8_t event_count = MatrixKeyboard_Process(key_events_buffer, MAX_PRESSED_KEYS);

        // 2. 如果有任何事件发生，则处理它们
        if (event_count > 0) {
            // 标记报告需要更新
            report_needs_update = true;

            for (uint8_t i = 0; i < event_count; i++) {
                KeyEvent event = key_events_buffer[i];

                if (event.event == KEY_EVENT_PRESS || 
                    event.event == KEY_EVENT_LONG_PRESS || 
                    event.event == KEY_EVENT_REPEAT) 
                {
                    // 按下事件: 将键码添加到HID报告中
                    // 查找一个空位来存放新的键码
                    for (int j = 0; j < 6; j++) {
                        if (keyboard_report.keycode[j] == 0x00) {
                            keyboard_report.keycode[j] = event.key_code;
                            break;
                        }
                    }
                } 
                else if (event.event == KEY_EVENT_RELEASE) 
                {
                    // 释放事件: 从HID报告中移除该键码
                    for (int j = 0; j < 6; j++) {
                        if (keyboard_report.keycode[j] == event.key_code) {
                            keyboard_report.keycode[j] = 0x00; // 清除
                        }
                    }
                }
            }
        }

        // 3. 检查是否需要发送HID报告
        // 只有在按键状态变化时才发送，这比每次循环都发送更高效
        if (report_needs_update) {
            // 清除更新标记
            report_needs_update = false;

            // 发送HID报告 (这里的 hUsbDeviceFS 是STM32CubeMX生成的标准句柄)
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
                 HAL_Delay(10); 
                 // 发送全零报告
                 memset(&keyboard_report, 0, sizeof(keyboard_report));
                 USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&keyboard_report, sizeof(keyboard_report));
            }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
