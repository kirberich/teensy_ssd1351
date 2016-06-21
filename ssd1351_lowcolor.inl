// Specific implementations for high-color mode
// This gets included from inside the template definition in ssd1351.h, which is crazy, but it's the only way I know to make this compile.

MEMBER_REQUIRES(std::is_same<C, LowColor>::value)
void setColorDepth() {
	sendCommandAndContinue(CMD_REMAP);
	sendDataAndContinue(0x74);
}

// push color for Low color mode
MEMBER_REQUIRES(std::is_same<C, LowColor>::value)
void pushColor(const C &color, bool lastCommand=false) {
	// Send color in low/indexed color mode - data gets sent as two bytes
	// The RGB class takes care of the conversion automatically.
	if (lastCommand) {
		sendLastData16(color);
	} else {
		sendDataAndContinue16(color);
	}
}
