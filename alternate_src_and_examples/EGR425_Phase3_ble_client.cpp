#include <M5Core2.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEClient.h>
#include "Adafruit_seesaw.h"

Adafruit_seesaw ss;

#define SERVICE_UUID        "fa200045-92b4-4677-9f8b-4e23d483988e"
#define CHARACTERISTIC_UUID "009ba967-6405-49ea-8f74-86ce1d815516"

static BLEUUID serviceUUID(SERVICE_UUID);
static BLEUUID charUUID(CHARACTERISTIC_UUID);
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLEAdvertisedDevice* myDevice;
static BLERemoteCharacteristic* pRemoteCharacteristic;

int local_dot_x = 160, local_dot_y = 120; // Initial position of the client's red dot
int remote_dot_x = 0, remote_dot_y = 0; // Position of the server's red dot (to be displayed as blue)
int movement_speed = 1; // Movement speed

void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData, size_t length, bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    
    // Parse the received data to update the remote dot's position
    String data = "";
    for (size_t i = 0; i < length; i++) {
        data += (char)pData[i];
    }
    int commaIndex = data.indexOf(',');
    if (commaIndex != -1) {
        remote_dot_x = data.substring(0, commaIndex).toInt();
        remote_dot_y = data.substring(commaIndex + 1).toInt();
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* client) {
    connected = true;
  }

  void onDisconnect(BLEClient* client) {
    connected = false;
    Serial.println("Disconnected");
    doScan = true; // Start scanning again
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the characteristic value
    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        Serial.print("BLE Device found: ");
        Serial.println(advertisedDevice.toString().c_str());

        // Check for a specific device name
        if (advertisedDevice.getName() == "Tyler") {
            Serial.println("Found our device!");

            // Stop scanning
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = false;
        }
    }
};

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

    BLEDevice::init("");

    // Start scanning for BLE devices
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, true); // Scan for 5 seconds
}

void loop() {
    if (doConnect) {
        if (connectToServer()) {
            Serial.println("Connected to the server");
        } else {
            Serial.println("Failed to connect. Restarting scan...");
            doScan = true;
        }
        doConnect = false;
    }

    if (doScan) {
        BLEDevice::getScan()->start(0); // 0 means scan forever
    }

    // Gamepad logic to move the local red dot
    int joystickX = ss.analogRead(14); // X-axis
    int joystickY = ss.analogRead(15); // Y-axis

    if (abs(joystickX - 512) > 10) local_dot_x += ((joystickX - 512) / 512.0) * movement_speed;
    if (abs(joystickY - 512) > 10) local_dot_y -= ((joystickY - 512) / 512.0) * movement_speed;

    // Constrain the dot to stay within screen bounds
    local_dot_x = constrain(local_dot_x, 5, M5.Lcd.width() - 5);
    local_dot_y = constrain(local_dot_y, 5, M5.Lcd.height() - 5);

    // Update the screen with the new dot positions
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.drawCircle(local_dot_x, local_dot_y, 5, RED); // Local red dot
    M5.Lcd.drawCircle(remote_dot_x, remote_dot_y, 5, BLUE); // Remote blue dot

    delay(10);
}
