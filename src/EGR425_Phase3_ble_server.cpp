#include <M5Core2.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "Adafruit_seesaw.h"

Adafruit_seesaw ss;

#define SERVICE_UUID        "fa200045-92b4-4677-9f8b-4e23d483988e" // Unique for BLE service
#define CHARACTERISTIC_UUID "009ba967-6405-49ea-8f74-86ce1d815516" // Unique for BLE characteristic

BLEServer *bleServer;
BLEService *bleService;
BLECharacteristic *bleCharacteristic;

int dot_x = 160, dot_y = 120; // Initial position of the red dot
int movement_speed = 8; // Initial movement speed

void setup() {
    M5.begin();
    Serial.begin(115200);
    Wire.begin();

    // Initialize seesaw (gamepad)
    if (!ss.begin(0x50)) {
        Serial.println("Seesaw not found! Please check your wiring.");
        while (1) delay(10); // Halt if gamepad not found
    }

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.drawCircle(dot_x, dot_y, 5, RED); // Draw initial red dot

    // Initialize BLE
    BLEDevice::init("Tyler");
    bleServer = BLEDevice::createServer();
    bleService = bleServer->createService(SERVICE_UUID);
    bleCharacteristic = bleService->createCharacteristic(
                            CHARACTERISTIC_UUID,
                            BLECharacteristic::PROPERTY_READ |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY
                        );
    bleCharacteristic->addDescriptor(new BLE2902());

    bleService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Waiting for a client connection to notify...");
}

void loop() {
    // Gamepad logic to move the red dot
    int joystickX = ss.analogRead(14); // X-axis
    int joystickY = ss.analogRead(15); // Y-axis

    if (abs(joystickX - 512) > 10) dot_x += ((joystickX - 512) / 512.0) * movement_speed;
    if (abs(joystickY - 512) > 10) dot_y -= ((joystickY - 512) / 512.0) * movement_speed;

    // Constrain the dot to stay within screen bounds
    dot_x = constrain(dot_x, 5, M5.Lcd.width() - 5);
    dot_y = constrain(dot_y, 5, M5.Lcd.height() - 5);

    // Update the screen with the new dot position
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.drawCircle(dot_x, dot_y, 5, RED);

    // Send the new position over BLE
    String dotPosition = String(dot_x) + "," + String(dot_y);
    bleCharacteristic->setValue(dotPosition.c_str());
    bleCharacteristic->notify();

    delay(10); // Delay to make the dot movement smoother
}
