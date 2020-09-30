#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

class Configuration {
public:
	bool read_file(const char *filename);

protected:
	virtual void configure(class JsonDocument &doc) = 0;
};

#define NETWORK_LEN     33
#define TOPIC_LEN       65

class graph_config {
public:
	float max, min;
	unsigned refresh_interval;

	void configure(JsonObject &o);
};

class config: public Configuration {
public:
	char ssid[NETWORK_LEN];
	char password[NETWORK_LEN];
	char hostname[NETWORK_LEN];
	char mqtt_server[NETWORK_LEN];
	char stat_topic[TOPIC_LEN];
	bool debug;

	struct graph_config light, battery, temperature, humidity;

	void configure(JsonDocument &doc);
};

#endif
