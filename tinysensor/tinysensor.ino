#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <EEPROM.h>
#include <DHT.h>

#include "tinysensor.h"
#include "sleepy.h"

const uint8_t CE_PIN = 8;
const uint8_t CS_PIN = 7;
const uint8_t DHT_PIN = 2;

RF24 radio(CE_PIN, CS_PIN);
RF24Network network(radio);
DHT dht;

const uint8_t channel = 90;
const uint16_t master_node = 0;
uint16_t this_node;
const uint8_t retry_count = 5;		// 250+5*15*250 = 19mS
const uint8_t retry_delay = 15;		// 15*250uS

const uint32_t interval = 60000;
const uint32_t min_interval = 15000;

void setup(void)
{
	pinMode(A1, INPUT_PULLUP);

	dht.setup(DHT_PIN);

	SPI.begin();
	radio.begin();

	radio.enableDynamicPayloads();
	radio.setRetries(retry_delay, retry_count);

	this_node = EEPROM.read(0);
	network.begin(channel, this_node);
}

ISR(WDT_vect) { Sleepy::watchdogEvent(); }

void loop(void)
{
	radio.powerUp();
	network.update();

	uint8_t light = 255 - analogRead(A1) / 4;
	uint16_t battery = analogRead(A0);

	sensor_payload_t payload = { millis(), light, dht.getStatus(), dht.getHumidity(), dht.getTemperature(), battery };
	RF24NetworkHeader header(master_node, sensor_type_id);
	bool ok = network.write(header, &payload, sizeof(payload));

	radio.powerDown();

	uint32_t l = light;
	uint32_t s = 1000 * (255 - l) / 3;
	if (s < min_interval)
		s = min_interval;
	do {
		uint32_t i = min(s, interval);
		Sleepy::loseSomeTime(i);
		s -= i;
	} while (s > 0);
}
