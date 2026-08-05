// ============================================================
// ESP32 ROBOT — v1
// Earliest working version.
// Features: Bluetooth manual drive (F/B/L/R/S), basic ultrasonic
// obstacle detection with automatic avoidance in Auto mode.
// No servos, MPU6050, or BLDC yet — pure 4WD drive test.
// ============================================================

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// =========================
// ULTRASONIC
// =========================
#define TRIG 18
#define ECHO 19

// =========================
// MOTORS (CORRECT WHEEL MAP)
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

  stopAll();

  Serial.println("Robot Ready");
}

// =========================
// LOOP
// =========================

void loop() {

  // ---------------- Bluetooth ----------------
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
  }

  long distance = getDistance();

  if (autoMode) {
    autoDrive(distance);
  }

  else {
    manualDrive();
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

    // Try RIGHT
    pivotRight();
    delay(400);

    if (getDistance() > 20) return;

    // Try LEFT
    pivotLeft();
    delay(600);

    if (getDistance() > 20) return;

    // Escape backward
    allBackward();
    delay(3000);

    pivotRight();
    delay(500);
  }

  allForward();
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
// FORWARD
// =========================

void allForward() {

  // Front Right
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Front Left
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  // Rear Left
  digitalWrite(IN5, HIGH);
  digitalWrite(IN6, LOW);

  // Rear Right
  digitalWrite(IN7, HIGH);
  digitalWrite(IN8, LOW);
}

// =========================
// BACKWARD
// =========================

void allBackward() {

  // Front Right
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  // Front Left
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  // Rear Left
  digitalWrite(IN5, LOW);
  digitalWrite(IN6, HIGH);

  // Rear Right
  digitalWrite(IN7, LOW);
  digitalWrite(IN8, HIGH);
}

// =========================
// PIVOT RIGHT
// =========================

void pivotRight() {

  // LEFT wheels forward

  // Front Left
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  // Rear Left
  digitalWrite(IN5, HIGH);
  digitalWrite(IN6, LOW);


  // RIGHT wheels backward

  // Front Right
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  // Rear Right
  digitalWrite(IN7, LOW);
  digitalWrite(IN8, HIGH);
}

// =========================
// PIVOT LEFT
// =========================

void pivotLeft() {

  // LEFT wheels backward

  // Front Left
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  // Rear Left
  digitalWrite(IN5, LOW);
  digitalWrite(IN6, HIGH);


  // RIGHT wheels forward

  // Front Right
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Rear Right
  digitalWrite(IN7, HIGH);
  digitalWrite(IN8, LOW);
}

// =========================
// STOP
// =========================

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
