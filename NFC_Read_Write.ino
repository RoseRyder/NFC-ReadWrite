/**************************************************************************/
/*!
    @file     readMifare_write.ino
    @author   Adafruit Industries (modified)
    @license  BSD
*/
/**************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// I2C pins (Arduino UNO: SDA=A4, SCL=A5)
// IRQ not used, Reset not connected
#define PN532_IRQ   0
#define PN532_RESET -1

// Use I2C connection
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

void setup(void) {
  Serial.begin(115200);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while (1); // halt
  }

  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // Configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443A Card ...");
}

void loop(void) {
  uint8_t success;
  uint8_t uid[7];   // Buffer to store the returned UID
  uint8_t uidLength;

  // Wait for an ISO14443A type card
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println();

    if (uidLength == 4) {
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

      // Authenticate block 4 with default KEYA
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);

      if (success) {
        Serial.println("Sector 1 authenticated");

        // Prepare data to write (16 bytes)
        uint8_t data[16] = { 'I','D','E','A','2',' ','L','A','B',0,0,0,0,0,0,0 };

        // Write to block 4
        success = nfc.mifareclassic_WriteDataBlock(4, data);
        if (success) {
          Serial.println("Block 4 written successfully!");
        } else {
          Serial.println("Failed to write Block 4");
        }

        // Read back to confirm
        success = nfc.mifareclassic_ReadDataBlock(4, data);
        if (success) {
          Serial.println("Reading Block 4 after write:");
          nfc.PrintHexChar(data, 16);
          Serial.println();
        } else {
          Serial.println("Failed to read Block 4 after write");
        }

      } else {
        Serial.println("Authentication failed");
      }
    }

    if (uidLength == 7) {
      Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");
      Serial.println("Reading page 4");
      uint8_t data[32];
      success = nfc.mifareultralight_ReadPage(4, data);
      if (success) {
        nfc.PrintHexChar(data, 4);
        Serial.println();
        delay(1000);
      } else {
        Serial.println("Unable to read page 4");
      }
    }

    delay(2000); // Wait 2s before reading next tag
  }
}
