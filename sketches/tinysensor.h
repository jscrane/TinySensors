#ifndef TINYSENSOR_H
#define TINYSENSOR_H

const uint8_t channel = 110;
const rf24_datarate_e data_rate = RF24_250KBPS;
const rf24_pa_dbm_e power = RF24_PA_MAX;
const rf24_crclength_e crc_len = RF24_CRC_8;

const uint64_t bridge_addr = 0xF0F0F0F0E1LL;

struct sensor_payload_t
{
	uint32_t ms, id;
	int16_t humidity, temperature;
	uint8_t node_id, light, battery, status;
};

#endif
