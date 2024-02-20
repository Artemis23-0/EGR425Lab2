#include <Arduino.h>
#include <M5Core2.h>
#include <Adafruit_seesaw.h>

// State
enum Screen { S_GAME, S_GAME_OVER };
static Screen screen = S_GAME;

// method header definitions
void drawDots(uint32_t xJoyIn, uint32_t yJoyIn, uint32_t xButIn, uint32_t yButIn);
void joyAccelIncrement();
void butAccelIncrement();
String milis_to_seconds(long milis);
void playGame();
void endGame();
bool checkDistance();
unsigned long lastTime = 0;

// Initialize Variables
Adafruit_seesaw gamePad;

#define BUTTON_X         6
#define BUTTON_Y         2
#define BUTTON_A         5
#define BUTTON_B         1
#define BUTTON_SELECT    0
#define BUTTON_START    16
uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) |
                       (1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);
void setup() {
    M5.begin();
    Serial.begin(115200);
    if(!gamePad.begin(0x50)){
        Serial.println("ERROR! seesaw not found");
        while(1) delay(1);
    }
  gamePad.pinModeBulk(button_mask, INPUT_PULLUP);
  gamePad.setGPIOInterrupts(button_mask, 1);
}

int last_x = 0, last_y = 0;

// joystick and button coordinates
int xJoy = 10, yJoy = 120, xBut = 300, yBut = 120;
// joystick and button acceleration
int joyAccel = 1, butAccel = 1;

void loop() {
  M5.update();
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
}

bool checkDistance() {
  long distance = abs(sqrt(pow((xBut - xJoy), 2) + pow((yBut - yJoy), 2)));
  if (distance <= 30) {
    return false;
  }
  return true;
}

void drawDots(uint32_t xJoyIn, uint32_t yJoyIn, uint32_t xButIn, uint32_t yButIn) {
  M5.Lcd.drawPixel(xJoyIn, yJoyIn, TFT_WHITE);
  M5.Lcd.drawPixel(xButIn, yButIn, TFT_WHITE);
}

void joyAccelIncrement() {
  if (joyAccel == 5) {
    joyAccel = 1;
  } else {
    joyAccel++;
  }
}

void butAccelIncrement() {
  if (butAccel == 5) {
    butAccel = 1;
  } else {
    butAccel++;
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

  // For the gamepad buttons
  uint32_t buttons = gamePad.digitalReadBulk(button_mask);

  if (! (buttons & (1UL << BUTTON_A))) {
    for (int i = 0; i < butAccel; i++) {
      if ((xBut + 1) < 320) {
        xBut++;
      }
    }
  }
  if (! (buttons & (1UL << BUTTON_B))) {
    for (int i = 0; i < butAccel; i++) {
      if ((yBut + 1) < 240) {
        yBut++;
      }
    }
  }
  if (! (buttons & (1UL << BUTTON_Y))) {
    for (int i = 0; i < butAccel; i++) {
      if ((xBut - 1) > 0) {
        xBut--;
      }
    }
  }
  if (! (buttons & (1UL << BUTTON_X))) {
    for (int i = 0; i < butAccel; i++) {
      if ((yBut - 1) > 0) {
        yBut--;
      }
    }
  }
  if (! (buttons & (1UL << BUTTON_SELECT))) {
    butAccelIncrement();
    Serial.print("Button Accel: "); Serial.print(butAccel);
    delay(500);
  }
  if (! (buttons & (1UL << BUTTON_START))) {
    joyAccelIncrement();
    Serial.print("Joy Accel: "); Serial.print(joyAccel);
    delay(500);
  }

  drawDots(xJoy, yJoy, xBut, yBut); 
}