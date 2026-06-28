#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "pwm_control.h"
#include "butterfly_motion.h"
#include "ble.h"

#define LED_PIN 7
#define BLE_MSG_MAX 64

static uint32_t last_status_ms = 0;
static bool status_notify_pending = false;

static const char* mode_to_str(FlightMode mode) {//将飞行模式转换为字符串
    switch (mode) {
        case FLIGHT_NEUTRAL:  return "N";
        case FLIGHT_LEFT:     return "L";
        case FLIGHT_RIGHT:    return "R";
        case FLIGHT_CLIMB:    return "U";
        case FLIGHT_DESCEND:  return "D";
        default:              return "S";
    }
}

static void send_status_notify() {//通过蓝牙发送状态通知
    char buf[128];
    snprintf(buf, sizeof(buf),
             "run:%d,mode:%s,cl:%d,cr:%d,fd:%d,ys:%lu,turn:%d,climb:%d,cspeed:%d,pt:%d",
             butterfly_is_running() ? 1 : 0,
             mode_to_str(butterfly_get_mode()),
             butterfly_get_center_left(),
             butterfly_get_center_right(),
             butterfly_get_amplitude(),
             butterfly_get_step_delay(),
             butterfly_get_turn_delta(),
             butterfly_get_climb_delta(),
             butterfly_get_climb_speed_delta(),
             butterfly_get_point_index());
    ble_send_status(buf);
}

static void send_help_notify() {//发送帮助通知
    ble_send_status(
        "help: S/P/H/L/R/U/D/N | cl:1640 cr:1640 fd:600 ys:8000 turn:150 | reset | ?"
    );
}

static void to_lower_inplace(char* s) {
    for (; *s; s++) {
        *s = (char)tolower((unsigned char)*s);
    }
}

static bool apply_param(const char* key, int value) {
    if (strcmp(key, "cl") == 0) {
        butterfly_set_center_left(value);
        return true;
    }
    if (strcmp(key, "cr") == 0) {
        butterfly_set_center_right(value);
        return true;
    }
    if (strcmp(key, "c") == 0) {
        butterfly_set_center_left(value);
        butterfly_set_center_right(value);
        return true;
    }
    if (strcmp(key, "fd") == 0) {
        butterfly_set_amplitude(value);
        return true;
    }
    if (strcmp(key, "ys") == 0) {
        butterfly_set_step_delay((uint32_t)value);
        return true;
    }
    if (strcmp(key, "turn") == 0) {
        butterfly_set_turn_delta(value);
        return true;
    }
    if (strcmp(key, "climb") == 0) {
        butterfly_set_climb_delta(value);
        return true;
    }
    if (strcmp(key, "cspeed") == 0) {
        butterfly_set_climb_speed_delta(value);
        return true;
    }
    return false;
}

static bool parse_param_pair(char* pair) {
    char* sep = strchr(pair, ':');
    if (!sep) {
        sep = strchr(pair, '=');
    }
    if (!sep) {
        return false;
    }

    *sep = '\0';
    char* key = pair;
    char* val_str = sep + 1;

    while (*key && isspace((unsigned char)*key)) key++;
    while (*val_str && isspace((unsigned char)*val_str)) val_str++;

    to_lower_inplace(key);
    int value = atoi(val_str);
    return apply_param(key, value);
}

static void handle_motion_char(char cmd) {
    switch (cmd) {
        case 'S':
        case 's':
            butterfly_start();
            break;
        case 'P':
        case 'p':
            butterfly_stop();
            break;
        case 'H':
        case 'h':
            butterfly_go_center();
            break;
        case 'L':
        case 'l':
            butterfly_set_mode(FLIGHT_LEFT);
            break;
        case 'R':
        case 'r':
            butterfly_set_mode(FLIGHT_RIGHT);
            break;
        case 'U':
        case 'u':
            butterfly_set_mode(FLIGHT_CLIMB);
            break;
        case 'D':
        case 'd':
            butterfly_set_mode(FLIGHT_DESCEND);
            break;
        case 'N':
        case 'n':
            butterfly_set_mode(FLIGHT_NEUTRAL);
            break;
        case '+':
            butterfly_adjust_amplitude(50);
            break;
        case '-':
            butterfly_adjust_amplitude(-50);
            break;
        case '>':
            butterfly_adjust_step_delay(-1000);
            break;
        case '<':
            butterfly_adjust_step_delay(1000);
            break;
        case '?':
            send_status_notify();
            return;
        default:
            Serial.printf("Unknown cmd: %c\n", cmd);
            return;
    }
    send_status_notify();
}

