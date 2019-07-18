#include <SPI.h>
#include <RF24.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include "wdt.h"

const uint8_t DHT_PIN = 2;
const uint8_t RX_PIN = 9, TX_PIN = 10;
const uint8_t CE_PIN = 8, CS_PIN = 7;

RF24 radio(CE_PIN, CS_PIN);
DHT dht;
SoftwareSerial serial(RX_PIN, TX_PIN);
uint8_t node_id;

void setup(void)
{
	pinMode(RX_PIN, INPUT);
	pinMode(TX_PIN, OUTPUT);

	pinMode(A1, INPUT_PULLUP);

	dht.setup(DHT_PIN);

	SPI.begin();
	radio.begin();

	serial.begin(TERMINAL_SPEED);
	serial.print(F("retries: "));
	serial.println(radio.getRetries(), 16);
	serial.print(F("data rate: "));
	serial.println(radio.getDataRate());

	serial.println(F("millis\tStatus\tHum\tTemp\tLight\tBattery\tTime"));
}

// average time to read sensors 5.5ms:
// t n
// 4 4
// 5 1
// 6 7
// 7 2
void loop(void)
{
	uint32_t start = millis();

	unsigned lsens = analogRead(A1);
	unsigned batt = analogRead(A0);
	uint8_t light = 255 - lsens / 4;
	unsigned secs = lsens / 8 + 1;

	dht.resetTimer();
	int16_t h = dht.getHumidity();
	int16_t t = dht.getTemperature();

	serial.print(millis() - start);
	serial.print('\t');
	serial.print(dht.getStatus());
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

	wdt_sleep(secs);
}
