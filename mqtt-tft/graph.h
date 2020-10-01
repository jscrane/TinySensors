#ifndef __GRAPH_H__
#define __GRAPH_H__

#define NSENSORS	10

class Graph {
public:
	Graph(TFT_eSPI &tft);
	void setYO(unsigned yo);
	void setBounds(const float &mn, const float &mx) { min = mn; max = mx; }
	void addReading(int sid, const float &val) { vals[sid] = val; }
	void update();

private:
	TFT_eSprite sprite;
	float min, max;
	const unsigned width, height;
	unsigned yorg, grid;
	float vals[NSENSORS];
};

#endif
