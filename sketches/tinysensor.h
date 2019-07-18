#ifndef TINYSENSOR_H
#define TINYSENSOR_H

const uint16_t master_node = 0;
const uint8_t sensor_type_id = 0;
const uint8_t channel = 108;
const rf24_datarate_e data_rate = RF24_250KBPS;
const rf24_pa_dbm_e power = RF24_PA_MAX;
const rf24_crclength_e crc_len = RF24_CRC_8;

struct sensor_payload_t
{
	uint32_t ms;
	int16_t humidity, temperature;
	uint8_t light, battery, status;
};

#endif
