#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <M5Core2.h>
#include <Adafruit_seesaw.h>

///////////////////////////////////////////////////////////////
// Forward Declarations
///////////////////////////////////////////////////////////////
void broadcastBleServer();
void drawScreenTextWithBackground(String text, int backgroundColor);
static void notifyXCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
static void notifyYCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
bool connectToServer();
void drawDots(uint32_t xJoyIn, uint32_t yJoyIn, uint32_t xButIn, uint32_t yButIn);
void joyAccelIncrement();
// void butAccelIncrement();
String milis_to_seconds(long milis);
void playGame();
void endGame();
bool checkDistance();

///////////////////////////////////////////////////////////////
// Game Variables
///////////////////////////////////////////////////////////////
#define BUTTON_SELECT    0
#define BUTTON_START    16

// State
enum Screen { S_GAME, S_GAME_OVER };
static Screen screen = S_GAME;

// Regular Variables
unsigned long lastTime = 0;
int xJoy = 300, yJoy = 120, xRemote = 0, yRemote = 0;
int joyAccel = 1;
uint32_t button_mask = (1ul << BUTTON_START) | (1UL << BUTTON_SELECT);

// Gamepad
Adafruit_seesaw gamePad;

///////////////////////////////////////////////////////////////
// Server Variables
///////////////////////////////////////////////////////////////
BLEServer *bleServer;
BLEService *bleService;
BLECharacteristic *bleCharacteristicX;
BLECharacteristic *bleCharacteristicY;
bool deviceConnected = false;
bool previouslyConnected = false;
int timer = 0;

// See the following for generating UUIDs: https://www.uuidgenerator.net/
#define SERVICE_UUID "secondM5Core"
#define CHARACTERISTIC_UUID_X "secondXCoordinate"
#define CHARACTERISTIC_UUID_Y "secondYCoordinate"

///////////////////////////////////////////////////////////////
// Client Variables
///////////////////////////////////////////////////////////////
static BLERemoteCharacteristic *bleRemoteCharacteristicX;
static BLERemoteCharacteristic *bleRemoteCharacteristicY;
static BLEAdvertisedDevice *bleRemoteServer;
static boolean doConnect = false;
static boolean doScan = false;
bool remoteDeviceConnected = false;

static BLEUUID REMOTE_SERVICE_UUID("firstM5Core");
static BLEUUID REMOTE_CHARACTERISTIC_UUID_X("firstXCoordinate");
static BLEUUID REMOTE_CHARACTERISTIC_UUID_Y("firstYCoordinate");

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

///////////////////////////////////////////////////////////////
// BLE Client Callback Methods
// This method is called when the server that this client is
// connected to NOTIFIES this client (or any client listening)
// that it has changed the remote characteristic
///////////////////////////////////////////////////////////////
static void notifyXCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    Serial.printf("Notify callback for characteristic %s of data length %d\n", pBLERemoteCharacteristic->getUUID().toString().c_str(), length);
    Serial.printf("\tData: %s", (char *)pData);
    std::string value = pBLERemoteCharacteristic->readValue();
    xRemote = int(value.c_str());
    Serial.printf("\tValue was: %s", value.c_str());
}

static void notifyYCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    Serial.printf("Notify callback for characteristic %s of data length %d\n", pBLERemoteCharacteristic->getUUID().toString().c_str(), length);
    Serial.printf("\tData: %s", (char *)pData);
    std::string value = pBLERemoteCharacteristic->readValue();
    yRemote = int(value.c_str());
    Serial.printf("\tValue was: %s", value.c_str());
}

///////////////////////////////////////////////////////////////
// BLE Server Callback Method
// These methods are called upon connection and disconnection
// to BLE service.
///////////////////////////////////////////////////////////////
class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
        deviceConnected = true;
        Serial.println("Device connected...");
    }

    void onDisconnect(BLEClient *pclient)
    {
        deviceConnected = false;
        Serial.println("Device disconnected...");
    }
};

