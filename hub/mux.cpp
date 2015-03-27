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

#define MAX_CLIENTS 16
#define MAX_SERVERS 4
int clients[MAX_CLIENTS];
sensor sensors[MAX_SENSORS];
MYSQL *db_conn;
int ss = -1;
int servers[MAX_SERVERS], ns = 0;

void close_exit()
{
	if (db_conn)
		mysql_close(db_conn);
	if (ss >= 0)
		close(ss);
	for (int i = 0; i < ns; i++)
		close(servers[i]);
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (clients[i])
			close(clients[i]);
}

void db_fatal(const char *op)
{
	fatal("%s: %s\n", op, mysql_error(db_conn));
}

void signal_handler(int signo)
{
	fatal("Caught: %s\n", strsignal(signo));
}

int update_sensor_data(sensor *s) {
	for (int i = 0; i < MAX_SENSORS; i++) {
		sensor *t = &sensors[i];
		if (t->node_id == s->node_id) {
			t->node_id = s->node_id;
			t->light = s->light;
			t->temperature = s->temperature;
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
	const char *mysql_host = "localhost";
	int opt;

	atexit(close_exit);
	while ((opt = getopt(argc, argv, "vm:")) != -1)
		switch(opt) {
		case 'v':
			verbose = true;
			daemon = false;
			break;
		case 'm':
			mysql_host = optarg;
			break;
		default:
			fatal("Usage: %s [-m mysql_host] [-v] sensorhost:port ...\n", argv[0]);
		}

	if (verbose) 
		printf("MySQL client version: %s\n", mysql_get_client_info());

	db_conn = mysql_init(0);

	if (mysql_real_connect(db_conn, mysql_host, USER, PASS, "sensors", 0, NULL, 0) == NULL)
		db_fatal("mysql_real_connect");
	
	if (mysql_query(db_conn, "SELECT id,short,device_type_id FROM nodes"))
		db_fatal("mysql_query");

	MYSQL_RES *rs = mysql_store_result(db_conn);
	if (!rs)
		db_fatal("mysql_store_result");

	MYSQL_ROW row;
	for (int i = 0; i < MAX_SENSORS && (row = mysql_fetch_row(rs)); i++) {
		sensors[i].node_id = atoi(row[0]);
		strncpy(sensors[i].short_name, row[1], sizeof(sensors[i].short_name));
		sensors[i].node_type = atoi(row[2]);
	}
	
	mysql_close(db_conn);
	db_conn = 0;

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
		// servers[ns] = connect_block(argv[ind], 5555);

	if (daemon) 
		daemon_mode();

	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	int nclients = 0;
	for (;;) {
		fd_set rd, wr;
		FD_ZERO(&rd);
		FD_ZERO(&wr);
		if (nclients < MAX_CLIENTS)
			FD_SET(ss, &rd);
		for (int i = 0; i < ns; i++) {
			if (servers[i] == -1) {
				servers[i] = connect_nonblock(argv[optind+i], 5555);
				if (servers[i] != -1)
					FD_SET(servers[i], &wr);
			} else 
				FD_SET(servers[i], &rd);
		}

		if (select(servers[ns-1] + 1, &rd, &wr, 0, 0) < 0)
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
			const char *header = "name,id,type,light,degC,hum%,Vbatt,status,msg_id,time\n";
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
					if (f == 10 && s.node_id < MAX_SENSORS) {
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
					}
				} else if (n == 0) {
					if (verbose)
						printf("Server died: %s\n", argv[optind + i]);
					close(servers[i]);
					servers[i] = -1;
				}
			} else if (FD_ISSET(servers[i], &wr)) {
				int e = 0;
				socklen_t len = sizeof(e);
				getsockopt(servers[i], SOL_SOCKET, SO_ERROR, &e, &len);
				if (e != 0) {
					printf("Server connect failed: %s %d\n", argv[optind + i], e);
					close(servers[i]);
					servers[i] = -1;
				}
			}
	}
	return 0;
}
