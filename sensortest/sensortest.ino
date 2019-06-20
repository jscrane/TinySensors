#include <SPI.h>
#include <RF24.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>
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

	serial.println(F("Status\tHumidity\tTemperature\tLight\tBattery"));
}

void loop(void)
{
	uint8_t light = 255 - analogRead(A1) / 4;

	serial.print(dht.getStatus());
	serial.print(F("\t"));
	serial.print(dht.getHumidity());
	serial.print(F("\t\t"));
	serial.print(dht.getTemperature());
	serial.print(F("\t\t"));
	serial.print((int)light);
	serial.print(F("\t"));
	serial.println(analogRead(A0));

	wdt_sleep(WDTO_1S, 5);
	//delay(dht.getMinimumSamplingPeriod());
}
