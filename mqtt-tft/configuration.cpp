#include <LittleFS.h>
#include <ArduinoJson.h>
#include "configuration.h"
#include "dbg.h"

bool Configuration::read_file(const char *filename) {
	File f = LittleFS.open(filename, "r");
	if (!f)
		return false;

	DynamicJsonDocument doc(JSON_OBJECT_SIZE(7) + 200);
	auto error = deserializeJson(doc, f);
	f.close();
	if (error) {
		ERR(print(F("config: ")));
		ERR(println(error.c_str()));
		return false;
	}

	configure(doc);
	return true;
}

void config::configure(JsonDocument &o) {
	strlcpy(ssid, o[F("ssid")] | "", sizeof(ssid));
	strlcpy(password, o[F("password")] | "", sizeof(password));
	strlcpy(hostname, o[F("hostname")] | "", sizeof(hostname));
	strlcpy(mqtt_server, o[F("mqtt_server")] | "", sizeof(mqtt_server));
	strlcpy(stat_topic, o[F("stat_topic")] | "", sizeof(stat_topic));
	refresh_interval = 1000 * (long)o[F("refresh_interval")];
	debug = o[F("debug")] | false;
}
