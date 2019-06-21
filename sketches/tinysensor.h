#ifndef TINYSENSOR_H
#define TINYSENSOR_H

const uint16_t master_node = 0;
const unsigned char sensor_type_id = 0;
const uint8_t channel = 108;
const rf24_datarate_e data_rate = RF24_250KBPS;

struct sensor_payload_t
{
	uint32_t ms;
	uint8_t light, status;
	int16_t humidity, temperature;
	uint16_t battery;
};

#endif
