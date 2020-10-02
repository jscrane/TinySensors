#ifndef __GRAPH_H__
#define __GRAPH_H__

#define NSENSORS	10

class Graph {
public:
	Graph(TFT_eSPI &tft, const char *t);
	void setYO(unsigned yo);
	void setBounds(const float &mn, const float &mx) { min = mn; range = mx - mn; }
	void addReading(int sid, const float &val) { vals[sid] = val; }
	void update();
	const char *show() { showing = true; doShow(); return t; }
	void hide() { showing = false; }

private:
	TFT_eSprite sprite;
	const char *t;
	float min, range;
	const unsigned width, height;
	unsigned yorg, grid;
	float vals[NSENSORS];
	bool showing;
	void doShow();
};

typedef std::function<void(Graph*)> graph_updater;

class Graphs {
public:
	Graphs(std::initializer_list<Graph *> g): graphs(g), it(graphs.begin()) {}

	Graph *curr() { return *it; }
	Graph *next() { if (++it == graphs.end()) it = graphs.begin(); return *it; }

	void each(graph_updater f) { 
		for (std::initializer_list<Graph*>::const_iterator g = graphs.begin(); g < graphs.end(); g++)
			f(*g);
	}
private:
	std::initializer_list<Graph*> graphs;
	std::initializer_list<Graph*>::const_iterator it;
};
#endif
