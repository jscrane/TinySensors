#ifndef __SENSORLIB_H__
#define __SENSORLIB_H__

typedef struct sensor {
	char location[16];
	unsigned id, light;
	float temperature, humidity, battery;
	struct timeval last_update;
} sensor_t;

void parse_sensor_data(char *buf, sensor_t *s);

void daemon_mode();

void fatal(const char *op, const char *error);

int connect_socket(const char *s, int defport);

int sock_read_line(int s, char *buf, int len);

#endif
