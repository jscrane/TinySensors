#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/select.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <rrd.h>

#include "sensorlib.h"

int mux = -1;
bool verbose = false;

void close_sockets() {
	if (mux >= 0)
		close(mux);
}

void signal_handler(int signo) {
	fatal("Caught: %s\n", strsignal(signo));
}

const char *rrd_path = "/var/lib/rrd";
char buf[128], db_path[128];

char *update_params[] = {
	"rrdupdate",
	db_path,
	buf,
	0
};

int update_wireless(sensor *s) {
	sprintf(db_path, "%s/sensor-%d.rrd", rrd_path, s->node_id);
	sprintf(buf, "N:%f:%d:%f:%f", s->temperature, s->light, s->humidity, s->battery);
	return rrd_update(3, update_params);
}

int update_none(sensor *s) {
	return -1;
}

int update_temperature_light(sensor *s) {
	sprintf(db_path, "%s/sensor-%d.rrd", rrd_path, s->node_id);
	sprintf(buf, "N:%f:%d", s->temperature, s->light);
	rrd_clear_error();
	return rrd_update(3, update_params);
}

int update_temperature(sensor *s) {
	sprintf(db_path, "%s/sensor-%d.rrd", rrd_path, s->node_id);
	sprintf(buf, "N:%f", s->temperature);
	rrd_clear_error();
	return rrd_update(3, update_params);
}

typedef int (*updatefn)(sensor *);

updatefn updaters[] = {
	update_wireless,
	update_none,
	update_temperature_light,
	update_temperature,
	update_temperature,
};

int main(int argc, char *argv[]) {
	int opt;
	bool daemon = true;

	atexit(close_sockets);
	while ((opt = getopt(argc, argv, "m:vf")) != -1)
		switch (opt) {
		case 'm':
			mux = connect_socket(optarg, 5678);
			break;
		case 'v':
			verbose = true;
			daemon = false;
			break;
		case 'f':
			daemon = false;
			break;
		default:
			fatal("Usage: %s: -m mux:port [-v] [-f]\n", argv[0]);
		}
	if (mux < 0)
		mux = connect_socket("localhost", 5678);
		
	if (daemon)
		daemon_mode();

	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	char buf[256];
	for (;;) {
		fd_set rd;
		FD_ZERO(&rd);
		FD_SET(mux, &rd);

		if (0 > select(mux+1, &rd, 0, 0, 0))
			fatal("select: %s\n", strerror(errno));

		if (FD_ISSET(mux, &rd)) {
			int n = sock_read_line(mux, buf, sizeof(buf));
			if (verbose)
				printf("%d [%s]\n", n, buf);
			if (n > 0) {
				sensor s;
				s.from_csv(buf);
				int r = updaters[s.node_type](&s);
				if (0 > r) {
					fputs(rrd_get_error(), stderr);
					rrd_clear_error();
				}
			} else if (n == 0)
				fatal("Mux died\n");
		}
	}
}
