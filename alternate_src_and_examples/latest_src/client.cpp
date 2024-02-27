///////////////////////////////////////////////////////////////
// Imports
///////////////////////////////////////////////////////////////
#include <BLEDevice.h>
#include <BLE2902.h>
#include <M5Core2.h>
#include <Adafruit_seesaw.h>

///////////////////////////////////////////////////////////////
// Variables
///////////////////////////////////////////////////////////////
BLERemoteCharacteristic *bleReadXCharacteristic;
BLERemoteCharacteristic *bleReadYCharacteristic;
BLERemoteCharacteristic *bleReadWriteXCharacteristic;
BLERemoteCharacteristic *bleReadWriteYCharacteristic;
static BLEAdvertisedDevice *bleRemoteServer;
static boolean doConnect = false;
static boolean doScan = false;
bool deviceConnected = false;
int timer = 0;

// Unique IDs
static BLEUUID SERVICE_UUID("7d7a7768-a9d0-4fb8-bf2b-fc994c662eb6");
static BLEUUID READ_X_CHARACTERISTIC_UUID("563c64b2-9634-4f7a-9f4f-d9e3231faa56");
static BLEUUID READ_Y_CHARACTERISTIC_UUID("aa88ac15-3e2b-4735-92ff-4c712173e9f3");
static BLEUUID READ_WRITE_X_CHARACTERISTIC_UUID("1da468d6-993d-4387-9e71-1c826b10fff9");
static BLEUUID READ_WRITE_Y_CHARACTERISTIC_UUID("cf7b4787-d412-4e69-8b61-e2cfba89ff19");

// State
enum Screen { S_GAME, S_GAME_OVER };
static Screen screen = S_GAME;

// Gameplay Variables
Adafruit_seesaw gamePad;

#define BUTTON_SELECT    0
#define BUTTON_START    16
uint32_t button_mask = (1UL << BUTTON_START) | (1UL << BUTTON_SELECT);

// coordinates

int xServer = 0, yServer = 0, xClient = 300, yClient = 120;

// acceleration
int acceleration = 1;

///////////////////////////////////////////////////////////////
// Forward Declarations
///////////////////////////////////////////////////////////////
void drawScreenTextWithBackground(String text, int backgroundColor);

// Gameplay
void drawDots(uint32_t serverX, uint32_t serverY, uint32_t clientX, uint32_t clientY);
void clientAccelIncrement();
String milis_to_seconds(long milis);
void playGame();
void endGame();
bool checkDistance();
void warpDot();

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
    std::string readXValue = pBLERemoteCharacteristic->readValue();
    String valXStr = readXValue.c_str();
    xServer = valXStr.toInt();
    Serial.printf("\tValue was: %s", readXValue.c_str());
}

static void notifyYCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    Serial.printf("Notify callback for characteristic %s of data length %d\n", pBLERemoteCharacteristic->getUUID().toString().c_str(), length);
    Serial.printf("\tData: %s", (char *)pData);
    std::string readYValue = pBLERemoteCharacteristic->readValue();
    String valYStr = readYValue.c_str();
    yServer = valYStr.toInt();
    Serial.printf("\tValue was: %s", readYValue.c_str());
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
        //drawScreenTextWithBackground("LOST connection to device.\n\nAttempting re-connection...", TFT_RED);
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
        Serial.printf("Failed to find our service UUID: %s\n", SERVICE_UUID.toString().c_str());
        bleClient->disconnect();
        return false;
    }
    Serial.printf("\tFound our service UUID: %s\n", SERVICE_UUID.toString().c_str());

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    bleReadXCharacteristic = bleRemoteService->getCharacteristic(READ_X_CHARACTERISTIC_UUID);
    if (bleReadXCharacteristic == nullptr) {
        Serial.printf("Failed to find our characteristic UUID: %s\n", READ_X_CHARACTERISTIC_UUID.toString().c_str());
        bleClient->disconnect();
        return false;
    }
    Serial.printf("\tFound our characteristic UUID: %s\n", READ_X_CHARACTERISTIC_UUID.toString().c_str());
    bleReadYCharacteristic = bleRemoteService->getCharacteristic(READ_Y_CHARACTERISTIC_UUID);
    if (bleReadYCharacteristic == nullptr) {
        Serial.printf("Failed to find our characteristic UUID: %s\n", READ_Y_CHARACTERISTIC_UUID.toString().c_str());
        bleClient->disconnect();
        return false;
    }
    Serial.printf("\tFound our characteristic UUID: %s\n", READ_Y_CHARACTERISTIC_UUID.toString().c_str());

    
    bleReadWriteXCharacteristic = bleRemoteService->getCharacteristic(READ_WRITE_X_CHARACTERISTIC_UUID);
    if (bleReadWriteXCharacteristic == nullptr) {
        Serial.printf("Failed to find our characteristic UUID: %s\n", READ_WRITE_X_CHARACTERISTIC_UUID.toString().c_str());
        bleClient->disconnect();
        return false;
    }
    Serial.printf("\tFound our characteristic UUID: %s\n", READ_WRITE_X_CHARACTERISTIC_UUID.toString().c_str());

    bleReadWriteYCharacteristic = bleRemoteService->getCharacteristic(READ_WRITE_Y_CHARACTERISTIC_UUID);
    if (bleReadWriteYCharacteristic == nullptr) {
        Serial.printf("Failed to find our characteristic UUID: %s\n", READ_WRITE_Y_CHARACTERISTIC_UUID.toString().c_str());
        bleClient->disconnect();
        return false;
    }
    Serial.printf("\tFound our characteristic UUID: %s\n", READ_WRITE_Y_CHARACTERISTIC_UUID.toString().c_str());
    
    // Check if server's characteristic can notify client of changes and register to listen if so
    if (bleReadXCharacteristic->canNotify())
        bleReadXCharacteristic->registerForNotify(notifyXCallback);
    if (bleReadYCharacteristic->canNotify())
        bleReadYCharacteristic->registerForNotify(notifyYCallback);
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

        if (advertisedDevice.haveServiceUUID() && 
                advertisedDevice.isAdvertisingService(SERVICE_UUID) && 
                advertisedDevice.getName() == "Duct Tape n' Prayer") {
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

    // Init M5Core2 as a BLE Client
    Serial.print("Starting BLE...");
    String bleClientDeviceName = "";
    BLEDevice::init(bleClientDeviceName.c_str());

    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(0, false);
    drawScreenTextWithBackground("Scanning for BLE server...", TFT_BLUE);
    
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
    
    // If the flag "doConnect" is true then we have scanned for and found the desired
    // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
    // connected we set the connected flag to be false.
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

    // If we are connected to a peer BLE Server, update the characteristic each time we are reached
    // with the current time since boot.
    if (deviceConnected)
    {
        bool stillPlaying = checkDistance();
          if (screen == S_GAME && stillPlaying) {
            playGame();
              Serial.print("XClient: ");
  Serial.println(xClient);
  Serial.print("YClient: ");
  Serial.println(yClient);
        } else {
            if (timer == 0) {
            timer = millis();
            }
            endGame();
            delay(50000);
        }
    }
    else if (doScan) {
        drawScreenTextWithBackground("Disconnected....re-scanning for BLE server...", TFT_ORANGE);
        BLEDevice::getScan()->start(0); // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
    }
}

