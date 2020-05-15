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
#include <fcntl.h>

#include "sensorlib.h"

int sensor::to_csv(char *buf, int len) {
	switch (node_type) {
	case 0:
		return snprintf(buf, len,
			"%s,%d,%d,%d,%3.1f,%3.1f,%4.2f,%u,%u,%u,%d\n",
			short_name, node_id, node_type, light,
			temperature, humidity, battery, node_status,
			msg_id, node_time, domoticz_id);
	case 1:
		// ???
		return -1;
	case 2:
		return snprintf(buf, len,
			"%s,%d,%d,%d,%3.1f,,,,,%u,%d\n",
			short_name, node_id, node_type, light,
			temperature, node_time, domoticz_id);
	case 3:
	case 4:
		return snprintf(buf, len,
			"%s,%d,%d,,%3.1f,,,,%u,%d\n",
			short_name, node_id, node_type,
			temperature, node_time, domoticz_id);
	default:
		return -1;
	}
}

int sensor::from_csv(char *buf) {
	int i = 0;
	domoticz_id = 0;
	for (char *p = buf, *q = 0; p; p = q) {
		q = strchr(p, ',');
		if (q)
			*q++ = 0;
		switch(i) {
		case 0:	
			strncpy(short_name, p, sizeof(short_name)-1);
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
		case 10:
			domoticz_id = atoi(p);
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
	if (0 > open("/dev/null", O_RDONLY))
		exit(-1);
	if (0 > open("/dev/null", O_WRONLY))
		exit(-1);
	if (0 > open("/dev/null", O_RDWR))
		exit(-1);
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

int host_port(const char *hp, int defport, char *host, int size) {
	int port = defport;
	strncpy(host, hp, size);
	char *sep = (char *)strchr(host, ':');
	if (sep) {
		*sep++ = 0;
		port = atoi(sep);
	}
	return port;
}

static int init_addr(struct sockaddr_in &a, const char *s, int defport, bool block = true) {
	char host[32];
	int port = host_port(s, defport, host, sizeof(host)-1);

	struct hostent *he = gethostbyname(host);
	if (he) {
		memset((void *)&a, 0, sizeof(sockaddr_in));
		a.sin_family = AF_INET;
		a.sin_port = htons(port);
		a.sin_addr = *(in_addr *)he->h_addr;
		return 0;
	}

	if (!block && h_errno == TRY_AGAIN)
		return -1;

	fatal("gethostbyname: %s: %s\n", host, hstrerror(h_errno));
	return -1;
}

int connect_block(const char *s, int defport) {
	struct sockaddr_in a;
	init_addr(a, s, defport);

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (0 > sock)
		fatal("socket: %s\n", strerror(errno));

	if (0 > connect(sock, (struct sockaddr *)&a, sizeof(struct sockaddr)))
		fatal("connect: %s: %s\n", s, strerror(errno));

	return sock;
}

int connect_nonblock(const char *s, int defport) {
	struct sockaddr_in a;
	if (0 > init_addr(a, s, defport, false))
		return -1;

	int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (0 > sock)
		fatal("socket: %s\n", strerror(errno));

	int e = connect(sock, (struct sockaddr *)&a, sizeof(struct sockaddr));
	if (0 == e || (0 > e && errno == EINPROGRESS))
		return sock;

	if (0 > e && errno != ECONNREFUSED)
		fatal("connect: %s: %s\n", s, strerror(errno));

	close(sock);
	return -1;
}

int sock_read_line(int s, char *buf, int len) {
	for (int i = 0; i < len; i++) {
		char c;
		int n = read(s, &c, 1);
		if (n == 0 || (n == -1 && errno == EAGAIN))
			return i;
		if (n == -1)
			return 0;
		if (c == '\n') {
			buf[i] = 0;
			return i;
		}
		buf[i] = c;
	}
	return len;
}

int on_connect(int s) {
	int e = 0;
	socklen_t len = sizeof(e);
	if (0 > getsockopt(s, SOL_SOCKET, SO_ERROR, &e, &len) || e != 0) {
		if (e != 0)
			errno = e;
		close(s);
		return -1;
	}
	return s;
}
