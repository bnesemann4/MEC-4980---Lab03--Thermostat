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
#include <Adafruit_ST7789.h>
#include <Arduino.h>

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS,TFT_DC,TFT_RST);
GFXcanvas16 canvas(240,135);

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
float targetTempF = targetTemp * 9. / 5. + 32.;
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
  display.init(135,240);
  display.setRotation(1);
  canvas.setTextColor(ST77XX_BLUE);
  pinMode(TFT_BACKLITE,OUTPUT);
  digitalWrite(TFT_BACKLITE,1);
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
  if (opMode == hvacState::Heating) {
    Serial.print("heating");
  }
  if (opMode == hvacState::Cooling) {
    Serial.print("cooling");
  }
  Serial.print(" in menu ");
  if (menuMode == menuState::TemperatureMenu) {
    Serial.print("temperature change.");
  }
  if (menuMode == menuState::OperationMenu) {
    Serial.print("heating/cooling menu.");
  }
    if (menuMode == menuState::UnitMenu) {
    Serial.print("unit change.");
  }

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
 
  canvas.fillScreen(ST77XX_WHITE);
  canvas.setCursor(0,20);
  if (menuMode == 0) {
    float current_temp = getCurrentTemp();
    canvas.println("Press to switch the set temperature.");
    canvas.print("Current temperature: ");
    canvas.print(current_temp);  
    if (unitMode == tempState::C) {
      canvas.println("*C.");
    }
    if (unitMode == tempState::F) {
      canvas.println("*F.");
    }
    canvas.print("Target temperature: "); 
    if (unitMode == tempState::C) {
      canvas.print(targetTemp);
      canvas.println("*C.");
    }
    if (unitMode == tempState::F) {
      canvas.print(targetTempF);
      canvas.println("*F.");
    }
    if (opMode == 0) {
      canvas.print("You are currently operating in heating mode.");
    }
    if (opMode == 1) {
      canvas.print("You are currently operating in cooling mode.");
    } 
  }
  if (menuMode == 1) {
      canvas.println("Press to switch between hVAC modes.");
      canvas.print("Currently operating in: ");
      if (opMode == 0) {
        canvas.print("heating mode.");
      }
      if (opMode == 1) {
        canvas.print("cooling mode.");
      }
  }
  if (menuMode == 2) {
      canvas.println("Press to switch between unit modes.");
      canvas.print("Units are currently in: ");
      if (unitMode == 0) {
        canvas.print("Celcius.");
      }
      if (unitMode == 1) {
        canvas.print("Fahrenheit.");
      }
  }

  display.drawRGBBitmap(0,0, canvas.getBuffer(), 240, 135);

  Serial.println();
  delay(100);
}