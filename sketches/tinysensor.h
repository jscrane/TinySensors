#ifndef TINYSENSOR_H
#define TINYSENSOR_H

const unsigned char sensor_type_id = 0;

struct sensor_payload_t
{
  uint32_t ms;
  uint8_t light, status;
  int16_t humidity, temperature;
  uint16_t battery;
};

#endif
