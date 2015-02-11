#ifndef __SENSORLIB_H__
#define __SENSORLIB_H__

typedef struct sensor {
	char location[16];
	unsigned light;
	float temperature, humidity, battery;
	unsigned node_id, node_time, node_status, msg_id;
} sensor_t;

int format_sensor_data(char *buf, int len, sensor_t *s);

int parse_sensor_data(char *buf, sensor_t *s);

void daemon_mode();

void fatal(const char *fmt, ...);

int connect_socket(const char *s, int defport);

int sock_read_line(int s, char *buf, int len);

#endif
