/*------------------------
PM2_5_PMSA003plusSHT30.ino 07feb25

Post data to my local influxdb/grafana servers. Consider using a screenshot in github.
Also consider linting the code before final commit.

key was rx, tx pins: 13, 14
30dec24: added code for DHT22, tested. working through 1000 iterations when read_temps() not called

07feb25: revisit: stripped DHT22 code and added SHT30 code from working SHT30_test.ino. WORKS!
08feb25: 1st foray into local git and the public githup to share my code.

Don't forget to 'anonymize' the code, specifically the WIFI sections.
------------------------*/


#include <M5Core2.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include <Ticker.h>
#include "Free_Fonts.h"
#include <math.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>

// Create an SHT31 object
Adafruit_SHT31 sht30 = Adafruit_SHT31();

#define BLINK_PERIOD_MS 60000  // milliseconds, 1 minute
jsc::Ticker blinkTicker(BLINK_PERIOD_MS);

// WiFi credentials
const char* ssid = "eero";
const char* password = "314159265";

// InfluxDB configuration
const char* influxdb_url = "http://192.168.4.20:8086/api/v2/write";
const char* influxdb_org = "8a5f4bbf92fe2dc3";
const char* influxdb_bucket = "PM25_bucket";
const char* influxdb_token = "PwXhH8sTpltdcgWk_Gk4IGV1onrxsCf0LyCfC7hdEFIGpGCK2WeUFZtKlKcEbRk8AwHf5t14pxlq1Ct3mMn3oA==";

#define PMS_RX_PIN 13  // RX from PMS5003
#define PMS_TX_PIN 14  // TX from PMS5003

HardwareSerial pms5003(2);  // Serial2 for PMS5003
uint8_t buffer[32];         // Buffer to store incoming data

#define DATA_LEN 32

#define X_LOCAL 20
#define Y_LOCAL 40

#define X_OFFSET 10
#define Y_OFFSET 20

#define WIFI
int DEBUG = 0;

int circleX = 222;
int circleY = 85;
int circleR = 48;
int circleOffset = 105;

int pm10 = 0;
int pm1_0 = 0;
int pm2_5 = 0;
int point3sum = 0;
int point5sum = 0;
int point10sum = 0;
int iterations = 0;

float temperature = 0;
float humidity = 0;

//====================================================================
void setup() {
  M5.begin();
  DEBUG && Serial.print("Starting\n"); delay(3000);

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

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLUE);
  M5.Lcd.fillRect(0, 0, 320, 30, TFT_BLUE);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.drawString("      P M 2.5 Air Quality V2.1", 65, 3, 4);

