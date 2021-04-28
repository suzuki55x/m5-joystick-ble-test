#include <M5StickC.h>
#include "Wire.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define JOY_ADDR 0x52
#define BLE_LOCAL_NAME "M5Stick-Joy-PoC"

// Please get 3 UNIQUE UUID from https://www.uuidgenerator.net/
#define BLE_SERVICE_UUID "1043de0e-a7da-11eb-bcbc-0242ac130002"
#define BLE_CHARACTERISTIC_UUID_RX "1043e232-a7da-11eb-bcbc-0242ac130002"
#define BLE_CHARACTERISTIC_UUID_NOTIFY "1043e340-a7da-11eb-bcbc-0242ac130002"

BLEServer *pServer = NULL;
BLECharacteristic *pNotifyCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// change BLE connect state
class MyBLEServiceCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

// BLE receive handler
class MyBLECallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      String cmd = String(rxValue.c_str());
      Serial.print("Received Value: ");
      // 受信はシリアルに表示するだけ。
      Serial.println(cmd);
    }
  }
};

void initBLE() {
  // Create BLE Device
  BLEDevice::init(BLE_LOCAL_NAME);

  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyBLEServiceCallbacks());

  // Create BLE Services
  BLEService *pService = pServer->createService(BLE_SERVICE_UUID);

  // Create BLE Characteristic
  pNotifyCharacteristic = pService->createCharacteristic(BLE_CHARACTERISTIC_UUID_NOTIFY, BLECharacteristic::PROPERTY_NOTIFY);
  pNotifyCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(BLE_CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyBLECallbacks());

  // start the service
  pService->start();

  // start advertising
  pServer->getAdvertising()->start();
}

void loopBLE() {
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}

void setup() {
  M5.begin();
  //M5.Lcd.clear();
  //disable the speak noise
  //dacWrite(25, 0);

  initBLE();

  Wire.begin();
  M5.Lcd.setRotation(3);
}

uint8_t x_data;
uint8_t y_data;
uint8_t button_data;
char data[100];
void loop() {
  // put your main code here, to run repeatedly:

  loopBLE();

  Wire.requestFrom(JOY_ADDR, 3);
  if (Wire.available()) {
    x_data = Wire.read();
    y_data = Wire.read();
    button_data = Wire.read();
    sprintf(data, "x:%d,y:%d,button:%d\n", x_data, y_data, button_data);
    Serial.print(data);

    pNotifyCharacteristic->setValue(data);
    pNotifyCharacteristic->notify();

    M5.Lcd.setCursor(1, 30, 2);
    M5.Lcd.printf("x:%04d y:%04d button:%d\n", x_data, y_data, button_data);
  }
  delay(10);
}
