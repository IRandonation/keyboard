# Rock Number Keyboard / KeyCode 项目说明

本仓库为基于 STM32F4（STM32F407）与 USB HID 的矩阵键盘固件工程，提供矩阵键盘扫描、按键映射与通过 USB 作为键盘设备向主机发送按键的能力。工程主要使用 STM32 HAL 库，配套 Keil MDK-ARM 工程文件，并保留 STM32CubeMX 的 `KeyCode.ioc` 以便后续外设与引脚配置维护。

## 功能特性
- 矩阵键盘扫描与去抖动处理（`Core/Src/matrix_keyboard.c`）
- 按键映射与事件上报（可在 `matrix_keyboard.h/.c` 中调整）
- 通过 USB 设备枚举为键盘（HID），向主机发送按键报告
- **WS2812 RGB背光系统**（`Core/Src/ws2812.c`）
  - 6种背光模式：关闭、静态、呼吸、彩虹、按键响应、波浪
  - Num Lock长按切换模式，亮度可调
  - DMA驱动，高效稳定的LED控制
  - 实时动态效果处理
- 采用 STM32 HAL 驱动与 ST 官方 USB Device Library

## 目录结构
- `Core/Inc` / `Core/Src`：应用入口与业务逻辑
  - `main.c`：程序入口与系统初始化
  - `matrix_keyboard.h/.c`：矩阵键盘扫描、映射与接口
  - `ws2812.h/.c`：WS2812 RGB背光驱动与多模式控制
  - `tim.c/h`：定时器配置（TIM4用于WS2812 PWM+DMA）
  - `gpio.c/h`：GPIO 引脚初始化
  - `stm32f4xx_it.c/h`：中断处理（包含DMA中断）
- `USB_DEVICE`：USB 设备层
  - `App/usb_device.*`：USB 栈初始化入口
  - `App/usbd_desc.*`：USB 设备描述符（VID/PID、字符串等）
  - `Target/usbd_conf.*`：与 HAL 的适配层
- `Drivers/STM32F4xx_HAL_Driver`、`Drivers/CMSIS`：HAL 与 CMSIS 依赖
- `MDK-ARM`：Keil uVision 工程（`KeyCode.uvprojx`、`KeyCode.uvoptx`）与启动文件
- `.eide`、`.vscode`：EIDE 与 VS Code 相关配置（可选）
- `KeyCode.ioc`：CubeMX 工程文件（用于生成/维护外设与引脚配置）

## 开发环境
- 工具链：
  - Keil MDK-ARM（推荐，已提供完整工程）
  - 或 STM32CubeIDE（可依据 `KeyCode.ioc` 重新生成工程后迁移代码）
- 依赖：
  - STM32F4 HAL Driver（已包含在 `Drivers` 目录）
  - ST USB Device Library（`Middlewares/ST/STM32_USB_Device_Library`）
- 目标芯片：STM32F407（参考启动文件 `startup_stm32f407xx.s`）

## 快速开始
1. 使用 Keil uVision 打开 `MDK-ARM/KeyCode.uvprojx`。
2. 连接硬件：
   - STM32F407 开发板 + 矩阵键盘
   - WS2812 LED灯带连接到 PB6 (TIM4_CH1)
   - USB设备口接PC
3. 选择正确的目标、下载器与时钟设置后，编译并烧录固件。
4. 上电后设备将枚举为 USB 键盘；按下矩阵键盘上的按键，主机应能收到对应键值。
5. **背光控制**：长按 Num Lock 键（>1秒）可切换背光模式。

## 配置说明
- 键盘矩阵与引脚：
  - 在 `Core/Inc/matrix_keyboard.h` 与 `Core/Src/gpio.c` 中查看/调整行列引脚定义与模式（上拉/输出等）。
  - 根据实际硬件连线更新对应的 GPIO 端口与引脚。
- 按键映射：
  - 在 `matrix_keyboard.c` 中维护从（行、列）到键值的映射表；可根据需求映射为数字、功能键或自定义 HID 键码。
- **WS2812背光配置**：
  - 在 `Core/Inc/ws2812.h` 中配置LED数量（`WS2812_LED_COUNT`）和引脚连接
  - 默认使用 PB6 (TIM4_CH1) 作为数据输出引脚
  - 可在 `ws2812.c` 中调整颜色、亮度、效果速度等参数
  - 支持的背光模式：OFF、STATIC、BREATHING、RAINBOW、KEY_REACTIVE、WAVE
- USB 描述符：
  - 在 `USB_DEVICE/App/usbd_desc.c` 中配置 `VID/PID`、制造商/产品字符串；如需产品化请申请合法 VID/PID。
  - HID 报告描述符位于 USB HID 类实现中（通常在 USB 类文件或键盘报告生成处维护）。如需修改键盘布局或支持组合键，需同步调整报告格式与发送逻辑。

