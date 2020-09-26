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

class config: public Configuration {
public:
	char ssid[NETWORK_LEN];
	char password[NETWORK_LEN];
	char hostname[NETWORK_LEN];
	char mqtt_server[NETWORK_LEN];
	char stat_topic[TOPIC_LEN];
	unsigned refresh_interval;
	bool debug;

	void configure(JsonDocument &doc);
};

#endif
