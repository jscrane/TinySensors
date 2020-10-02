#include <Arduino.h>
#include <TFT_eSPI.h>
#include "graph.h"

Graph::Graph(TFT_eSPI &tft, const char *n): sprite(&tft), t(n), width(tft.width()), height(tft.height()) {
	sprite.setColorDepth(4);
}

void Graph::setYO(unsigned yo) {
	yorg = yo;
	sprite.createSprite(width, height - yo);
}

void Graph::update() {
	unsigned sh = height - yorg - 1, sx = width - 1;
	for (int i = 1; i < NSENSORS; i++) {
		int y = sh * (1.0 - (vals[i] - min) / range);
		sprite.drawFastVLine(sx, y, 1, i);
	}

	if (showing)
		doShow();
	sprite.scroll(-1, 0);

	grid++;
	if (grid >= 10) {
		grid = 0;
		sprite.drawFastVLine(sx, 0, sh, 14);
	} else
		for (int p = 0; p <= sh; p += 10)
			sprite.drawPixel(sx, p, 14);
}

void Graph::doShow() {
	sprite.pushSprite(0, yorg);
}
