//#include <SPI.h>
#include <RF24.h>
#include <DHT.h>
#include <SoftwareSerial.h>

#include "tinysensor.h"
#include "wdt.h"

const uint8_t DHT_PIN = 2;
const uint8_t CE_PIN = 8, CS_PIN = 7;
const uint8_t RX_PIN = 9, TX_PIN = 10;

#ifdef DEBUG
SoftwareSerial serial(RX_PIN, TX_PIN);
#endif

RF24 radio(CE_PIN, CS_PIN);
DHT dht;

const uint8_t retry_count = 5;		// 250+5*15*250 = 19mS
const uint8_t retry_delay = 1;		// 1*250uS

void setup(void)
{
	pinMode(A1, INPUT_PULLUP);

	dht.setup(DHT_PIN);

#ifdef DEBUG
	serial.begin(TERMINAL_SPEED);
	serial.println(F("millis\tStatus\tHum\tTemp\tLight\tBattery\tTime"));
#endif

	SPI.begin();
	radio.begin();

	radio.setRetries(retry_delay, retry_count);
	radio.setDataRate(data_rate);
	radio.setCRCLength(crc_len);
	radio.setPALevel(power);
	radio.setChannel(channel);
	radio.setPayloadSize(sizeof(sensor_payload_t));
	radio.setAutoAck(true);
	radio.openWritingPipe(bridge_addr);
}

void loop(void)
{
	static uint32_t msgid;
	uint32_t start = millis();

	radio.powerUp();
	radio.stopListening();

	unsigned lsens = analogRead(A1);
	uint8_t light = 255 - lsens / 4;
	unsigned secs = lsens / 8 + 1;
	uint8_t batt = analogRead(A0) / 4;

	// millis() only counts time when the sketch is not sleeping
	dht.resetTimer();
	int16_t h = dht.getHumidity();
	int16_t t = dht.getTemperature();
	uint8_t status = dht.getStatus();

	sensor_payload_t payload = { millis(), msgid++, h, t, light, batt, status, NODE_ID };
	radio.write(&payload, sizeof(payload));
	radio.powerDown();

#ifdef DEBUG
	serial.print(millis() - start);
	serial.print('\t');
	serial.print(status);
	serial.print('\t');
	serial.print(h);
	serial.print('\t');
	serial.print(t);
	serial.print('\t');
	serial.print((int)light);
	serial.print('\t');
	serial.print(batt);
	serial.print('\t');
	serial.println(secs);
#endif

	wdt_sleep(secs);
}
