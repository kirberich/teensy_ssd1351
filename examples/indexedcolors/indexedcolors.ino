#include <Arduino.h>
#include <SPI.h>
#include <ssd1351.h>

// use this to do Color c = RGB(...) instead of `RGB c = RGB(...)` or ssd1351::LowColor c = RGB(...)
// because it's slightly faster and guarantees you won't be sending wrong colours to the display.

// Choose color depth - LowColor and HighColor currently supported
// typedef ssd1351::LowColor Color;
typedef ssd1351::IndexedColor Color;

// Choose display buffering - NoBuffer or SingleBuffer currently supported
// auto display = ssd1351::SSD1351<Color, ssd1351::NoBuffer, 128, 96>();
auto display = ssd1351::SSD1351<Color, ssd1351::SingleBuffer, 128, 96>();

bool up = false;
int pos = 127;
const int particles = 256;
int offsets[particles];
int x_pos[particles];
int y_pos[particles];
Color particle_colors[particles];

void setup() {
  Serial.begin(9600);
  Serial.println("Booting...");
  display.begin();
  Serial.println("Display set up.");

  for (int i = 0; i < particles; i++) {
    x_pos[i] = random(0, 128);
    y_pos[i] = random(0, 96);
    particle_colors[i] = ssd1351::RGB(0, 0, random(0, 255));
  }
}

void loop() {
  display.fillScreen(0);
  for(int i=0; i<128; i++) {
    display.drawLine(i, 0, i, 48, i);
  }
  for(int i=128; i<256; i++) {
    display.drawLine(i-128, 48, i-128, 96, i);
  }
  display.updateScreen();
  delay(1000);

  for(int i=128; i<256; i++) {
    display.fillScreen(i);
    display.updateScreen();
  }
}
