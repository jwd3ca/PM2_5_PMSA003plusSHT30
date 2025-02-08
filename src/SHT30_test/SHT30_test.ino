#include <Wire.h>
#include <Adafruit_SHT31.h>

// Create an SHT31 object
Adafruit_SHT31 sht30 = Adafruit_SHT31();

void setup() {
  // Start the serial port for debugging.
  Serial.begin(115200);
  // Wait a moment for the Serial Monitor to open (optional on ESP32).
  delay(1000);
  Serial.println("SHT30 test starting...");

  // Initialize I²C explicitly with the Core2 default pins:
  Wire.begin(21, 22); // SDA=21, SCL=22

  // Initialize the SHT30 sensor at address 0x44
  if (!sht30.begin(0x44)) {
    Serial.println("Couldn't find SHT30 sensor at 0x44");
    while (1) {
      delay(1);
    }
  }
}

void loop() {
  // Read temperature in °C and relative humidity in %
  float temperature = sht30.readTemperature();
  float humidity = sht30.readHumidity();

  // Check if readings are valid (not NaN)
  if (!isnan(temperature) && !isnan(humidity)) {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  } else {
    Serial.println("Failed to read from SHT30 sensor!");
  }

  // Wait 2 seconds before the next reading.
  delay(2000);
}
