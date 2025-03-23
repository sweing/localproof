#include <RTClib.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <GxEPD2_BW.h>
#include <QRCode_Library.h>
#include <Wire.h>
#include <Preferences.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <TOTP.h>
#include <ArduinoBearSSL.h>
//#include <AES128.h>
#include <Base64.h>  // For Base64 encoding
#include <Preferences.h>
#include <mbedtls/aes.h>
#include <mbedtls/base64.h>
#include <string.h>

// SETUP
//String baseUrl = "http://localhost:5005"; // Dynamic base URL
String baseUrl = "https://map.localproof.org/";
const char *deviceId = "0001";  // The account ID to be used in the QR code
const unsigned char key[32] = "yoursecret";

Preferences preferences;

// Create an instance of the DS3231 RTC
RTC_DS3231 rtc;

// Create an instance of the TinyGPS++ object
TinyGPSPlus gps;

// Define the serial connection for the GPS module
SoftwareSerial gpsSerial(16, 17); // TX, RX

// Define the GPIO pin to control GPS power
#define GPS_POWER_PIN 4 // Change this to the GPIO pin you're using

#define CS_PIN    26
#define DC_PIN    25
#define RST_PIN   33 //ACHTUNG FALSCH BESCHRIFTET!!
#define BUSY_PIN  27

// Define the display type (1.54" b/w)
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(/*CS=*/ CS_PIN, /*DC=*/ DC_PIN, /*RST=*/ RST_PIN, /*BUSY=*/ BUSY_PIN));

void drawQRCode(const char *text);
void displaySettingUpMessage();
void setRTCTimeFromGPS();

const char* spinnerFrames[] = {"|", "/", "-", "\\"};
int currentFrame = 0;

unsigned long setupStartTime = millis();
const unsigned long timeoutDuration = 10 * 60 * 1000; // 5 minutes
const unsigned long animationInterval = 5000; // Update animation every 1 second
unsigned long lastAnimationUpdate = 0;

// enter your hmacKey (10 digits)
uint8_t hmacKey[] = {secret};
TOTP totp = TOTP(hmacKey, 10);

String generateTOTP();

String totpCode = String("");

void setup() {
  Serial.begin(115200);

  // Initialize the GPS power control pin
  //pinMode(GPS_POWER_PIN, OUTPUT);
  //digitalWrite(GPS_POWER_PIN, HIGH); // Turn on the GPS module

  // Initialize preferences
  preferences.begin("totp", false); // Open NVS namespace "totp" in read/write mode

  // Initialize totpCode from NVS (or set to empty string if not found)
  totpCode = preferences.getString("totpCode", "");

  
  gpsSerial.begin(9600); // Start the GPS serial connection
  SPI.begin(18, -1, 23, -1);  // SCK=18, MISO not used (-1), MOSI=23, CS=-1
  display.init(115200, false, 10, false); // Initialize the e-ink display
  // Initialize the RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    display.clearScreen();
    display.setCursor(10, 30);
    display.print("RTC not found. Halting.");
    while (1); // Halt if RTC is not found
  }

  // Check the reset reason
  esp_reset_reason_t resetReason = esp_reset_reason();

  // Only set the RTC time from GPS on power-up or software reset
  if (resetReason == ESP_RST_POWERON || resetReason == ESP_RST_SW) {
    preferences.putString("totpCode", ""); // Clear the TOTP code in NVS

    displaySettingUpMessage();
    drawOverlay("AB", "CD");
    
    // Wait for GPS to get a fix or timeout
    bool gpsFixAcquired = false;
    bool gpsLocationValid = false;
    while (millis() - setupStartTime < timeoutDuration) {
      // Check for GPS data
      while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
      }

      // Check if GPS has a valid fix (time and date)
      if (gps.time.isValid() && gps.date.isValid()) {
        gpsFixAcquired = true;

        // Check if GPS has valid location data
        if (gps.location.isValid()) {
          gpsLocationValid = true;
          break; // Exit the loop if GPS fix and location are acquired
        }
      }
      

      // Update the spinner animation every second
//      if (millis() - lastAnimationUpdate >= animationInterval) {
//        updateSpinner(currentFrame);
//        currentFrame = (currentFrame + 1) % 4; // Cycle through frames
//        lastAnimationUpdate = millis();
//      }
    }
    
    // Check if the GPS fix and location were acquired
    if (gpsFixAcquired && gpsLocationValid) {
      // Set the RTC time from the GPS
      rtc.adjust(DateTime(gps.date.year(), gps.date.month(), gps.date.day(),
                 gps.time.hour(), gps.time.minute(), gps.time.second()));
      Serial.println("RTC time set from GPS");

      // Save the location to NVS
      preferences.putFloat("lat", gps.location.lat());
      preferences.putFloat("lng", gps.location.lng());
      Serial.print(gps.location.lat(), 6);
      Serial.print(F(","));
      Serial.println(gps.location.lng(), 6);

      // Calculate the delay to align with the next 30-second interval
      int currentSecond = gps.time.second(); // Get the current second from GPS
      int delaySeconds = (30 - (currentSecond % 30)) % 30; // Calculate remaining seconds

      // Turn off the GPS module to save power
      //digitalWrite(GPS_POWER_PIN, LOW);

      Serial.println("GPS module turned off.");
      
      // Add the delay
      if (delaySeconds > 0) {
          Serial.print("Delaying for ");
          Serial.print(delaySeconds);
          Serial.println(" seconds to align with the next 30-second interval...");
          // delay(delaySeconds * 1000); // Convert seconds to milliseconds
      }
                
      // Read the TOTP code from NVS
      totpCode = preferences.getString("totpCode", "");
      
      // Print the saved TOTP code (for debugging)
      Serial.print("Saved TOTP Code: ");
      Serial.println(totpCode);
      //preferences.putString("totpCode", totpCode);
      
    } else {
      // Timeout or invalid location occurred
      display.clearScreen();
      display.setCursor(10, 30);
      if (!gpsFixAcquired) {
        display.print("GPS signal not found.");
      } else if (!gpsLocationValid) {
        display.print("GPS location invalid.");
      }
      display.display(true); // Full refresh
      Serial.println("GPS signal or location not found within timeout.");
    }
  }

  // Hibernate the display to save power
  display.hibernate();
}

