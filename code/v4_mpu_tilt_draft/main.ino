// ============================================================
// ESP32 ROBOT — v4 (19-05-2026)
// First MPU6050 integration draft. Adds pitch reading and an
// early attempt at tilt-based front/rear motor cutoff.
// NOTE: this version has a known bug — pitch is clamped to 0
// above 90 degrees, and the frontOK/rearOK booleans are computed
// but not actually used in manualDrive(). This was fixed in v5.
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

// Front Right
#define IN1 16
#define IN2 14

// Front Left
#define IN3 27
#define IN4 26

// Rear Left
#define IN5 25
#define IN6 33

// Rear Right
#define IN7 32
#define IN8 21

// =========================
// SERVOS
// =========================
#define SERVO1_PIN 23
#define SERVO2_PIN 13

Servo liftServo1;
Servo liftServo2;

// =========================
// STATE
// =========================
char command = 'S';
bool autoMode = false;

// =========================
// SETUP
// =========================

void setup() {

  Serial.begin(115200);
  SerialBT.begin("ESP32_ROBOT");

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(IN5, OUTPUT); pinMode(IN6, OUTPUT);
  pinMode(IN7, OUTPUT); pinMode(IN8, OUTPUT);

  mpu6050.begin();
  Wire.begin(22, 4);


  Serial.println("Keep robot still...");
  mpu6050.calcGyroOffsets(true);

  Serial.println("MPU6050 READY");

  // Servo setup
  liftServo1.attach(SERVO1_PIN);
  liftServo2.attach(SERVO2_PIN);

  liftServo1.write(0);
  liftServo2.write(0);

  stopAll();

  Serial.println("Robot Ready");
}

// =========================
// LOOP
// =========================

void loop() {

  // MPU UPDATE
  mpu6050.update();

  float pitch = abs(mpu6050.getAngleX());
  if (pitch > 90) pitch = 0;

  Serial.print("Pitch: ");
  Serial.println(pitch);

  // Bluetooth
  if (SerialBT.available()) {

    command = SerialBT.read();

    Serial.println(command);

    if (command == 'A') {

      autoMode = true;
      stopAll();
    }

    else if (command == 'M') {

      autoMode = false;
      stopAll();
    }

    else if (command == 'U' && !autoMode) {

      dualLiftCycle();
    }
  }

  long distance = getDistance();

  if (autoMode) {

    autoDrive(distance);
  }

  else {

    manualDrive(pitch);
  }

  delay(20);
}

// =========================
// AUTO MODE
// =========================

void autoDrive(long distance) {

  if (distance > 0 && distance < 20) {

    stopAll();
    delay(150);

    pivotRight();
    delay(400);

    if (getDistance() > 20) return;

    pivotLeft();
    delay(600);

    if (getDistance() > 20) return;

    allBackward(0);
    delay(3000);

    pivotRight();
    delay(500);
  }

  allForward(0);
}

// =========================
// MANUAL MODE
// =========================

void manualDrive(float pitch) {

  // These two lines decide which motors are allowed to run
  bool frontOK = !(pitch >= 75 && pitch <= 80);  // false = front motors cut off
  bool rearOK  = !(pitch >= 45 && pitch <= 50);  // false = rear motors cut off

  if (command == 'F') {
    allForward(pitch);
  }

  else if (command == 'B') {
    allBackward(pitch);
  }

  else if (command == 'L') {
    pivotLeft();
  }

  else if (command == 'R') {
    pivotRight();
  }

  else if (command == 'S') {
    stopAll();
  }
}

// =========================
// DUAL SERVO LIFT
// =========================

void dualLiftCycle() {

  stopAll();

  // 0 -> 90 slowly in 1 sec
  for (int pos = 0; pos <= 90; pos++) {
    liftServo1.write(pos);
    liftServo2.write(pos);
    delay(11);
  }

  // stay lifted 3 sec
  delay(3000);

  // 90 -> 0 slowly in 1 sec
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
// MOTORS
// =========================
void allForward(float pitch){

  // =====================================
  // EXTREME TILT → FRONT OFF
  // =====================================

  if (pitch >= 75) {

    // FRONT OFF
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

    // REAR ON
    digitalWrite(IN5, HIGH);
    digitalWrite(IN6, LOW);

    digitalWrite(IN7, HIGH);
    digitalWrite(IN8, LOW);

    return;
  }

  // =====================================
  // MEDIUM TILT → REAR OFF
  // =====================================

  if (pitch >= 45) {

    // FRONT ON
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);

    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);

    // REAR OFF
    digitalWrite(IN5, LOW);
    digitalWrite(IN6, LOW);

    digitalWrite(IN7, LOW);
    digitalWrite(IN8, LOW);

    return;
  }

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  digitalWrite(IN5, HIGH);
  digitalWrite(IN6, LOW);

  digitalWrite(IN7, HIGH);
  digitalWrite(IN8, LOW);
}

void allBackward(float pitch) {

  // =====================================
  // EXTREME TILT → FRONT OFF
  // =====================================

  if (pitch >= 75) {

    // FRONT OFF
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

    // REAR ON
    digitalWrite(IN5, LOW);
    digitalWrite(IN6, HIGH);

    digitalWrite(IN7, LOW);
    digitalWrite(IN8, HIGH);

    return;
  }

  // =====================================
  // MEDIUM TILT → REAR OFF
  // =====================================

  if (pitch >= 45) {

    // FRONT ON
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);

    // REAR OFF
    digitalWrite(IN5, LOW);
    digitalWrite(IN6, LOW);

    digitalWrite(IN7, LOW);
    digitalWrite(IN8, LOW);

    return;
  }

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  digitalWrite(IN5, LOW);
  digitalWrite(IN6, HIGH);

  digitalWrite(IN7, LOW);
  digitalWrite(IN8, HIGH);
}

void pivotRight() {

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  digitalWrite(IN5, HIGH);
  digitalWrite(IN6, LOW);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN7, LOW);
  digitalWrite(IN8, HIGH);
}

void pivotLeft() {

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  digitalWrite(IN5, LOW);
  digitalWrite(IN6, HIGH);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN7, HIGH);
  digitalWrite(IN8, LOW);
}

void stopAll() {

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  digitalWrite(IN5, LOW);
  digitalWrite(IN6, LOW);

  digitalWrite(IN7, LOW);
  digitalWrite(IN8, LOW);
}
