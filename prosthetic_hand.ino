#include <ESP32Servo.h>

// Button and LED pins
#define POWER_BUTTON 18
#define POWER_LED 10
#define MODE_BUTTON 15
#define MODE_LED 21

// Touch pins and motor control pin
const int touchPin1 = 4;
const int touchPin2 = 32;
const int motorPin = 13;

// EMG sensor settings
const int emgPin = 34;
const int sampleWindow = 50;
int emgValues[sampleWindow];
int bufferIndex = 0;
long total = 0;

// Servo pins
const int servoPin1 = 14;
const int servoPin2 = 27;
const int servoPin3 = 26;
const int servoPin4 = 25;

// Touch threshold
const int touchThreshold = 30;

// State variables
bool modeState = false;
bool powerButtonPressed = false;
bool modeButtonPressed = false;

// RTC memory to persist power state across deep sleep
RTC_DATA_ATTR bool powerState = false;

// Servo objects
Servo servo1, servo2, servo3, servo4;

// Global EMG to angle
int angle = 180;               // Default starting angle
float smoothedAvg = 0;         // For smoothing EMG signal
const float alpha = 0.1;       // Smoothing factor

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Check wake-up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    powerState = true; // Woken up by power button
  }

  // Configure EXT0 wake-up on POWER_BUTTON (active LOW)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)POWER_BUTTON, 0);

  // Button and LED setup
  pinMode(POWER_BUTTON, INPUT_PULLUP);
  pinMode(POWER_LED, OUTPUT);
  digitalWrite(POWER_LED, powerState ? HIGH : LOW);

  pinMode(MODE_BUTTON, INPUT_PULLUP);
  pinMode(MODE_LED, OUTPUT);
  digitalWrite(MODE_LED, LOW);

  // Motor output
  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);

  // Init EMG buffer
  for (int i = 0; i < sampleWindow; i++) emgValues[i] = 0;

  // Attach servos
  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);
  servo3.setPeriodHertz(50);
  servo4.setPeriodHertz(50);
  servo1.attach(servoPin1, 500, 2400);
  servo2.attach(servoPin2, 500, 2400);
  servo3.attach(servoPin3, 500, 2400);
  servo4.attach(servoPin4, 500, 2400);

  if (powerState) {
    performWakeUpAnimation();
  }

  Serial.print("Power LED State: ");
  Serial.println(powerState ? "ON" : "OFF");
}

void performWakeUpAnimation() {
  Serial.println("Wake-up Animation: Closing hand...");
  for (int angle = 180; angle >= 0; angle -= 2) {
    servo1.write(angle);
    servo2.write(angle);
    servo3.write(angle);
    servo4.write(angle);
    delay(10);
  }
  delay(500);

  Serial.println("Wake-up Animation: Opening hand...");
  for (int angle = 0; angle <= 180; angle += 2) {
    servo1.write(angle);
    servo2.write(angle);
    servo3.write(angle);
    servo4.write(angle);
    delay(10);
  }
  delay(500);
}

void moveToNeutralPosition() {
  Serial.println("Moving to neutral position...");
  for (int angle = 180; angle >= 0; angle -= 2) {
    servo1.write(angle);
    servo2.write(angle);
    servo3.write(angle);
    servo4.write(angle);
    delay(10);
  }
  delay(500);
}

void performServoAction(bool allMotors) {
  // Move selected servos based on EMG mapping
  servo1.write(angle);
  servo2.write(angle);
  servo3.write(angle);
  if (allMotors) servo4.write(angle);
  delay(50);
}

void enterDeepSleep() {
  // Turn off LEDs and motor before sleep
  digitalWrite(POWER_LED, LOW);
  digitalWrite(MODE_LED, LOW);
  digitalWrite(motorPin, LOW);

  // Detach servos to reduce power consumption
  servo1.detach();
  servo2.detach();
  servo3.detach();
  servo4.detach();

  Serial.println("Entering deep sleep...");
  delay(100);
  esp_deep_sleep_start();
}

void loop() {
  if (!powerState) {
    enterDeepSleep();
  }

  // Read button states
  int powerButtonReading = digitalRead(POWER_BUTTON);
  int modeButtonReading = digitalRead(MODE_BUTTON);

  // Debounce power button
  if (powerButtonReading == LOW && !powerButtonPressed) {
    powerButtonPressed = true;
    powerState = !powerState;
    digitalWrite(POWER_LED, powerState ? HIGH : LOW);
    Serial.print("Power LED State: ");
    Serial.println(powerState ? "ON" : "OFF");

    if (!powerState) {
      enterDeepSleep();
    } else {
      performWakeUpAnimation();
    }
  } else if (powerButtonReading == HIGH) {
    powerButtonPressed = false;
  }

  // Debounce mode
  if (modeButtonReading == LOW && !modeButtonPressed) {
    modeButtonPressed = true;
    modeState = !modeState;
    digitalWrite(MODE_LED, modeState ? HIGH : LOW);
    Serial.print("Mode LED State: ");
    Serial.println(modeState ? "ON" : "OFF");

    // If modeState is true (second press), move to neutral position
    if (modeState) {
      moveToNeutralPosition();

      // Sequentially open fingers 1, 2, and 3
      servo1.write(180); delay(300);
      servo2.write(180); delay(300);
      servo3.write(180); delay(300);
    }
  } else if (modeButtonReading == HIGH) {
    modeButtonPressed = false;
  }

  // Touch sensing
  int touchValue1 = touchRead(touchPin1);
  int touchValue2 = touchRead(touchPin2);
  if (touchValue1 < touchThreshold || touchValue2 < touchThreshold) {
    digitalWrite(motorPin, HIGH);
    Serial.println("Motor: ON");
  } else {
    digitalWrite(motorPin, LOW);
    Serial.println("Motor: OFF");
  }

  // EMG processing and smoothing
  int rawValue = analogRead(emgPin);
  total -= emgValues[bufferIndex];
  emgValues[bufferIndex] = rawValue;
  total += emgValues[bufferIndex];
  bufferIndex = (bufferIndex + 1) % sampleWindow;
  int avgValue = total / sampleWindow;

  // Mapping EMG to servo angle
  // Threshold tuned for user's muscle
  smoothedAvg = alpha * avgValue + (1 - alpha) * smoothedAvg;
  angle = map(smoothedAvg, 50, 400, 180, 0);
  angle = constrain(angle, 0, 180);

  performServoAction(!modeState); // If mode is OFF, move all; if ON, skip servo4

  // Debug info
  Serial.print("Power Button: ");
  Serial.print(powerButtonReading);
  Serial.print(" | Mode Button: ");
  Serial.print(modeButtonReading);
  Serial.print(" | Power LED: ");
  Serial.print(digitalRead(POWER_LED) ? "ON" : "OFF");
  Serial.print(" | Mode LED: ");
  Serial.print(digitalRead(MODE_LED) ? "ON" : "OFF");
  Serial.print(" | Touch 1: ");
  Serial.print(touchValue1);
  Serial.print(" | Touch 2: ");
  Serial.print(touchValue2);
  Serial.print("EMG Avg: "); 
  Serial.print(avgValue);
  Serial.print(" | Smoothed: "); 
  Serial.print(smoothedAvg);
  Serial.print(" | Angle: "); 
  Serial.println(angle);
  Serial.print(" | Servo Angle: ");
  Serial.println(angle);

  delay(50);
}
