#include <ArduinoJson.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <SPI.h>
#include <RF24.h>
#include <tinysensor.h>

#include "Configuration.h"

MDNSResponder mdns;
WiFiClient wifiClient;
WiFiServer *wifiServer;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
DNSServer dnsServer;

class config: public Configuration {
public:
	char ssid[33];
	char password[33];
	char hostname[17];
	uint16_t listen_port;
	rf24_datarate_e data_rate;
	rf24_crclength_e crc_len;
	rf24_pa_dbm_e power;
	uint8_t channel;
	uint8_t node_id;

	void configure(JsonDocument &doc);
} cfg;

void config::configure(JsonDocument &doc) {
	strlcpy(ssid, doc[F("ssid")] | "", sizeof(ssid));
	strlcpy(password, doc[F("password")] | "", sizeof(password));
	strlcpy(hostname, doc[F("hostname")] | "", sizeof(hostname));
	listen_port = doc[F("listen_port")] | LISTEN_PORT;
	data_rate = (rf24_datarate_e)(int)doc[F("data_rate")];
	crc_len = (rf24_crclength_e)(int)doc[F("crc_len")];
	power = (rf24_pa_dbm_e)(int)doc[F("power")];
	channel = doc[F("channel")] | ::channel;
	node_id = doc[F("node_id")];
}

bool connected;
const char *config_file = "/config.json";
RF24 radio(D3, D8);

void setup() {
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	Serial.begin(TERMINAL_SPEED);
	Serial.println(ESP.getResetInfo().c_str());

	bool result = SPIFFS.begin();
	if (!result) {
		Serial.print(F("SPIFFS: "));
		Serial.println(result);
		for(;;);
	}

	if (!cfg.read_file(config_file)) {
		Serial.print(F("config!"));
		for(;;);
	}

	WiFi.mode(WIFI_STA);
	WiFi.hostname(cfg.hostname);
	if (*cfg.ssid) {
		const char s[] = "|/-\\";
		WiFi.setAutoReconnect(true);
		WiFi.begin(cfg.ssid, cfg.password);
		for (int i = 0; i < 60 && WiFi.status() != WL_CONNECTED; i++) {
			delay(500);
			Serial.print(s[i % 4]);
			Serial.print('\r');
		}
		connected = WiFi.status() == WL_CONNECTED;
		Serial.println();
	}

	server.on("/config", HTTP_POST, []() {
		if (server.hasArg("plain")) {
			String body = server.arg("plain");
			File f = SPIFFS.open(config_file, "w");
			f.print(body);
			f.close();
			server.send(200);
			ESP.restart();
		} else
			server.send(400, "text/plain", "No body!");
	});
	server.serveStatic("/", SPIFFS, "/index.html");
	server.serveStatic("/config", SPIFFS, config_file);
	server.serveStatic("/js/transparency.min.js", SPIFFS, "/transparency.min.js");
	server.serveStatic("/info.png", SPIFFS, "/info.png");

	httpUpdater.setup(&server);
	server.begin();

	if (mdns.begin(cfg.hostname, WiFi.localIP())) {
		Serial.println(F("mDNS started"));
		mdns.addService("http", "tcp", 80);
	} else
		Serial.println(F("Error starting MDNS"));

	if (connected) {
		Serial.print(F("Connected to "));
		Serial.println(cfg.ssid);
		Serial.println(WiFi.localIP());
		wifiServer = new WiFiServer(cfg.listen_port);
		wifiServer->begin();
	} else {
		WiFi.softAP(cfg.hostname);
		Serial.print(F("Connect to SSID: "));
		Serial.print(cfg.hostname);
		Serial.println(F(" to configure WIFI"));
		dnsServer.start(53, "*", WiFi.softAPIP());
	}

	SPI.begin();
	radio.begin();
	radio.setAutoAck(true);
	radio.setDataRate(cfg.data_rate);
	radio.setCRCLength(cfg.crc_len);
	radio.setPALevel(cfg.power);
	radio.setChannel(cfg.channel);
	radio.setPayloadSize(sizeof(sensor_payload_t));
	radio.powerUp();

	Serial.printf("Channel: %d\r\n", radio.getChannel());
	Serial.printf("Data-Rate: %d\r\n", radio.getDataRate());
	Serial.printf("Payload: %d\r\n", radio.getPayloadSize());
	Serial.printf("Power: %d\r\n", radio.getPALevel());
	Serial.printf("CRC: %d\r\n", radio.getCRCLength());

	radio.openReadingPipe(1, bridge_addr);
	radio.startListening();
	radio.printDetails();
}

void loop() {

	mdns.update();
	server.handleClient();

	while (radio.available()) {
		digitalWrite(LED_BUILTIN, LOW);
		sensor_payload_t payload;
		radio.read(&payload, sizeof(payload));

		// fixup for negative temperature
		short temp = payload.temperature;
		if ((temp >> 8) == 0x7f)
			temp = temp - 32768;

		float temperature = ((float)temp) / 10;
		float humidity = ((float)payload.humidity) / 10;
		float battery = ((float)payload.battery) * 3.3 / 255.0;

		char buf[128];
		int n = snprintf(buf, sizeof(buf),
                        ",%d,0,%d,%3.1f,%3.1f,%4.2f,%u,%u,%u,",
                        payload.node_id, payload.light,
                        temperature, humidity, battery, payload.status,
                        payload.id, payload.ms);

		Serial.println(buf);
		if (wifiClient)
			wifiClient.println(buf);
		digitalWrite(LED_BUILTIN, HIGH);
	}

	if (!connected) {
		dnsServer.processNextRequest();
		return;
	}

	if (!wifiClient)
		wifiClient = wifiServer->available();
}
