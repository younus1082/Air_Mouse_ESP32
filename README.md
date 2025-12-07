# 🖱️ ESP32 Air Mouse (Gyro-Controlled)

This project converts an ESP32 and an MPU6050 gyroscope into a wireless Bluetooth "Air Mouse." Unlike standard air mice that require constant wrist rotation, this project uses **Joystick-Style Control** (tilt to accelerate) and features a **"Clutch" mechanism** (hold-to-move) for precise, drift-free navigation.

## ✨ Features

  * **Joystick Control:** Tilt the device to move the cursor. The more you tilt, the faster it goes.
  * **Deadman Switch (Clutch):** The cursor remains frozen until you hold the "Move" button. This prevents accidental drifting when you put the mouse down.
  * **Bluetooth Low Energy (BLE):** Connects wirelessly to Windows, Mac, Android, and iOS without a USB dongle.
  * **Smooth Gliding:** Includes acceleration logic to prevent cursor jitter.

## 🛠️ Hardware Requirements

  * **Microcontroller:** ESP32 Development Board (e.g., DOIT ESP32 DevKit V1)
  * **Sensor:** MPU6050 (or MPU6500) Gyroscope/Accelerometer Module
  * **Input:** 4x Push Buttons
  * **Wires:** Jumper wires
  * **Power:** USB Cable or 3.7V LiPo Battery

## 🔌 Wiring Diagram

| ESP32 Pin | Component | Connection Type | Description |
| :--- | :--- | :--- | :--- |
| **3V3** | MPU6050 VCC | Power | Sensor Power |
| **GND** | MPU6050 GND | Ground | Sensor Ground |
| **22** | MPU6050 SCL | I2C | Clock Line |
| **21** | MPU6050 SDA | I2C | Data Line |
| **18** | Button 1 | Switch to GND | **Left Click** |
| **19** | Button 2 | Switch to GND | **Right Click** |
| **14** | Button 3 | Switch to GND | **CLUTCH (Hold to Move)** |
| **27** | Button 4 | Switch to GND | **Scroll Down** |

> **⚠️ Important:** All buttons must connect one leg to the ESP32 Pin and the other leg to **GND** (Ground). The code uses internal pull-up resistors (`INPUT_PULLUP`).

## 💻 Software Installation

### 1\. Setup Arduino IDE

1.  Install the **Arduino IDE**.
2.  Add ESP32 support: Go to **File \> Preferences** and paste this URL into "Additional Boards Manager URLs":
    `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3.  Go to **Tools \> Board \> Boards Manager**, search for **"esp32"**, and install it.

### 2\. Install Libraries

You need to install two libraries via **Sketch \> Include Library \> Manage Libraries...**:

1.  **MPU6500\_WE** (by Wolfgang Ewald) - For the gyroscope sensor.
2.  **ESP32 BLE Mouse** (by T-vK) - For Bluetooth HID functionality.

### 3\. Upload the Code

1.  Select your board: **Tools \> Board \> DOIT ESP32 DEVKIT V1**.
2.  Copy the code below into the IDE.
3.  Click **Upload**.

## 📄 The Source Code

```cpp
#include <BleMouse.h>
#include <MPU6500_WE.h>
#include <Wire.h>

#define MPU6500_ADDR      0x68    
#define LEFT_CLICK_PIN    18      
#define RIGHT_CLICK_PIN   19      
#define MOVE_ENABLE_PIN   14     // CLUTCH: Hold this to move cursor
#define SCROLL_DOWN_PIN   27     // Scroll Down Button

// --- MOUSE TUNING ---
bool INVERT_X = false; 
bool INVERT_Y = true;    // Set to true if Up/Down is reversed

const float SENSITIVITY_X = 0.5f; 
const float SENSITIVITY_Y = 0.5f; 
const float DEADZONE      = 8.0f; // Tilt threshold (degrees)

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
    Serial.println("❌ MPU6500 ERROR! Check Wiring.");
    while(1); 
  }
   
  Serial.println("Calibrating... PLACE DEVICE FLAT!");
  delay(2000); // 2 Seconds to keep it still
   
  float sumX = 0, sumY = 0;
  for(int i=0; i<100; i++) { 
    xyzFloat ang = sensor.getAngles();
    sumX += ang.x;
    sumY += ang.y;
    delay(5);
  }
  zeroX = sumX / 100.0;
  zeroY = sumY / 100.0;
  
  Serial.println("✅ Ready! Connect to Bluetooth 'ESP32 Air Mouse'.");
  bleMouse.begin();
}

void loop() {
  static unsigned long lastScrollTime = 0;

  if (bleMouse.isConnected()) {
    
    // 1. CLUTCH & MOVEMENT LOGIC
    // Only move if Button 14 is pressed (LOW)
    if (digitalRead(MOVE_ENABLE_PIN) == LOW) {
      
      xyzFloat angles = sensor.getAngles();
      float rawTiltX = angles.y - zeroY; 
      float rawTiltY = angles.x - zeroX;

      // Deadzone Filter
      if (abs(rawTiltX) < DEADZONE) rawTiltX = 0.0;
      if (abs(rawTiltY) < DEADZONE) rawTiltY = 0.0;

      // Calculate Speed
      int moveX = (int)(rawTiltX * SENSITIVITY_X);
      int moveY = (int)(rawTiltY * SENSITIVITY_Y);

      // Invert Axis
      if (INVERT_X) moveX = -moveX;
      if (INVERT_Y) moveY = -moveY;

      // Apply Movement
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
```

## 🎮 How to Use

1.  **Power On:** Plug in your ESP32.
2.  **Calibrate:** **IMMEDIATELY** place the device flat on a table. **Do not touch it for 3 seconds.** The blue LED (if available) or Serial Monitor will confirm when ready.
3.  **Connect:** Open Bluetooth settings on your computer or phone and pair with **"ESP32 Air Mouse"**.
4.  **Operate:**
      * **Hold Button 14 (Up)** to engage the "clutch" and move the cursor by tilting.
      * **Release Button 14** to stop the cursor instantly.
      * Use **Button 18** and **19** for Left/Right clicks.
      * Tap **Button 27** to scroll down.

## ❓ Troubleshooting

  * **Cursor drifts on its own:** You likely moved the device during the startup calibration. Press the Reset (EN) button on the ESP32 and let it sit perfectly flat for 3 seconds.
  * **Buttons not working:** Ensure the button legs are connected to **GND**, not 3.3V. The code expects a logic LOW signal (GND) to activate.
  * **"MPU6500 ERROR":** Check your 4 wires (VCC, GND, SDA, SCL). Ensure SDA is on Pin 21 and SCL is on Pin 22.
