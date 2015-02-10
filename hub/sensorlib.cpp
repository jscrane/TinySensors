#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <stdarg.h>

#include "sensorlib.h"

int format_sensor_data(char *buf, int len, sensor_t *s) {
	return snprintf(buf, len, "%s,%d,%d,%3.1f,%3.1f,%4.2f,%u,%u,%u\n", 
			s->location, s->node_id, s->light, s->temperature, 
			s->humidity, s->battery, s->node_status, s->msg_id, 
			s->node_millis);
}

void parse_sensor_data(char *buf, sensor_t *s) {
	int i = 0;
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
			s->node_id = atoi(p);
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
		case 6:
			s->node_millis = atoi(p);
			break;
		case 7:
			s->node_status = atoi(p);
			break;
		case 8:
			s->msg_id = atoi(p);
			break;
		}
		i++;
	}
}

void daemon_mode() {
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

void fatal(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	openlog(0, LOG_PID | LOG_PERROR, LOG_DAEMON);
	vsyslog(LOG_ERR, fmt, ap);
	closelog();
	va_end(ap);
	exit(1);
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
		fatal("gethostbyname: %s\n", s);

	struct sockaddr_in addr;
	memset((void *)&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = *(in_addr *)he->h_addr;

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (0 > sock)
		fatal("socket: %s\n", strerror(errno));
	if (0 > connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr)))
		fatal("connect: %s: %s\n", s, strerror(errno));

	return sock;
}

int sock_read_line(int s, char *buf, int len) {
	for (int i = 0; i < len; i++) {
		char c;
		int n = read(s, &c, 1);
		if (n == 0)
			return i;
		if (c == '\n') {
			buf[i] = 0;
			return i;
		}
		buf[i] = c;
	}
	return len;
}
