#include <Arduino.h>

// Version: 0.1.0

// Distance sensor HC-SR04 pins
#define distanceSensorTrigPin 12
#define distanceSensorEchoPin 11

// Relay control pin
#define poweredOnDeviceTrigPin 6

// Debounce and filtering settings
const long distanceInCmToPowerOn = 70; // Less than that will cause power on
const long maxValidDistanceCm = 120; // Less than that will cause power on
const int lastDistancesNumber = 20; // Number of distances kept to compare when debouncing
const int debounceTimeMs = 500; // Time in ms to collect lastDistancesNumber
const float requiredAccuracy = .7; // Fraction of required matched distances to change power state

const long initialDistanceCm = distanceInCmToPowerOn + 10;
const int delayBetweenMeasurements = round(debounceTimeMs / lastDistancesNumber);

long lastDistances[lastDistancesNumber];
bool isPoweredOn = false;

void setPower(bool enabled = false) {
  if (enabled) {
    digitalWrite(poweredOnDeviceTrigPin, LOW);
  } else {
    digitalWrite(poweredOnDeviceTrigPin, HIGH);
  }
}

int getDistance() {
  digitalWrite(distanceSensorTrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(distanceSensorTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(distanceSensorTrigPin, LOW);

  int timeUs = pulseIn(distanceSensorEchoPin, HIGH);
  return timeUs / 58;
}

void initDistances() {
  for (int x = 0; x < lastDistancesNumber; x++) {
    lastDistances[x] = initialDistanceCm;
  }
}

void updateDistances(long distanceCm) {
  // Copy array holding distances
  long lastDistancesCopy[lastDistancesNumber] = {};
  for (int x = 0; x < lastDistancesNumber; x++) {
    lastDistancesCopy[x] = lastDistances[x];
  }

  // Update distances values
  for (int x = 0; x < lastDistancesNumber - 1; x++) {
    lastDistances[x] = lastDistancesCopy[x + 1];
  }
  lastDistances[lastDistancesNumber - 1] = distanceCm;
}

void setup() {
  // Enable serial port for debugging
  Serial.begin(9600);

  // Init distance sensor
  pinMode(distanceSensorTrigPin, OUTPUT);
  pinMode(distanceSensorEchoPin, INPUT);

  // Init powered on device
  pinMode(poweredOnDeviceTrigPin, OUTPUT);
  digitalWrite(poweredOnDeviceTrigPin, LOW);

  initDistances();
}

/*
 * powerOn = true; verify is acccuracy matched for toggling power on
 * powerOff = false; verify is acccuracy matched for toggling power off
 */
bool isAccuracyMatchingCriteriaTo(bool powerOn = true) {
  // Get number of ok and failed checks
  int checksOk = 0;
  int checksFailed = 0;
  for (int x = 0; x < lastDistancesNumber; x++) {
    if (lastDistances[x] <= distanceInCmToPowerOn) {
      checksOk++;
    } else {
      checksFailed++;
    }
  }

  float checksPart;

  // Verify against configured accuracy
  if (powerOn) {
    checksPart = (float) checksOk / (float) (checksOk + checksFailed);
  } else {
    checksPart = (float) checksFailed / (float) (checksOk + checksFailed);
  }

  // Return true if accuracy met, otherwise false
  if (checksPart >= requiredAccuracy) {
    return true;
  }
  return false;
}

void loop() {
  long distanceCm = getDistance();

  if (distanceCm > 0 && distanceCm < maxValidDistanceCm) {
    updateDistances(distanceCm);

    if (isPoweredOn && isAccuracyMatchingCriteriaTo(false)) {
      isPoweredOn = false;
    } else if (!isPoweredOn && isAccuracyMatchingCriteriaTo(true)) {
      isPoweredOn = true;
    }
  }

  Serial.print("State: ");
  Serial.print(isPoweredOn ? "ON" : "OFF");
  Serial.print("; Distance[cm]: ");
  Serial.println(distanceCm);

  setPower(isPoweredOn);

  delay(delayBetweenMeasurements);
}
