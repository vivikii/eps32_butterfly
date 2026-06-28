#include "ble.h"
#include "config.h"
#include <Arduino.h>
#include <string.h>

static BLEServer* pServer = nullptr;
static BLECharacteristic* pCharacteristic = nullptr;
static bool connected = false;

static char pending_msg[BLE_RX_MAX];
static volatile size_t pending_len = 0;
static volatile bool msg_ready = false;

static char pending_notify[128];
static volatile bool notify_ready = false;
static uint32_t last_notify_ms = 0;

static const uint32_t NOTIFY_MIN_INTERVAL_MS = 150;

void (*on_ble_message)(const char* msg, size_t len) = nullptr;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* server) override {
        connected = true;
        Serial.println("BLE connected");
    }

    void onDisconnect(BLEServer* server) override {
        connected = false;
        msg_ready = false;
        notify_ready = false;
        Serial.println("BLE disconnected");
        server->getAdvertising()->start();
    }
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        std::string rxValue = characteristic->getValue();
        if (rxValue.length() == 0 || rxValue.length() >= BLE_RX_MAX) {
            return;
        }
        memcpy(pending_msg, rxValue.data(), rxValue.length());
        pending_msg[rxValue.length()] = '\0';
        pending_len = rxValue.length();
        msg_ready = true;
    }
};

void ble_init() {
    BLEDevice::init(BLE_DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService* pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    pCharacteristic->setValue("ready");

    pService->start();

    BLEAdvertising* pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    Serial.printf("BLE ready, MAC: %s\n", BLEDevice::getAddress().toString().c_str());
}

void ble_poll() {
    if (msg_ready) {
        msg_ready = false;
        size_t len = pending_len;
        if (on_ble_message && len > 0) {
            Serial.printf("BLE rx: %s\n", pending_msg);
            on_ble_message(pending_msg, len);
        }
    }

    if (!notify_ready || !connected || !pCharacteristic) {
        return;
    }

    uint32_t now = millis();
    if (now - last_notify_ms < NOTIFY_MIN_INTERVAL_MS) {
        return;
    }

    notify_ready = false;
    pCharacteristic->setValue(pending_notify);
    pCharacteristic->notify();
    last_notify_ms = now;
    Serial.printf("BLE notify: %s\n", pending_notify);
}

bool ble_is_connected() {
    return connected;
}

void ble_send_status(const char* msg) {
    if (!msg || !connected) {
        return;
    }
    strncpy(pending_notify, msg, sizeof(pending_notify) - 1);
    pending_notify[sizeof(pending_notify) - 1] = '\0';
    notify_ready = true;
}
