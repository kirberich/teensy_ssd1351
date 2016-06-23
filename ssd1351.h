// All speed gains and original optimized library for the ILI9341 are credit to Paul Stoffregen.

/***************************************************
This is adapted from the library for the Adafruit ILI9341 display, but for the SSD1351.
----> https://www.adafruit.com/products/1673

Check out the links above for our tutorials and wiring diagrams
These displays use SPI to communicate, 4 or 5 pins are required to
interface (RST is optional)
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
MIT license, all text above must be included in any redistribution
****************************************************/

#pragma once
#include <Arduino.h>
#include <array>
#include <string>
#include <SPI.h>
#include "color.h"
#include "buffer.h"
#include "gfxfont.h"
#include "Fonts/all_fonts.h"

extern "C" {
	int _getpid(){ return -1;}
	int _kill(int pid, int sig){ return -1; }
	int _write(){return -1;}
}

#ifndef swap
template <typename T> void __attribute__((always_inline)) swap(T &a, T &b) {
	T t = a;
	a = b;
	b = t;
}
#endif

// Magical member templating magic to make special members for buffering / non-buffering more readable.
// Taken from http://lists.boost.org/Archives/boost/2014/08/215954.php
#define REQUIRES(...) typename std::enable_if<(__VA_ARGS__), int>::type = 0
#define MEMBER_REQUIRES(...) template<bool HiddenMemberBool=true, REQUIRES(HiddenMemberBool && (__VA_ARGS__))>

namespace ssd1351 {
// Teensy 3.1 can only generate 30 MHz SPI when running at 120 MHz (overclock)
// At all other speeds, SPI.beginTransaction() will use the fastest available clock
#define SPICLOCK 20000000

#define CMD_COMMAND_LOCK 0xFD
// These two bytes are used to issue some display lock commands for the init. I don't know what they do, but they seem necessary.
#define COMMAND_LOCK_INIT1 0x12 // "Unlock OLED driver IC MCU interface from entering command"
#define COMMAND_LOCK_INIT2 0xB1 // "Command A2,B1,B3,BB,BE accessible if in unlock state"
#define CMD_DISPLAY_SLEEP 0xAE // Set display to sleep mode
#define CMD_DISPLAY_WAKE 0xAF // Wake up display from sleep mode
#define CMD_CLOCK_DIVIDER 0xB3 // Set clock divider and display frequency
#define CMD_REMAP 0xA0 // Remap various display settings, like hardware mapping and most importantly color mode
#define CMD_START_LINE 0xA1 // Set display start line, needs to be set to 96 for 128x96 displays
#define CMD_DISPLAY_OFFSET 0xA2 // Set display offset (hardware dependent, needs to be set to 0)
#define CMD_FUNCTION_SELECTION 0xAB // Used to activate/deactivate internal voltage regulator
#define CMD_NORMAL_MODE 0xA6 // Normal display mode (display contents of video RAM)
#define INTERNAL_VREG 0x01 // internal voltage regulator, other value is never used
#define CMD_COLUMN_ADDRESS 0x15 // Set start and end column of active video RAM area
#define CMD_ROW_ADDRESS 0x75 // Set start and end row of active video RAM area
#define CMD_WRITE_TO_RAM 0x5C // Start writing to the video ram. After this, color data can be sent.
#define CMD_NOOP 0xAD // Sometimes used as a last command - doesn't do anything.

// Text alignments
static const uint8_t ALIGN_LEFT = 0;
static const uint8_t ALIGN_CENTER = 1;
static const uint8_t ALIGN_RIGHT = 2;

static const uint8_t HIGH_COLOR = 0;
static const uint8_t LOW_COLOR = 1;

const auto black = RGB();

static const SPISettings spi_settings(SPICLOCK, MSBFIRST, SPI_MODE0);

struct Rect {
	int16_t x;
	int16_t y;
	int16_t w;
	int16_t h;
};

template <typename C, typename B, int W = 128, int H = 128>
class SSD1351 : public Print {
public:
	SSD1351(
		uint8_t _cs = 10,
		uint8_t _dc = 15,
		uint8_t _reset = 14,
		uint8_t _mosi=11,
		uint8_t _sclk=13
	) : cs(_cs), dc(_dc), reset(_reset), mosi(_mosi), sclk(_sclk) {}

