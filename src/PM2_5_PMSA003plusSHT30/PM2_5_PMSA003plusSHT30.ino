/*
PM2_5_PMSA003plusSHT30.ino 07feb25 J. Davis
key was rx, tx pins: 13, 14

30dec24: added code for DHT22, tested. working through 1000 iterations when read_temps() not called 

07feb25: stripped DHT22 code and added SHT30 code from SHT30_test.ino. WORKS! Thanks, Chad!

08feb25: committed to my github repository jwd3ca/PM2_5_PMSA003plusSHT30
  removed Ticker, changed over to M5Unified
  now using a full-display sprite for all text and circles
*/


#include <M5Unified.h>
#include <Preferences.h>  // To store restart flag in non-volatile memory
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <math.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_GFX.h>  // Ensure this is included
#include "config.h"

// Create an SHT31 object
Adafruit_SHT31 sht30 = Adafruit_SHT31();

// WIFI_SSID, WIFI_PASSWORD, influxdb credentials, etc. all defined in github-hidden config file

#define PMS_RX_PIN 13  // RX from PMS5003
#define PMS_TX_PIN 14  // TX from PMS5003

HardwareSerial pms5003(2);  // Serial2 for PMS5003
uint8_t buffer[32];         // Buffer to store incoming data

#define DATA_LEN 32

#define X_LOCAL 20
#define Y_LOCAL 40
#define Y_OFFSET 20

#define MyFont &FreeSansBold24pt7b

#define WIFI

int pm10 = 0;
int pm1_0 = 0;
int pm2_5 = 0;
int point3sum = 0;
int point5sum = 0;
int point10sum = 0;
int iterations = 0;

float temperature = 0;
float humidity = 0;

const unsigned long eventTime_1_post = 60000;  // interval in ms
const unsigned long eventTime_2_circle = 5000;

unsigned long previousTime_1 = 0;
unsigned long previousTime_2 = 0;

bool toggleState = true;  // Boolean flag for flip-flop

LGFX_Sprite fullScreenSprite = LGFX_Sprite(&M5.Display);

Preferences prefs;

//====================================================================
void setup() {
  auto cfg = M5.config();

  // reboot if neccessary
  prefs.begin("boot", false);  // Open storage
  bool hasRestarted = prefs.getBool("restarted", false);
  if (!hasRestarted) {
    Serial.println("Restarting to clear memory...");
    prefs.putBool("restarted", true);  // Mark restart as done
    prefs.end();
    ESP.restart();  
  }
  prefs.end();

  M5.begin(cfg);
  Serial.begin(115200);  // Must do this here for M5Unified!
  
  Serial.println("M5Unified Initialized!");
  delay(500);

  // Toggle the reset flag after a successful boot
  prefs.begin("boot", false);
  prefs.putBool("restarted", false);
  prefs.end();
  Serial.println("Restarting to clear memory...");

  // Try creating a 320x240 sprite
  fullScreenSprite.setPsram(true); // force using psram
  fullScreenSprite.createSprite(320, 240);  // Create sprite buffer

  // Draw your background and everything you want on the sprite
  fullScreenSprite.fillSprite(TFT_BLACK);  // Optional: Set background color
  fullScreenSprite.setTextDatum(MC_DATUM);

  fullScreenSprite.setTextFont(1);
  fullScreenSprite.setTextSize(2);

  fullScreenSprite.setTextColor(TFT_MAGENTA);
  fullScreenSprite.fillRect(0, 0, 320, 30, TFT_BLUE);
  fullScreenSprite.drawString("PARTICULATE MATTER SENSOR", 160, 20);  // why 160??

  fullScreenSprite.drawRect(0, 0, 320, 240, TFT_RED);
  delay(100);  

  // Initialize PMS5003 serial communication
  pms5003.begin(9600, SERIAL_8N1, PMS_RX_PIN, PMS_TX_PIN);
  // Initialize the SHT30 sensor at address 0x44

  // Initialize I²C explicitly with the Core2 default pins:
  Wire.begin(21, 22);  // SDA=21, SCL=22

  if (!sht30.begin(0x44)) {
    Serial.println("Couldn't find SHT30 sensor at 0x44");
    while (1) {
      delay(1);
    }
  }

#ifdef WIFI
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
#endif

}  // setup()

