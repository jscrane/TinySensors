#include <my_global.h>
#include <mysql.h>
#include <getopt.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "sensorlib.h"

#define NCLIENTS 16

sensor_t sensors[MAX_SENSORS];
MYSQL *db_conn;
int ss = -1, cs = -1;

void close_exit()
{
	if (db_conn)
		mysql_close(db_conn);
	if (ss >= 0)
		close(ss);
	if (cs >= 0)
		close(cs);
}

void db_fatal(const char *op)
{
	fatal(op, mysql_error(db_conn));
}

void signal_handler(int signo)
{
	fatal("caught", strsignal(signo));
}

int update_sensor_data(sensor_t *s) {
	for (int i = 0; i < MAX_SENSORS; i++) {
		sensor_t *t = &sensors[i];
		if (t->node_id == s->node_id) {
			t->node_id = s->node_id;
			t->light = s->light;
			t->temperature = s->temperature;
			t->humidity = s->humidity;
			t->battery = s->battery;
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
	while ((opt = getopt(argc, argv, "vs:")) != -1)
		switch(opt) {
		case 'v':
			verbose = true;
			daemon = false;
			break;
		case 's':
			cs = connect_socket(optarg, 5555);
			break;
		default:
			fprintf(stderr, "Usage: %s [-s svr:port] [-v]\n", argv[0]);
			exit(1);
		}

	if (verbose) 
		printf("MySQL client version: %s\n", mysql_get_client_info());

	db_conn = mysql_init(0);

	if (mysql_real_connect(db_conn, "localhost", USER, PASS, "sensors", 0, NULL, 0) == NULL)
		db_fatal("mysql_real_connect");
	
	if (mysql_query(db_conn, "SELECT id,location FROM nodes WHERE device_type_id=0"))
		db_fatal("mysql_query");

	MYSQL_RES *rs = mysql_store_result(db_conn);
	if (!rs)
		db_fatal("mysql_store_result");

	MYSQL_ROW row;
	for (int i = 0; i < MAX_SENSORS && (row = mysql_fetch_row(rs)); i++) {
		sensors[i].node_id = atoi(row[0]);
		strncpy(sensors[i].location, row[1], sizeof(sensors[i].location));
	}
	
	mysql_close(db_conn);
	db_conn = 0;

	ss = socket(AF_INET, SOCK_STREAM, 0);
	if (ss < 0)
		fatal("socket", strerror(errno));

	struct sockaddr_in lsnr;
	memset(&lsnr, 0, sizeof(lsnr));
	lsnr.sin_family = AF_INET;
	lsnr.sin_addr.s_addr = htonl(INADDR_ANY);
	lsnr.sin_port = htons(5678);
	if (0 > bind(ss, (struct sockaddr *)&lsnr, sizeof(struct sockaddr)))
		fatal("bind", strerror(errno));

	if (0 > listen(ss, 1))
		fatal("listen", strerror(errno));

	if (cs < 0)
		cs = connect_socket("localhost", 5555);

	if (daemon) 
		daemon_mode();

	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	int clients[NCLIENTS], nclients = 0;
	for (int i = 0; i < NCLIENTS; i++)
		clients[i] = 0;
	for (;;) {
		fd_set rd;
		FD_ZERO(&rd);
		if (nclients < NCLIENTS)
			FD_SET(ss, &rd);
		FD_SET(cs, &rd);

		if (select(cs + 1, &rd, 0, 0, 0) < 0)
			fatal("select", strerror(errno));

		char obuf[256];
		if (FD_ISSET(ss, &rd)) {
			struct sockaddr_in client;
			socklen_t addrlen = sizeof(struct sockaddr_in);
			int c = accept(ss, (struct sockaddr *)&client, &addrlen);
			if (c < 0)
				fatal("accept", strerror(errno));

			// find next free slot
			for (int i = 0; i < NCLIENTS; i++)
				if (clients[i] == 0) {
					nclients++;
					clients[i] = c;
					break;
				}
			const char *header = "location,id,light,degC,hum%,Vbatt,status,msg_id,millis\n";
			write(c, header, strlen(header));
			for (int i = 0; i < MAX_SENSORS; i++) {
				sensor_t *t = &sensors[i];
				if (t->battery != 0.0) {
					int n = format_sensor_data(obuf, sizeof(obuf), t);
					if (verbose)
						printf("%s", obuf);
					write(c, obuf, n);
				}
			}
		}
		if (FD_ISSET(cs, &rd)) {
			char ibuf[256];
			int n = read(cs, ibuf, sizeof(ibuf));
			if (n > 0) {
				ibuf[n] = 0;
				if (verbose)
					printf("%s", ibuf);
				sensor_t s;
				int f = sscanf(ibuf, "%u\t%u\t%f\t%f\t%f\t%u\t%u\t%u\n", &s.node_id, &s.light, &s.temperature, &s.humidity, &s.battery, &s.node_status, &s.msg_id, &s.node_millis);
				if (f == 8 && s.node_id < MAX_SENSORS) {
					int si = update_sensor_data(&s);
					n = format_sensor_data(obuf, sizeof(obuf), &sensors[si]);
					if (verbose)
						printf("%s", obuf);
					for (int i = 0; i < NCLIENTS; i++)
						if (clients[i] && 0 > write(clients[i], obuf, n)) {
							close(clients[i]);
							clients[i] = 0;
							nclients--;
						}
				}
			} else if (n == 0) {
				if (verbose)
					printf("server died\n");
				break;
			}
		}
	}
	// FIXME: do this in the exit handler
	close(cs);
	for (int i = 0; i < NCLIENTS; i++)
		if (clients[i])
			close(clients[i]);
	return 0;
}
