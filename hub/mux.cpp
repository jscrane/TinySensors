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

#define NCLIENTS 16
#define NODES 20

MYSQL *db_conn;
int ss, cs;

void close_exit()
{
	if (db_conn)
		mysql_close(db_conn);
	if (ss > 0)
		close(ss);
	if (cs > 0)
		close(cs);
	exit(1);
}

void fatal(const char *op, const char *error)
{
	fprintf(stderr, "%s: %s\n", op, error);
	close_exit();
}

void db_fatal(const char *op)
{
	fatal(op, mysql_error(db_conn));
}

void signal_handler(int signo)
{
	fatal("caught", strsignal(signo));
}

int main(int argc, char *argv[])
{
	bool verbose = false, daemon = true;
	int opt;
	while ((opt = getopt(argc, argv, "v")) != -1)
		switch(opt) {
		case 'v':
			verbose = true;
			daemon = false;
			break;
		default:
			fprintf(stderr, "Usage: %s [-v]\n", argv[0]);
			exit(1);
		}

	if (daemon) {
		pid_t pid = fork();

		if (pid < 0)
			exit(-1);
		if (pid > 0)
			exit(0);
		if (setsid() < 0)
			exit(-1);

		pid = fork();
		if (pid < 0)
			exit(-1);
		if (pid > 0)
			exit(0);

		umask(0);
		chdir("/tmp");
		close(0);
		close(1);
		close(2);
	}

	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

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
	char *nodes[NODES];
	for (int i = 0; i < NODES; i++)
		nodes[i] = 0;
	while ((row = mysql_fetch_row(rs))) {
		int id = atoi(row[0]);
		nodes[id] = strdup(row[1]);
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

	cs = socket(AF_INET, SOCK_STREAM, 0);
	if (cs < 0)
		fatal("socket", strerror(errno));

	struct sockaddr_in srvr;
	memset(&lsnr, 0, sizeof(srvr));
	srvr.sin_family = AF_INET;
	srvr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvr.sin_port = htons(5555);
	if (0 > connect(cs, (struct sockaddr *)&srvr, sizeof(struct sockaddr)))
		fatal("connect", strerror(errno));

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

		if (FD_ISSET(ss, &rd)) {
			struct sockaddr_in client;
			socklen_t addrlen = sizeof(struct sockaddr_in);
			int c = accept(ss, (struct sockaddr *)&client, &addrlen);
			if (c < 0)
				fatal("accept", strerror(errno));

			clients[nclients++] = c;
			const char *header = "location,id,light,degC,hum%,Vbatt\n";
			write(c, header, strlen(header));
		}
		if (FD_ISSET(cs, &rd)) {
			char ibuf[256];
			int n = read(cs, ibuf, sizeof(ibuf));
			if (n > 0) {
				ibuf[n] = 0;
				if (verbose)
					printf("%s", ibuf);
				unsigned int id, light;
				float temperature, humidity, battery;
				char obuf[256];
				int f = sscanf(ibuf, "%u\t%u\t%f\t%f\t%*d\t%f\n", &id, &light, &temperature, &humidity, &battery);
				if (f == 5 && id < NODES && nodes[id]) {
					n = snprintf(obuf, sizeof(obuf), "%s,%d,%d,%3.1f,%3.1f,%4.2f\n", nodes[id], id, light, temperature, humidity, battery);
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
	close(cs);
	for (int i = 0; i < NCLIENTS; i++)
		if (clients[i])
			close(clients[i]);
	return 0;
}
