#include <LittleFS.h>
#include <ArduinoJson.h>
#include "configuration.h"
#include "dbg.h"

bool Configuration::read_file(const char *filename) {
	File f = LittleFS.open(filename, "r");
	if (!f)
		return false;

	DynamicJsonDocument doc(4*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(9) + 250);
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

	JsonObject l = o[F("light")];
	light.configure(l);
	JsonObject b = o[F("battery")];
	battery.configure(b);
	JsonObject t = o[F("temperature")];
	temperature.configure(t);
	JsonObject h = o[F("humidity")];
	humidity.configure(h);
}

void graph_config::configure(JsonObject &o) {
	if (!o.isNull()) {
		refresh_interval = 1000 * (long)o[F("refresh_interval")];
		max = (float)o[F("max")];
		min = (float)o[F("min")];
	}
}
