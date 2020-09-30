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

#define SWITCH  D3

TFT_eSPI tft;
TFT_eSprite graph(&tft);

#define SDA	D2
#define SCL	D1

MDNSResponder mdns;
WiFiClient wifiClient;
PubSubClient mqtt_client(wifiClient);

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
DNSServer dnsServer;

bool debugging;
unsigned bgcolor;
const unsigned fgcolor = TFT_CYAN;
config cfg;

static const char *config_file = "/config.json";
static const unsigned long UPDATE_RSSI = 500, UPDATE_VI = 250, SAMPLE_VI = 50;
static const unsigned long SWITCH_INTERVAL = 1000;
static const unsigned long UPDATE_CONNECT = 500, CONNECT_TIME = 30000;

static RSSI rssi(tft, 5);
const int rssi_error = 31;
const size_t N = UPDATE_VI / SAMPLE_VI;

static Smoother<N> shunt_mV, bus_V, current_mA, power_mW;
static Label status(tft), bus(tft), shunt(tft), current(tft), debug(tft);
static SimpleTimer timers;
static int connectTimer, spriteY, spriteH, spriteX;

static Stator<bool> swtch;

void ICACHE_RAM_ATTR switch_handler() { swtch = true; }

static void draw_rssi() {
	int r = WiFi.RSSI();
	if (r != rssi_error) {
		const int t[] = {-90, -80, -70, -67, -40};
		rssi.update(updater([r, t](int i)->bool { return r > t[i]; }));
	}
}

typedef struct sensor {
	char name[5];
	float temp, humi, batt;
	int id, light;
} sensor_t;

sensor_t sensors[10];
const int num_sensors = sizeof(sensors) / sizeof(sensor_t);
static int grid = 0;

static void update_display() {

	for (int i = 1; i < num_sensors; i++) {
		sensor_t &s = sensors[i];
		if (s.id) {
			int y = spriteH * (1.0 - float(s.light) / (cfg.light.max - cfg.light.min));
			graph.drawFastVLine(spriteX, y, 1, i);
		}
	}

	graph.pushSprite(0, spriteY);
	graph.scroll(-1, 0);

	grid++;
	if (grid >= 10) {
		grid = 0;
		graph.drawFastVLine(spriteX, 0, spriteH, 14);
	} else
		for (int p = 0; p <= spriteH; p += 10) 
			graph.drawPixel(spriteX, p, 14);
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
	if (id <= 0 || id >= num_sensors)
		return;
	
	sensor_t &s = sensors[id];
	s.id = id;
	s.temp = doc[F("t")];
	s.humi = doc[F("h")];
	s.light = doc[F("l")];
	s.batt = doc[F("b")];
	const char *n = strrchr(topic, '/');
	if (n)
		strncpy(s.name, n+1, sizeof(s.name));
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
	bgcolor = debugging? TFT_RED: TFT_NAVY;

	tft.init();
	tft.setTextColor(fgcolor, bgcolor);
	tft.fillScreen(bgcolor);
	tft.setCursor(0, 0);
	tft.setRotation(3);

	int y = 1;
	status.setPosition(0, y);
	status.setColor(fgcolor, bgcolor);
	y += status.setFont(1);

	bus.setPosition(0, y);
	bus.setColor(fgcolor, bgcolor);
	y += bus.setFont(1);

	shunt.setPosition(0, y);
	shunt.setColor(fgcolor, bgcolor);
	y += shunt.setFont(1);

	current.setPosition(0, y);
	current.setColor(fgcolor, bgcolor);
	y += current.setFont(1);

	debug.setPosition(0, y);
	debug.setColor(fgcolor, bgcolor);
	y += debug.setFont(1);
	spriteY = y;
	spriteX = tft.width() - 1;
	spriteH = tft.height() - spriteY - 1;

	graph.setColorDepth(4);
	graph.createSprite(tft.width(), tft.height() - spriteY);

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

	timers.setInterval(cfg.light.refresh_interval, update_display);
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
		// FIXME: switch pressed: cycle between graphs
	}
	swtch = false;
}
