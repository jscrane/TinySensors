#include <SPI.h>
#include <Adafruit_GFX.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>
#include <SimpleTimer.h>

#include "configuration.h"
#include "dbg.h"
#include "label.h"
#include "rssi.h"
#include "stator.h"
#include "smoother.h"
#include "graph.h"

#define SWITCH  D3

TFT_eSPI tft;
Graph light(tft, "light %.0f-%.0f"), battery(tft, "batt %.1f-%.1fv"), temperature(tft, "temp %.0f-%.0fC"), humidity(tft, "humi %.0f-%.0f%%");
Graphs graphs({ &light, &battery, &temperature, &humidity });

#define SDA	D2
#define SCL	D1

MDNSResponder mdns;
WiFiClient wifiClient;
PubSubClient mqtt_client(wifiClient);

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
DNSServer dnsServer;

const unsigned bgcolor = TFT_NAVY, fgcolor = TFT_CYAN;
config cfg;
bool debugging;

static const char *config_file = "/config.json";
static const unsigned long UPDATE_RSSI = 500;
static const unsigned long SWITCH_INTERVAL = 1000;
static const unsigned long UPDATE_CONNECT = 500, CONNECT_TIME = 30000;

static RSSI rssi(tft, 5);
const int rssi_error = 31;

static Label status(tft), title(tft);
static SimpleTimer timers;
static int connectTimer;
static Stator<bool> swtch;

static uint16_t ly[2];

void ICACHE_RAM_ATTR switch_handler() { swtch = true; }

static void draw_rssi() {
	int r = WiFi.RSSI();
	if (r != rssi_error) {
		const int t[] = {-90, -80, -70, -67, -40};
		rssi.update(updater([r, t](int i)->bool { return r > t[i]; }));
	}
}

static void captive_portal() {
	WiFi.mode(WIFI_AP);
	if (WiFi.softAP(cfg.hostname)) {
		status.printf("SSID: %s", cfg.hostname);
		DBG(print(F("Connect to SSID: ")));
		DBG(print(cfg.hostname));
		DBG(println(F(" to configure WIFI")));
		dnsServer.start(53, "*", WiFi.softAPIP());
	} else {
		status.draw("Error starting softAP");
		ERR(println(F("Error starting softAP")));
	}
}

static void connecting() {
	if (WiFi.status() == WL_CONNECTED) {
		status.draw(cfg.ssid);
		timers.disable(connectTimer);
		timers.setInterval(UPDATE_RSSI, draw_rssi);
		return;
	}
	unsigned now = millis();
	if (now > CONNECT_TIME) {
		timers.disable(connectTimer);
		captive_portal();
		return;
	}
	const char busy[] = "|/-\\";
	int i = (now / UPDATE_RSSI);
	status.printf("Connecting %c", busy[i % 4]);
	rssi.update(updater([i](int b) { return i % 5 == b; }));
}

static bool mqtt_connect(PubSubClient &c) {
	if (c.connected())
		return true;
	if (c.connect(cfg.hostname)) {
		if (c.subscribe(cfg.stat_topic))
			return true;
		ERR(print(F("error subscribing: ")));
		ERR(println(cfg.stat_topic));
		return false;
	}
	ERR(print(F("MQTT connection to: ")));
	ERR(print(cfg.mqtt_server));
	ERR(print(F(" failed, rc=")));
	ERR(println(mqtt_client.state()));
	return false;
}

static void mqtt_callback(const char *topic, byte *payload, unsigned int length) {
	DynamicJsonDocument doc(JSON_OBJECT_SIZE(5) + 20);
	auto error = deserializeJson(doc, payload);
	if (error || !doc[F("i")])
		return;

	int id = doc[F("i")];
	if (id <= 0 || id >= NSENSORS)
		return;
	
	const char *n = strrchr(topic, '/');
	if (n) {
		const char *l = n+1;
		uint16_t x, y;
		if (id > 4) {
			y = ly[1];
			x = id - 5;
		} else {
			y = ly[0];
			x = id - 1;
		}
		x *= tft.textWidth(l, 1);
		tft.setTextColor(pgm_read_word(default_4bit_palette + id));
		tft.drawString(l, x, y, 1);
	}

	light.addReading(id, float(doc[F("l")]));
	battery.addReading(id, float(doc[F("b")]));
	temperature.addReading(id, float(doc[F("t")]));
	humidity.addReading(id, float(doc[F("h")]));
}