void loop() {
  //Serial.println("waky waky");
  // Get the current time from the RTC
  DateTime currentTime = rtc.now();

  // Check if the RTC time is valid
  if (!currentTime.isValid()) {
    Serial.println("RTC time is invalid!");
    return;
  }

  // Format and print the time
//  char str[20];   // Declare a string as an array of chars
//  sprintf(str, "%d/%d/%d %d:%d:%d",     // %d allows to print an integer to the string
//          currentTime.year(),   // Get year method
//          currentTime.month(),  // Get month method
//          currentTime.day(),    // Get day method
//          currentTime.hour(),   // Get hour method
//          currentTime.minute(), // Get minute method
//          currentTime.second()  // Get second method
//         );
  //Serial.println(currentTime.timestamp()); 
  
  // Generate TOTP code
  String newCode = String(totp.getCode(currentTime.unixtime()));

  if (totpCode != newCode) {
    totpCode = String(newCode);

    // Save the TOTP code to NVS
    preferences.putString("totpCode", totpCode);

    // Retrieve latitude and longitude from NVS
    float lat = preferences.getFloat("lat", 0.0); // Default to 0.0 if not found
    float lng = preferences.getFloat("lng", 0.0); // Default to 0.0 if not found

    // ---------------------------------------------------------------------------
    // 2. Prepare plaintext
    char plaintext[128];
    snprintf(plaintext, sizeof(plaintext), "%s|%f|%f", totpCode, lat, lng);
    size_t plaintext_len = strlen(plaintext);
    
    // 3. Generate IV
    uint8_t iv[16];
    esp_fill_random(iv, 16);
  
    Serial.println(plaintext);
  
    // 4. Pad the data
    unsigned char padded_data[128];
    size_t padded_len = plaintext_len;
    memcpy(padded_data, plaintext, plaintext_len);
    pad_data(padded_data, &padded_len);
  
    // 5. Encrypt
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 256);
    
    uint8_t ciphertext[padded_len];
    uint8_t iv_copy[16];
    memcpy(iv_copy, iv, 16);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_len, iv_copy, padded_data, ciphertext);
    mbedtls_aes_free(&aes);
  
    // 6. Combine IV + ciphertext
    uint8_t iv_cipher[16 + padded_len];
    memcpy(iv_cipher, iv, 16);
    memcpy(iv_cipher + 16, ciphertext, padded_len);
  
    // 7. Base64 encode
    size_t base64_len;
    unsigned char base64_output[256];
    mbedtls_base64_encode(base64_output, sizeof(base64_output), &base64_len, iv_cipher, sizeof(iv_cipher));
    
    // URL-safe replacements
    for (size_t i=0; i<base64_len; i++) {
      if (base64_output[i] == '+') base64_output[i] = '-';
      if (base64_output[i] == '/') base64_output[i] = '_';
    }
  
    // 8. Build URL
    String url = baseUrl + String(deviceId) + "/" + String((char *)base64_output);
    // ---------------------------------------------------------------------------

    
    
    // Print the URL to the serial monitor for debugging
    Serial.println("Generated URL: " + url);

    // Convert the URL to a char array for the QR code library
    char urlBuffer[150];  // Ensure this buffer is large enough
    url.toCharArray(urlBuffer, sizeof(urlBuffer));

    String firstTwoChars = totpCode.substring(0, 2); // Extract first 2 characters
    String lastTwoChars = totpCode.substring(totpCode.length() - 2); // Extract last 2 characters

    // Draw the QR code on the display
    drawQRCode(urlBuffer);

    drawOverlay(firstTwoChars.c_str(), lastTwoChars.c_str());

    // Put the display into low-power mode
    display.hibernate();

    // Get time again
    currentTime = rtc.now();
    int currentSecond = currentTime.second(); // Get the current second from GPS
    int sleepSeconds = (30 - (currentSecond % 30)) % 30; // Calculate remaining seconds

    // Edge case
    if (sleepSeconds == 0) {
      sleepSeconds = 30;
    }

    // Add 1 additional second to avoid loop running more than once
    sleepSeconds = sleepSeconds + 1;
    
    Serial.println("Entering deep sleep... for " + String(sleepSeconds) + " seconds.");
    esp_deep_sleep(sleepSeconds * 1000000); // Sleep for 27 seconds (27e6 microseconds)
  }
}

