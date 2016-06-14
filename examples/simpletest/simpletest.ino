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
    particle_colors[i] = ssd1351::RGB(0, i + 10, i/2 + 10);
  }
}

void loop() {
  unsigned long before = millis();
  display.fillScreen(ssd1351::RGB());
  Color circleColor = ssd1351::RGB(0, 128, 255);

  for (int i = 0; i < particles; i++) {
    offsets[i] += random(-2, 3);
    display.drawLine(
      x_pos[i] + offsets[i],
      y_pos[i] + offsets[i],
      pos,
      80 + sin(pos / 4.0) * 20,
      particle_colors[i]
    );
    display.drawCircle(
      x_pos[i] + offsets[i],
      y_pos[i] + offsets[i],
      1,
      circleColor
    );
  }
  display.updateScreen();
  Serial.println(millis() - before);

  if (up) {
    pos++;
    if (pos >= 127) {
      up = false;
    }
  } else {
    pos--;
    if (pos < 0) {
      up = true;
    }
  }
}
