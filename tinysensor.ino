#include <JeeLib.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <EEPROM.h>
#include <DHT.h>

RF24 radio(2, 3);
RF24Network network(radio);
DHT dht;

const uint16_t master_node = 0;
uint16_t this_node;

const unsigned long interval = 60000; //ms
const unsigned char node_type = 0;

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
  RF24NetworkHeader header(master_node, node_type);
  bool ok = network.write(header, &payload, sizeof(payload));

  radio.powerDown();

  int32_t l = light;
  int32_t s = 1000 * (255 - l) / 3;
  if (s < 15000)
    s = 15000;
  do {
    int32_t i = min(s, interval);  
    Sleepy::loseSomeTime(i);
    s -= i;
  } while (s > 0);
}
