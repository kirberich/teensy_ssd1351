This is a library for Adafruit SSD1351 displays (and others): https://www.adafruit.com/products/1673

There are two versions of this display: 1.5" 128x128 and 1.27" 128x96.
In theory this should work on both, but I only own the former, so that's the only one I've tried it on.

**Description**

This library is a hacked together modification of Paul Stoffregen's ILI9341_t3 library: https://github.com/PaulStoffregen/ILI9341_t3

I've taken most of the low-level communication code and adapted it slightly to work with the SSD1351 display.

Apart from adapting it to the SSD1351 display, the majority of the work is in using single-buffering to make arbitrary pixel operations as fast as solid-color fills. Depending on the color depth and display size used, the buffer uses up to 50k of ram, though there is some space for optimization that I haven't gotten around to yet (bit-packing the 18 bit colour should reduce the size of the biggest buffer to 36k).

On a non-overclocked teensy, using a 128x96 display and single buffering, it should achieve up to 75fps using 65k colours and 40fps using 262k colours. The real fps will be lower depending on computational work happening in addition to just pushing data to the display.

**Some example benchmarks:**

 - HighColor *(262k colours, 6 bits per channel, needs 3 bytes to transmit)* single buffered:
   - ~24ms to update the screen, doesn't change depending on what pixels changed (-> 41fps when no changes to content)
   - ~4ms to write to each pixel in the buffer (-> 35fps when updating every pixel)
   - ~2ms to fill buffer with a solid color (-> 38fps for solid color fill)
 - LowColor *(64k colours, 5/6/5 bits per channel, needs 2 bytes to transmit)* single buffered:
   - ~13ms to update the screen (-> 77fps)
   - ~3ms to write to each pixel in the buffer (-> 62.5fps)
   - ~1ms to fill buffer with a solid color (-> 71fps)
 - HighColor without buffering (this is copied almost 1:1 from the base library):
   - ~98ms to manually write to every pixel to the screen (-> 10fps)
   - ~21ms to fill the screen with a solid color (->48fps)
 - LowColor without buffering (this is copied almost 1:1 from the base library):
   - ~91ms to manually write to every pixel on the screen (-> 11fps)
   - ~12ms to fill the screen with a solid color (->83fps)

The library uses an optional display buffer for its drawing operations. When used, all operations write to a buffer, the display is only updated once display.update() is called. Updating the display takes about 24ms in 262k colour mode, 13ms in 65k colour mode. Because of the large overhead of addressing a single pixel in the display, this becomes faster than sending data straight to the display if more than about 1/4 of the pixels are directly accessed. The overhead of filling the entire buffer in a simple loop is about 4ms.

**Wiring**

The whole library only works when using hardware SPI. I have only tried it on spi0, and I haven't yet tried sharing the bus with
another device, so don't expect that to work.

These are the pins I use (which are also the defaults for the constructors)
 - MOSI 11
 - SCLK 13
 - DC 15
 - CS 10
 - RESET 14

**Overclocking**
 - Depending on your circumstances, everything might work fine at 96MHz. But if your display is like mine, it won't do anything at all.
 - If you find the display doesn't work at 96MHz, simple add `#define SLOW_SPI` before including the library. Note that this will slow down the display communication quite a lot, you might find a non-overclocked teensy is faster unless your application is very CPU-heavy.

**Notes**

 - readPixel and readRect aren't implemented yet. The display also doesn't support reading data over SPI, so this only works in buffered mode. It'll be super fast thanks to that though.

**Thanks**

 - First thanks to Adafruit for building the original Adafruit-GFX library. Buy their stuff!
 - Almost all credit goes to Paul Stoffregen (https://github.com/PaulStoffregen) for his original work on the optimized ILI9341_t3 library.
 - Many thanks to Luke Benstead (https://github.com/kazade) for helping me with all the crazy C++ things.
 - Special thanks to David Will (https://github.com/openhoon) for the original help with getting the display Running

The base library itself is based on Adafruit's Adafruit-GFX library: https://github.com/adafruit/Adafruit-GFX-Library

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
MIT license, all text above must be included in any redistribution