///////////////////////////////////////////////////////////////
// Method is called to connect to server
///////////////////////////////////////////////////////////////
bool connectToServer()
{
    // Create the client
    Serial.printf("Forming a connection to %s\n", bleRemoteServer->getName().c_str());
    BLEClient *bleClient = BLEDevice::createClient();
    bleClient->setClientCallbacks(new MyClientCallback());
    Serial.println("\tClient connected");

    // Connect to the remote BLE Server.
    if (!bleClient->connect(bleRemoteServer))
        Serial.printf("FAILED to connect to server (%s)\n", bleRemoteServer->getName().c_str());
    Serial.printf("\tConnected to server (%s)\n", bleRemoteServer->getName().c_str());

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService *bleRemoteService = bleClient->getService(SERVICE_UUID);
    if (bleRemoteService == nullptr) {
        Serial.printf("Failed to find our service UUID: %s\n", REMOTE_SERVICE_UUID.toString().c_str());
        bleClient->disconnect();
        return false;
    }
    Serial.printf("\tFound our service UUID: %s\n", REMOTE_SERVICE_UUID.toString().c_str());

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    bleRemoteCharacteristicX = bleRemoteService->getCharacteristic(REMOTE_CHARACTERISTIC_UUID_X);
    if (bleRemoteCharacteristicX == nullptr) {
        Serial.printf("Failed to find our characteristic UUID: %s\n", REMOTE_CHARACTERISTIC_UUID_X.toString().c_str());
        bleClient->disconnect();
        return false;
    }
    Serial.printf("\tFound our characteristic UUID: %s\n", REMOTE_CHARACTERISTIC_UUID_X.toString().c_str());

    // Read the value of the characteristic
    if (bleRemoteCharacteristicX->canRead()) {
        std::string value = bleRemoteCharacteristicX->readValue();
        xRemote = int(value.c_str());
        Serial.printf("The characteristic value was: %s", value.c_str());
        drawScreenTextWithBackground("Initial characteristic value read from server:\n\n" + String(value.c_str()), TFT_GREEN);
        delay(3000);
    }
    
    // Check if server's characteristic can notify client of changes and register to listen if so
    if (bleRemoteCharacteristicX->canNotify())
        bleRemoteCharacteristicX->registerForNotify(notifyXCallback);

        // Obtain a reference to the characteristic in the service of the remote BLE server.
    bleRemoteCharacteristicY = bleRemoteService->getCharacteristic(REMOTE_CHARACTERISTIC_UUID_Y);
    if (bleRemoteCharacteristicY == nullptr) {
        Serial.printf("Failed to find our characteristic UUID: %s\n", REMOTE_CHARACTERISTIC_UUID_Y.toString().c_str());
        bleClient->disconnect();
        return false;
    }
    Serial.printf("\tFound our characteristic UUID: %s\n", REMOTE_CHARACTERISTIC_UUID_Y.toString().c_str());

    // Read the value of the characteristic
    if (bleRemoteCharacteristicY->canRead()) {
        std::string value = bleRemoteCharacteristicX->readValue();
        yRemote = int(value.c_str());
        Serial.printf("The characteristic value was: %s", value.c_str());
        drawScreenTextWithBackground("Initial characteristic value read from server:\n\n" + String(value.c_str()), TFT_GREEN);
        delay(3000);
    }
    
    // Check if server's characteristic can notify client of changes and register to listen if so
    if (bleRemoteCharacteristicY->canNotify())
        bleRemoteCharacteristicY->registerForNotify(notifyYCallback);

    //deviceConnected = true;
    return true;
}

