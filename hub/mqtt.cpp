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

void do_pub(const char *topic, const char *val) {
	if (verbose)
		printf("%s: %s\n", topic, val);
	int ret = mosquitto_publish(mosq, 0, topic, strlen(val), val, 0, true);
	if (ret == MOSQ_ERR_ERRNO)
		fatal("Publish: %s\n", strerror(errno));
	else if (ret)
		fatal("Publish: %d\n", ret);
}

void pub(const char *root, const char *name, const char *sub, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	char topic[80], val[16];
	sprintf(topic, "%s/%s/%s", root, sub, name);
	vsprintf(val, fmt, ap);
	va_end(ap);

	do_pub(topic, val);
}

void as_text(const char *root, const sensor &s) {
	pub(root, s.short_name, "t", "%3.1f", s.temperature);
	if (s.node_type == 0) {
		pub(root, s.short_name, "h", "%3.1f", s.humidity);
		pub(root, s.short_name, "b", "%1.2f", s.battery);
	}
	if (s.node_type == 0 || s.node_type == 2)
		pub(root, s.short_name, "l", "%d", s.light);
}

void as_json(const char *root, const sensor &s) {
	const char *fmt;
	switch (s.node_type) {
	case 0:
		fmt = "{ \"t\":%3.1f, \"l\":%d, \"h\":%3.1f, \"b\":%1.2f }";
		break;
	case 1:
		fmt = "{ \"t\":%3.1f }";
		break;
	case 2:
		fmt = "{ \"t\":%3.1f, \"l\":%d }";
		break;
	default:
		return;
	}
	char val[80], topic[80];
	sprintf(val, fmt, s.temperature, s.light, s.humidity, s.battery);
	sprintf(topic, "%s/%s", root, s.short_name);
	do_pub(topic, val);
}

void as_domoticz(const char *root, const sensor &s) {
	if (s.domoticz_id > 0) {
		char val[80];
		sprintf(val, "{ \"idx\": %d, \"svalue\": \"%d;%d;1\" }",
			s.domoticz_id, (int)(s.temperature + 0.5),
			(int)(s.humidity + 0.5));
		do_pub(root, val);
	}
}

int main(int argc, char *argv[])
{
	int opt;
	bool daemon = true, json = false, domoticz = false;
	const char *mux_host = "localhost", *mqtt_host = "localhost";
	const char *user = 0, *pass = 0, *root = "stat";
	const char *client = "sensors";

	while ((opt = getopt(argc, argv, "q:m:vfu:p:r:c:j:z")) != -1)
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
		case 'u':
			user = optarg;
			break;
		case 'p':
			pass = optarg;
			break;
		case 'r':
			root = optarg;
			break;
		case 'c':
			client = optarg;
			break;
		case 'j':
			json = true;
			domoticz = false;
			break;
		case 'z':
			domoticz = true;
			json = false;
		}

	if (daemon)
		daemon_mode();

	atexit(close_sockets);

	mosquitto_lib_init();

	mosq = mosquitto_new(client, false, NULL);
	if (!mosq)
		fatal("Can't initialize Mosquitto\n");

	int ret = mosquitto_username_pw_set(mosq, user, pass);
	if (ret)
		fatal("Can't set user/pass %s:%s: %d\n", user, pass, ret);

	char host[32];
	int port = host_port(mqtt_host, 1883, host, sizeof(host));
	ret = mosquitto_connect(mosq, host, port, 0);
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
			if (!s.is_valid())
				continue;
			if (domoticz)
				as_domoticz(root, s);
			else if (json)
				as_json(root, s);
			else
				as_text(root, s);
		}
	}
}
