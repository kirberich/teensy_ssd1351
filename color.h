#pragma once
#include <Arduino.h>

namespace ssd1351 {

	// r = new Color();
	// r.A = 1 - (1 - fg.A) * (1 - bg.A);
	// if (r.A < 1.0e-6) return r; // Fully transparent -- R,G,B not important
	// r.R = fg.R * fg.A / r.A + bg.R * bg.A * (1 - fg.A) / r.A;
	// r.G = fg.G * fg.A / r.A + bg.G * bg.A * (1 - fg.A) / r.A;
	// r.B = fg.B * fg.A / r.A + bg.B * bg.A * (1 - fg.A) / r.A;

typedef uint8_t IndexedColor;
typedef	uint16_t LowColor;
struct HighColor {
	uint8_t r : 6;
	uint8_t g : 6;
	uint8_t b : 6;
	uint8_t a : 6;

	HighColor() : a(63) {}
	HighColor(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 63) : r(_r), g(_g), b(_b), a(_a) {}

	HighColor& operator+=(const HighColor& rhs) {
		if (rhs.a == 63) {
			r = rhs.r;
			g = rhs.g;
			b = rhs.b;
			a = rhs.a;
			return *this;
		}
		uint16_t newAlpha = 63 - ((63 - a) * (63 - rhs.a) >> 6);

		// uint8_t inverseAlpha = 63 - rhs.a;

		// r = (r & inverseAlpha) + (rhs.r & rhs.a);
		// g = (g & inverseAlpha) + (rhs.g & rhs.a);
		// b = (b & inverseAlpha) + (rhs.b & rhs.a);
		// a = 63;

		r = (rhs.r * rhs.a) / newAlpha + (r * a) * (63 - rhs.a) / newAlpha;
		g = (rhs.g * rhs.a) / newAlpha + (g * a) * (63 - rhs.a) / newAlpha;
		b = (rhs.b * rhs.a) / newAlpha + (b * a) * (63 - rhs.a) / newAlpha;
		a = newAlpha;


		return *this;
	}
};

struct RGB {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 255;

	uint8_t __attribute__((always_inline)) clamp(int16_t val) {
		val = val > 255 ? 255 : val;
		return val < 0 ? 0 : val;
	}

	RGB(const LowColor encoded) {
		r = (encoded & 0xf800) >> 8;
		g = (encoded & 0x7E0) >> 3;
		b = (encoded & 0x1F) << 3;
	}

	RGB(const HighColor &c) {
		r = c.r << 2;
		g = c.g << 2;
		b = c.b << 2;
		a = c.a << 2;
	}

	RGB(int16_t _r = 0, int16_t _g = 0, int16_t _b = 0,  int16_t _a = 255) {
		r = clamp(_r);
		g = clamp(_g);
		b = clamp(_b);
		a = clamp(_a);
	}

	operator HighColor() const {
		return HighColor(r >> 2, g >> 2, b >> 2, a >> 2);
	}

	operator LowColor() const {
		// Encode 3 byte colour into two byte (5r/6g/5b) color
		// Masks the last three bits of red/blue, last two bits of green
		// and shifts colours up to make RRRR RGGG GGGB BBBB
		return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
	}

	operator IndexedColor() const {
		return (r & 0xE0) | ((g & 0xE0) >> 3) | (b >> 6);
	}
};
}