## 运行时行为
- 系统初始化后启动矩阵扫描定时逻辑（或在主循环中轮询）。
- 按键状态变化将转换为 HID 报告并通过 USB 发送至主机。
- **WS2812背光系统**：
  - 系统启动时初始化为静态白色背光模式，亮度50%
  - 主循环中持续调用 `WS2812_ProcessEffects()` 处理动态效果
  - Num Lock键长按检测（>1秒）触发模式切换
  - 按键事件同步通知背光系统，实现按键响应效果
  - DMA传输完成后自动更新LED显示
- 去抖动、长按/重复等策略可在 `matrix_keyboard.c` 中扩展。

## 常用开发建议
- 使用 CubeMX 调整外设或引脚：
  - 打开 `KeyCode.ioc`，修改后仅生成驱动层，避免覆盖 `matrix_keyboard.*` 与业务逻辑文件。
- VS Code/EIDE：
  - 仓库提供 `.vscode/tasks.json` 与 `.eide`，可参考进行编译或工程管理（以实际环境为准）。

## 故障排查
- 设备未能枚举为键盘：
  - 检查 USB 供电与 D+/D- 连接；确认 `usb_device_init()` 被调用。
  - 确认 `usbd_conf.c` 与时钟配置（`system_stm32f4xx.c`）正确。
- 按键无响应或乱码：
  - 校验矩阵行列引脚配置与上拉/下拉设置；检查去抖动逻辑。
  - 检查按键映射是否与 HID 报告格式一致。
- **WS2812背光问题**：
  - LED不亮：检查 PB6 引脚连接，确认 TIM4 时钟使能，检查DMA配置
  - 颜色异常：确认WS2812时序参数（0码、1码脉宽），检查供电电压（5V推荐）
  - 模式切换无效：确认Num Lock键映射正确，检查长按检测逻辑
  - 效果卡顿：检查主循环中 `WS2812_ProcessEffects()` 调用频率
  - DMA传输错误：确认DMA中断处理函数 `DMA1_Stream0_IRQHandler()` 正确实现

## 示例电路连接图

### 矩阵键盘连接
- 行（输出，推挽，高速）：`GPIOE` — `PE15(R0)`, `PE14(R1)`, `PE13(R2)`, `PE12(R3)`, `PE11(R4)`。
- 列（输入，下拉）：`GPIOA` — `PA7(C0)`, `PA6(C1)`, `PA5(C2)`, `PA4(C3)`。
- 连接方式：每个交点为一个按键开关，可选串接二极管（建议 `1N4148`）以减少按键串扰与"鬼键"。推荐二极管方向：`Row →|— Diode —→ Column`（行拉高时，闭合将列拉至高电平）。

### WS2812 LED背光连接
- **数据线**：PB6 (TIM4_CH1) → WS2812 DIN
- **电源**：5V → WS2812 VDD，GND → WS2812 GND
- **布局建议**：LED按键盘布局排列，每个按键对应一个LED
- **信号完整性**：数据线尽量短，可加33Ω-100Ω串联电阻减少反射

```
Columns:   C0(PA7)    C1(PA6)    C2(PA5)    C3(PA4)
Rows
R0(PE15) ──[SW]───────[SW]───────[SW]───────[SW]
R1(PE14) ──[SW]───────[SW]───────[SW]───────(空)
R2(PE13) ──[SW]───────[SW]───────[SW]───────[SW]
R3(PE12) ──[SW]───────[SW]───────[SW]───────(空)
R4(PE11) ──[SW]───────[SW]───────[SW]───────(空)
```

说明：行由固件轮询依次拉高，列为下拉输入；当某行与某列的开关闭合时，该列读取为高电平，触发对应按键事件。

## 按键映射表（与 `matrix_keyboard.c` 保持一致）
- 行索引与引脚：`R0=PE15`, `R1=PE14`, `R2=PE13`, `R3=PE12`, `R4=PE11`
- 列索引与引脚：`C0=PA7`, `C1=PA6`, `C2=PA5`, `C3=PA4`

