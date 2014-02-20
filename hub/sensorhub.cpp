#include <RF24Network.h>
#include <RF24.h>
#include <my_global.h>
#include <mysql.h>
#include <getopt.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>

struct payload_t
{
	uint32_t ms;
	uint8_t light, status;
	int16_t humidity, temperature;
	uint16_t battery;
};

static MYSQL *db_conn;
static int ss, cs;

void signal_handler(int signo)
{
	fprintf(stderr, "Caught %d\n", signo);
	if (db_conn)
		mysql_close(db_conn);
	if (cs > 0)
		close(cs);
	if (ss > 0)
		close(ss);
	exit(1);
}

void close_exit(const char *msg, MYSQL *db_conn)
{
	fprintf(stderr, "%s: %s\n", msg, mysql_error(db_conn));
	mysql_close(db_conn);
	exit(1);
}

int main(int argc, char *argv[])
{
	bool verbose = false, sock = true;
	int opt;
	while ((opt = getopt(argc, argv, "vs")) != -1)
		switch(opt) {
		case 'v':
			verbose = true;
			break;
		case 's':
			sock = false;
			break;
		default:
			fprintf(stderr, "Usage: %s [-v] [-s]\n", argv[0]);
			exit(1);
		}

	signal(SIGINT, signal_handler);

	if (verbose) 
		printf("MySQL client version: %s\n", mysql_get_client_info());

	MYSQL *db_conn = mysql_init(0);

	if (mysql_real_connect(db_conn, "localhost", "sensors", "s3ns0rs", "sensors", 0, NULL, 0) == NULL)
		close_exit("mysql_real_connect", db_conn);

	if (sock) {
		ss = socket(AF_INET, SOCK_STREAM, 0);
		if (ss < 0) {
			perror("socket");
			sock = false;
		} else {
			struct sockaddr_in serv;
			memset(&serv, 0, sizeof(serv));
			serv.sin_family = AF_INET;
			serv.sin_addr.s_addr = htonl(INADDR_ANY);
			serv.sin_port = htons(5555);
			if (0 > bind(ss, (struct sockaddr *)&serv, sizeof(struct sockaddr))) {
				perror("bind");
			} else if (0 > listen(ss, 1)) {
				perror("listen");
			}
		}
	}

	RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_26, BCM2835_SPI_SPEED_8MHZ);	
	radio.begin();
	radio.enableDynamicPayloads();
	radio.setAutoAck(true);
	radio.powerUp();

	const uint16_t this_node = 0;
	RF24Network network(radio);
	network.begin(90, this_node);

	for (;;) {
		network.update();
		while (network.available()) {
			RF24NetworkHeader header;
			payload_t payload;
			network.read(header, &payload, sizeof(payload));

			float humidity = ((float)payload.humidity)/10;
			float temperature = ((float)payload.temperature)/10;
			float battery = ((float)payload.battery) * 3.3 / 1023.0;

			if (cs > 0) {
				char buf[1024];
				int n = sprintf(buf, "#%d from %d %u at %d: %d %d %3.1f %3.1f %4.2f\n", 
						header.id, header.from_node, header.type,
						payload.ms, payload.status, payload.light, 
						humidity, temperature, battery);
				if (0 > write(cs, buf, n)) {
					perror("write");
					close(cs);
					cs = 0;
				}
			}

			if (header.from_node > 0) {
				char buf[1024];
				sprintf(buf, "INSERT INTO sensordata (node_id,node_ms,light,humidity,temperature,battery,th_status,msg_id,device_type_id) VALUES(%d,%d,%d,%.1f,%.1f,%.2f,%d,%d,%u)", 
						header.from_node, payload.ms, payload.light, humidity, temperature, battery, payload.status, header.id, header.type);

				if (verbose) {
					puts(buf);
					printf("#%d from %d %u at %d: %d %d %3.1f %3.1f %4.2f\n", 
						header.id, header.from_node, header.type, 
						payload.ms, payload.status, payload.light, 
						humidity, temperature, battery);
				}

				if (mysql_query(db_conn, buf))
					close_exit("insert", db_conn);
			}
		}

		struct timeval timeout;
		timeout.tv_usec = 100000;
		timeout.tv_sec = 0;
		fd_set rd;
		FD_ZERO(&rd);
		if (ss > 0)
			FD_SET(ss, &rd);
/* not yet
		if (cs > 0)
			FD_SET(cs, &rd);
*/
		if (select(ss + 1, &rd, 0, 0, &timeout) > 0) {
			struct sockaddr_in client;
			socklen_t addrlen = sizeof(struct sockaddr_in);
			cs = accept(ss, (struct sockaddr *)&client, &addrlen);
			if (cs < 0)
				perror("accept");
		}
	}
	return 0;
}
