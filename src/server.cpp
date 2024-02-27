///////////////////////////////////////////////////////////////
// Imports
///////////////////////////////////////////////////////////////
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <M5Core2.h>
#include <Adafruit_seesaw.h>

///////////////////////////////////////////////////////////////
// Variables
///////////////////////////////////////////////////////////////
BLEServer *bleServer;
BLEService *bleService;
BLECharacteristic *bleReadXCharacteristic;
BLECharacteristic *bleReadYCharacteristic;
BLECharacteristic *bleReadWriteXCharacteristic;
BLECharacteristic *bleReadWriteYCharacteristic;
bool deviceConnected = false;
bool previouslyConnected = false;
int timer = 0;

// Unique IDs
#define SERVICE_UUID "7d7a7768-a9d0-4fb8-bf2b-fc994c662eb6"
#define READ_X_CHARACTERISTIC_UUID "563c64b2-9634-4f7a-9f4f-d9e3231faa56"
#define READ_Y_CHARACTERISTIC_UUID "aa88ac15-3e2b-4735-92ff-4c712173e9f3"
#define READ_WRITE_X_CHARACTERISTIC_UUID "1da468d6-993d-4387-9e71-1c826b10fff9"
#define READ_WRITE_Y_CHARACTERISTIC_UUID "cf7b4787-d412-4e69-8b61-e2cfba89ff19"

// State
enum Screen { S_GAME, S_GAME_OVER };
static Screen screen = S_GAME;

// Gameplay Variables
Adafruit_seesaw gamePad;

#define BUTTON_SELECT    0
#define BUTTON_START    16
uint32_t button_mask = (1UL << BUTTON_START) | (1UL << BUTTON_SELECT);

// joystick and button coordinates
int xServer = 10, yServer = 120, xClient = 0, yClient = 0;
// joystick and button acceleration
int acceleration = 1;

///////////////////////////////////////////////////////////////
// BLE Server Callback Methods
///////////////////////////////////////////////////////////////
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        deviceConnected = true;
        previouslyConnected = true;
        Serial.println("Device connected...");
    }
    void onDisconnect(BLEServer *pServer) {
        deviceConnected = false;
        Serial.println("Device disconnected...");
    }
};

//////////////////////////////////////////////////////////////
// BLE Client Characteristic Callback Methods
//////////////////////////////////////////////////////////////
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    // callback function to support a read request
    void onRead(BLECharacteristic* pCharacteristic) {
        String characteristicUUID = pCharacteristic->getUUID().toString().c_str();
        String characteristicValue = pCharacteristic->getValue().c_str();
        Serial.printf("Client JUST read from %s: %s", characteristicUUID, characteristicValue.c_str());

        // // check if it is x or y and update the coordinates of the server's dot
        // if (characteristicUUID.equals(READ_X_CHARACTERISTIC_UUID)) {
        //     // update x
        //     std::string readXValue = pCharacteristic->getValue();
        //     String valXStr = readXValue.c_str();
        //     xClient = valXStr.toInt();
        // }
        // if (characteristicUUID.equals(READ_Y_CHARACTERISTIC_UUID)) {
        //     // update y
        //     std::string readYValue = pCharacteristic->getValue();
        //     String valYStr = readYValue.c_str();
        //     yClient = valYStr.toInt();
        // }
    }
    
    // callback function to support a write request
    void onWrite(BLECharacteristic* pCharacteristic) {
        String characteristicUUID = pCharacteristic->getUUID().toString().c_str();
        String characteristcValue = pCharacteristic->getValue().c_str();
        Serial.printf("Client JUST wrote to %s: %s", characteristicUUID, characteristcValue.c_str());

        // check if characteristicUUID matches a known UUID
        if (characteristicUUID.equals(READ_WRITE_X_CHARACTERISTIC_UUID)) {
            // extract the x value 
            std::string readXValue = pCharacteristic->getValue();
            String valXStr = readXValue.c_str();
            xClient = valXStr.toInt();
        }
        if (characteristicUUID.equals(READ_WRITE_Y_CHARACTERISTIC_UUID)) {
            // extrac the y value
            std::string readYValue = pCharacteristic->getValue();
            String valYStr = readYValue.c_str();
            yClient = valYStr.toInt();
        }
    }

    // callback function to support a Notify request
    void onNotify(BLECharacteristic* pCharacteristic) {
        String characteristicUUID = pCharacteristic->getUUID().toString().c_str();
        Serial.printf("Client JUST notified about change to %s: %s", characteristicUUID, pCharacteristic->getValue().c_str());
    }

    // callback function to support when a client subscribes to notifications/indications
    void onSubscribe(BLECharacteristic* pCharacteristic, uint16_t subValu) {}

    // calllback function to support a Notify/Indicate Status report
    void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code) {
        // print appropriate response
        String characteristicUUID = pCharacteristic->getUUID().toString().c_str();
        switch(s) {
            case SUCCESS_INDICATE:
                break;
            case SUCCESS_NOTIFY:
                Serial.printf("Status for %s: Successful Notification", characteristicUUID.c_str());
                break;
            case ERROR_INDICATE_DISABLED:
                Serial.printf("Status for %s: Failure; Indication Disabled on Client", characteristicUUID.c_str());
                break;
            case ERROR_NOTIFY_DISABLED:
                Serial.printf("Status for %s: Failure; Notification Disabled on Client", characteristicUUID.c_str());
                break;
            case ERROR_GATT:
                Serial.printf("Status for %s: Failure; GATT Issue", characteristicUUID.c_str());
                break;
            case ERROR_NO_CLIENT:
                Serial.printf("Status for %s: Failure; No BLE Client", characteristicUUID.c_str());
                break;
            case ERROR_INDICATE_TIMEOUT:
                Serial.printf("Status for %s: Failure; Indication Timeout", characteristicUUID.c_str());
                break;
            case ERROR_INDICATE_FAILURE:
                Serial.printf("Status for %s: Failure; Indication Failure", characteristicUUID.c_str());
                break;
        }
    }    

};