	void begin() {
		// Initialize the display. This validates the used pins for hardware SPI use,
		// goes through the displays init routine and sets some important options like
		// color depth and display size.
		// Only size and color depth are settable - everything else is hardcoded.

		// verify SPI pins are valid;
		if ((mosi == 11 || mosi == 7) && (sclk == 13 || sclk == 14)) {
			SPI.setMOSI(mosi);
			SPI.setSCK(sclk);
		} else {
			Serial.println("SPI pins are invalid.");
			return;
		}

		SPI.begin();

		if (SPI.pinIsChipSelect(cs, dc)) {
			// Configure both cs and dc as chip selects, which allows triggering them extremely fast
			// pcs_data and pcs_command contain the bitmasks used when setting the pin states.
			pcs_data = SPI.setCS(cs);
			pcs_command = pcs_data | SPI.setCS(dc);
		} else {
			Serial.println("CS and DC need to be special chip select pins.");
			pcs_data = 0;
			pcs_command = 0;
			return;
		}

		// toggle reset low to reset
		if (reset < 255) {
			pinMode(reset, OUTPUT);
			digitalWrite(reset, HIGH);
			delay(5);
			digitalWrite(reset, LOW);
			delay(20);
			digitalWrite(reset, HIGH);
			delay(150);
		}

		SPI.beginTransaction(SPISettings(SPICLOCK, MSBFIRST, SPI_MODE0));

		// Set display command lock settings - they have something to do with when the display can receive which commands,
		// but I don't exactly understand what the implications are.
		sendCommandAndContinue(CMD_COMMAND_LOCK);
		sendDataAndContinue(COMMAND_LOCK_INIT1);
		sendCommandAndContinue(CMD_COMMAND_LOCK);
		sendDataAndContinue(COMMAND_LOCK_INIT2);

		sendCommandAndContinue(CMD_DISPLAY_SLEEP);

		sendCommandAndContinue(CMD_CLOCK_DIVIDER);
		// First 4 bits (1111) are the display frequency (highest), last 4 bits (0001) are the clock divider (lowest)
		sendDataAndContinue(0xF1);

		// Set various mapping settings (0x74 = 0111 0100 for low color mode and 0xB4 (1011 0100) for high color mode)

		// 01: color mode (01 is 64k colors, 10 is 262k colors)
		// 1: COM pins split (hardware dependent)
		// 1: COM Scan direction up to down (hardware dependent)

		// 0: Reserved
		// 1: Colour sequence C -> B -> A instead of A -> B -> C
		// 0: Column layout mapping (hardware dependent)
		// 0: Horizontal address increment mode (x is increased after each write and wraps)
		setColorDepth();

		// Set start line - this needs to be 0 for a 128x128 display and 96 for a 128x96 display
		sendCommandAndContinue(CMD_START_LINE);
		sendDataAndContinue(H == 128 ? 0 : 96);

		// Set display offset - this is always zero
		sendCommandAndContinue(CMD_DISPLAY_OFFSET);
		sendDataAndContinue(0);

		// Select the internal voltage regulator
		sendCommandAndContinue(CMD_FUNCTION_SELECTION);
		sendDataAndContinue(INTERNAL_VREG);

		// Set display to normal operation and leave sleep mode
		sendCommandAndContinue(CMD_NORMAL_MODE);
		sendLastCommand(CMD_DISPLAY_WAKE);

		SPI.endTransaction();
	}

	void sleep(bool enable) {
		SPI.beginTransaction(spi_settings);
		sendLastCommand(enable ? CMD_DISPLAY_SLEEP : CMD_DISPLAY_WAKE);
		SPI.endTransaction();
	}

	void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, const C &color) {
		drawFastHLine(x, y, w, color);
		drawFastHLine(x, y + h - 1, w, color);
		drawFastVLine(x, y, h, color);
		drawFastVLine(x + w - 1, y, h, color);
	}

