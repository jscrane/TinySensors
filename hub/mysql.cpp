#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <my_global.h>
#include <mysql.h>

#include "sensorlib.h"

int mux = -1;
bool verbose = false;
MYSQL *db_conn;

void close_sockets() {
	if (mux >= 0)
		close(mux);
	if (db_conn)
		mysql_close(db_conn);
}

void signal_handler(int signo) {
	fatal("Caught", strsignal(signo));
}

int main(int argc, char *argv[]) {
	int opt;
	bool daemon = true;
	const char *mysql_host = "localhost";

	atexit(close_sockets);
	while ((opt = getopt(argc, argv, "s:m:vf")) != -1)
		switch (opt) {
		case 's':
			mysql_host = optarg;
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
			fprintf(stderr, "Usage: %s: -s mysql_host -m mux_host:port [-v] [-f]\n", argv[0]);
			exit(-1);
		}

	if (daemon)
		daemon_mode();

	if (verbose) 
		printf("MySQL client version: %s\n", mysql_get_client_info());

	db_conn = mysql_init(0);
	if (mysql_real_connect(db_conn, mysql_host, USER, PASS, "sensors", 0, NULL, 0) == NULL)
		fatal("mysql_real_connect", mysql_error(db_conn));

	if (mux < 0)
		mux = connect_socket("localhost", 5678);

	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	char buf[1024];
	for (;;) {
		int n = sock_read_line(mux, buf, sizeof(buf));
		if (verbose)
			printf("%d: %d [%s]\n", mux, n, buf);
		if (n > 0) {
			sensor_t s;
			parse_sensor_data(buf, &s);
			if (s.battery != 0.0) {
				sprintf(buf, "INSERT INTO sensor_data (node_id,node_ms,light,humidity,temperature,battery,status,msg_id) VALUES(%d,%d,%d,%.1f,%.1f,%.2f,%d,%d)", 
					s.node_id, s.node_millis, s.light, s.humidity, s.temperature, s.battery, s.node_status, s.msg_id);

				if (verbose)
					puts(buf);

				if (mysql_query(db_conn, buf))
					fatal("insert", mysql_error(db_conn));
			}
		} else if (n == 0) {
			fprintf(stderr, "Mux died\n");
			break;
		}
	}
}
