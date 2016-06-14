#include <Arduino.h>
#include <SPI.h>
#include <ssd1351.h>

// This example is used to illustrate the different color modes. Select a different mode to see
// different amounts of colour banding.

typedef ssd1351::IndexedColor Color;
// typedef ssd1351::LowColor Color;
// typedef ssd1351::HighColor Color;

// Choose display buffering - NoBuffer or SingleBuffer currently supported
// auto display = ssd1351::SSD1351<Color, ssd1351::NoBuffer, 128, 96>();
auto display = ssd1351::SSD1351<Color, ssd1351::SingleBuffer, 128, 96>();

void setup() {
  Serial.begin(9600);
  Serial.println("Booting...");
  display.begin();
  Serial.println("Display set up.");
}

void loop() {
  for(int i=0; i<128; i++) {
    display.drawLine(i, 0, i, 15, ssd1351::RGB(i, 0, 0));
  }
  for(int i=128; i<256; i++) {
    display.drawLine(256 - i, 16, 256 - i, 31, ssd1351::RGB(i, 0, 0));
  }

  for(int i=0; i<128; i++) {
    display.drawLine(i, 32, i, 47, ssd1351::RGB(0, i, 0));
  }
  for(int i=128; i<256; i++) {
    display.drawLine(256 - i, 48, 256 - i, 63, ssd1351::RGB(0, i, 0));
  }

  for(int i=0; i<128; i++) {
    display.drawLine(i, 64, i, 79, ssd1351::RGB(0, 0, i));
  }
  for(int i=128; i<256; i++) {
    display.drawLine(256 - i, 80, 256 - i, 95, ssd1351::RGB(0, 0, i));
  }
  display.updateScreen();
  delay(2000);
}
