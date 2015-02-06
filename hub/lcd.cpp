#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>

int lcd, mux;
bool verbose = false;

void close_exit() {
	if (lcd >= 0)
		close(lcd);
	if (mux >= 0)
		close(mux);
	exit(1);
}

void fatal(const char *op, const char *error) {
	fprintf(stderr, "%s: %s\n", op, error);
	close_exit();
}

void signal_handler(int signo) {
	fatal("caught", strsignal(signo));
}

int connect_socket(const char *s, int defport) {
	int port = defport;
	char *sep = (char *)strchr(s, ':');
	if (sep) {
		*sep++ = 0;
		port = atoi(sep);
	}

	struct hostent *he = gethostbyname(s);
	if (!he)
		fatal("gethostbyname", s);

	struct sockaddr_in addr;
	memset((void *)&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = *(in_addr *)he->h_addr;

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (0 > sock)
		fatal("socket", strerror(errno));
	if (0 > connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr)))
		fatal(s, strerror(errno));

	return sock;
}

int sock_read_line(int s, char *buf, int len) {
	int n = read(s, buf, len);
	char *eol = strchr(buf, '\n');
	if (eol)
		*eol = 0;
	if (verbose)
		printf("%d: %d [%s]\n", s, n, buf);
	return n;
}

int lcdproc(char *buf, int len, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int n = vsnprintf(buf, len, fmt, args);
	if (verbose)
		printf("%d: %s", lcd, buf);
	write(lcd, buf, n);
	n = sock_read_line(lcd, buf, len);
	va_end(args);
	return n;
}

typedef struct sensor {
	char location[16];
	unsigned id, light;
	float temperature, humidity, battery;
	struct timeval last_update;
} sensor_t;

#define MAX_SENSORS	8
sensor_t sensors[MAX_SENSORS];

void parse_sensor_data(char *buf, sensor_t *s) {
	int i = 0;
	char *x;
	for (char *p = buf, *q = 0; p; p = q) {
		q = strchr(p, ',');
		if (q)
			*q++ = 0;
		switch(i) {
		case 0:	
			strncpy(s->location, p, sizeof(s->location));
			for (char *x = s->location; *x; x++)
				*x = tolower(*x);
			break;
		case 1:
			s->id = atoi(p);
			break;
		case 2:
			s->light = atoi(p);
			break;
		case 3:
			s->temperature = atof(p);
			break;
		case 4:
			s->humidity = atof(p);
			break;
		case 5:
			s->battery = atof(p);
			break;
		}
		i++;
	}
}

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
			gettimeofday(&t->last_update, 0);
			return i;
		}
	}
	return -1;
}

void update_lcd(int i, sensor_t *s) {
	char t[16], buf[64];
	snprintf(t, sizeof(t), "%.4s %4.1f", s->location, s->temperature);
	int x = 1, y = i+1;
	// FIXME: hardwired screen size
	if (y > 4) {
		y -= 4;
		x += 10;
	}
	int n = snprintf(buf, sizeof(buf), "widget_set sens sensor%d %d %d {%s}\n", i, x, y, t);
	write(lcd, buf, n);
}

int main(int argc, char *argv[]) {
	int opt;
	bool daemon = true;
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
		
	if (daemon) {
		pid_t pid = fork();
		if (pid < 0)		// fork failed
			exit(-1);
		if (pid > 0)		// i'm the parent
			exit(0);
		if (setsid() < 0)	// new session
			exit(-1);

		umask(0);
		chdir("/tmp");
		close(0);
		close(1);
		close(2);
	}
	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	char buf[128];
	int n = lcdproc(buf, sizeof(buf), "hello\n");
	lcdproc(buf, sizeof(buf), "client_set name {Sensors}\n");
	lcdproc(buf, sizeof(buf), "screen_add sens\n");
	lcdproc(buf, sizeof(buf), "screen_set sens name {Sensors}\n");
	for (int i = 0; i < MAX_SENSORS; i++)
		lcdproc(buf, sizeof(buf), "widget_add sens sensor%d string\n", i);

	for (;;) {
		fd_set rd;
		FD_ZERO(&rd);
		FD_SET(lcd, &rd);
		FD_SET(mux, &rd);

		if (0 > select(mux+1, &rd, 0, 0, 0))
			fatal("select", strerror(errno));

		if (FD_ISSET(lcd, &rd)) {
			n = sock_read_line(lcd, buf, sizeof(buf));
			// FIXME?
		}
		if (FD_ISSET(mux, &rd)) {
			n = sock_read_line(mux, buf, sizeof(buf));
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
