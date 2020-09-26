#include <stdarg.h>
#include <TFT_eSPI.h>
#include "label.h"

int16_t Label::setFont(int16_t f) { 
	_f = f; 
	_h = _tft.fontHeight(f); 
	return _h; 
}

void Label::draw(const char *s) {
	_tft.setCursor(_x, _y);
	_tft.setTextFont(_f);
	_tft.setTextColor(_fg, _bg);
	int16_t x = _tft.drawString(s, _x, _y);
	if (_pad > x)
		_tft.fillRect(x, _y, _pad - x, _h, _bg);
	_pad = x;
}

void Label::printf(const char *fmt, ...) {
	char buf[32];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	draw(buf);
}
