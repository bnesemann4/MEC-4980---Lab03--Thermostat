/***************************************************************************
  This is a library for the BME680 gas, humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BME680 Breakout
  ----> http://www.adafruit.com/products/3660

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

enum hvacState {
  Heating, // 0
  Cooling, // 1
  hCount   // 2
};

enum menuState {
  TemperatureMenu, // 0
  OperationMenu,   // 1
  UnitMenu,        // 2
  mCount           // 3
};

enum tempState {
  C,      // 0
  F,      // 1
  uCount  // 2
};

hvacState opMode = Heating;
menuState menuMode = TemperatureMenu;
tempState unitMode = C;
float targetTemp = 26.;
float targetTempF = 26. * 9. / 5. + 32.;
volatile long prevChangeTime = 0;
volatile long prevChangeTimeTwo = 0;
long debounceTime = 50;
volatile bool changeButtonFlag = false;
volatile bool menuButtonFlag = false;

void IRAM_ATTR buttonToChangeThings() {
  long now = millis();
  if (now > prevChangeTime + debounceTime) {
    changeButtonFlag = true;
    prevChangeTime = now;
  }
}

void IRAM_ATTR buttonToChangeMenu() {
  long now = millis();
  if (now > prevChangeTimeTwo + debounceTime) {
    menuButtonFlag = true;
    prevChangeTimeTwo = now;
  }
}


Adafruit_BME680 bme(&Wire); // I2C

float getCurrentTemp() {
  if (unitMode == tempState::C) {
    return bme.temperature;
  }
  if (unitMode == tempState::F) {
    return bme.temperature * 9. /5. + 32.;
  }
  return 44444444;
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("BME680 test"));

  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }

  pinMode(1, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(1), buttonToChangeThings, RISING);

  pinMode(2, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(2), buttonToChangeMenu, RISING);

  bme.setTemperatureOversampling(BME680_OS_2X);
}



void loop() {
  if (! bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }

  float currentTemp = getCurrentTemp();
  Serial.print("Temperature = "); 
  Serial.print(currentTemp);
  if (unitMode == tempState::C) {
    Serial.print("*C");
    }
  if (unitMode == tempState::F) {
    Serial.print("*F");
    }
  Serial.print(" with target ");
  if (unitMode == tempState::C) {
    Serial.print(targetTemp);
    Serial.print("*C");
  }
  if (unitMode == tempState::F) {
    Serial.print(targetTempF);
    Serial.print("*F");
  }
  Serial.print(" operating in mode ");
  Serial.print((int)opMode);
  Serial.print(" in menu ");
  Serial.println(menuMode);

  if (menuButtonFlag) {
    menuButtonFlag = false;
    menuMode = (menuState)(((int)menuMode + 1) % (int)menuState::mCount);
    Serial.print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Moving to menu: ");
    Serial.println(menuMode);
  }

  if (changeButtonFlag) {
    if (menuMode == TemperatureMenu) {
      targetTemp += 1.0;
      targetTempF += 1.0;
      if (unitMode == tempState::C) {
        if (targetTemp > 30) {
          targetTemp = targetTemp - 10.;
        }
      }
      if (unitMode == tempState::F) {
        if (targetTempF > 86) {
          targetTempF = targetTempF - 10.;
        }
      }
    }

  if (menuMode == OperationMenu) {
    opMode = (hvacState)(((int)opMode + 1) % (int)hvacState::hCount);
  }

  if (menuMode == UnitMenu) {
    unitMode = (tempState)(((int)unitMode + 1) % (int)tempState::uCount);
  }
  changeButtonFlag = false;
  }

  if (unitMode == tempState::C) {
    if (opMode == Heating) {
      if (currentTemp < targetTemp) {
        Serial.println("Heater is on now!");
      } 
    }  else if (opMode == Cooling) {
    if (currentTemp > targetTemp) {
      Serial.println("AC is on now!");
    }
    }
  }

  if (unitMode == tempState::F) {
    if (opMode == Heating) {
      if (currentTemp < targetTempF) {
        Serial.println("Heater is on now!");
      } 
    }  else if (opMode == Cooling) {
    if (currentTemp > targetTempF) {
      Serial.println("AC is on now!");
    }
    }
  }
 
  Serial.println();
  delay(100);
}