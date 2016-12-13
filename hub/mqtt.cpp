#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>

#include "sensorlib.h"

static int mux = -1;
static struct mosquitto *mosq;
static bool verbose = false;

void close_sockets() {
	if (mux >= 0)
		close(mux);
	if (mosq) {
		mosquitto_disconnect(mosq);
		mosquitto_destroy(mosq);
		mosquitto_lib_cleanup();
	}
}

void signal_handler(int signo) {
        fatal("Caught: %s\n", strsignal(signo));
}

void pub(const char *name, const char *sub, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	char topic[80], val[16];
	sprintf(topic, "stat/%s/%s", sub, name);
	vsprintf(val, fmt, ap);
	va_end(ap);

	if (verbose)
		printf("%s: %s\n", topic, val);
	int ret = mosquitto_publish(mosq, 0, topic, strlen(val), val, 0, true);
	if (ret == MOSQ_ERR_ERRNO)
		fatal("Publish: %s\n", strerror(errno));
	else if (ret)
		fatal("Publish: %d\n", ret);
}

int main(int argc, char *argv[])
{
	int opt;
	bool daemon = true;
	const char *mux_host = "localhost", *mqtt_host = "localhost";

	atexit(close_sockets);
	while ((opt = getopt(argc, argv, "q:m:vf")) != -1)
		switch (opt) {
		case 'q':
			mqtt_host = optarg;
			break;
		case 'm':
			mux_host = optarg;
			break;
		case 'v':
			verbose = true;
			daemon = false;
			break;
		case 'f':
			daemon = false;
			break;
		}

	if (daemon)
		daemon_mode();

	mosquitto_lib_init();

	mosq = mosquitto_new("sensors", false, NULL);
	if (!mosq)
		fatal("Can't initialize Mosquitto\n");

	char host[32];
	int port = host_port(mqtt_host, 1883, host, sizeof(host));
	int ret = mosquitto_connect(mosq, host, port, 0);
	if (ret)
		fatal("Can't connect to %s:%d: %d\n", host, port, ret);

	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	for (;;) {
		if (mux < 0)
			mux = connect_block(mux_host, 5678);

		char buf[128];
		int n = sock_read_line(mux, buf, sizeof(buf));
		if (n == 0) {
			if (verbose)
				printf("Mux died\n");
			close(mux);
			mux = -1;
		} else if (n > 0) {
			if (verbose)
				printf("%d: %d [%s]\n", mux, n, buf);
			sensor s;
			s.from_csv(buf);
			pub(s.short_name, "t", "%3.1f", s.temperature);
			if (s.node_type == 0) {
				pub(s.short_name, "h", "%3.1f", s.humidity);
				pub(s.short_name, "b", "%1.2f", s.battery);
			}
			if (s.node_type == 0 || s.node_type == 2)
				pub(s.short_name, "l", "%d", s.light);
		}
	}
}
