#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "sensorlib.h"

#define MAX_CLIENTS 16
#define MAX_SERVERS 4
int clients[MAX_CLIENTS];
sensor sensors[MAX_SENSORS];
int ss = -1;
int servers[MAX_SERVERS], ns = 0;
bool raw = false;

void close_exit()
{
	if (ss >= 0)
		close(ss);
	for (int i = 0; i < ns; i++)
		close(servers[i]);
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (clients[i])
			close(clients[i]);
}

void signal_handler(int signo)
{
	fatal("Caught: %s\n", strsignal(signo));
}

int update_sensor_data(sensor *s) {
	for (int i = 0; i < MAX_SENSORS; i++) {
		sensor *t = &sensors[i];
		if (t->node_id == s->node_id) {
			t->light = s->light;
			if (raw || s->temperature < 75)
				t->temperature = s->temperature;
			if (raw || (s->humidity >= 1 && s->humidity <= 100))
				t->humidity = s->humidity;
			t->battery = s->battery;
			t->node_status = s->node_status;
			t->node_time = s->node_time;
			t->msg_id = s->msg_id;
			return i;
		}
	}
	return -1;
}

int main(int argc, char *argv[])
{
	bool verbose = false, daemon = true;
	int opt;

	atexit(close_exit);
	while ((opt = getopt(argc, argv, "vr")) != -1)
		switch(opt) {
		case 'v':
			verbose = true;
			daemon = false;
			break;
		case 'r':
			raw = true;
			break;
		default:
			fatal("Usage: %s [-v] sensorhost:port ... < nodes.txt\n", argv[0]);
		}

	char buf[80];
	for (int i = 0; i < MAX_SENSORS; ) {
		if (!fgets(buf, sizeof(buf), stdin))
			break;
		if (*buf != '#') {
			sensor *t = &sensors[i];
			sscanf(buf, "%d %d %d %4s", &t->node_id, &t->node_type, &t->domoticz_id, t->short_name);
			if (verbose)
				printf("%d %d %d %s\n", t->node_id, t->node_type, t->domoticz_id, t->short_name);
			i++;
		}
	}
	
	ss = socket(AF_INET, SOCK_STREAM, 0);
	if (ss < 0)
		fatal("socket: %s\n", strerror(errno));

	int yes = 1;
	if (0 > setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)))
		fatal("setsockopt: %s\n", strerror(errno));

	struct sockaddr_in lsnr;
	memset(&lsnr, 0, sizeof(lsnr));
	lsnr.sin_family = AF_INET;
	lsnr.sin_addr.s_addr = htonl(INADDR_ANY);
	lsnr.sin_port = htons(5678);
	if (0 > bind(ss, (struct sockaddr *)&lsnr, sizeof(struct sockaddr)))
		fatal("bind: %s\n", strerror(errno));

	if (0 > listen(ss, MAX_CLIENTS))
		fatal("listen: %s\n", strerror(errno));

	// connect non-blocking below
	for (int ind = optind; ind < argc && ns < MAX_SERVERS; ind++, ns++)
		servers[ns] = -1;

	if (daemon) 
		daemon_mode();

	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	int nclients = 0;
	fd_set rd, wr, srd, swr;
	FD_ZERO(&rd);
	FD_ZERO(&srd);
	FD_ZERO(&wr);
	FD_ZERO(&swr);
	for (;;) {
		if (nclients < MAX_CLIENTS)
			FD_SET(ss, &srd);
		else
			FD_CLR(ss, &srd);

		int nfds = -1;
		for (int i = 0; i < ns; i++) {
			if (servers[i] == -1) {
				servers[i] = connect_nonblock(argv[optind+i], 5555);
				if (servers[i] >= 0)
					FD_SET(servers[i], &swr);
			}
			if (nfds < servers[i])
				nfds = servers[i];
		}

		rd = srd;
		wr = swr;
		if (select(nfds + 1, &rd, &wr, 0, 0) < 0)
			fatal("select: %s\n", strerror(errno));

		char obuf[256];
		if (FD_ISSET(ss, &rd)) {
			struct sockaddr_in client;
			socklen_t addrlen = sizeof(struct sockaddr_in);
			int c = accept(ss, (struct sockaddr *)&client, &addrlen);
			if (c < 0)
				fatal("accept: %s\n", strerror(errno));

			// find next free slot
			for (int i = 0; i < MAX_CLIENTS; i++)
				if (clients[i] == 0) {
					nclients++;
					clients[i] = c;
					break;
				}
			const char *header = "name,id,type,light,degC,hum%,Vbatt,status,msg_id,time,domoticz\n";
			write(c, header, strlen(header));
			for (int i = 0; i < MAX_SENSORS; i++) {
				sensor *t = &sensors[i];
				if (t->short_name[0]) {
					int n = t->to_csv(obuf, sizeof(obuf));
					if (verbose)
						printf("out: %s", obuf);
					write(c, obuf, n);
				}
			}
		}
		for (int i = 0; i < ns; i++)
			if (FD_ISSET(servers[i], &rd)) {
				char ibuf[256];
				int n = sock_read_line(servers[i], ibuf, sizeof(ibuf));
				if (n > 0) {
					if (verbose)
						printf("i: %s\n", ibuf);
					sensor s;
					int f = s.from_csv(ibuf);
					if (f >= 10 && s.node_id < MAX_SENSORS) {
						int si = update_sensor_data(&s);
						n = sensors[si].to_csv(obuf, sizeof(obuf));
						if (verbose)
							printf("o: %s", obuf);
						for (int i = 0; i < MAX_CLIENTS; i++)
							if (clients[i] && 0 > write(clients[i], obuf, n)) {
								close(clients[i]);
								clients[i] = 0;
								nclients--;
							}
					} else if (verbose)
						printf("invalid data: %d or %d\n", f, s.node_id);

				} else if (n == 0) {
					if (verbose)
						printf("Server died: %s\n", argv[optind + i]);
					FD_CLR(servers[i], &srd);
					close(servers[i]);
					servers[i] = -1;
				}
			} else if (FD_ISSET(servers[i], &wr)) {
				FD_CLR(servers[i], &swr);
				servers[i] = on_connect(servers[i]);
				if (servers[i] >= 0)
					FD_SET(servers[i], &srd);
				else
					printf("Server connect failed: %s\n", argv[optind + i]);
			}
	}
	return 0;
}
