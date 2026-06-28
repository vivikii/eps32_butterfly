#ifndef CONFIG_H
#define CONFIG_H

// 舵机 PWM 引脚（GPIO7 为板载 LED）
#define SERVO1_PIN 6   // 舵机 1 → GPIO6
#define SERVO2_PIN 4   // 舵机 2 → GPIO4

// 舵机 PWM：50Hz，脉宽 500~2500μs
#define SERVO_FREQ 50  // 频率（Hz）
#define SERVO_RES  14 // 分辨率（位）

// 扑翼默认参数
#define DEFAULT_CENTER_LEFT_US  1640  // 左翅中立（GPIO6，μs）
#define DEFAULT_CENTER_RIGHT_US 1640  // 右翅中立（GPIO4，μs）
#define DEFAULT_AMPLITUDE_US 600    // 幅度 fd（μs）
#define DEFAULT_STEP_DELAY_US 10000 // 每点延时 ys（μs，很慢）
#define HALF_CYCLE_POINTS    18     // 半周期离散点数

// 转向 / 升降微调量（μs 或 μs 延时）
#define TURN_AMP_DELTA       150    // 转向时两翼幅度差
#define CLIMB_AMP_DELTA      100    // 上升/下降幅度调整
#define CLIMB_SPEED_DELTA    2200   // 上升/下降速度调整（step_delay 加减）

// BLE 设备名称
#define BLE_DEVICE_NAME "Butterfly_C3"

// BLE Service & Characteristic UUID
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#endif
