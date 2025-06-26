#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

SoftwareSerial fingerSerial(12, 13); // RX, TX
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1);
  }

  Serial.println("Type ID # (1 to 127) to enroll:");
}

void loop() {
  if (Serial.available()) {
    int id = Serial.parseInt();
    if (id < 1 || id > 127) {
      Serial.println("Invalid ID");
      return;
    }
    Serial.print("Enrolling ID #");
    Serial.println(id);

    while (!getFingerprintEnroll(id));
  }
}

uint8_t getFingerprintEnroll(int id) {
  int p = -1;
  Serial.println("Place finger...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) return p;

  Serial.println("Remove finger");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  Serial.println("Place same finger again...");
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) return p;

  p = finger.createModel();
  if (p != FINGERPRINT_OK) return p;

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Fingerprint enrolled successfully!");
  } else {
    Serial.println("Failed to enroll fingerprint.");
  }
  return p;
}
