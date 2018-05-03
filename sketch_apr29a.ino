#include <FastLED.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include "DHT.h"

#define NUM_LEDS 14
#define NUM_DIGITS 2
#define DATA_PIN 6
#define TIME_INTERVAL 500
#define DHT_SAMPLING_RATE 5000

CRGB leds[NUM_LEDS];

DHT dht;

volatile uint8_t currentColor;
volatile int16_t lastValue, currentValue;
const char *numbers[10] = { "012456", "04", "12345", "01345", "0346", "01356", "012356", "045", "0123456", "013456"};

float humidity = 0.0;
float temperature = 0.0;

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
  showNumber();
  readDht();
}

void showNumber() {
  static unsigned long previousMillis = 0;
  static uint8_t currentNumber = 0;
  if (millis() - previousMillis > TIME_INTERVAL) {
    previousMillis = millis();
    Serial.println(currentColor);
    const char *number = numbers[currentNumber];
    FastLED.clear();
    for (int j = 0; j < NUM_DIGITS; j++) {
      for (int i = 0; i < strlen(number); i++) {
          int index = number[i] - '0';
          leds[index + j * 7].setHSV(currentColor, 255, 255);
        }
    }
    FastLED.show();
    currentNumber = (currentNumber + 1) % 10;
  }
}

void readEncoder() {
  currentValue += encoder->getValue();
  if (currentValue != lastValue) {
    lastValue = currentValue;
    currentColor = currentValue;
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

