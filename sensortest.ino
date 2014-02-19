#include <DHT.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

DHT dht;
const int rx = 1, tx = 0;
SoftwareSerial serial(rx, tx);

void setup(void)
{
  pinMode(rx, INPUT);
  pinMode(tx, OUTPUT);
  
  serial.begin(9600);
  serial.println("Sensor Test. Enter node-id and hit return.");
  serial.println("Status\tHumidity\tTemperature\tLight\tBattery\tID");

  pinMode(9, INPUT);
  digitalWrite(9, HIGH);   // enable pullup on A1
  
  dht.setup(8);
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
