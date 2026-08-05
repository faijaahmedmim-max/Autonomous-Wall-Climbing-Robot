// ============================================================
// ESP32 ROBOT — v8 (V5.0) — FINAL DEMO VERSION
// Wall-climbing robot with auto BLDC thrust control.
// Manual drive only: F/B/L/R/S.
// BLDC control simplified to a single threshold for reliability
// under demo conditions:
//   pitch >= 20 deg -> BLDC ON  (full thrust, 2000us)
//   pitch <  20 deg -> BLDC OFF (1000us)
// MPU polled non-blocking every 50ms so driving stays responsive.
// This is the version used for the final wall-climb demonstration.
// ============================================================

#include <ESP32Servo.h>
#include "BluetoothSerial.h"
#include <Wire.h>
#include <MPU6050_tockn.h>

BluetoothSerial SerialBT;
MPU6050 mpu6050(Wire);

// =========================
// MOTORS
// =========================
#define IN1 16  // Front Right
#define IN2 23
#define IN3 27  // Front Left
#define IN4 13
#define IN5 25  // Rear Left
#define IN6 33
#define IN7 32  // Rear Right
#define IN8 21

// =========================
// ESC (BLDC)
// =========================
#define ESC1_PIN 18
#define ESC2_PIN 17

#define ESC_OFF 1000
#define ESC_ON  2000

Servo esc1;
Servo esc2;

// =========================
// STATE
// =========================
char lastMove  = 'S';
bool masterOff = true;
bool bldcOn    = false;

// =========================
// SETUP
// =========================
void setup() {

  // Force all motor pins LOW on boot
  int motorPins[] = {IN1, IN2, IN3, IN4, IN5, IN6, IN7, IN8};
  for (int i = 0; i < 8; i++) {
    pinMode(motorPins[i], OUTPUT);
    digitalWrite(motorPins[i], LOW);
  }

  Serial.begin(115200);
  SerialBT.begin("ESP32_ROBOT");

  // --- MPU6050 ---
  Wire.begin(22, 4);
  mpu6050.begin();
  Serial.println("Keep robot flat and still for calibration...");
  mpu6050.calcGyroOffsets(true);
  Serial.println("MPU6050 Ready");

  // --- ESC Arming ---
  esc1.attach(ESC1_PIN, 1000, 2000);
  esc2.attach(ESC2_PIN, 1000, 2000);
  Serial.println("Arming ESCs...");
  esc1.writeMicroseconds(ESC_OFF);
  esc2.writeMicroseconds(ESC_OFF);
  delay(3000);
  Serial.println("ESCs Armed");

  Serial.println("Robot Ready");
}

// =========================
// LOOP
// =========================
void loop() {

  // --- MPU update, non-blocking, every 50ms ---
  static float pitch = 0;
  static unsigned long lastMPU = 0;
  if (millis() - lastMPU > 50) {
    mpu6050.update();
    pitch = mpu6050.getAngleX();
    lastMPU = millis();
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.print("Pitch: ");
    Serial.print(pitch);
    Serial.print(" | BLDC: ");
    Serial.print(bldcOn ? ESC_ON : ESC_OFF);
    Serial.print(" | bldcOn: ");
    Serial.println(bldcOn ? "YES" : "NO");
    lastPrint = millis();
  }

  while (SerialBT.available()) {

    char command = SerialBT.read();
    if (command == '\n' || command == '\r') continue;

    Serial.print("CMD: ");
    Serial.println(command);

    if (command == 'S') {
      masterOff = true;
      lastMove  = 'S';
      bldcOn    = false;
      stopAll();
      esc1.writeMicroseconds(ESC_OFF);
      esc2.writeMicroseconds(ESC_OFF);
      Serial.println("STOPPED - BLDC OFF");
    }
    else if (command == 'F' || command == 'B' ||
             command == 'L' || command == 'R') {
      masterOff = false;
      lastMove  = command;
    }
  }

  // --- Drive (always) ---
  manualDrive(lastMove);

  // --- Auto BLDC (only when not stopped) ---
  if (!masterOff) {
    autoBldc(pitch);
  }

  delay(20);
}

// =========================
// AUTO BLDC THRUST
// Simple: ON at 20 deg+, OFF below 20 deg
// =========================
void autoBldc(float pitch) {
  pitch = fabs(pitch);

  bldcOn = (pitch >= 20);

  int speed = bldcOn ? ESC_ON : ESC_OFF;
  esc1.writeMicroseconds(speed);
  esc2.writeMicroseconds(speed);
}

// =========================
// MANUAL DRIVE
// =========================
void manualDrive(char move) {

  if (move == 'F') {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);
    digitalWrite(IN7, HIGH); digitalWrite(IN8, LOW);
  }
  else if (move == 'B') {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
    digitalWrite(IN5, LOW); digitalWrite(IN6, HIGH);
    digitalWrite(IN7, LOW); digitalWrite(IN8, HIGH);
  }
  else if (move == 'L') pivotLeft();
  else if (move == 'R') pivotRight();
  else if (move == 'S') stopAll();
}

// =========================
// MOTOR FUNCTIONS
// =========================
void pivotLeft() {
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  digitalWrite(IN5, LOW);  digitalWrite(IN6, HIGH);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN7, HIGH); digitalWrite(IN8, LOW);
}

void pivotRight() {
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN7, LOW);  digitalWrite(IN8, HIGH);
}

void stopAll() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  digitalWrite(IN5, LOW); digitalWrite(IN6, LOW);
  digitalWrite(IN7, LOW); digitalWrite(IN8, LOW);
}
