#include <Keypad.h>
#include <Servo.h>
#include <Adafruit_Fingerprint.h>
#include <MFRC522.h>
#include <SPI.h>
#include <SoftwareSerial.h>

// ----- Pins -----
#define BUZZER_PIN 10
#define SERVO_PIN 11
#define FINGER_RX 12
#define FINGER_TX 13
#define RFID_SS 5
#define RFID_RST 6

// ----- Password -----
const String correctPassword = "1234";
String inputPassword = "";

// ----- Keypad Setup -----
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ----- Servo -----
Servo lockServo;

// ----- Fingerprint Sensor -----
SoftwareSerial fingerSerial(FINGER_RX, FINGER_TX);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

// ----- RFID -----
MFRC522 rfid(RFID_SS, RFID_RST);

// ----- States -----
bool passwordAccepted = false;
bool fingerprintAccepted = false;
bool rfidAccepted = false;

void setup() {
  Serial.begin(9600);
  keypad.setDebounceTime(100);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  // Servo
  lockServo.attach(SERVO_PIN);
  lockServo.write(0); // Locked position

  // Fingerprint Sensor
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor ready.");
  } else {
    Serial.println("Fingerprint sensor not detected.");
  }

  // RFID
  SPI.begin();
  rfid.PCD_Init();

  Serial.println("System ready. Enter 4-digit password.");
}

void loop() {
  checkKeypad();
  checkFingerprint();
  checkRFID();

  if (passwordAccepted && fingerprintAccepted && rfidAccepted) {
    unlockDoor();
    delay(5000); // Wait 5 seconds before locking again
    lockDoor();
    resetAuth();
  }
}

void checkKeypad() {
  char key = keypad.getKey();
  if (key) {
    tone(BUZZER_PIN, 1000, 100); // Short beep
    Serial.print("*");

    if (key == '#') {
      if (inputPassword == correctPassword) {
        passwordAccepted = true;
        Serial.println("\nPassword correct.");
      } else {
        Serial.println("\nIncorrect password.");
      }
      inputPassword = "";
    } else if (key == '*') {
      inputPassword = "";
      Serial.println("\nPassword cleared.");
    } else {
      inputPassword += key;
    }
  }
}

void checkFingerprint() {
  if (fingerprintAccepted) return;

  if (finger.getImage() == FINGERPRINT_OK) {
    if (finger.image2Tz() == FINGERPRINT_OK) {
      if (finger.fingerSearch() == FINGERPRINT_OK) {
        fingerprintAccepted = true;
        Serial.println("Fingerprint recognized.");
      } else {
        Serial.println("Fingerprint not found.");
      }
    }
  }
}

void checkRFID() {
  if (rfidAccepted) return;

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.print("RFID UID: ");
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i], HEX);
    }
    Serial.println();

    // Simple UID check example
    if (rfid.uid.uidByte[0] == 0xDE && rfid.uid.uidByte[1] == 0xAD) {
      rfidAccepted = true;
      Serial.println("RFID accepted.");
    } else {
      Serial.println("RFID not recognized.");
    }
    rfid.PICC_HaltA();
  }
}

void unlockDoor() {
  Serial.println("Access granted. Unlocking door...");
  lockServo.write(90); // Unlock position
  tone(BUZZER_PIN, 1500, 200);
}

void lockDoor() {
  Serial.println("Locking door...");
  lockServo.write(0);
}

void resetAuth() {
  passwordAccepted = false;
  fingerprintAccepted = false;
  rfidAccepted = false;
}
