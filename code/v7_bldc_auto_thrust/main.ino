// ============================================================
// ESP32 ROBOT — v7 (V4.1)
// Wall-climbing robot with auto BLDC thrust control.
// Simplified back down to manual-only drive (F/B/L/R/S) so the
// BLDC/pitch integration could be isolated and tested cleanly.
// BLDC speed is automatic, based on pitch, whenever the robot
// is not stopped:
//   pitch >= 60 deg -> medium thrust
//   pitch >= 30 deg -> low thrust
//   otherwise        -> off
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
#define IN2 14
#define IN3 27  // Front Left
#define IN4 26
#define IN5 25  // Rear Left
#define IN6 33
#define IN7 32  // Rear Right
#define IN8 21

// =========================
// ESC (BLDC)
// =========================
#define ESC1_PIN 17
#define ESC2_PIN 18

#define ESC_OFF    1000
#define ESC_LOW    1200
#define ESC_MEDIUM 1500

Servo esc1;
Servo esc2;

// =========================
// STATE
// =========================
char lastMove = 'S';
bool stopped  = true;

// =========================
// SETUP
// =========================
void setup() {

  // Force all motor pins LOW immediately.
  // Prevents floating pin state from driving motors on boot.
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
  esc1.attach(ESC1_PIN, ESC_OFF, ESC_MEDIUM);
  esc2.attach(ESC2_PIN, ESC_OFF, ESC_MEDIUM);
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

  mpu6050.update();
  float pitch = mpu6050.getAngleX();

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.print("Pitch: ");
    Serial.print(pitch);
    Serial.print(" | BLDC: ");
    Serial.println(getBLDCSpeed(pitch));
    lastPrint = millis();
  }

  if (SerialBT.available()) {
    char command = SerialBT.read();
    Serial.print("CMD: ");
    Serial.println(command);

    if (command == 'S') {
      stopped  = true;
      lastMove = 'S';
      stopAll();
      esc1.writeMicroseconds(ESC_OFF);
      esc2.writeMicroseconds(ESC_OFF);
      Serial.println("STOPPED - BLDC OFF");
    }
    else if (command == 'F' || command == 'B' ||
             command == 'L' || command == 'R') {
      stopped  = false;
      lastMove = command;
    }
  }

  if (!stopped) {
    autoBldc(pitch);
  }

  manualDrive(lastMove);

  delay(20);
}

// =========================
// AUTO BLDC THRUST
// =========================
void autoBldc(float pitch) {
  int speed = getBLDCSpeed(pitch);
  esc1.writeMicroseconds(speed);
  esc2.writeMicroseconds(speed);
}

int getBLDCSpeed(float pitch) {
  pitch = abs(pitch);
  if (pitch >= 60) return ESC_MEDIUM;
  if (pitch >= 30) return ESC_LOW;
  return ESC_OFF;
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