	void drawCircle(int16_t x0, int16_t y0, int16_t r, const C &color) {
		int16_t f = 1 - r;
		int16_t ddF_x = 1;
		int16_t ddF_y = -2 * r;
		int16_t x = 0;
		int16_t y = r;

		drawPixel(x0  , y0+r, color);
		drawPixel(x0  , y0-r, color);
		drawPixel(x0+r, y0  , color);
		drawPixel(x0-r, y0  , color);

		while (x<y) {
			if (f >= 0) {
				y--;
				ddF_y += 2;
				f += ddF_y;
			}
			x++;
			ddF_x += 2;
			f += ddF_x;

			drawPixel(x0 + x, y0 + y, color);
			drawPixel(x0 - x, y0 + y, color);
			drawPixel(x0 + x, y0 - y, color);
			drawPixel(x0 - x, y0 - y, color);
			drawPixel(x0 + y, y0 + x, color);
			drawPixel(x0 - y, y0 + x, color);
			drawPixel(x0 + y, y0 - x, color);
			drawPixel(x0 - y, y0 - x, color);
		}
	}

	void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, const C &color) {
		int16_t f     = 1 - r;
		int16_t ddF_x = 1;
		int16_t ddF_y = -2 * r;
		int16_t x     = 0;
		int16_t y     = r;

		while (x<y) {
			if (f >= 0) {
				y--;
				ddF_y += 2;
				f     += ddF_y;
			}
			x++;
			ddF_x += 2;
			f     += ddF_x;
			if (cornername & 0x4) {
				drawPixel(x0 + x, y0 + y, color);
				drawPixel(x0 + y, y0 + x, color);
			}
			if (cornername & 0x2) {
				drawPixel(x0 + x, y0 - y, color);
				drawPixel(x0 + y, y0 - x, color);
			}
			if (cornername & 0x8) {
				drawPixel(x0 - y, y0 + x, color);
				drawPixel(x0 - x, y0 + y, color);
			}
			if (cornername & 0x1) {
				drawPixel(x0 - y, y0 - x, color);
				drawPixel(x0 - x, y0 - y, color);
			}
		}
	}

	void fillCircle(int16_t x0, int16_t y0, int16_t r, const C &color) {
		drawFastVLine(x0, y0-r, 2*r+1, color);
		fillCircleHelper(x0, y0, r, 3, 0, color);
	}

	void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, const C &color) {
		int16_t f     = 1 - r;
		int16_t ddF_x = 1;
		int16_t ddF_y = -2 * r;
		int16_t x     = 0;
		int16_t y     = r;

		while (x<y) {
			if (f >= 0) {
				y--;
				ddF_y += 2;
				f     += ddF_y;
			}
			x++;
			ddF_x += 2;
			f     += ddF_x;

			if (cornername & 0x1) {
				drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
				drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
			}
			if (cornername & 0x2) {
				drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
				drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
			}
		}
	}

	void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, const C &color) {
		drawLine(x0, y0, x1, y1, color);
		drawLine(x1, y1, x2, y2, color);
		drawLine(x2, y2, x0, y0, color);
	}

	void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, const C &color) {
		int16_t a, b, y, last;

		// Sort coordinates by Y order (y2 >= y1 >= y0)
		if (y0 > y1) {
			swap(y0, y1); swap(x0, x1);
		}
		if (y1 > y2) {
			swap(y2, y1); swap(x2, x1);
		}
		if (y0 > y1) {
			swap(y0, y1); swap(x0, x1);
		}

		if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
			a = b = x0;
			if(x1 < a)      a = x1;
			else if(x1 > b) b = x1;
			if(x2 < a)      a = x2;
			else if(x2 > b) b = x2;
			drawFastHLine(a, y0, b-a+1, color);
			return;
		}

		int16_t
		dx01 = x1 - x0,
		dy01 = y1 - y0,
		dx02 = x2 - x0,
		dy02 = y2 - y0,
		dx12 = x2 - x1,
		dy12 = y2 - y1,
		sa   = 0,
		sb   = 0;

		// For upper part of triangle, find scanline crossings for segments
		// 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
		// is included here (and second loop will be skipped, avoiding a /0
		// error there), otherwise scanline y1 is skipped here and handled
		// in the second loop...which also avoids a /0 error here if y0=y1
		// (flat-topped triangle).
		if(y1 == y2) {
			last = y1;   // Include y1 scanline
		} else {
			last = y1-1; // Skip it
		}

		for(y = y0; y <= last; y++) {
			a   = x0 + sa / dy01;
			b   = x0 + sb / dy02;
			sa += dx01;
			sb += dx02;
			/* longhand:
			a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
			b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
			*/
			if(a > b) {
				swap(a,b);
			}
			drawFastHLine(a, y, b - a + 1, color);
		}

		// For lower part of triangle, find scanline crossings for segments
		// 0-2 and 1-2.  This loop is skipped if y1=y2.
		sa = dx12 * (y - y1);
		sb = dx02 * (y - y0);
		for(; y <= y2; y++) {
			a   = x1 + sa / dy12;
			b   = x0 + sb / dy02;
			sa += dx12;
			sb += dx02;
			/* longhand:
			a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
			b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
			*/
			if(a > b) {
				swap(a,b);
			}
			drawFastHLine(a, y, b-a+1, color);
		}
	}

	void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, const C &color) {
		drawFastHLine(x + r, y, w - 2 * r, color); // Top
		drawFastHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
		drawFastVLine(x, y + r , h - 2 * r, color); // Left
		drawFastVLine(x + w - 1, y + r, h - 2 * r, color); // Right
		// draw four corners
		drawCircleHelper(x + r, y + r, r, 1, color);
		drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
		drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
		drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
	}

	void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, const C &color) {
		fillRect(x + r, y, w - 2 * r, h, color);

		// draw four corners
		fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
		fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 	1, color);
	}

	void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, const C &color) {
		int16_t i, j, byteWidth = (w + 7) / 8;

		for(j = 0; j < h; j++) {
			for(i = 0; i < w; i++) {
				if(pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
					drawPixel(x + i, y + j, color);
				}
			}
		}
	}

	static int16_t getWidth(void)  { return W; }
	static int16_t getHeight(void) { return H; }

	// Text methods
	void setCursor(int16_t x, int16_t y) {
		cursor_x = x;
		line_start_x = x;
		cursor_y = y;
	}

	// get current cursor position (get rotation safe maximum values, using: width() for x, height() for y)
	int16_t getCursorX() const {
		return cursor_x;
	}

	int16_t getCursorY() const {
		return cursor_y;
	}

	void setTextColor(C color) {
		// Setting just a text color implies a transparent background, which is implied
		// by using the same color for background and foreground.
		text_color = text_bg_color = color;
	}
	void setTextColor(const C &foreground, const C &background) {
		text_color = foreground;
		text_bg_color = background;
	}

	void setTextSize(uint8_t new_size) {
		text_size = (new_size > 0) ? new_size : 1;
	}

	void setTextWrap(boolean _wrap) {
		wrap = _wrap;
	}

	void cp437(bool use_cp437 = true) {
		cp437 = use_cp437;
	}

	void setFont(const GFXfont &new_font) {
	    font = (GFXfont *)&new_font;
	}

	void drawText(const char *str, int16_t x, int16_t y, uint8_t align=ALIGN_LEFT) {
		uint8_t string_length = strlen(str);

		Rect bounds = getTextBounds(str, x, y);

		// Store previous settings for cursor and wrapping
		uint8_t prev_cursor_x = cursor_x;
		uint8_t prev_cursor_y = cursor_y;
		bool prevWrap = wrap;


		// Set cursor and disable wrapping
		switch(align) {
			case ALIGN_LEFT:
				cursor_x = x;
				break;
			case ALIGN_CENTER:
				cursor_x = x - bounds.w / 2 - (bounds.x - x);
				break;
			case ALIGN_RIGHT:
				cursor_x = x - bounds.w;
				break;
		}
		cursor_y = y;
		wrap = false;

		print(str);

		// Restore previous cursor and wrap setting
		cursor_x = prev_cursor_x;
		cursor_y = prev_cursor_y;
		wrap = prevWrap;
	}

	uint16_t getTextWidth(const char *str) {
		return getTextBounds(str, 0, 0).w;
	}

	// Pass string and a cursor position, returns UL corner and W,H.
	Rect getTextBounds(const char *str, int16_t x, int16_t y) {
		uint8_t c; // Current character
		GFXglyph *glyph;
		uint8_t first = font->first;
		uint8_t last  = font->last;
		int16_t min_x = W;
		int16_t min_y = H;
		int16_t max_x = -1;
		int16_t max_y = -1;

		// Bounding box coordinates for the current glyph
		int16_t glyph_x1 = 0;
		int16_t glyph_y1 = 0;
		int16_t glyph_x2 = 0;
		int16_t glyph_y2 = 0;

		while((c = *str++)) {
			if(c != '\n') { // Not a newline
				if((c == '\r') || (c < font->first) || (c > font->last)) {
					// Char not present in current font
					continue;
				}

				c -= font->first;
				glyph = &(font->glyph[c]);

				if(wrap && (x + (glyph->xOffset + glyph->width) * text_size >= W)) {
					// Line wrap
					x  = line_start_x;  // Reset x to 0
					y += font->yAdvance; // Advance y by 1 line
				}
				glyph_x1 = x + glyph->xOffset * text_size;
				glyph_y1 = y + glyph->yOffset * text_size;
				glyph_x2 = glyph_x1 + glyph->width * text_size - 1;
				glyph_y2 = glyph_y1 + glyph->height * text_size - 1;
				if(glyph_x1 < min_x) {
					min_x = glyph_x1;
				}
				if(glyph_y1 < min_y) {
					min_y = glyph_y1;
				}
				if(glyph_x2 > max_x) {
					max_x = glyph_x2;
				}
				if(glyph_y2 > max_y) {
					max_y = glyph_y2;
				}

				if ((font != &TomThumb) || *(str + 1)) {
					x += glyph->xAdvance * text_size;
				}
			} else { // Newline
				x  = line_start_x;  // Reset x
				y += font->yAdvance; // Advance y by 1 line
			}
		}

		return {min_x, min_y, max_x - min_x + 1, max_y - min_y + 1};
	}

	size_t write(uint8_t c) {
		if(!font) {
			return 1;
		}

		if(c == '\n') {
			cursor_x = line_start_x;
			cursor_y += (int16_t)text_size * (uint8_t)font->yAdvance;
		} else if(c != '\r') {
			uint8_t first = font->first;
			if((c >= first) && (c <= font->last)) {
				uint8_t c2 = c - font->first;
				GFXglyph *glyph = &(font->glyph[c2]);
				uint8_t w = glyph->width;
				uint8_t h = glyph->height;
				if((w > 0) && (h > 0)) { // Is there an associated bitmap?
					int16_t xo = glyph->xOffset; // sic
					if(wrap && ((cursor_x + text_size * (xo + w)) >= W)) {
						// Drawing character would go off right edge; wrap to new line
						cursor_x = line_start_x;
						cursor_y += (int16_t)text_size * font->yAdvance;
					}
					drawChar(cursor_x, cursor_y, c, text_color, text_bg_color, text_size);
				}
				cursor_x += glyph->xAdvance * (int16_t)text_size;
			}
		}

		return 1;
	}

	void drawChar(int16_t x, int16_t y, unsigned char c, C color, C bg, uint8_t size) {
		if (!font) {
			return;
		}
		// Character is assumed previously filtered by write() to eliminate
		// newlines, returns, non-printable characters, etc.  Calling drawChar()
		// directly with 'bad' characters of font may cause mayhem!

		c -= font->first;
		GFXglyph *glyph = &(font->glyph[c]);
		uint8_t *bitmap = font->bitmap;

		uint16_t bo = glyph->bitmapOffset;
		uint8_t w = glyph->width;
		uint8_t h = glyph->height;
		int8_t xo = glyph->xOffset;
		int8_t yo = glyph->yOffset;
		uint8_t xx = 0;
		uint8_t yy = 0;
		uint8_t bits = 0;
		uint8_t bit = 0;
		int16_t xo16 = 0;
		int16_t yo16 = 0;

		if(size > 1) {
			xo16 = xo;
			yo16 = yo;
		}

		for(yy=0; yy<h; yy++) {
			for(xx=0; xx<w; xx++) {
				if(!(bit++ & 7)) {
					bits = bitmap[bo++];
				}
				if(bits & 0x80) {
					if(size == 1) {
						drawPixel(x+xo+xx, y+yo+yy, color);
					} else {
						fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size, color);
					}
				}
				bits <<= 1;
			}
		}
	}

	// Yeah, this is somewhere between silly and crazy.
	// Suggestions on how to include the implementations that work without getting rid of MEMBER_REQUIRES are more than welcome.
	#include "ssd1351_highcolor.inl"
	#include "ssd1351_lowcolor.inl"
	#include "ssd1351_indexedcolor.inl"

	#include "ssd1351_nobuffer.inl"
	#include "ssd1351_singlebuffer.inl"

