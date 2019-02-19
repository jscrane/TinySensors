#include <DHT.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

/*
const uint8_t DHT_PIN = 8;
const int RX_PIN = 1, TX_PIN = 0;
*/
const uint8_t DHT_PIN = 2;
const uint8_t RX_PIN = 9, TX_PIN = 10;

DHT dht;
SoftwareSerial serial(RX_PIN, TX_PIN);

void setup(void)
{
	pinMode(RX_PIN, INPUT);
	pinMode(TX_PIN, OUTPUT);

	serial.begin(9600);
	serial.println("Sensor Test. Enter node-id and hit return.");
	serial.println("Status\tHumidity\tTemperature\tLight\tBattery\tID");

	pinMode(A1, INPUT_PULLUP);

	dht.setup(DHT_PIN);
}

void loop(void)
{
	uint8_t light = 255 - analogRead(A1) / 4;

	serial.print(dht.getStatus());
	serial.print("\t");
	serial.print(dht.getHumidity());
	serial.print("\t\t");
	serial.print(dht.getTemperature());
	serial.print("\t\t");
	serial.print((int)light);
	serial.print("\t");
	serial.print(analogRead(A0));
	serial.print("\t");
	serial.println(EEPROM.read(0), 8);

	uint8_t id = 0;
	while (serial.available()) {
		int ch = serial.read();
		if (ch == '\n')
			EEPROM.write(0, id);
		else if (ch >= '0' && ch < '8')
			id = (id << 3) | (ch - '0');
	}
	delay(dht.getMinimumSamplingPeriod());
}