///////////////////////////////////////////////////////////////
// Forward Declarations
///////////////////////////////////////////////////////////////
void broadcastBleServer();
void drawScreenTextWithBackground(String text, int backgroundColor);

// Gameplay
void drawDots(uint32_t serverX, uint32_t serverY, uint32_t clientX, uint32_t clientY);
void serverAccelIncrement();
String milis_to_seconds(long milis);
void playGame();
void endGame();
bool checkDistance();
void warpDot();

///////////////////////////////////////////////////////////////
// Put your setup code here, to run once
///////////////////////////////////////////////////////////////
void setup()
{
    // Init device
    M5.begin();
    M5.Lcd.setTextSize(3);

    // Initialize M5Core2 as a BLE server
    Serial.print("Starting BLE...");
    String bleDeviceName = "Duct Tape n' Prayer";
    BLEDevice::init(bleDeviceName.c_str());

    // Broadcast the BLE server
    drawScreenTextWithBackground("Initializing BLE...", TFT_CYAN);
    broadcastBleServer();
    drawScreenTextWithBackground("Broadcasting as BLE server named:\n\n" + bleDeviceName, TFT_BLUE);

    // Gameplay setup
    if(!gamePad.begin(0x50)){
        Serial.println("ERROR! seesaw not found");
        while(1) delay(1);
    }
    gamePad.pinModeBulk(button_mask, INPUT_PULLUP);
    gamePad.setGPIOInterrupts(button_mask, 1);
}

///////////////////////////////////////////////////////////////
// Put your main code here, to run repeatedly
///////////////////////////////////////////////////////////////
void loop()
{
    M5.update();
    if (deviceConnected) {
        bool stillPlaying = checkDistance();
        if (screen == S_GAME && stillPlaying) {
          playGame();
        } else {
          if (timer == 0) {
          timer = millis();
          }
          endGame();
          delay(50000);
        }
    } else if (previouslyConnected) {
      drawScreenTextWithBackground("Disconnected. Reset M5 device to reinitialize BLE.", TFT_RED); // Give feedback on screen
      timer = 0;
    }
    delay(500);
}

///////////////////////////////////////////////////////////////
// Colors the background and then writes the text on top
///////////////////////////////////////////////////////////////
void drawScreenTextWithBackground(String text, int backgroundColor) {
    M5.Lcd.fillScreen(backgroundColor);
    M5.Lcd.setCursor(0,0);
    M5.Lcd.println(text);
}

