// ============================================================
// ESP32 ROBOT — v6 (08-06-2026)
// Added: Auto V2 mode (command: 'V') — obstacle-triggered
// servo lift + tilt-checked climb-assist sequence, alongside
// the existing Auto V1 (basic obstacle avoidance) and Manual
// modes with MPU6050 tilt-based motor cutoff.
// ============================================================

#include <ESP32Servo.h>
#include "BluetoothSerial.h"
#include <Wire.h>
#include <MPU6050_tockn.h>

BluetoothSerial SerialBT;
MPU6050 mpu6050(Wire);

// =========================
// ULTRASONIC
// =========================
#define TRIG 18
#define ECHO 19

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
// SERVOS
// =========================
#define SERVO1_PIN 23
#define SERVO2_PIN 12

// =========================
// TURN TIMING (ms)
// =========================
#define TURN_90_MS  500
#define TURN_135_MS 750
#define TURN_180_MS 1000

Servo liftServo1;
Servo liftServo2;

// =========================
// STATE
// =========================
char command         = 'S';
char lastMove        = 'S';
bool autoMode        = false;
bool autoV2Mode      = false;
bool obstacleHandled = false;

// =========================
// SETUP
// =========================
void setup() {

  Serial.begin(115200);
  SerialBT.begin("ESP32_ROBOT");

  Wire.begin(22, 4);
  mpu6050.begin();
  Serial.println("Keep robot flat and still for calibration...");
  mpu6050.calcGyroOffsets(true);
  Serial.println("MPU6050 Ready");

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(IN5, OUTPUT); pinMode(IN6, OUTPUT);
  pinMode(IN7, OUTPUT); pinMode(IN8, OUTPUT);

  stopAll();

  liftServo1.attach(SERVO1_PIN);
  liftServo2.attach(SERVO2_PIN);
  liftServo1.write(0);
  liftServo2.write(0);

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
    Serial.println(pitch);
    lastPrint = millis();
  }

  if (SerialBT.available()) {

    command = SerialBT.read();
    Serial.print("CMD: ");
    Serial.println(command);

    if (command == 'A') {
      autoMode        = true;
      autoV2Mode      = false;
      obstacleHandled = false;
      stopAll();
    }
    else if (command == 'V') {
      autoV2Mode = true;
      autoMode   = false;
      stopAll();
    }
    else if (command == 'M') {
      autoMode   = false;
      autoV2Mode = false;
      stopAll();
      lastMove = 'S';
    }
    else if (command == 'U' && !autoMode && !autoV2Mode) {
      dualLiftCycle();
    }
    else if (command == 'S') {
      lastMove = 'S';
    }
    else if (command == 'F' || command == 'B' ||
             command == 'L' || command == 'R') {
      lastMove = command;
    }
  }

  long distance = getDistance();

  if (distance == -1 || distance > 25) {
    obstacleHandled = false;
  }

  if (autoV2Mode) {
    autoDriveV2(distance, pitch);
  }
  else if (autoMode) {
    autoDrive(distance);
  }
  else {
    manualDrive(pitch, lastMove);
  }

  delay(20);
}

// =========================
// AUTO MODE V1 — basic obstacle avoidance
// =========================
void autoDrive(long distance) {

  if (!autoMode) {
    stopAll();
    return;
  }

  if (distance == -1 || distance > 20) {
    obstacleHandled = false;
    allForwardFull();
    return;
  }

  if (obstacleHandled) {
    stopAll();
    return;
  }

  if (distance > 5 && distance <= 20) {

    obstacleHandled = true;
    stopAll();
    delay(150);

    // STEP 1: Turn 90 LEFT
    pivotLeft();
    delay(TURN_90_MS);
    stopAll();
    delay(150);
    if (!autoMode) return;

    long d1 = getDistance();
    if (d1 == -1 || d1 > 20) {
      obstacleHandled = false;
      return;
    }

    // STEP 2: Turn 180 RIGHT
    pivotRight();
    delay(TURN_180_MS);
    stopAll();
    delay(150);
    if (!autoMode) return;

    long d2 = getDistance();
    if (d2 == -1 || d2 > 20) {
      obstacleHandled = false;
      return;
    }

    // STEP 3: Turn 90 LEFT, reverse until 50cm, turn 135 RIGHT
    pivotLeft();
    delay(TURN_90_MS);
    stopAll();
    delay(150);
    if (!autoMode) return;

    unsigned long backStart = millis();
    while (millis() - backStart < 4000) {
      if (!autoMode) { stopAll(); return; }
      long dBack = getDistance();
      if (dBack == -1 || dBack > 50) break;
      allBackwardFull();
      delay(40);
    }

    stopAll();
    delay(150);
    if (!autoMode) return;

    pivotRight();
    delay(TURN_135_MS);
    stopAll();
    delay(150);

    obstacleHandled = false;
  }
}