static void handle_ble_message(const char* msg, size_t len) {//处理蓝牙消息
    if (!msg || len == 0) {//如果消息为空或长度为0，则返回
        return;
    }

    char buf[BLE_MSG_MAX];//创建一个缓冲区
    size_t copy_len = len < (BLE_MSG_MAX - 1) ? len : (BLE_MSG_MAX - 1);//复制消息长度，如果长度大于缓冲区大小，则截取缓冲区大小
    memcpy(buf, msg, copy_len);
    buf[copy_len] = '\0';//添加结束符

    while (copy_len > 0 && isspace((unsigned char)buf[copy_len - 1])) {//如果缓冲区长度大于0且最后一个字符为空格，则删除最后一个字符
        buf[--copy_len] = '\0';
    }

    char* start = buf;      //开始指针
    while (*start && isspace((unsigned char)*start)) {//如果开始指针不为空且开始指针指向的字符为空格，则移动开始指针
        start++;
    }

    if (*start == '\0') {       //如果开始指针为空，则返回
        return;
    }

    to_lower_inplace(start);   //将开始指针指向的字符串转换为小写字母       

    if (strcmp(start, "help") == 0) {       //如果开始指针指向的字符串为help，则发送帮助通知
        send_help_notify();
        return;
    }
    if (strcmp(start, "reset") == 0) {       //如果开始指针指向的字符串为reset，则重置参数  
        butterfly_reset_params();
        if (!butterfly_is_running()) {       //如果蝴蝶不运行，则将蝴蝶移动到中心位置
            butterfly_go_center();
        }
        send_status_notify();       //发送状态通知
        return;
    }
    if (strcmp(start, "status") == 0 || strcmp(start, "?") == 0) {       //如果开始指针指向的字符串为status或?，则发送状态通知  
        send_status_notify();
        return;
    }

    if (strlen(start) == 1) {       //如果开始指针指向的字符串长度为1，则处理运动字符  
        handle_motion_char(start[0]);
        return;
    }

    bool changed = false;       //是否改变标志
    char* saveptr = nullptr;
    char* token = strtok_r(start, ",", &saveptr);       //分割字符串
    while (token) {
        if (parse_param_pair(token)) {       //如果解析参数对成功，则改变标志
            changed = true;
        } else {
            Serial.printf("Unknown param: %s\n", token);
        }
        token = strtok_r(NULL, ",", &saveptr);
    }

    if (changed) {       //如果改变标志，则发送状态通知
        send_status_notify();
    } else {
        ble_send_status("err:unknown cmd, send help");       //发送错误通知
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32-C3 Butterfly init...");

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    servo_init();
    butterfly_init();

    Serial.printf("Center servos L=%d R=%d us, wait 1s...\n",
                  DEFAULT_CENTER_LEFT_US, DEFAULT_CENTER_RIGHT_US);
    delay(1000);

    on_ble_message = handle_ble_message;
    ble_init();

    Serial.println("Ready. BLE motion: S/P/H/L/R/U/D/N/+/-/></<?");
    Serial.println("BLE tune: cl:1640 cr:1640 fd:600 ys:8000 turn:150 climb:100 cspeed:2000");
    Serial.println("BLE utils: help | reset | status");
}

void loop() {
    ble_poll();//处理蓝牙消息
    butterfly_update();//更新蝴蝶状态

    if (butterfly_is_running()) {
        digitalWrite(LED_PIN, (millis() / 500) % 2);
    } else {
        digitalWrite(LED_PIN, ble_is_connected() ? HIGH : LOW);
    }

    if (ble_is_connected() && millis() - last_status_ms >= 2000) {
        last_status_ms = millis();
        send_status_notify();//
    }
}