///////////////////////////////////////////////////////////////
// Colors the background and then writes the text on top
///////////////////////////////////////////////////////////////
void drawScreenTextWithBackground(String text, int backgroundColor) {
    M5.Lcd.fillScreen(backgroundColor);
    M5.Lcd.setCursor(0,0);
    M5.Lcd.println(text);
}

String milis_to_seconds(long milis) {
    unsigned long seconds = milis / 1000;
    String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);
    unsigned long miliseconds = milis % 60;
    String milisecondsStr = miliseconds < 10 ? "0" + String(miliseconds) : String(miliseconds);
    return secondStr + "." + milisecondsStr + "s";
}

bool checkDistance() {
  long distance = abs(sqrt(pow((xServer - xClient), 2) + pow((yServer - yClient), 2)));
  if (distance <= 30) {
    return false;
  }
  return true;
}

void clientAccelIncrement() {
  if (acceleration == 5) {
    acceleration = 1;
  } else {
    acceleration++;
  }
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

void drawDots(uint32_t serverX, uint32_t serverY, uint32_t clientX, uint32_t clientY){
  Serial.println("Drawing dots on client's device");
  M5.Lcd.drawPixel(serverX, serverY, TFT_BLUE);
  M5.Lcd.drawPixel(clientX, clientY, TFT_RED);
}

void playGame() {
  M5.Lcd.fillScreen(TFT_BLACK);
  // Reverse x/y values to match joystick orientation
  int x = 1023 - gamePad.analogRead(14);
  int y = 1023 - gamePad.analogRead(15);

  // Left & Right For Joystick
  if (x > 600) {
    for (int i = 0; i < acceleration; i++) {
      if ((xClient + 1) < 320) {
        xClient++;
        String x = String(xClient);
        bleReadWriteXCharacteristic->writeValue(x.c_str(), false);
      }
    }
  } else if (x < 500) {
    for (int i = 0; i < acceleration; i++) {
      if ((xClient - 1) > 0) {
        xClient--;
        String x = String(xClient);
        bleReadWriteXCharacteristic->writeValue(x.c_str(), false);
      }
    }
  }

  if (y < 480) {
    for (int i = 0; i < acceleration; i++) {
      if ((yClient + 1) < 240) {
        yClient++;
        String y = String(yClient);
        bleReadWriteYCharacteristic->writeValue(y.c_str(), false);
      }
    }
  } else if (y > 560) {
    for (int i = 0; i < acceleration; i++) {
      if ((yClient - 1) > 0) {
        yClient--;
        String y = String(yClient);
        bleReadWriteYCharacteristic->writeValue(y.c_str(), false);
      }
    }
  }

  // For the gamepad buttons
  uint32_t buttons = gamePad.digitalReadBulk(button_mask);
  if (! (buttons & (1UL << BUTTON_SELECT))) {
    clientAccelIncrement();
    Serial.print("Button Accel: "); Serial.print(acceleration);
    delay(500);
  }
  if (! (buttons & (1UL << BUTTON_START))) {
    warpDot();
    delay(500);
  }

  Serial.println("About to draw dots to client device");
  drawDots(xServer, yServer, xClient, yClient); 
}

void warpDot() {
  // create random ints for x and y and have the dot drawn there next
  int randx = rand() % M5.Lcd.width();
  int randy = rand() % M5.Lcd.height();
  xClient = randx;
  yClient = randy;
  
  String x = String(xClient);
  String y = String(yClient);
  bleReadWriteXCharacteristic->writeValue(x.c_str(), false); //x.length()
  bleReadWriteYCharacteristic->writeValue(y.c_str(), false); //y.length()
  drawDots(xServer, yServer, xClient, yClient);
}