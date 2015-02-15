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

int sensor::to_csv(char *buf, int len) {
	switch (node_type) {
	case 0:
		return snprintf(buf, len, 
			"%s,%d,%d,%d,%3.1f,%3.1f,%4.2f,%u,%u,%u\n", 
			location, node_id, node_type, light, temperature, 
			humidity, battery, node_status, msg_id, node_time);
	case 1:
		// ???
		return -1;
	case 2:
		return snprintf(buf, len, 
			"%s,%d,%d,%d,%3.1f,,,,,%u\n", 
			location, node_id, node_type, light, temperature, 
			node_time);
	case 3:
	case 4:
		return snprintf(buf, len, 
			"%s,%d,%d,,%3.1f,,,,%u\n", 
			location, node_id, node_type, temperature, node_time);
	default:
		return -1;
	}
}

int sensor::from_csv(char *buf) {
	int i = 0;
	for (char *p = buf, *q = 0; p; p = q) {
		q = strchr(p, ',');
		if (q)
			*q++ = 0;
		switch(i) {
		case 0:	
			strncpy(location, p, sizeof(location));
			break;
		case 1:
			node_id = atoi(p);
			break;
		case 2:
			node_type = atoi(p);
			break;
		case 3:
			light = *p? atoi(p): 0;
			break;
		case 4:
			temperature = atof(p);
			break;
		case 5:
			humidity = *p? atof(p): -1;
			break;
		case 6:
			battery = *p? atof(p): -1;
			break;
		case 7:
			node_status = atoi(p);
			break;
		case 8:
			msg_id = *p? atoi(p): 0;
			break;
		case 9:
			node_time = atoi(p);
			break;
		}
		i++;
	}
	return i;
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