#ifdef WIFI
  // Connect to WiFi
  WiFi.begin(ssid, password);
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

  if (pms5003.available() > 0) {
    int bytesRead = pms5003.readBytes(buffer, 32);

    // Ensure we received enough data for a valid reading
    if (bytesRead >= 32 && buffer[0] == 0x42 && buffer[1] == 0x4d) {
      DEBUG && Serial.printf("%d: bytesRead OK!\n", iterations++);

      M5.Lcd.setTextFont(1);
      M5.Lcd.setTextSize(1);
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

      M5.Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);

      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET, FRONT);
      M5.Lcd.print("               ");

      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET, FRONT);
      M5.Lcd.printf("PM1.0 :   %d ug/m3", p_val[5]);
      pm1_0 = p_val[5];

      if (temperature > 5.0) {
        M5.Lcd.setCursor(X_LOCAL + 150, Y_LOCAL + Y_OFFSET, FRONT);
        M5.Lcd.printf("Temp :      %.1f C", temperature);
        M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 2, FRONT);
      }

      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 2, FRONT);
      M5.Lcd.printf("PM2.5 :   %d ug/m3", p_val[6]);
      pm2_5 = p_val[6];

      if (humidity > 5.0) {
        M5.Lcd.setCursor(X_LOCAL + 150, Y_LOCAL + Y_OFFSET * 2, FRONT);
        M5.Lcd.printf("Humidity :   %.1f %%", humidity);
        M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 3, FRONT);
      }

      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 3, FRONT);
      M5.Lcd.printf("PM10  :   %d ug/m3", p_val[7]);
      pm10 = p_val[7];

      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 4, FRONT);
      M5.Lcd.print("                ");
      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 4, FRONT);
      M5.Lcd.printf("0.3 :      %d ", p_val[8]);
      point3sum = p_val[8];

      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 5, FRONT);
      M5.Lcd.print("                ");
      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 5, FRONT);
      M5.Lcd.printf("0.5 :      %d ", p_val[9]);
      point5sum = p_val[9];

      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 6, FRONT);
      M5.Lcd.print("                ");
      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 6, FRONT);
      M5.Lcd.printf("1.0 :      %d ", p_val[10]);
      point10sum = p_val[10];

      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 7, FRONT);
      M5.Lcd.print("                ");
      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 7, FRONT);
      M5.Lcd.printf("2.5 :      %d ", p_val[11]);

      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 8, FRONT);
      M5.Lcd.print("                ");
      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 8, FRONT);
      M5.Lcd.printf("5.0 :      %d  ", p_val[12]);

      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 9, FRONT);
      M5.Lcd.print("                ");
      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET * 9, FRONT);
      M5.Lcd.printf("10 :       %d ", p_val[13]);

      // draw_two_standard_circles();
      draw_single_Coway_circle(temperature, humidity);

      // LCD_Display_Val();

      // using ticker for interrupt
      if (blinkTicker.elapsedTicks() > 0) {
        blinkTicker.restart();

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
          String url = String(influxdb_url) + "?org=" + influxdb_org + "&bucket=" + influxdb_bucket + "&precision=s";
          http.begin(url);
          http.addHeader("Authorization", String("Token ") + influxdb_token);
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

//====================================================================
void draw_single_Coway_circle(float H, float T) {
  circleY = 175;
  circleR = 60;
  M5.Lcd.setFreeFont(FSSB24);

  M5.Lcd.drawCircle(circleX, circleY, circleR + 1, TFT_RED);
  if (pm2_5 > 151) {
    M5.Lcd.setTextColor(TFT_BLACK);
    M5.Lcd.fillCircle(circleX, circleY, circleR, TFT_RED);
  } else if (pm2_5 > 81) {
    M5.Lcd.setTextColor(TFT_BLACK);
    M5.Lcd.fillCircle(circleX, circleY, circleR, TFT_YELLOW);
  } else if (pm2_5 > 31) {
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.fillCircle(circleX, circleY, circleR, TFT_GREEN);
  } else {
    M5.Lcd.setTextColor(TFT_YELLOW);
    M5.Lcd.fillCircle(circleX, circleY, circleR, TFT_BLUE);
  }

  // overwrite circle with temp and humidity
  // not exactly working...
  /*
  int round_H = round(H);
  String humid = String(round_H) + " %";
  M5.Lcd.drawCentreString(humid, circleX, circleY - 20, GFXFF);
  delay(1000);

  M5.Lcd.drawCircle(circleX, circleY, circleR + 1, TFT_RED);
  if (pm2_5 > 151) {
    M5.Lcd.setTextColor(TFT_BLACK);
    M5.Lcd.fillCircle(circleX, circleY, circleR, TFT_RED);
  } else if (pm2_5 > 81) {
    M5.Lcd.setTextColor(TFT_BLACK);
    M5.Lcd.fillCircle(circleX, circleY, circleR, TFT_YELLOW);
  } else if (pm2_5 > 31) {
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.fillCircle(circleX, circleY, circleR, TFT_GREEN);
  } else {
    M5.Lcd.setTextColor(TFT_YELLOW);
    M5.Lcd.fillCircle(circleX, circleY, circleR, TFT_BLUE);
  }
  int round_T = round(T);
  String temp = String(round_T) + " C";
  M5.Lcd.drawCentreString(temp, circleX, circleY - 20, GFXFF);
  delay(1000);
*/
}

//====================================================================
static bool get_SHT30data() {
  // Read temperature in °C and relative humidity in %
  temperature = sht30.readTemperature();
  humidity    = sht30.readHumidity();

  // Check if readings are valid (not NaN)
  if (!isnan(temperature) && !isnan(humidity)) {
    /*
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    */
    return (true);
  } else {
    Serial.println("Failed to read from SHT30 sensor!");
    return (false);
  }
}
