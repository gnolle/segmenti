#include <FastLED.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include "DHT.h"

#define NUM_LEDS 21
#define NUM_DIGITS 3
#define DATA_PIN 6
#define TIME_INTERVAL 1000
#define MODE_INTERVAL 5000
#define COLOR_CHANGE_TIMEOUT 2000
#define DHT_SAMPLING_RATE 5000

CRGB leds[NUM_LEDS];

DHT dht;

volatile uint8_t currentColor;
volatile int16_t lastValue, currentValue;
const char *characters[10] = { "012456", "04", "12345", "01345", "0346", "01356", "012356", "045", "0123456", "013456"};

float humidity = 0.0;
float temperature = 0.0;

uint8_t mode = 0;

volatile unsigned long timeOfLastColorChange = 0;

ClickEncoder *encoder;

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  Serial.begin(9600);

  dht.setup(2);
  
  encoder = new ClickEncoder(3, 4);
  encoder->setAccelerationEnabled(false);
  
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 
  
  lastValue = -1;
}

void timerIsr() {
  encoder->service();
}

void loop() {
  readEncoder(); 
  readDht();
  updateMode();

  if (isColorSettingVisible()) {
    showColorSetting();
  } else
    if (mode == 0) {
      showTemperature();
    } else
      if (mode == 1) {
        showHumidity();
      } else
      if (mode == 2) {
        showNumber();
      }
}

bool isColorSettingVisible() {
  if (timeOfLastColorChange == 0) {
    return false;
  }
  if (millis() - timeOfLastColorChange > COLOR_CHANGE_TIMEOUT) {
    return false;
  }
  return true;
}

void updateMode() {
  static unsigned long previousMillis = 0;
  if (millis() - previousMillis > MODE_INTERVAL) {
    previousMillis = millis();
    mode = (mode + 1) % 3;
  }
}

void showNumber() {
  static unsigned long previousMillis = 0;
  static uint8_t currentNumber = 0;
  if (millis() - previousMillis > TIME_INTERVAL) {
    previousMillis = millis();
    Serial.println(currentColor);
    FastLED.clear();
    for (int j = 0; j < NUM_DIGITS; j++) {
      setCharOnDigit(currentNumber, j, currentColor);
    }
    FastLED.show();
    currentNumber = (currentNumber + 1) % 10;
  }
}

void showColorSetting() {
  FastLED.clear();
  setCharOnDigit(8, 2, currentColor);
  setCharOnDigit(8, 1, currentColor);
  setCharOnDigit(8, 0, currentColor);
  FastLED.show();
}

void showTemperature() {
  static unsigned long previousMillis = 0;
  if (millis() - previousMillis > TIME_INTERVAL) {
    previousMillis = millis();
    
    uint8_t roundedTemperature = (uint8_t) (temperature + 0.5);
    uint8_t firstTempChar = roundedTemperature / 10;
    uint8_t secondTempChar = roundedTemperature % 10;

    Serial.println("temp start");
    Serial.println(firstTempChar);
    Serial.println(secondTempChar);
    Serial.println("temp end");
    
    FastLED.clear();
    setCharOnDigit(firstTempChar, 1, currentColor);
    setCharOnDigit(secondTempChar, 0, currentColor);
    FastLED.show();
  }
}

void showHumidity() {
  static unsigned long previousMillis = 0;
  if (millis() - previousMillis > TIME_INTERVAL) {
    previousMillis = millis();
    
    uint8_t roundedHumidity = (uint8_t) (humidity + 0.5);
    uint8_t firstHumidChar = roundedHumidity / 10;
    uint8_t secondHumidChar = roundedHumidity % 10;

    Serial.println("humid start");
    Serial.println(firstHumidChar);
    Serial.println(secondHumidChar);
    Serial.println("humid end");
    
    FastLED.clear();
    setCharOnDigit(firstHumidChar, 1, currentColor);
    setCharOnDigit(secondHumidChar, 0, currentColor);
    FastLED.show();
  }
}

void setCharOnDigit(uint8_t characterIndex, uint8_t digitIndex, uint8_t color) {
  const char *character = characters[characterIndex];
  for (int i = 0; i < strlen(character); i++) {
    int index = character[i] - '0';
    leds[index + getDigitOffset(digitIndex)].setHSV(color, 255, 255);
  }
}

uint8_t getDigitOffset(uint8_t digitIndex) {
  //if (digitIndex < 2)
    return digitIndex * 7;
  //return digitIndex * 7 + 1;
}

void readEncoder() {
  currentValue += encoder->getValue();
  if (currentValue != lastValue) {
    lastValue = currentValue;
    currentColor = currentValue;
    timeOfLastColorChange = millis();
  }
}

void readDht() {
  static unsigned long previousMillis = 0;
  if (millis() - previousMillis > DHT_SAMPLING_RATE) {
    previousMillis = millis();
    float humidityReading = dht.getHumidity();
    if (humidityReading == humidityReading) {
     humidity = humidityReading;
    }

    float temperatureReading = dht.getTemperature();
    if (temperatureReading == temperatureReading) {
      temperature = temperatureReading;
    }

    Serial.println(humidity);
    Serial.println(temperature);
  }
}

