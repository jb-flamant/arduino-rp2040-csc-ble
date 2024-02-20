/*
  Cadence meter

  Measure bike crank cadence

  The circuit :
  - Arduino Nano RP2040 Connect

  This example code is in the public domain.
*/

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <Arduino_LSM6DSOX.h>

BLEService cscService("1816"); // Bluetooth® Low Energy Cycling Speed and Cadence Service
BLEService accService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy Accelerometer Service

// Bluetooth® Low Energy CSC Measurement Characteristic
BLECharacteristic cscMeasurement("2A5B",
  BLERead | BLENotify, 5); // remote clients will be able to get notifications if this characteristic changes

// Bluetooth® Low Energy CSC Feature Characteristic
BLECharacteristic cscFeature("2A5C",
  BLERead,2);

// Bluetooth® Low Energy Accelerometer Characteristic - custom 128-bit UUID, read by central
BLEByteCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead);

//CSC measurement and feature buffers
unsigned char bleBuffer[5];
unsigned char fBuffer[2];

unsigned short revolutions = 0;
unsigned short timestamp = 0;
unsigned short flags = 0x02;
unsigned short sampleRate = 1; // per second
unsigned short cscEventTimeRes = 1024; // 1024 sample per second
unsigned short max_timestamp = (unsigned short)(64*cscEventTimeRes - 1);

bool ledStatus = false;

void initSerial() {
  Serial.begin(9600);
  while (!Serial);
}

void initAccelerometer() {
  
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");

    while (1);
  }

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Acceleration in g's");
  Serial.println("X\tY\tZ");
}

void updateBuffer() {
  bleBuffer[0] = flags & 0xff;
  bleBuffer[1] = revolutions & 0xff;
  bleBuffer[2] = (revolutions >> 8) & 0xff;
  bleBuffer[3] = timestamp & 0xff;
  bleBuffer[4] = (timestamp >> 8) & 0xff;
  fBuffer[0] = flags & 0xff;
  fBuffer[1] = 0x00;
}

void initBLE() {
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("CadenceMeter");
  BLE.setAdvertisedService(cscService);

  // add the characteristic to the service
  cscService.addCharacteristic(cscMeasurement);
  cscService.addCharacteristic(cscFeature);

  // add service
  BLE.addService(cscService);

  updateBuffer();

  // set the initial value for the characeristic:
  cscMeasurement.writeValue(bleBuffer, sizeof(bleBuffer));
  cscFeature.writeValue(fBuffer,sizeof(fBuffer));

  // start advertising
  BLE.advertise();

  Serial.println("BLE CadenceMeter Peripheral");
}

void readAccelerometer() {
  float x, y, z;

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

    Serial.print(x);
    Serial.print(',');
    Serial.print(y);
    Serial.print(',');
    Serial.println(z);
  }
}

void detectRevolution() {
  revolutions = revolutions + 1;
  if( timestamp < max_timestamp ) {
    // timestamp = timestamp + (unsigned short)(cscEventTimeRes/sampleRate);
    timestamp = timestamp + cscEventTimeRes;
  } else {
    timestamp = 0;
  }
  Serial.print(F("Timestamp: "));
  Serial.println(timestamp);
  Serial.print(F("MaxTimestamp: "));
  Serial.println(max_timestamp);
  Serial.print(F("Revolutions: "));
  Serial.println(revolutions);
  updateBuffer();
  cscMeasurement.writeValue(bleBuffer,sizeof(bleBuffer));
  delay(sampleRate*1000);
}

void setup() {

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);

  initSerial();

  digitalWrite(LED_BUILTIN, HIGH);

  initAccelerometer();

  digitalWrite(LED_BUILTIN, LOW);

  initBLE();

  digitalWrite(LED_BUILTIN, HIGH);
}

// the loop function runs over and over again forever
void loop() {
  // listen for Bluetooth® Low Energy peripherals to connect:
  BLEDevice central = BLE.central();

  readAccelerometer();

  // if a central is connected to peripheral:
  if (central) {
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    // while the central is still connected to peripheral:
    while (central.connected()) {
      detectRevolution();
    }

    // when the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
  }

  digitalWrite(LED_BUILTIN, LOW);
}
