// ============================================================
// ESP32 ROBOT — v3 (18-05-2026) "with soft curves"
// Added: Soft L/R curve turning (only outer wheels driven) vs
// full pivot turns (toggled via M/N commands into P/Q states).
// ============================================================

#include <ESP32Servo.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

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

  if (SerialBT.available()) {

    command = SerialBT.read();
    Serial.println(command);

    if (command == 'A') {
      autoMode = true;
      stopAll();
    }

    else if (command == 'M') {
      // FULL LEFT PIVOT
      command = 'P';
    }

    else if (command == 'N') {
      // FULL RIGHT PIVOT
      command = 'Q';
    }

    else if (command == 'U' && !autoMode) {
      dualLiftCycle();
    }
  }

  long distance = getDistance();

  if (autoMode) {
    autoDrive(distance);
  } else {
    manualDrive();
  }

  delay(20);
}

// =========================
// MANUAL MODE
// =========================
void manualDrive() {

  if (command == 'F') {
    allForward();
  }

  else if (command == 'B') {
    allBackward();
  }

  // Soft LEFT curve
  else if (command == 'L') {

    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);

    digitalWrite(IN7, HIGH);
    digitalWrite(IN8, LOW);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

    digitalWrite(IN5, LOW);
    digitalWrite(IN6, LOW);
  }

  // Soft RIGHT curve
  else if (command == 'R') {

    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);

    digitalWrite(IN5, HIGH);
    digitalWrite(IN6, LOW);

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    digitalWrite(IN7, LOW);
    digitalWrite(IN8, LOW);
  }

  // FULL LEFT PIVOT
  else if (command == 'P') {
    pivotLeft();
  }

  // FULL RIGHT PIVOT
  else if (command == 'Q') {
    pivotRight();
  }

  else if (command == 'S') {
    stopAll();
  }
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

    allBackward();
    delay(3000);

    pivotRight();
    delay(500);
  }

  allForward();
}

// =========================
// SERVO LIFT
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
// MOTORS
// =========================
void allForward() {

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  digitalWrite(IN5, HIGH);
  digitalWrite(IN6, LOW);

  digitalWrite(IN7, HIGH);
  digitalWrite(IN8, LOW);
}

void allBackward() {

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
