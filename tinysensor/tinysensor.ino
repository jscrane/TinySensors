#include <avr/power.h>
#include <SPI.h>
#include <RF24.h>
#include <DHT.h>
#include <SoftwareSerial.h>

#include "tinysensor.h"
#include "wdt.h"

const uint8_t DHT_PIN = PIN_PA2;
const uint8_t CE_PIN = PIN_PB2, CS_PIN = PIN_PA7;
const uint8_t RX_PIN = PIN_PB1, TX_PIN = PIN_PB0;
const uint8_t LIGHT_PIN = A1, BATTERY_PIN = A0;

#if defined(DEBUG)
#pragma message "debugging"
SoftwareSerial serial(RX_PIN, TX_PIN);
#endif

RF24 radio(CE_PIN, CS_PIN);
DHT dht;

const uint8_t retry_count = 5;		// 250+5*15*250 = 19mS
const uint8_t retry_delay = 1;		// 1*250uS
const rf24_pa_dbm_e power = RF24_PA_LOW;

void setup(void)
{
	power_timer1_disable();

	dht.setup(DHT_PIN);

	pinMode(LIGHT_PIN, INPUT_PULLUP);

#if defined(DEBUG)
	serial.begin(TERMINAL_SPEED);
	serial.println(F("Time\tnRF\tDHT\tADC\tStatus\tHum\tTemp\tLight\tBattery\tSleep"));
#else
	pinMode(RX_PIN, INPUT_PULLUP);
	pinMode(TX_PIN, INPUT_PULLUP);
#endif

	SPI.begin();
	radio.begin();

	radio.setRetries(retry_delay, retry_count);
	radio.setDataRate(data_rate);
	radio.setCRCLength(crc_len);
	radio.setPALevel(power);
	radio.setChannel(channel);
	radio.setPayloadSize(sizeof(sensor_payload_t));
	radio.openWritingPipe(bridge_addr);
}

void loop(void)
{
	static uint32_t msgid;

	uint32_t start = micros();

	// power-up here to overlap with dht read
	radio.powerUp();

	unsigned lsens = analogRead(LIGHT_PIN);
	uint8_t batt = analogRead(BATTERY_PIN) / 4;
	uint8_t light = 255 - lsens / 4;
	unsigned sleep = lsens / 4 + 1;

	uint8_t adcsra = ADCSRA;
	ADCSRA = 0;
	power_adc_disable();

	// millis() only counts time when the sketch is not sleeping
	uint32_t sd = micros();
	dht.resetTimer();
	int16_t h = dht.getHumidity();
	int16_t t = dht.getTemperature();
	uint8_t status = dht.getStatus();
	sensor_payload_t payload = { millis(), msgid++, h, t, NODE_ID, light, batt, status };

	uint32_t sr = micros();
	radio.stopListening();
	radio.write(&payload, sizeof(payload));
	radio.powerDown();

#if defined(DEBUG)
	uint32_t now = micros();
	serial.print(now - start);
	serial.print('\t');
	serial.print(now - sr);
	serial.print('\t');
	serial.print(sr - sd);
	serial.print('\t');
	serial.print(sd - start);
	serial.print('\t');
	serial.print(status);
	serial.print('\t');
	serial.print(h);
	serial.print('\t');
	serial.print(t);
	serial.print('\t');
	serial.print(light);
	serial.print('\t');
	serial.print(batt);
	serial.print('\t');
	serial.println(sleep);
#endif

	uint8_t prr = PRR;
	power_all_disable();

	wdt_sleep(sleep);

	PRR = prr;

	power_adc_enable();
	ADCSRA = adcsra;
}