///////////////////////////////////////////////////////////////
// Scan for BLE servers and find the first one that advertises
// the service we are looking for.
///////////////////////////////////////////////////////////////
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    /**
     * Called for each advertising BLE server.
     */
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        // Print device found
        Serial.print("BLE Advertised Device found:");
        Serial.printf("\tName: %s\n", advertisedDevice.getName().c_str());
        
        // We have found a device, let us now see if it contains the service we are looking for.
        if (advertisedDevice.haveServiceUUID() && 
                advertisedDevice.isAdvertisingService(REMOTE_SERVICE_UUID) && 
                advertisedDevice.getName() == "First M5Core2") {
            BLEDevice::getScan()->stop();
            bleRemoteServer = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = true;
        }

    }     
};  

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
    String bleDeviceName = "Second M5Core2";
    BLEDevice::init(bleDeviceName.c_str());

    // Broadcast the BLE server
    drawScreenTextWithBackground("Initializing BLE...", TFT_CYAN);
    broadcastBleServer();
    drawScreenTextWithBackground("Broadcasting as BLE server named:\n\n" + bleDeviceName, TFT_BLUE);

    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);

    if (doConnect == true)
    {
        if (connectToServer()) {
            Serial.println("We are now connected to the BLE Server.");
            drawScreenTextWithBackground("Connected to BLE server: " + String(bleRemoteServer->getName().c_str()), TFT_GREEN);
            doConnect = false;
            delay(3000);
        }
        else {
            Serial.println("We have failed to connect to the server; there is nothin more we will do.");
            drawScreenTextWithBackground("FAILED to connect to BLE server: " + String(bleRemoteServer->getName().c_str()), TFT_GREEN);
            delay(3000);
        }
    }

    // Sets up Gamepad QT
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
    if (deviceConnected) {

        //TODO: Add ping pong code
        
    bool stillPlaying = checkDistance();

    if (screen == S_GAME && stillPlaying) {
        playGame();
    } else {
        if (lastTime == 0) {
        lastTime = millis();
        }
        endGame();
        delay(50000);
    }

    } else if (previouslyConnected) {
        drawScreenTextWithBackground("Disconnected. Reset M5 device to reinitialize BLE.", TFT_RED); // Give feedback on screen
        timer = 0;
    }
    
    // Only update the timer (if connected) every 1 second
    delay(1000);
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
    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new MyServerCallbacks());
    bleService = bleServer->createService(SERVICE_UUID);
        //TODO
    bleCharacteristicX = bleService->createCharacteristic(CHARACTERISTIC_UUID_X,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );
    //TODO
    bleCharacteristicX->setValue(xJoy);
        //TODO
    bleCharacteristicY = bleService->createCharacteristic(CHARACTERISTIC_UUID_Y,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );
    //TODO
    bleCharacteristicX->setValue(yJoy);
    bleService->start();

    // Start broadcasting (advertising) BLE service
    BLEAdvertising *bleAdvertising = BLEDevice::getAdvertising();
    bleAdvertising->addServiceUUID(SERVICE_UUID);
    bleAdvertising->setScanResponse(true);
    bleAdvertising->setMinPreferred(0x12); // Use this value most of the time 
    // bleAdvertising->setMinPreferred(0x06); // Functions that help w/ iPhone connection issues 
    // bleAdvertising->setMinPreferred(0x00); // Set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined...you can connect with your phone!"); 

}

/////////////////////////////
// dots
////////////////////////////
bool checkDistance() {
  long distance = abs(sqrt(pow((xRemote - xJoy), 2) + pow((yRemote - yJoy), 2)));
  if (distance <= 30) {
    return false;
  }
  return true;
}

void drawDots(uint32_t xJoyIn, uint32_t yJoyIn) {
    M5.Lcd.drawPixel(xJoyIn, yJoyIn, TFT_RED);
    M5.Lcd.drawPixel(xRemote, yRemote, TFT_BLUE);
}

void joyAccelIncrement() {
  if (joyAccel == 5) {
    joyAccel = 1;
  } else {
    joyAccel++;
  }
}

void warpDot() {
  // create random ints for x and y and have the dot drawn there next
  int randx = rand() % M5.Lcd.width();
  int randy = rand() % M5.Lcd.height();
  xJoy = randx;
  yJoy = randy;
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
  M5.Lcd.drawString(milis_to_seconds(lastTime), M5.Lcd.width() / 4, M5.Lcd.height() - 100);
}

void playGame() {
  M5.Lcd.fillScreen(TFT_BLACK);
  // Reverse x/y values to match joystick orientation
  int x = 1023 - gamePad.analogRead(14);
  int y = 1023 - gamePad.analogRead(15);

  // Left & Right For Joystick
  if (x > 600) {
    for (int i = 0; i < joyAccel; i++) {
      if ((xJoy + 1) < 320) {
        xJoy++;
      }
    }
  } else if (x < 500) {
    for (int i = 0; i < joyAccel; i++) {
      if ((xJoy - 1) > 0) {
        xJoy--;
      }
    }
  }

  if (y < 480) {
    for (int i = 0; i < joyAccel; i++) {
      if ((yJoy + 1) < 240) {
        yJoy++;
      }
    }
  } else if (y > 560) {
    for (int i = 0; i < joyAccel; i++) {
      if ((yJoy - 1) > 0) {
        yJoy--;
      }
    }
  }

  // For the gamepad buttons - select and start
  uint32_t buttons = gamePad.digitalReadBulk(button_mask);

  if (! (buttons & (1UL << BUTTON_SELECT))) {
    joyAccelIncrement();
    Serial.print("Joy Accel: "); Serial.print(joyAccel);
    delay(500);
  }
  if (! (buttons & (1UL << BUTTON_START))) {
    warpDot();
    Serial.print("Dot Warped to : "); Serial.print(x + ", " + y);
    delay(500);
  }

  drawDots(xJoy, yJoy);
}