void pad_data(unsigned char *data, size_t *data_len) {
    size_t block_size = 16;
    size_t padding_len = block_size - (*data_len % block_size);
    memset(data + *data_len, padding_len, padding_len);
    *data_len += padding_len;
}

void drawOverlay(const char *line1, const char *line2) {
  // Get screen dimensions
  uint16_t screenWidth = display.width();
  uint16_t screenHeight = display.height();
  
  // Define overlay dimensions
  uint16_t overlayHeight = 30;
  uint16_t overlayWidth = 25;

  // Calculate the top-left corner of the overlay window
  uint16_t overlayX = (screenWidth - overlayWidth) / 2;  // Top-left X coordinate
  uint16_t overlayY = (screenHeight - overlayHeight) / 2; // Top-left Y coordinate

  // Set the partial window
  display.setPartialWindow(overlayX, overlayY, overlayWidth, overlayHeight);

  uint16_t cursor_x = overlayX-2;
  uint16_t cursor_y1 = overlayY + 13;
  uint16_t cursor_y2 = overlayY + 27;

  // Display the current frame
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE); // Clear the overlay area
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(cursor_x, cursor_y1); // Set cursor position for line1
    display.print(line1);
    display.setCursor(cursor_x, cursor_y2); // Set cursor position for line2
    display.print(line2);
  } while (display.nextPage());
}

void drawQRCode(const char *text)
{
  // Create the QR code
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(6)];
  qrcode_initText(&qrcode, qrcodeData, 6, 0, text);
  
  // Calculate the size of each QR code module (pixel) to fill the screen
  uint16_t screenWidth = display.width();
  uint16_t screenHeight = display.height();
  uint16_t qrSize = qrcode.size;
  uint16_t moduleSize = min(screenWidth, screenHeight) / qrSize;

  // Center the QR code on the screen
  uint16_t xOffset = (screenWidth - (qrSize * moduleSize)) / 2;
  uint16_t yOffset = (screenHeight - (qrSize * moduleSize)) / 2;

  // Draw the QR code
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    for (uint8_t y = 0; y < qrSize; y++)
    {
      for (uint8_t x = 0; x < qrSize; x++)
      {
        if (qrcode_getModule(&qrcode, x, y))
        {
          display.fillRect(xOffset + x * moduleSize, yOffset + y * moduleSize, moduleSize, moduleSize, GxEPD_BLACK);
        }
      }
    }
  }
  while (display.nextPage());
}

void displaySettingUpMessage() {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(10, 30);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold9pt7b);
    display.println("Searching for GPS signal...");
  } while (display.nextPage());
}

void checkTimeout() {
  if (millis() - setupStartTime > timeoutDuration) {
    // Timeout occurred
    display.clearScreen();
    display.setCursor(10, 30);
    display.print("GPS signal not found.");
    while (true); // Halt or enter low-power mode
  }
}

void setRTCTimeFromGPS() {
  Serial.println("Waiting for GPS to get a fix...");
  while (!gps.time.isValid() || !gps.date.isValid()) {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
    delay(100);
  }

  // Set the RTC time from the GPS time
  rtc.adjust(DateTime(gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second()));
  Serial.println("RTC time set from GPS");
}