#include <BleMouse.h>
#include <MPU6500_WE.h>
#include <Wire.h>

#define MPU6500_ADDR      0x68    
#define LEFT_CLICK_PIN    18      
#define RIGHT_CLICK_PIN   19      
#define MOVE_ENABLE_PIN   14     // <--- WIRED TO UP BUTTON: Hold to move
#define SCROLL_DOWN_PIN   27     // <--- WIRED TO DOWN BUTTON: Click to scroll

// --- SETTINGS ---
bool INVERT_X = false; 
bool INVERT_Y = true;    

const float SENSITIVITY_X = 0.5f; 
const float SENSITIVITY_Y = 0.5f; 
const float DEADZONE      = 8.0f; 

BleMouse bleMouse("ESP32 Air Mouse");
MPU6500_WE sensor = MPU6500_WE(MPU6500_ADDR);

float zeroX = 0, zeroY = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
   
  pinMode(LEFT_CLICK_PIN,  INPUT_PULLUP);
  pinMode(RIGHT_CLICK_PIN, INPUT_PULLUP);
  pinMode(MOVE_ENABLE_PIN, INPUT_PULLUP); 
  pinMode(SCROLL_DOWN_PIN, INPUT_PULLUP);

  if (!sensor.init()) {
    while(1); 
  }
   
  // CALIBRATION (Keep flat for 2s)
  delay(2000); 
  float sumX = 0, sumY = 0;
  for(int i=0; i<100; i++) { 
    xyzFloat ang = sensor.getAngles();
    sumX += ang.x;
    sumY += ang.y;
    delay(5);
  }
  zeroX = sumX / 100.0;
  zeroY = sumY / 100.0;
  
  bleMouse.begin();
}

void loop() {
  static unsigned long lastScrollTime = 0;

  if (bleMouse.isConnected()) {
    
    // 1. CLUTCH LOGIC (Pin 14)
    // Only move if the Up Button is pressed (LOW)
    if (digitalRead(MOVE_ENABLE_PIN) == LOW) {
      
      xyzFloat angles = sensor.getAngles();
      float rawTiltX = angles.y - zeroY; 
      float rawTiltY = angles.x - zeroX;

      if (abs(rawTiltX) < DEADZONE) rawTiltX = 0.0;
      if (abs(rawTiltY) < DEADZONE) rawTiltY = 0.0;

      int moveX = (int)(rawTiltX * SENSITIVITY_X);
      int moveY = (int)(rawTiltY * SENSITIVITY_Y);

      if (INVERT_X) moveX = -moveX;
      if (INVERT_Y) moveY = -moveY;

      if (moveX != 0 || moveY != 0) {
        bleMouse.move(moveX, moveY);
      }
    }

    // 2. CLICKS (Always Active)
    if (digitalRead(LEFT_CLICK_PIN) == LOW) {
      if (!bleMouse.isPressed(MOUSE_LEFT)) bleMouse.press(MOUSE_LEFT);
    } else {
      if (bleMouse.isPressed(MOUSE_LEFT)) bleMouse.release(MOUSE_LEFT);
    }
    
    if (digitalRead(RIGHT_CLICK_PIN) == LOW) {
      if (!bleMouse.isPressed(MOUSE_RIGHT)) bleMouse.press(MOUSE_RIGHT);
    } else {
      if (bleMouse.isPressed(MOUSE_RIGHT)) bleMouse.release(MOUSE_RIGHT);
    }

    // 3. SCROLL DOWN (Pin 27)
    if (millis() - lastScrollTime > 150) {
      if (digitalRead(SCROLL_DOWN_PIN) == LOW) {
        bleMouse.move(0, 0, -1);
        lastScrollTime = millis();
      }
    }
  }
  delay(20); 
}