# 🖱️ ESP32 Air Mouse (Gesture & Joystick Control)

This project turns an ESP32 and an MPU6050 sensor into a high-performance Bluetooth Air Mouse. It moves beyond basic air mice by adding **Joystick-Style Velocity Control**, a **"Clutch" mechanism** for precision, and **Motion Gestures** for rapid scrolling.

## ✨ Key Features

  * **🕹️ Joystick Control:** Tilt the device to move the cursor. The more you tilt, the faster it accelerates (no constant wrist twisting).
  * **🛑 Deadman Clutch:** The cursor remains frozen until you hold the "Move" button. Release it to stop instantly—perfect for repositioning your hand without losing the cursor.
  * **⚡ Motion Gestures:**
      * **Flick Down:** Instantly scrolls to the bottom of the page.
      * **Flick Up:** Instantly scrolls to the top of the page.
  * **📱 Bluetooth BLE:** Works wirelessly with Windows, Mac, Android, and iOS (no dongle required).

## 🛠️ Hardware Requirements

  * **ESP32 Development Board** (e.g., DOIT ESP32 DevKit V1)
  * **MPU6050** (or MPU6500) Gyroscope/Accelerometer
  * **4x Push Buttons**
  * **Wires & Breadboard/PCB**

## 🔌 Wiring Diagram

| ESP32 Pin | Component | Connection | Function |
| :--- | :--- | :--- | :--- |
| **3V3** | MPU6050 VCC | Power | Sensor Power |
| **GND** | MPU6050 GND | Ground | Sensor Ground |
| **22** | MPU6050 SCL | I2C | Clock Line |
| **21** | MPU6050 SDA | I2C | Data Line |
| **18** | Button 1 | to GND | **Left Click** |
| **19** | Button 2 | to GND | **Right Click** |
| **14** | Button 3 | to GND | **CLUTCH (Hold to Move)** |
| **27** | Button 4 | to GND | **Scroll Down (Click)** |

> **⚠️ Note:** All buttons uses internal pull-up resistors. Connect one leg of the button to the ESP32 Pin and the other leg directly to **GND**.

## 💻 Installation Guide

### 1\. Setup Arduino IDE

1.  Install **Arduino IDE**.
2.  Add ESP32 support in **File \> Preferences** \> "Additional Boards Manager URLs":
    `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3.  Go to **Tools \> Board \> Boards Manager**, search **"esp32"**, and install it.

### 2\. Install Libraries

Go to **Sketch \> Include Library \> Manage Libraries...** and install:

1.  **MPU6500\_WE** (by Wolfgang Ewald)
2.  **ESP32 BLE Mouse** (by T-vK)

### 3\. Upload Code

Select your board (**DOIT ESP32 DEVKIT V1**) and upload the code below.

## 📄 The Final Code

```cpp
#include <BleMouse.h>
#include <MPU6500_WE.h>
#include <Wire.h>

#define MPU6500_ADDR      0x68    
#define LEFT_CLICK_PIN    18      
#define RIGHT_CLICK_PIN   19      
#define MOVE_ENABLE_PIN   14     // CLUTCH: Hold this to move cursor
#define SCROLL_DOWN_PIN   27     // Scroll Down Button

// --- MOUSE SETTINGS ---
bool INVERT_X = false; 
bool INVERT_Y = true;    // Set to true if Up/Down is reversed

const float SENSITIVITY_X = 0.5f; 
const float SENSITIVITY_Y = 0.5f; 
const float DEADZONE      = 8.0f; 

// --- GESTURE SETTINGS ---
const float GESTURE_THRESHOLD = 300.0f; // Speed needed to trigger flick (deg/s)
const int GESTURE_COOLDOWN_MS = 800;    // Time between gestures

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
    Serial.println("❌ MPU6500 ERROR"); while(1); 
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
  
  // Set high range for fast gesture detection
  sensor.setGyrRange(MPU6500_GYRO_RANGE_500);

  Serial.println("✅ Ready.");
  bleMouse.begin();
}

// --- GESTURE ACTIONS ---
void scrollToBottom() {
  for(int i=0; i<10; i++) { 
     bleMouse.move(0, 0, -100); 
     delay(15);
  }
}

void scrollToTop() {
  for(int i=0; i<10; i++) {
     bleMouse.move(0, 0, 100);
     delay(15);
  }
}

void loop() {
  static unsigned long lastScrollTime = 0;

  if (bleMouse.isConnected()) {
    
    // 1. GESTURE DETECTION (Always Active)
    if (millis() - lastGestureTime > GESTURE_COOLDOWN_MS) {
      xyzFloat gyrVal = sensor.getGyrValues();
      
      // Check for FAST FLICKS (Up/Down Speed)
      if (gyrVal.y > GESTURE_THRESHOLD) {
        scrollToBottom();
        lastGestureTime = millis(); 
      } 
      else if (gyrVal.y < -GESTURE_THRESHOLD) {
        scrollToTop();
        lastGestureTime = millis(); 
      }
    }

    // 2. CLUTCH & MOVEMENT (Only if Button 14 is Held)
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

    // 3. BUTTONS
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
```

## 🎮 How to Operate

1.  **Power On:** Plug in the device.
2.  **Calibrate:** **IMMEDIATELY lay it flat on a table.** Do not touch it for 3 seconds while it calibrates the "Zero Point."
3.  **Connect:** Pair with Bluetooth device **"ESP32 Air Mouse"**.
4.  **Move Cursor:** Press and **HOLD Button 14** (Up Button), then tilt the device. Release the button to stop.
5.  **Scroll:** Tap Button 27 (Down Button) to scroll normally.
6.  **Gestures:** Without holding the clutch, give the device a **sharp flick Down** to jump to the bottom of a page, or **flick Up** to jump to the top.
