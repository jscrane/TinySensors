#include <sys/time.h>
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

#include "sensorlib.h"

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
	gettimeofday(&s->last_update, 0);
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

void fatal(const char *op, const char *error) {
	fprintf(stderr, "%s: %s\n", op, error);
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
	return n;
}