void setup() {
	Serial.begin(TERMINAL_SPEED);
	Serial.println(F("Booting!"));
	Serial.println(F(VERSION));

	bool result = LittleFS.begin();
	if (!result) {
		ERR(print(F("LittleFS: ")));
		ERR(println(result));
		return;
	}

	if (!cfg.read_file(config_file)) {
		ERR(print(F("config!")));
		return;
	}

	pinMode(SWITCH, INPUT_PULLUP);
	debugging = cfg.debug;

	tft.init();
	tft.setTextColor(fgcolor, bgcolor);
	tft.fillScreen(bgcolor);
	tft.setCursor(0, 0);
	tft.setRotation(3);

	int y = 1;
	status.setPosition(0, y);
	status.setColor(fgcolor, bgcolor);
	y += status.setFont(1);

	ly[0] = y;
	y += tft.fontHeight(1);
	ly[1] = y;
	y += tft.fontHeight(1);

	title.setPosition(0, y);
	title.setColor(fgcolor, bgcolor);
	y += title.setFont(1);

	light.setBounds(cfg.light.min, cfg.light.max);
	battery.setBounds(cfg.battery.min, cfg.battery.max);
	temperature.setBounds(cfg.temperature.min, cfg.temperature.max);
	humidity.setBounds(cfg.humidity.min, cfg.humidity.max);

	graphs.each([y](Graph *g) { g->setYO(y); });
	title.draw(graphs.curr()->show());

	rssi.setColor(TFT_WHITE, bgcolor);
	rssi.setBounds(tft.width() - 21, 0, 20, 20);

	WiFi.mode(WIFI_STA);
	WiFi.hostname(cfg.hostname);

	if (*cfg.ssid) {
		WiFi.setAutoReconnect(true);
		WiFi.begin(cfg.ssid, cfg.password);
	} else
		captive_portal();

	server.on("/config", HTTP_POST, []() {
		if (server.hasArg("plain")) {
			String body = server.arg("plain");
			File f = LittleFS.open(config_file, "w");
			f.print(body);
			f.close();
			server.send(200);
			ESP.restart();
		} else
			server.send(400, "text/plain", "No body!");
	});
	server.serveStatic("/", LittleFS, "/index.html");
	server.serveStatic("/config", LittleFS, config_file);
	server.serveStatic("/js/transparency.min.js", LittleFS, "/transparency.min.js");
	server.serveStatic("/info.png", LittleFS, "/info.png");

	httpUpdater.setup(&server);
	server.begin();

	mqtt_client.setServer(cfg.mqtt_server, 1883);
	mqtt_client.setCallback(mqtt_callback);

	if (mdns.begin(cfg.hostname, WiFi.localIP())) {
		DBG(println(F("mDNS started")));
		mdns.addService("http", "tcp", 80);
	} else
		ERR(println(F("Error starting mDNS")));

	attachInterrupt(digitalPinToInterrupt(SWITCH), switch_handler, RISING);

	timers.setInterval(cfg.light.refresh_interval, []() { light.update(); });
	timers.setInterval(cfg.battery.refresh_interval, []() { battery.update(); });
	timers.setInterval(cfg.temperature.refresh_interval, []() { temperature.update(); });
	timers.setInterval(cfg.humidity.refresh_interval, []() { humidity.update(); });
	connectTimer = timers.setInterval(UPDATE_CONNECT, connecting);
}

void loop() {
	mdns.update();
	server.handleClient();
	dnsServer.processNextRequest();
	timers.run();
	if (WiFi.status() == WL_CONNECTED && !mqtt_connect(mqtt_client))
		status.draw("MQTT connect failed");
	if (mqtt_client.connected())
		mqtt_client.loop();

	if (swtch && swtch.changedAfter(SWITCH_INTERVAL)) {
		graphs.curr()->hide();
		title.draw(graphs.next()->show());
	}
	swtch = false;
}