// =========================
// AUTO MODE V2 — climb-assist sequence
// Trigger: distance < 2cm
// 1. Stop
// 2. Servos sweep to 90 deg, hold 5s
// 3. During hold: if pitch 5-12 deg, run front motors 3s
// 4. Servos back to 0
// 5. Reverse 3s, pivot right twice, resume forward
// =========================
void autoDriveV2(long distance, float pitch) {

  if (!autoV2Mode) {
    stopAll();
    return;
  }

  if (distance != -1 && distance < 2) {

    stopAll();
    delay(200);
    if (!autoV2Mode) return;

    for (int pos = 0; pos <= 90; pos++) {
      liftServo1.write(pos);
      liftServo2.write(pos);
      delay(11);
    }
    if (!autoV2Mode) {
      liftServo1.write(0);
      liftServo2.write(0);
      return;
    }

    bool frontRan = false;
    unsigned long holdStart = millis();

    while (millis() - holdStart < 5000) {

      if (!autoV2Mode) {
        stopAll();
        liftServo1.write(0);
        liftServo2.write(0);
        return;
      }

      mpu6050.update();
      float currentPitch = mpu6050.getAngleX();

      Serial.print("Hold Pitch: ");
      Serial.println(currentPitch);

      if (!frontRan && currentPitch >= 5 && currentPitch <= 12) {

        frontRan = true;

        digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
        digitalWrite(IN5, LOW);  digitalWrite(IN6, LOW);
        digitalWrite(IN7, LOW);  digitalWrite(IN8, LOW);

        delay(3000);
        stopAll();

        if (!autoV2Mode) {
          liftServo1.write(0);
          liftServo2.write(0);
          return;
        }
      }

      delay(50);
    }

    for (int pos = 90; pos >= 0; pos--) {
      liftServo1.write(pos);
      liftServo2.write(pos);
      delay(11);
    }
    if (!autoV2Mode) return;

    allBackwardFull();
    delay(3000);
    stopAll();
    delay(150);
    if (!autoV2Mode) return;

    pivotRight();
    delay(TURN_90_MS);
    stopAll();
    delay(150);
    if (!autoV2Mode) return;

    pivotRight();
    delay(TURN_90_MS);
    stopAll();
    delay(150);
    if (!autoV2Mode) return;

    return;
  }

  allForwardFull();
}

// =========================
// MANUAL MODE — tilt-based motor cutoff
// =========================
void manualDrive(float pitch, char move) {

  bool frontOK = (pitch < 75);
  bool rearOK  = (pitch < 45 || pitch >= 75);

  if (move == 'F') {

    if (frontOK) {
      digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    } else {
      digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
    }

    if (rearOK) {
      digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);
      digitalWrite(IN7, HIGH); digitalWrite(IN8, LOW);
    } else {
      digitalWrite(IN5, LOW); digitalWrite(IN6, LOW);
      digitalWrite(IN7, LOW); digitalWrite(IN8, LOW);
    }
  }

  else if (move == 'B') {

    if (frontOK) {
      digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
    } else {
      digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
    }

    if (rearOK) {
      digitalWrite(IN5, LOW); digitalWrite(IN6, HIGH);
      digitalWrite(IN7, LOW); digitalWrite(IN8, HIGH);
    } else {
      digitalWrite(IN5, LOW); digitalWrite(IN6, LOW);
      digitalWrite(IN7, LOW); digitalWrite(IN8, LOW);
    }
  }

  else if (move == 'L') pivotLeft();
  else if (move == 'R') pivotRight();
  else if (move == 'S') stopAll();
}

// =========================
// DUAL SERVO LIFT (manual U command)
// =========================
void dualLiftCycle() {

  stopAll();

  for (int pos = 0; pos <= 90; pos++) {
    liftServo1.write(pos);
    liftServo2.write(pos);
    delay(11);
  }

  delay(3000);

  for (int pos = 90; pos >= 0; pos--) {
    liftServo1.write(pos);
    liftServo2.write(pos);
    delay(11);
  }

  command = 'S';
}

// =========================
// ULTRASONIC
// =========================
long getDistance() {

  digitalWrite(TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 50000);
  if (duration == 0) return -1;

  return duration * 0.034 / 2;
}

// =========================
// MOTOR FUNCTIONS
// =========================
void allForwardFull() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  digitalWrite(IN5, HIGH); digitalWrite(IN6, LOW);
  digitalWrite(IN7, HIGH); digitalWrite(IN8, LOW);
}

void allBackwardFull() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  digitalWrite(IN5, LOW); digitalWrite(IN6, HIGH);
  digitalWrite(IN7, LOW); digitalWrite(IN8, HIGH);
}

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
