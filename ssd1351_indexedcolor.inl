// Specific implementations for high-color mode
// This gets included from inside the template definition in ssd1351.h, which is crazy, but it's the only way I know to make this compile.

MEMBER_REQUIRES(std::is_same<C, IndexedColor>::value)
void setColorDepth() {
	// this is the same as high color mode for now.
	// This is because indexed colours should ultimately be able to send any colour - just only 256 different ones.
	// However, to make IndexedColor mode the fastest, I might make this use 2 bytes instead.
	sendCommandAndContinue(CMD_REMAP);
	sendDataAndContinue(0xB4);
};

MEMBER_REQUIRES(std::is_same<C, IndexedColor>::value)
void pushColor(const C &color, bool lastCommand=false) {
	// Send color in indexed color mode - the data gets sent as three bytes,
	// only using the low 6 bits for the color (for 18bit color in total)

	sendDataAndContinue((color & 0xE0) >> 2);
	sendDataAndContinue((color & 0x7C) << 1);
	if (lastCommand) {
		sendLastData((color & 0x3) << 4);
	} else {
		sendDataAndContinue((color & 0x3) << 4);
	}
};
