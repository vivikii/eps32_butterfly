#ifndef BLE_H
#define BLE_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define BLE_RX_MAX 64

void ble_init();
void ble_poll();
bool ble_is_connected();
void ble_send_status(const char* msg);

extern void (*on_ble_message)(const char* msg, size_t len);

#endif
