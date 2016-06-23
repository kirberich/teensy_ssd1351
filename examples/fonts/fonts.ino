#include <Arduino.h>
#include <SPI.h>
#include <ssd1351.h>

// use this to do Color c = RGB(...) instead of `RGB c = RGB(...)` or ssd1351::LowColor c = RGB(...)
// because it's slightly faster and guarantees you won't be sending wrong colours to the display.

// Choose color depth - IndexedColor, LowColor and HighColor currently supported
// typedef ssd1351::IndexedColor Color;
// typedef ssd1351::LowColor Color;
typedef ssd1351::HighColor Color;

// Choose display buffering - NoBuffer or SingleBuffer currently supported
// auto display = ssd1351::SSD1351<Color, ssd1351::NoBuffer, 128, 96>();
auto display = ssd1351::SSD1351<Color, ssd1351::SingleBuffer, 128, 96>();


void setup() {
  Serial.begin(9600);
  Serial.println("Booting...");
  display.begin();
  display.setTextSize(1);
  Serial.println("Display set up.");
}

void loop() {
  unsigned long before = millis();
  display.fillScreen(ssd1351::RGB());

  if (millis() > 5000) {
	  display.setFont(FreeMonoBold24pt7b);
	  display.setTextSize(1);
  }

  char test_string[] = "Test";
  uint16_t w = display.getTextWidth(test_string);

  display.setCursor(64 - w/2, 40);
  display.setTextColor(ssd1351::RGB(255, 255, 255));
  display.print(test_string);
  display.drawLine(63, 0, 63, 96, ssd1351::RGB(255, 0, 0));

  display.updateScreen();
  Serial.println(millis() - before);
}
