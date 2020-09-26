#ifndef __LABEL_H__
#define __LABEL_H__

class TFT_eSPI;

class Label {
public:
	Label(TFT_eSPI &tft): _tft(tft) {}

	void setPosition(int16_t x, int16_t y) { _x = x; _y = y; }

	int16_t setFont(int16_t f);

	void setColor(int16_t fg, int16_t bg) { _fg = fg; _bg = bg; }

	void draw(const char *s);

	void printf(const char *fmt, ...);
private:
	int16_t _x, _y, _pad, _h;
	int16_t _fg, _bg, _f;
	TFT_eSPI &_tft;
};

#endif
