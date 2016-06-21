// Specific implementations for high-color mode
// This gets included from inside the template definition in ssd1351.h, which is crazy, but it's the only way I know to make this compile.

MEMBER_REQUIRES(std::is_same<B, SingleBuffer>::value)
void drawPixel(int16_t x, int16_t y, const C &color) {
	// Single-buffered pixel drawing is trivial: just put the color in the buffer.
	if((x < 0) || (x >= W) || (y < 0) || (y >= H)) {
		return;
	}

	frontBuffer()[x + (W * y)] = color;
}

MEMBER_REQUIRES(std::is_same<B, SingleBuffer>::value)
void updateScreen() {
	// Updating the screen in single buffer mode means first setting the video ram
	// to the entire display, and then pushing out every pixel of the buffer.
	// The display automatically increments its internal pointer to point to the next pixel.
	SPI.beginTransaction(spi_settings);
	setVideoRamPosition(0, 0, W - 1, H - 1);
	sendCommandAndContinue(CMD_WRITE_TO_RAM);

	ArrayType &buffer = frontBuffer();
	for (int16_t i = 0; i < W * H; i++) {
		if (i % W) {
			pushColor(buffer[i]);
		} else {
			// Once every row, start a new SPI transaction to give other devices a chance to communicate.
			pushColor(buffer[i], true);
			SPI.endTransaction();
			SPI.beginTransaction(spi_settings);
		}
	}
	SPI.endTransaction();
}

MEMBER_REQUIRES(std::is_same<B, SingleBuffer>::value)
void fillScreen(const C &color) {
	// just writing to every pixel in the buffer is fast, but std::fill is faster.
	std::fill(frontBuffer().begin(), frontBuffer().end(), color);
}

MEMBER_REQUIRES(std::is_same<B, SingleBuffer>::value)
void drawFastVLine(int16_t x, int16_t y, int16_t h, const C &color) {
	// Trying to optimize line drawing in buffered mode is pretty pointless, it's stupid fast anyway.
	drawLine(x, y, x, y + h - 1, color);
}

MEMBER_REQUIRES(std::is_same<B, SingleBuffer>::value)
void drawFastHLine(int16_t x, int16_t y, int16_t w, const C &color) {
	// Trying to optimize line drawing in buffered mode is pretty pointless, it's stupid fast anyway.
	drawLine(x, y, x + w - 1, y, color);
}

MEMBER_REQUIRES(std::is_same<B, SingleBuffer>::value)
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, const C &color) {
	if((x >= W) || (y >= H)) {
		return;
	}
	if (x < 0) {
		w -= abs(x);
		x = 0;
	}
	if (y < 0) {
		y -= abs(y);
		y = 0;
	}
	if((x + w - 1) >= W) {

		w = W - x;
	}
	if((y + h - 1) >= H) {
		h = H - y;
	}

	ArrayType &buffer = frontBuffer();
	for(int _y = y; _y < (y + h); _y++) {
		for(int _x = x; _x < (x + w); _x++) {
			buffer[_x + (W * _y)] = color;
		}
	}
}

MEMBER_REQUIRES(std::is_same<B, SingleBuffer>::value)
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const C &color) {
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap(x0, y0);
		swap(x1, y1);
	}

	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
	} else {
		ystep = -1;
	}

	for (; x0<=x1; x0++) {
		if (steep) {
			drawPixel(y0, x0, color);
		} else {
			drawPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0) {
			y0 += ystep;
			err += dx;
		}
	}
}
