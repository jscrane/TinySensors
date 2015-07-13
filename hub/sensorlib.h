#ifndef __SENSORLIB_H__
#define __SENSORLIB_H__

class sensor {
public:
	char short_name[5];
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

int connect_nonblock(const char *host, int defport);

int on_connect(int sock);

int connect_block(const char *host, int defport);

int sock_read_line(int sock, char *buf, int len);

#endif