| 行(R) | 列(C) | 行引脚 | 列引脚 | HID 键码 | 说明 |
|-------|------|--------|--------|----------|------|
| R0 | C0 | PE15 | PA7 | `0x53` | Num Lock |
| R0 | C1 | PE15 | PA6 | `0x54` | Keypad `/` |
| R0 | C2 | PE15 | PA5 | `0x55` | Keypad `*` |
| R0 | C3 | PE15 | PA4 | `0x56` | Keypad `-` |
| R1 | C0 | PE14 | PA7 | `0x59` | Keypad `1` |
| R1 | C1 | PE14 | PA6 | `0x5A` | Keypad `2` |
| R1 | C2 | PE14 | PA5 | `0x5B` | Keypad `3` |
| R1 | C3 | PE14 | PA4 | `0x00` | 空位 |
| R2 | C0 | PE13 | PA7 | `0x5C` | Keypad `4` |
| R2 | C1 | PE13 | PA6 | `0x5D` | Keypad `5` |
| R2 | C2 | PE13 | PA5 | `0x5E` | Keypad `6` |
| R2 | C3 | PE13 | PA4 | `0x57` | Keypad `+` |
| R3 | C0 | PE12 | PA7 | `0x5F` | Keypad `7` |
| R3 | C1 | PE12 | PA6 | `0x60` | Keypad `8` |
| R3 | C2 | PE12 | PA5 | `0x61` | Keypad `9` |
| R3 | C3 | PE12 | PA4 | `0x00` | 空位 |
| R4 | C0 | PE11 | PA7 | `0x62` | Keypad `0` |
| R4 | C1 | PE11 | PA6 | `0x63` | Keypad `.` |
| R4 | C2 | PE11 | PA5 | `0x58` | Keypad `Enter` |
| R4 | C3 | PE11 | PA4 | `0x00` | 空位 |

提示：如需更改布局或引脚，请同步更新 `Core/Src/matrix_keyboard.c` 中的 `ROW_Port/ROW_Pin`、`COL_Port/COL_Pin` 与 `Key_Map`，并确保 USB HID 报告的生成逻辑能够正确处理新的键码。

## WS2812 背光模式详细说明

### 支持的背光模式

1. **OFF模式** - 所有LED关闭，节能模式
2. **STATIC模式** - 静态白色背光，适合日常使用
3. **BREATHING模式** - 呼吸灯效果，亮度平滑变化（周期约2秒）
4. **RAINBOW模式** - 彩虹色循环效果，色彩丰富动态
5. **KEY_REACTIVE模式** - 按键响应模式，按下时LED亮起，释放后渐变熄灭
6. **WAVE模式** - 青色波浪效果，从左到右流动

### 模式切换操作

- **切换方法**：长按 Num Lock 键超过1秒
- **切换顺序**：OFF → STATIC → BREATHING → RAINBOW → KEY_REACTIVE → WAVE → OFF（循环）
- **状态保持**：模式切换后立即生效，重启后恢复默认STATIC模式

### 技术参数

- **LED数量**：20个（可在 `ws2812.h` 中的 `WS2812_LED_COUNT` 修改）
- **默认亮度**：50%（可通过 `WS2812_SetBrightness()` 调节0-100%）
- **更新频率**：主循环调用，约1kHz
- **DMA传输**：使用DMA1 Stream0，自动传输24位RGB数据
- **时序标准**：符合WS2812B规范（T0H=0.4μs, T1H=0.8μs, T0L=0.85μs, T1L=0.45μs）

### API接口说明

```c
// 模式控制
void WS2812_SetMode(WS2812_Mode mode);          // 设置背光模式
WS2812_Mode WS2812_GetMode(void);               // 获取当前模式
void WS2812_NextMode(void);                     // 切换到下一个模式

// 亮度控制
void WS2812_SetBrightness(uint8_t brightness);  // 设置亮度(0-100)
uint8_t WS2812_GetBrightness(void);             // 获取当前亮度

// 效果处理
void WS2812_ProcessEffects(void);               // 处理动态效果（主循环调用）
void WS2812_OnKeyPress(uint8_t row, uint8_t col);   // 按键按下事件
void WS2812_OnKeyRelease(uint8_t row, uint8_t col); // 按键释放事件

// 颜色控制
void WS2812_SetColor(uint16_t led_index, uint8_t r, uint8_t g, uint8_t b);
void WS2812_SetAllColor(uint8_t r, uint8_t g, uint8_t b);
void WS2812_Update(void);                       // 更新LED显示
```

### 自定义开发

如需添加新的背光模式或修改现有效果：

1. 在 `ws2812.h` 中的 `WS2812_Mode` 枚举添加新模式
2. 在 `ws2812.c` 的 `WS2812_ProcessEffects()` 函数中添加对应的处理逻辑
3. 根据需要调整颜色、速度、亮度等参数
4. 可参考现有模式的实现方式，使用定时器和数学函数创建动态效果

## 许可证
- 本项目未显式声明整体许可证；第三方依赖（HAL/USB 库）遵循各自目录下的许可文件。
- 如需开源或商用，请添加或遵循相应的许可证与合规要求。

## 贡献
- 欢迎通过 Issue 或 PR 提交改进建议与修复。
- 提交前请尽量维持代码风格与目录结构一致性。