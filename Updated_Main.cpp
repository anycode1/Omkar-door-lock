//Libraries
#include <Keypad.h>
#include <Servo.h>
#include <MFRC522.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//Pins (change as needed)
#define BUZZER_PIN 10
#define SERVO_PIN 11
#define RFID_SS 5
#define RFID_RST 6

//Password (change as needed)
const String correctPassword = "1234";
String inputPassword = "";

//Keypad
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

//Servo
Servo lockServo;

//RFID
MFRC522 rfid(RFID_SS, RFID_RST);

//States
bool passwordAccepted = false;
bool rfidAccepted = false;

void setup() {
  Serial.begin(9600);
  keypad.setDebounceTime(100);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  // Servo
  lockServo.attach(SERVO_PIN);
  lockServo.write(0); // Locked position

  // RFID
  SPI.begin();
  rfid.PCD_Init();

  Serial.println("System ready. Enter 4-digit password.");
}

void loop() {
  checkKeypad();
  checkRFID();

  if (passwordAccepted || rfidAccepted) {
    unlockDoor();
    delay(10000); // Wait 10 seconds before locking again
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

void checkRFID() {
  if (rfidAccepted) return;

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.print("RFID UID: ");
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i], HEX);
    }
    Serial.println();

    if (checkRFID_UID(rfid.uid.uidByte, rfid.uid.size)) {
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
  lockServo.write(0); // Locked position
}

void resetAuth() {
  passwordAccepted = false;
  rfidAccepted = false;
}

void saveRFIDToEEPROM(byte slot, byte *uid, byte uidSize) {
  int addr = slot * 5; // 5 bytes per UID slot
  EEPROM.write(addr, uidSize);
  for (byte i = 0; i < uidSize; i++) {
    EEPROM.write(addr + 1 + i, uid[i]);
  }
}

bool checkRFID_UID(byte *uid, byte uidSize) {
  for (int slot = 0; slot < 10; slot++) {
    int addr = slot * 5;
    byte storedSize = EEPROM.read(addr);
    if (storedSize != uidSize) continue;

    bool match = true;
    for (byte i = 0; i < uidSize; i++) {
      if (EEPROM.read(addr + 1 + i) != uid[i]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

void checkSerialForNewCard() {
  if (Serial.available()) {
    if (Serial.read() == 'E') {
      Serial.println("Scan RFID card to enroll...");

      while (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial());
      byte newUID[10];
      byte size = rfid.uid.size;
      memcpy(newUID, rfid.uid.uidByte, size);

      saveRFIDToEEPROM(0, newUID, size); // Store in slot 0 for now
      Serial.println("RFID card saved!");
      rfid.PICC_HaltA();
    }
  }
}