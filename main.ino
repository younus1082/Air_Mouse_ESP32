#include <BleMouse.h>
#include <MPU6500_WE.h>
#include <Wire.h>

#define MPU6500_ADDR      0x68    
#define LEFT_CLICK_PIN    18      
#define RIGHT_CLICK_PIN   19      
#define MOVE_ENABLE_PIN   14     // Clutch Button
#define SCROLL_DOWN_PIN   27     // Scroll Button

// --- MOUSE SETTINGS ---
bool INVERT_X = false; 
bool INVERT_Y = true;    

const float SENSITIVITY_X = 0.5f; 
const float SENSITIVITY_Y = 0.5f; 
const float DEADZONE      = 8.0f; 

// --- GESTURE SETTINGS ---
// Threshold: How fast (deg/s) you must flick to trigger.
const float GESTURE_THRESHOLD = 300.0f; 
const int GESTURE_COOLDOWN_MS = 800; 

BleMouse bleMouse("ESP32 Air Mouse");
MPU6500_WE sensor = MPU6500_WE(MPU6500_ADDR);

float zeroX = 0, zeroY = 0;
unsigned long lastGestureTime = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
   
  pinMode(LEFT_CLICK_PIN,  INPUT_PULLUP);
  pinMode(RIGHT_CLICK_PIN, INPUT_PULLUP);
  pinMode(MOVE_ENABLE_PIN, INPUT_PULLUP); 
  pinMode(SCROLL_DOWN_PIN, INPUT_PULLUP);

  if (!sensor.init()) {
    Serial.println("MPU Error"); while(1); 
  }
   
  Serial.println("Calibrating... KEEP FLAT!");
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
  
  // High range needed for fast flicks
  sensor.setGyrRange(MPU6500_GYRO_RANGE_500);

  Serial.println("Ready.");
  bleMouse.begin();
}

// --- BURST SCROLL FUNCTIONS ---
void scrollToBottom() {
  Serial.println(">>> FLICK DETECTED: Going Down");
  for(int i=0; i<10; i++) { // Increased bursts for longer scroll
     bleMouse.move(0, 0, -100); 
     delay(15);
  }
}

void scrollToTop() {
  Serial.println(">>> FLICK DETECTED: Going Up");
  for(int i=0; i<10; i++) {
     bleMouse.move(0, 0, 100);
     delay(15);
  }
}

void loop() {
  static unsigned long lastScrollTime = 0;

  if (bleMouse.isConnected()) {
    
    // --- 1. GESTURE DETECTION (Changed to Y-Axis) ---
    if (millis() - lastGestureTime > GESTURE_COOLDOWN_MS) {
      
      xyzFloat gyrVal = sensor.getGyrValues();
      
      // We are now checking gyrVal.y instead of x
      // NOTE: If Flick Down goes Up, swap the ">" and "<" logic below.
      
      // Check for FAST DOWN TILT 
      if (gyrVal.y > GESTURE_THRESHOLD) {
        scrollToBottom();
        lastGestureTime = millis(); 
      } 
      // Check for FAST UP TILT
      else if (gyrVal.y < -GESTURE_THRESHOLD) {
        scrollToTop();
        lastGestureTime = millis(); 
      }
    }

    // --- 2. STANDARD CLUTCH MOVEMENT ---
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

    // --- 3. BUTTONS ---
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

    if (millis() - lastScrollTime > 150) {
      if (digitalRead(SCROLL_DOWN_PIN) == LOW) {
        bleMouse.move(0, 0, -1);
        lastScrollTime = millis();
      }
    }
  }
  delay(20); 
}
