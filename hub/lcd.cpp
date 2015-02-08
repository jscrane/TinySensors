#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "sensorlib.h"

int lcd, mux;
bool verbose = false;

void close_sockets() {
	if (lcd >= 0)
		close(lcd);
	if (mux >= 0)
		close(mux);
}

void signal_handler(int signo) {
	fatal("caught", strsignal(signo));
}

int lcdproc(char *buf, int len, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int n = vsnprintf(buf, len, fmt, args);
	if (verbose)
		printf("%d: %s", lcd, buf);
	write(lcd, buf, n);
	n = sock_read_line(lcd, buf, len);
	if (verbose)
		printf("%d: %d [%s]\n", lcd, n, buf);
	va_end(args);
	return n;
}

#define MAX_SENSORS	7
sensor_t sensors[MAX_SENSORS];

int update_sensor_data(sensor_t *s) {
	for (int i = 0; i < MAX_SENSORS; i++) {
		sensor_t *t = &sensors[i];
		if (!t->location[0] || t->id == s->id) {
			strcpy(t->location, s->location);
			t->id = s->id;
			t->light = s->light;
			t->temperature = s->temperature;
			t->humidity = s->humidity;
			t->battery = s->battery;
			t->last_update = s->last_update;
			return i;
		}
	}
	return -1;
}

int width, height;

void parse_lcdproc_header(char *buf, int n) {
	int i = 0;
	for (char *p = buf, *q = 0; p; p = q) {
		q = strchr(p, ' ');
		if (q)
			*q++ = 0;
		switch (i) {
		case 7:
			width = atoi(p);
			break;
		case 9:
			height = atoi(p);
			break;
		}
		i++;
	}
}

void update_lcd(int i, sensor_t *s) {
	char t[16], buf[64];
	snprintf(t, sizeof(t), "%.4s %4.1f", s->location, s->temperature);
	int x = 1, y = i+1;
	if (y > height) {
		y -= height;
		x += width / 2;
	}
	lcdproc(buf, sizeof(buf), "widget_set sens sensor%d %d %d {%s}\n", i, x, y, t);
	time_t now = s->last_update.tv_sec;
	strftime(t, sizeof(t), "%H:%M:%S", localtime(&now));
	lcdproc(buf, sizeof(buf), "widget_set sens update %d %d {%s}\n", width-strlen(t)+1, height, t);
}

int main(int argc, char *argv[]) {
	int opt;
	bool daemon = true;

	atexit(close_sockets);
	while ((opt = getopt(argc, argv, "l:m:vf")) != -1)
		switch (opt) {
		case 'l':
			lcd = connect_socket(optarg, 13666);
			break;
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
			fprintf(stderr, "Usage: %s: -l lcd:port -m mux:port [-v] [-f]\n", argv[0]);
			exit(-1);
		}
	if (!lcd)
		lcd = connect_socket("localhost", 13666);
	if (!mux)
		mux = connect_socket("localhost", 5678);
		
	if (daemon)
		daemon_mode();

	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	char buf[128];
	int n = lcdproc(buf, sizeof(buf), "hello\n");
	parse_lcdproc_header(buf, n);

	lcdproc(buf, sizeof(buf), "client_set name {Sensors}\n");
	lcdproc(buf, sizeof(buf), "screen_add sens\n");
	lcdproc(buf, sizeof(buf), "screen_set sens name {Sensors}\n");
	for (int i = 0; i < MAX_SENSORS; i++)
		lcdproc(buf, sizeof(buf), "widget_add sens sensor%d string\n", i);
	lcdproc(buf, sizeof(buf), "widget_add sens update string\n");

	for (;;) {
		fd_set rd;
		FD_ZERO(&rd);
		FD_SET(lcd, &rd);
		FD_SET(mux, &rd);

		if (0 > select(mux+1, &rd, 0, 0, 0))
			fatal("select", strerror(errno));

		if (FD_ISSET(lcd, &rd)) {
			n = sock_read_line(lcd, buf, sizeof(buf));
			if (verbose)
				printf("%d: %d [%s]\n", lcd, n, buf);
			// FIXME?
		}
		if (FD_ISSET(mux, &rd)) {
			n = sock_read_line(mux, buf, sizeof(buf));
			if (verbose)
				printf("%d: %d [%s]\n", mux, n, buf);
			if (n > 0) {
				sensor_t s;
				parse_sensor_data(buf, &s);
				if (s.battery != 0.0) {
					int i = update_sensor_data(&s);
					update_lcd(i, &s);
				}
			}
		}
	}
}
