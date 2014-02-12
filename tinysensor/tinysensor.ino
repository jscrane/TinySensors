#include <JeeLib.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <EEPROM.h>
#include <DHT.h>

RF24 radio(2, 3);
RF24Network network(radio);
DHT dht;

const uint16_t other_node = 0;
uint16_t this_node;

const unsigned long interval = 5000; //ms

struct payload_t
{
  uint32_t ms;
  uint8_t light, status;
  int16_t humidity, temperature;
  uint16_t battery;
};

void setup(void)
{
  pinMode(9, INPUT);
  digitalWrite(9, HIGH);   // enable pullup

  dht.setup(8);
  
  SPI.begin();
  radio.begin();
  
  radio.enableDynamicPayloads();
  radio.setAutoAck(true);

  this_node = EEPROM.read(0);
  network.begin(90, this_node);
}

ISR(WDT_vect) { Sleepy::watchdogEvent(); }

void loop(void)
{
  radio.powerUp();
  network.update();  

  uint8_t light = 255 - analogRead(A1) / 4;
  uint16_t battery = analogRead(A0);

  payload_t payload = { millis(), light, dht.getStatus(), dht.getHumidity(), dht.getTemperature(), battery };
  RF24NetworkHeader header(other_node);
  bool ok = network.write(header, &payload, sizeof(payload));

  radio.powerDown();
  Sleepy::loseSomeTime(interval);
}
