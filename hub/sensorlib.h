#ifndef __SENSORLIB_H__
#define __SENSORLIB_H__

class sensor {
public:
	char location[16];
	unsigned light;
	float temperature, humidity, battery;
	unsigned node_id, node_time, node_status, msg_id;
	unsigned node_type;

	int to_csv(char *buf, int len);
	int from_csv(char *buf);
	bool is_wireless() { return node_type == 0; }
};

void daemon_mode();

void fatal(const char *fmt, ...);

int connect_socket(const char *s, int defport);

int sock_read_line(int s, char *buf, int len);

#endif