#define FRONT 2
#define RIGHT 120


//====================================================================
void loop() {
  int16_t p_val[16] = { 0 };
  uint8_t i = 0;
  uint8_t buffer[32];  // Buffer to store incoming data
  char buff[10];

  /* Updates frequently */
  unsigned long currentTime = millis();

  // use small text for data printing
  fullScreenSprite.setTextFont(1);
  fullScreenSprite.setTextSize(1);

  if (pms5003.available() > 0) {
    int bytesRead = pms5003.readBytes(buffer, 32);

    // Ensure we received enough data for a valid reading
    if (bytesRead >= 32 && buffer[0] == 0x42 && buffer[1] == 0x4d) {
      // Serial.printf("%d: bytesRead OK!\n", iterations++);

      for (int i = 0, j = 0; i < 32; i++) {
        if (i % 2 == 0) {
          p_val[j] = buffer[i];
          p_val[j] = p_val[j] << 8;
        } else {
          p_val[j] |= buffer[i];
          j++;
        }
      }

      get_SHT30data();

      fullScreenSprite.setTextColor(TFT_YELLOW, TFT_BLACK);

      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET, FRONT);
      fullScreenSprite.print("               ");

      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET, FRONT);
      fullScreenSprite.printf("PM1.0 : %d ug/m3", p_val[5]);
      pm1_0 = p_val[5];

/*
      if (temperature > 5.0) {
        fullScreenSprite.setCursor(X_LOCAL + 150, Y_LOCAL + Y_OFFSET, FRONT);
        fullScreenSprite.printf("Temp :      %.2f C", temperature);
        fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 2, FRONT);
      }
*/

      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 2, FRONT);
      fullScreenSprite.printf("PM2.5 : %d ug/m3", p_val[6]);
      pm2_5 = p_val[6];

