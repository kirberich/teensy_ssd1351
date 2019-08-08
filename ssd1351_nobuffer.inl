// Specific implementations for unbuffered mode.
// This gets included from inside the template definition in ssd1351.h, which is crazy, but it's the only way I know to make this compile.

MEMBER_REQUIRES(std::is_same<B, NoBuffer>::value)
void drawPixel(int16_t x, int16_t y, const C &color) {
	// Drawing pixels directly to the display requires first setting the correct
	// video ram position, from x/y to x+1/y+1.
	if(x < 0 || x >= W || y < 0 || y >= H) {
		return;
	}

	beginSPITransaction();
	setVideoRamPosition(x, y, x+1, y+1);
	sendCommandAndContinue(CMD_WRITE_TO_RAM);
	pushColor(color, true);
	endSPITransaction();
}

MEMBER_REQUIRES(std::is_same<B, NoBuffer>::value)
void updateScreen() {
	// An unbuffered display doesn't need to update its screen, but we want to
	// supply a common interface for all operational modes.
}

MEMBER_REQUIRES(std::is_same<B, NoBuffer>::value)
void fillScreen(const C &color) {
	// Instead of drawing each pixel to the screen with the same color, we make
	// use of the fact that fillRect is optimized to only incur a single overhead
	// for addressing a cordinate on the display
	fillRect(0, 0, W, H, color);
}

MEMBER_REQUIRES(std::is_same<B, NoBuffer>::value)
void drawFastVLine(int16_t x, int16_t y, int16_t h, const C &color) {
	// Setting the video ram on the display to only contain a single constrained
	// column of data allows us to write vertical lines super fast.
	// The x/y position is only set to the start, the display then takes care of
	// pointing to the next pixel after the first is written.
	if((x >= W) || (y >= H)) {
		return;
	}
	if((y + h - 1) >= H) {
		h = H - y;
	}
	beginSPITransaction();
	setVideoRamPosition(x, y, x, y + h - 1);
	sendCommandAndContinue(CMD_WRITE_TO_RAM);
	while (h-- > 1) {
		pushColor(color);
	}
	pushColor(color, true);
	endSPITransaction();
}

MEMBER_REQUIRES(std::is_same<B, NoBuffer>::value)
void drawFastHLine(int16_t x, int16_t y, int16_t w, const C &color) {
	// Rudimentary clipping
	if((x >= W) || (y >= H)) {
		return;
	}
	if((x + w - 1) >= W) {
		w = W - x;
	}
	beginSPITransaction();
	setVideoRamPosition(x, y, x + w - 1, y);
	sendCommandAndContinue(CMD_WRITE_TO_RAM);
	while (w-- > 1) {
		pushColor(color);
	}
	pushColor(color, true);
	endSPITransaction();
}

MEMBER_REQUIRES(std::is_same<B, NoBuffer>::value)
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, const C &color) {
	// rudimentary clipping (drawChar w/big text requires this)
	if((x >= W) || (y >= H)) {
		return;
	}
	if((x + w - 1) >= W) {
		w = W - x;
	}
	if((y + h - 1) >= H) {
		h = H - y;
	}

	beginSPITransaction();
	setVideoRamPosition(x, y, x + w - 1, y + h - 1);
	sendCommandAndContinue(CMD_WRITE_TO_RAM);
	for(y = h; y > 0; --y) {
		for(x = w; x > 1; --x) {
			pushColor(color);
		}
		pushColor(color, true);
		// At the end of every row, end the transaction to give other SPI devices a chance to communicate.
		endSPITransaction();

		// Start a new transaction, unless this is the last row
		if (y) {
			beginSPITransaction();
		}
	}
}

MEMBER_REQUIRES(std::is_same<B, NoBuffer>::value)
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const C &color) {
	// Bresenham's algorithm - thx wikpedia
	if (y0 == y1) {
		if (x1 > x0) {
			drawFastHLine(x0, y0, x1 - x0 + 1, color);
		} else if (x1 < x0) {
			drawFastHLine(x1, y0, x0 - x1 + 1, color);
		} else {
			drawPixel(x0, y0, color);
		}
		return;
	} else if (x0 == x1) {
		if (y1 > y0) {
			drawFastVLine(x0, y0, y1 - y0 + 1, color);
		} else {
			drawFastVLine(x0, y1, y0 - y1 + 1, color);
		}
		return;
	}

	bool steep = abs(y1 - y0) > abs(x1 - x0);
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

	int16_t xbegin = x0;
	if (steep) {
		for (; x0<=x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					drawFastVLine(y0, xbegin, len + 1, color);
				} else {
					drawPixel(y0, x0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			drawFastVLine(y0, xbegin, x0 - xbegin, color);
		}

	} else {
		for (; x0<=x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					drawFastHLine(xbegin, y0, len + 1, color);
				} else {
					drawPixel(x0, y0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			drawFastHLine(xbegin, y0, x0 - xbegin, color);
		}
	}
}
