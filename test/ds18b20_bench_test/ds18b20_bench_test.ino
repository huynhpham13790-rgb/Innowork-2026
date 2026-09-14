#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4
#define EXPECTED_SENSORS 8

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

uint8_t sensorCount = 0;
DeviceAddress tempDeviceAddress;

// Format 64-bit ROM address to hex string
void printAddress(DeviceAddress deviceAddress) {
  Serial.print("{ ");
  for (uint8_t i = 0; i < 8; i++) {
    Serial.printf("0x%02X", deviceAddress[i]);
    if (i < 7) Serial.print(", ");
  }
  Serial.print(" }");
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  Serial.println("\n==========================================");
  Serial.println("  DS18B20 8-Channel Bus Enumeration Test  ");
  Serial.println("==========================================");

  sensors.begin();
  sensorCount = sensors.getDeviceCount();

  Serial.printf("[INFO] Devices found on GPIO %d: %d\n", ONE_WIRE_BUS, sensorCount);

  if (sensorCount != EXPECTED_SENSORS) {
    Serial.printf("[WARN] Expected %d sensors, but found %d!\n", EXPECTED_SENSORS, sensorCount);
    Serial.println("[WARN] Check breadboard contacts, pull-up resistor (4.7k), and split rails.");
  }

  // Enumerate ROMs in copy-pasteable C++ array syntax
  Serial.println("\n--- Copy-Paste ROM Addresses ---");
  for (uint8_t i = 0; i < sensorCount; i++) {
    if (sensors.getAddress(tempDeviceAddress, i)) {
      Serial.printf("const uint8_t PROBE_%02d[8] = ", i + 1);
      printAddress(tempDeviceAddress);
      Serial.println(";");

      // Set resolution to 12-bit (0.0625°C step, ~750ms conversion time)
      sensors.setResolution(tempDeviceAddress, 12);
    }
  }
  Serial.println("--------------------------------\n");
  Serial.println("Starting live monitoring loop (pinch a probe to locate index)...");
  delay(1000);
}

void loop() {
  // Request conversion for all devices simultaneously
  sensors.requestTemperatures();

  Serial.printf("[%08lu ms] ", millis());
  for (uint8_t i = 0; i < sensorCount; i++) {
    float tempC = sensors.getTempCByIndex(i);

    if (tempC == DEVICE_DISCONNECTED_C) {
      Serial.printf("P%02d: [DISC]  ", i + 1);
    } else if (tempC == 85.0f) {
      // 85.0°C is the DS18B20 power-on reset value (read before conversion finished)
      Serial.printf("P%02d: [RESET] ", i + 1);
    } else {
      Serial.printf("P%02d:%5.2fC  ", i + 1, tempC);
    }
  }
  Serial.println();

  delay(1500);
}
