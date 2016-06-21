// Specific implementations for high-color mode
// This gets included from inside the template definition in ssd1351.h, which is crazy, but it's the only way I know to make this compile.

MEMBER_REQUIRES(std::is_same<C, HighColor>::value)
void setColorDepth() {
	sendCommandAndContinue(CMD_REMAP);
	sendDataAndContinue(0xB4);
};

MEMBER_REQUIRES(std::is_same<C, HighColor>::value)
void pushColor(const C &color, bool lastCommand=false) {
	// Send color in high color mode - the data gets sent as three bytes,
	// only using the low 6 bits for the color (for 18bit color in total)
	sendDataAndContinue(color.r >> 2);
	sendDataAndContinue(color.g >> 2);
	if (lastCommand) {
		sendLastData(color.b >> 2);
	} else {
		sendDataAndContinue(color.b >> 2);
	}
};