private:
	typedef std::array<C, W * H> ArrayType;

	MEMBER_REQUIRES(std::is_same<B, SingleBuffer>::value)
	__attribute__((always_inline)) ArrayType& frontBuffer() {
		static ArrayType buffer;
		return buffer;
	}

	// Pins
	uint8_t cs;
	uint8_t dc;
	uint8_t reset;
	uint8_t mosi;
	uint8_t sclk;

	// Magical registers (I think?) to make toggling DC pin super fast.
	uint8_t pcs_data, pcs_command;

	int16_t cursor_x;
	int16_t cursor_y;
	int16_t line_start_x;
	C text_color = black;
	C text_bg_color = black;
	uint8_t text_size = 1;
	bool wrap = true;   // If set, 'wrap' text at right edge of display
	bool _cp437 = false; // If set, use correct CP437 charset (default is off)
	GFXfont *font = (GFXfont *)&TomThumb;

	void __attribute__((always_inline)) setVideoRamPosition(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
		// Sets the active video RAM area of the display. After sending this command
		// (and sending the 'write to ram' command), color data can be sent do the display without
		// having to set the x/y address for each pixel. After each pixel, the display will internally
		// increment to point to the next pixel:
		// x0,y0 -> x0+1, y0, ..., x1,y0, x0,y0+1, x0+1,y0+1, ..., x1,y1

		sendCommandAndContinue(CMD_COLUMN_ADDRESS);
		sendDataAndContinue(x0);
		sendDataAndContinue(x1);
		sendCommandAndContinue(CMD_ROW_ADDRESS);
		sendDataAndContinue(y0);
		sendDataAndContinue(y1);
	}

	// ****
	// Low-level data pushing functions
	// ****
	void __attribute__((always_inline)) waitFifoNotFull() {
		uint32_t sr;
		uint32_t tmp __attribute__((unused));
		do {
			sr = KINETISK_SPI0.SR;
			if (sr & 0xF0) tmp = KINETISK_SPI0.POPR;  // drain RX FIFO
		} while ((sr & (15 << 12)) > (3 << 12));
	}

	void __attribute__((always_inline)) waitTransmitComplete(uint32_t mcr) {
		uint32_t tmp __attribute__((unused));
		while (1) {
			uint32_t sr = KINETISK_SPI0.SR;
			if (sr & SPI_SR_EOQF) break;  // wait for last transmit
			if (sr &  0xF0) tmp = KINETISK_SPI0.POPR;
		}
		KINETISK_SPI0.SR = SPI_SR_EOQF;
		SPI0_MCR = mcr;
		while (KINETISK_SPI0.SR & 0xF0) {
			tmp = KINETISK_SPI0.POPR;
		}
	}

	void __attribute__((always_inline)) sendCommandAndContinue(uint8_t command) {
		KINETISK_SPI0.PUSHR = command | (pcs_command << 16) | SPI_PUSHR_CTAS(0) | SPI_PUSHR_CONT;
		waitFifoNotFull();
	}
	void __attribute__((always_inline)) sendLastCommand(uint8_t command) {
		uint32_t mcr = SPI0_MCR;
		KINETISK_SPI0.PUSHR = command | (pcs_command << 16) | SPI_PUSHR_CTAS(0) | SPI_PUSHR_EOQ;
		waitTransmitComplete(mcr);
	}

	void __attribute__((always_inline)) sendDataAndContinue(uint8_t data) {
		KINETISK_SPI0.PUSHR = data | (pcs_data << 16) | SPI_PUSHR_CTAS(0) | SPI_PUSHR_CONT;
		waitFifoNotFull();
	}
	void __attribute__((always_inline)) sendLastData(uint8_t data) {
		uint32_t mcr = SPI0_MCR;
		KINETISK_SPI0.PUSHR = data | (pcs_data << 16) | SPI_PUSHR_CTAS(0) | SPI_PUSHR_EOQ;
		waitTransmitComplete(mcr);
	}
	void __attribute__((always_inline)) sendDataAndContinue16(uint16_t data) {
		KINETISK_SPI0.PUSHR = data | (pcs_data << 16) | SPI_PUSHR_CTAS(1) | SPI_PUSHR_CONT;
		waitFifoNotFull();
	}
	void __attribute__((always_inline)) sendLastData16(uint16_t data) {
		uint32_t mcr = SPI0_MCR;
		KINETISK_SPI0.PUSHR = data | (pcs_data << 16) | SPI_PUSHR_CTAS(1) | SPI_PUSHR_EOQ;
		waitTransmitComplete(mcr);
	}
};

}
