// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @brief This example demonstrates Zigbee Color Dimmable light bulb.
 *
 * The example demonstrates how to use Zigbee library to create an end device with
 * color dimmable light end point.
 * The light bulb is a Zigbee end device, which is controlled by a Zigbee coordinator.
 *
 * Proper Zigbee mode must be selected in Tools->Zigbee mode
 * and also the correct partition scheme must be selected in Tools->Partition Scheme.
 *
 * Please check the README.md for instructions and more detailed description.
 *
 * Created by Jan Procházka (https://github.com/P-R-O-C-H-Y/)
 */

#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"

/* Zigbee color dimmable light configuration */
#define ZIGBEE_RGB_LIGHT_ENDPOINT 10

#define PIN_ORANGE_0  19
#define PIN_ORANGE_1  20
#define PIN_ORANGE_2  18
#define PIN_ORANGE_3  21
#define PIN_ORANGE_4  22
#define PIN_ORANGE_5  23
#define PIN_ORANGE_6  16
#define PIN_ORANGE_7  17
#define PIN_RED       0
#define PIN_GREEN     1
#define PIN_BLUE      2

#define BREAD_PIN_ON   HIGH
#define BREAD_PIN_OFF  LOW

#define ALL_BREAD_GPIO_COUNT 8
#define ALL_BREAD_ANLG_COUNT 3

uint8_t allBreadGPIOs[ALL_BREAD_GPIO_COUNT] = {
  PIN_ORANGE_0,
  PIN_ORANGE_1,
  PIN_ORANGE_2,
  PIN_ORANGE_3,
  PIN_ORANGE_4,
  PIN_ORANGE_5,
  PIN_ORANGE_6,
  PIN_ORANGE_7
};
uint8_t allBreadAnalogs[ALL_BREAD_ANLG_COUNT] = {
  PIN_RED,
  PIN_GREEN,
  PIN_BLUE
};

uint8_t led = LED_BUILTIN;
uint8_t button = BOOT_PIN;

ZigbeeColorDimmableLight zbColorLight = ZigbeeColorDimmableLight(ZIGBEE_RGB_LIGHT_ENDPOINT);

/********************* RGB LED functions **************************/
void setRGBLight(bool state, uint8_t red, uint8_t green, uint8_t blue, uint8_t level) {
  Serial.printf(
    "setting RGB: state=%s, red=%d, green=%d, blue=%d, level=%d\n",
    state ? "on" : "off",
    red,
    green,
    blue,
    level
  );
  if (!state) {
    analogWrite(PIN_GREEN, 0);
    analogWrite(PIN_GREEN, 0);
    analogWrite(PIN_BLUE, 0);
    return;
  }
  float brightness = (float)level / 255;
  analogWrite(PIN_RED, (uint8_t)(red * brightness));
  analogWrite(PIN_GREEN, (uint8_t)(green * brightness));
  analogWrite(PIN_BLUE, (uint8_t)(blue * brightness));

  // use 8 orange LEDs as binary representation of red channel
  outputChannelOnLEDs(red);
}

void outputChannelOnLEDs(uint8_t value) {
    for (int i = 0; i < ALL_BREAD_GPIO_COUNT; i++) {
        digitalWrite(allBreadGPIOs[i], (value >> (7 - i)) & 1);
    }
}

// Create a task on identify call to handle the identify function
void identify(uint16_t time) {
  static uint8_t blink = 1;
  log_d("Identify called for %d seconds", time);
  if (time == 0) {
    // If identify time is 0, stop blinking and restore light as it was used for identify
    zbColorLight.restoreLight();
    return;
  }
  rgbLedWrite(led, 255 * blink, 255 * blink, 255 * blink);
  blink = !blink;
}

/********************* Arduino functions **************************/
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("Serial ready");

  // Init RMT and leave light OFF
  rgbLedWrite(led, 0, 0, 0);

  // Init button for factory reset
  pinMode(button, INPUT_PULLUP);

  // Init breadboard LEDs
  setupBreadLEDs(50);

  // init RGB
  setRGBLight(true, 170, 170, 170, 255);

  // Set callback function for light change
  zbColorLight.onLightChange(setRGBLight);

  // Optional: Set callback function for device identify
  zbColorLight.onIdentify(identify);

  // Optional: Set Zigbee device name and model
  zbColorLight.setManufacturerAndModel("Espressif", "ZBColorLightBulb");

  // Add endpoint to Zigbee Core
  Serial.println("Adding ZigbeeLight endpoint to Zigbee Core");
  Zigbee.addEndpoint(&zbColorLight);

  // When all EPs are registered, start Zigbee in End Device mode
  if (!Zigbee.begin()) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  }
  Serial.println("Connecting to network");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
}

// ----------------------------------------------------------------------------
// -- RGB LED functions
void setupBreadLEDs(uint flashSpeed) {
  uint pin;
  for (pin=0; pin<ALL_BREAD_GPIO_COUNT; pin++) {
    pinMode(allBreadGPIOs[pin], OUTPUT);
    digitalWrite(allBreadGPIOs[pin], BREAD_PIN_ON);
    delay(flashSpeed);
    digitalWrite(allBreadGPIOs[pin], BREAD_PIN_OFF);
    delay(flashSpeed);
  }
  for (pin=0; pin<ALL_BREAD_ANLG_COUNT; pin++) {
    pinMode(allBreadAnalogs[pin], OUTPUT);
    analogWrite(allBreadAnalogs[pin], 255);
    delay(flashSpeed);
    analogWrite(allBreadAnalogs[pin], 0);
    delay(flashSpeed);
  }
}

void loop() {
  // Checking button for factory reset
  if (digitalRead(button) == LOW) {  // Push button pressed
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(button) == LOW) {
      delay(50);
      if ((millis() - startTime) > 3000) {
        // If key pressed for more than 3secs, factory reset Zigbee and reboot
        Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
        delay(1000);
        Zigbee.factoryReset();
      }
    }
    // Increase blightness by 50 every time the button is pressed
    zbColorLight.setLightLevel(zbColorLight.getLightLevel() + 50);
  }
  delay(100);
}
