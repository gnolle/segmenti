#include <FastLED.h>
#include <DS3232RTC.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include "DHT.h"
#include <Time.h>
#include <Timezone.h>

#define NUM_LEDS 29
#define NUM_DIGITS 4
#define DATA_PIN 6
#define TIME_INTERVAL 1000
#define MODE_INTERVAL 5000
#define COLOR_CHANGE_TIMEOUT 2000
#define DHT_SAMPLING_RATE 5000

CRGB leds[NUM_LEDS];

DHT dht;

volatile uint8_t currentColor;
volatile int16_t lastValue, currentValue;
const char *numbers[10] = { "012456", "04", "12345", "01345", "0346", "01356", "012356", "045", "0123456", "013456"};
const char *characters[3] = { 
  "3456",      // upper circle
  "1256",      // C 
  "0123"       // lower circle
};

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

  setSyncProvider(RTC.get);
}

time_t getLocalTime() {
  static TimeChangeRule germanSummerTime = {"DEUS", Last, Sun, Mar, 2, 120};
  static TimeChangeRule germanWinterTime = {"WINS", Last, Sun, Oct, 3, 60};
  static Timezone germanTime(germanSummerTime, germanWinterTime);
  time_t localTime = germanTime.toLocal(now());
  return localTime;
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
        showTime();
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
      setCharOnDigit(numbers[currentNumber], j, currentColor);
    }
    FastLED.show();
    currentNumber = (currentNumber + 1) % 10;
  }
}

void showColorSetting() {
  FastLED.clear();
  setCharOnDigit(numbers[8], 3, currentColor);
  setCharOnDigit(numbers[8], 2, currentColor);
  setCharOnDigit(numbers[8], 1, currentColor);
  setCharOnDigit(numbers[8], 0, currentColor);
  setColon(currentColor);
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
    setCharOnDigit(numbers[firstTempChar], 3, currentColor);
    setCharOnDigit(numbers[secondTempChar], 2, currentColor);
    setCharOnDigit(characters[0], 1, currentColor);
    setCharOnDigit(characters[1], 0, currentColor);
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
    
    FastLED.clear();
    setCharOnDigit(numbers[firstHumidChar], 3, currentColor);
    setCharOnDigit(numbers[secondHumidChar], 2, currentColor);
    setCharOnDigit(characters[0], 1, currentColor);
    setCharOnDigit(characters[2], 0, currentColor);
    FastLED.show();
  }
}

void showTime() {
  static unsigned long previousMillis = 0;

  static bool isColonVisible = true;
  if (millis() - previousMillis > TIME_INTERVAL) {
    previousMillis = millis();
    time_t localTime = getLocalTime();

    byte remainingHourDigit1 = hour(localTime) / 10;
    byte remainingHourDigit2 = hour(localTime) % 10;
    byte remainingMinuteDigit1 = minute(localTime) / 10;
    byte remainingMinuteDigit2 = minute(localTime) % 10;

    FastLED.clear();
    setCharOnDigit(numbers[remainingHourDigit1], 3, currentColor);
    setCharOnDigit(numbers[remainingHourDigit2], 2, currentColor);
    if (isColonVisible) setColon(currentColor);
    setCharOnDigit(numbers[remainingMinuteDigit1], 1, currentColor);
    setCharOnDigit(numbers[remainingMinuteDigit2], 0, currentColor);
    FastLED.show();
    isColonVisible = !isColonVisible;
  }
}

void setCharOnDigit(const char *character, uint8_t digitIndex, uint8_t color) {
  for (int i = 0; i < strlen(character); i++) {
    int index = character[i] - '0';
    leds[index + getDigitOffset(digitIndex)].setHSV(color, 255, 255);
  }
}

void setColon(uint8_t color) {
  leds[14].setHSV(color, 255, 255);
}

uint8_t getDigitOffset(uint8_t digitIndex) {
  if (digitIndex < 2)
    return digitIndex * 7;
  return digitIndex * 7 + 1;
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