/*
      if (humidity > 5.0) {
        fullScreenSprite.setCursor(X_LOCAL + 150, Y_LOCAL + Y_OFFSET * 2, FRONT);
        fullScreenSprite.printf("Humidity :   %.2f %%", humidity);
        fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 3, FRONT);
      }
*/

      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 3, FRONT);
      fullScreenSprite.printf("PM10  : %d ug/m3", p_val[7]);
      pm10 = p_val[7];

      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 4, FRONT);
      fullScreenSprite.print("                ");
      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 4, FRONT);
      fullScreenSprite.printf("0.3 :    %d ", p_val[8]);
      point3sum = p_val[8];

      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 5, FRONT);
      fullScreenSprite.print("                ");
      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 5, FRONT);
      fullScreenSprite.printf("0.5 :    %d ", p_val[9]);
      point5sum = p_val[9];

      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 6, FRONT);
      fullScreenSprite.print("                ");
      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 6, FRONT);
      fullScreenSprite.printf("1.0 :    %d ", p_val[10]);
      point10sum = p_val[10];

      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 7, FRONT);
      fullScreenSprite.print("                ");
      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 7, FRONT);
      fullScreenSprite.printf("2.5 :    %d ", p_val[11]);

      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 8, FRONT);
      fullScreenSprite.print("                ");
      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 8, FRONT);
      fullScreenSprite.printf("5.0 :    %d  ", p_val[12]);

      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 9, FRONT);
      fullScreenSprite.print("                ");
      fullScreenSprite.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 9, FRONT);
      fullScreenSprite.printf("10 :     %d ", p_val[13]);

      fullScreenSprite.pushSprite(0, 0);  // Push sprite to display

      /* This is my circle event */
      // using millis for interrupt to draw circle

      if (currentTime - previousTime_2 >= eventTime_2_circle) {
        /* Update the timing for the next event*/
        previousTime_2 = currentTime;

        toggleState = !toggleState;  // Flip-flop between true and false
        if (toggleState) {
          snprintf(buff, sizeof(buff), "%.1f C", temperature);

        } else {
          snprintf(buff, sizeof(buff), "%d %%", int(humidity + 0.5f));
        }

        drawCircleWithText(buff);
      }

      // time to post data?
      // using millis for interrupt to influxdb post data
      if (currentTime - previousTime_1 >= eventTime_1_post) {
        previousTime_1 = currentTime;

        // Format the data in InfluxDB line protocol
        String data_low = String("data_point,group=low pm1_0=") + pm1_0 + ",pm2_5=" + pm2_5 + ",pm10=" + pm10;
        String data_high = String("data_point,group=high point3sum=") + point3sum + ",point5sum=" + point5sum + ",point10sum=" + point10sum;
        String SHT30string = String("data_point,group=SHT30 temperature=") + temperature + ",humidity=" + humidity;
        Serial.println();
        Serial.println(data_low);
        Serial.println(data_high);
        Serial.println(SHT30string);

        // Send data to InfluxDB
        if (WiFi.status() == WL_CONNECTED) {
          HTTPClient http;
          String url = String(INFLUXDB_URL) + "?org=" + INFLUXDB_ORG + "&bucket=" + INFLUXDB_BUCKET + "&precision=s";
          http.begin(url);
          http.addHeader("Authorization", String("Token ") + INFLUXDB_TOKEN);
          http.addHeader("Content-Type", "text/plain");

          int httpResponseCode1 = http.POST(data_low);
          if (httpResponseCode1 > 0) {
            Serial.printf("Data_low sent successfully! HTTP response code: %d\n", httpResponseCode1);
          } else {
            Serial.printf("Error sending data. HTTP response code: %d\n", httpResponseCode1);
          }

          int httpResponseCode2 = http.POST(data_high);
          if (httpResponseCode2 > 0) {
            Serial.printf("Data_high sent successfully! HTTP response code: %d\n", httpResponseCode2);
          } else {
            Serial.printf("Error sending data. HTTP response code: %d\n", httpResponseCode2);
          }

          int httpResponseCode3 = http.POST(SHT30string);
          if (httpResponseCode3 > 0) {
            Serial.printf("SHT30string sent successfully! HTTP response code: %d\n", httpResponseCode3);
          } else {
            Serial.printf("Error sending data. HTTP response code: %d\n", httpResponseCode3);
          }

          http.end();
        } else {
          Serial.println("WiFi not connected.");
        }
      }
    } else (Serial.printf("\nBURP!\n"));
  }
}

//===================================================================
static bool get_SHT30data() {
  // Read temperature in °C and relative humidity in %
  temperature = sht30.readTemperature();
  humidity = sht30.readHumidity();

  // Check if readings are valid (not NaN)
  if (!isnan(temperature) && !isnan(humidity)) {
    return (true);
  } else {
    Serial.println("Failed to read from SHT30 sensor!");
    return (false);
  }
}

//===================================================================
// now using full-screen sprite
void drawCircleWithText(const char* text) {
  int circleX = 225;
  int circleY = 135;
  int circleR = 80;

  // Draw the outer circle band in red
  fullScreenSprite.fillCircle(circleX, circleY, circleR + 2, TFT_RED);

  if (pm2_5 > 151) {
    fullScreenSprite.setTextColor(TFT_BLACK);
    fullScreenSprite.fillCircle(circleX, circleY, circleR, TFT_RED);

  } else if (pm2_5 > 81) {
    fullScreenSprite.setTextColor(TFT_BLACK);
    fullScreenSprite.fillCircle(circleX, circleY, circleR, TFT_YELLOW);

  } else if (pm2_5 > 31) {
    fullScreenSprite.setTextColor(TFT_RED);
    fullScreenSprite.fillCircle(circleX, circleY, circleR, TFT_GREEN);

  } else {
    fullScreenSprite.setTextColor(TFT_YELLOW);
    fullScreenSprite.fillCircle(circleX, circleY, circleR, TFT_BLUE);
  }

  // Set text properties for the string
  fullScreenSprite.setTextColor(TFT_MAGENTA);
  fullScreenSprite.setFreeFont(MyFont);  // Set custom font

  // Draw the text at the center of the circle
  fullScreenSprite.drawString(text, circleX, circleY);

  fullScreenSprite.pushSprite(0, 0);  // Push sprite to display
}