///////////////////////////////////////////////////////////////
// This code creates the BLE server and broadcasts it
///////////////////////////////////////////////////////////////
void broadcastBleServer() {    
    // Initializing the server, a service and a characteristic 
    Serial.println("Broadcasting!!!");
    bleServer = BLEDevice::createServer();
    Serial.println("Created Server");
    bleServer->setCallbacks(new MyServerCallbacks());
    Serial.println("Set Callbacks");
    bleService = bleServer->createService(SERVICE_UUID);
    Serial.println("Created Service");
    
    bleReadXCharacteristic = bleService->createCharacteristic(READ_X_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );
    bleReadXCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    Serial.println("Created Characteristic");

    bleReadXCharacteristic->setValue(xServer);
    Serial.println("set value");

    bleReadYCharacteristic = bleService->createCharacteristic(READ_Y_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );
    bleReadYCharacteristic->setValue(yServer);
    bleReadYCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

    bleReadWriteXCharacteristic = bleService->createCharacteristic(READ_WRITE_X_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    bleReadWriteXCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

    bleReadWriteYCharacteristic = bleService->createCharacteristic(READ_WRITE_Y_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    bleReadWriteYCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    bleService->start();

    // Start broadcasting (advertising) BLE service
    BLEAdvertising *bleAdvertising = BLEDevice::getAdvertising();
    bleAdvertising->addServiceUUID(SERVICE_UUID);
    bleAdvertising->setScanResponse(true);
    bleAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined...you can connect with your phone!"); 
}

bool checkDistance() {
  long distance = abs(sqrt(pow((xServer - xClient), 2) + pow((yServer - yClient), 2)));
  if (distance <= 30) {
    return false;
  }
  return true;
}

void serverAccelIncrement() {
  if (acceleration == 5) {
    acceleration = 1;
  } else {
    acceleration++;
  }
}

String milis_to_seconds(long milis) {
    unsigned long seconds = milis / 1000;
    String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);
    unsigned long miliseconds = milis % 60;
    String milisecondsStr = miliseconds < 10 ? "0" + String(miliseconds) : String(miliseconds);
    return secondStr + "." + milisecondsStr + "s";
}

void endGame() {
  M5.Lcd.fillScreen(TFT_MAGENTA);
  M5.Lcd.setTextColor(TFT_BLACK);
  M5.Lcd.setTextSize(3);
  M5.Lcd.drawString("GAME OVER", M5.Lcd.width() / 4, M5.Lcd.height() / 2 - 30);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawString("YOU LASTED FOR", M5.Lcd.width() / 4, M5.Lcd.height() / 2);
  M5.Lcd.drawString(milis_to_seconds(timer), M5.Lcd.width() / 4, M5.Lcd.height() - 100);
}

void playGame() {
  M5.Lcd.fillScreen(TFT_BLACK);
  // Reverse x/y values to match joystick orientation
  int x = 1023 - gamePad.analogRead(14);
  int y = 1023 - gamePad.analogRead(15);

  // Left & Right For Joystick
  if (x > 600) {
    for (int i = 0; i < acceleration; i++) {
      if ((xServer + 1) < 320) {
        xServer++;
        String x = String(xServer);
        bleReadXCharacteristic->setValue(x.c_str());
        bleReadXCharacteristic->notify();
      }
    }
  } else if (x < 500) {
    for (int i = 0; i < acceleration; i++) {
      if ((xServer - 1) > 0) {
        xServer--;
        String x = String(xServer);
        bleReadXCharacteristic->setValue(x.c_str());
        bleReadXCharacteristic->notify();
      }
    }
  }

  if (y < 480) {
    for (int i = 0; i < acceleration; i++) {
      if ((yServer + 1) < 240) {
        yServer++;
        String y = String(yServer);
        bleReadYCharacteristic->setValue(y.c_str());
        bleReadYCharacteristic->notify();
      }
    }
  } else if (y > 560) {
    for (int i = 0; i < acceleration; i++) {
      if ((yServer - 1) > 0) {
        yServer--;
        String y = String(yServer);
        bleReadYCharacteristic->setValue(y.c_str());
        bleReadYCharacteristic->notify();
      }
    }
  }

  // For the gamepad buttons
  uint32_t buttons = gamePad.digitalReadBulk(button_mask);
  if (! (buttons & (1UL << BUTTON_SELECT))) {
    serverAccelIncrement();
    Serial.print("Button Accel: "); Serial.print(acceleration);
    delay(500);
  }
  if (! (buttons & (1UL << BUTTON_START))) {
    warpDot();
    delay(500);
  }

  drawDots(xServer, yServer, xClient, yClient); 
}

void warpDot() {
  // create random ints for x and y and have the dot drawn there next
  int randx = rand() % M5.Lcd.width();
  int randy = rand() % M5.Lcd.height();
  xServer = randx;
  yServer = randy;
  
  bleReadXCharacteristic->setValue(xServer);
  bleReadYCharacteristic->setValue(yServer);

  bleReadXCharacteristic->notify();
  bleReadYCharacteristic->notify();
}

void drawDots(uint32_t serverX, uint32_t serverY, uint32_t clientX, uint32_t clientY){
  M5.Lcd.drawPixel(serverX, serverY, TFT_RED);
  M5.Lcd.drawPixel(clientX, clientY, TFT_BLUE);
}