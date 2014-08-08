#include <RF24Network.h>
#include <RF24.h>
#include <my_global.h>
#include <mysql.h>
#include <getopt.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <tinysensor.h>

static MYSQL *db_conn;
static int ss, cs;

void close_exit()
{
	if (db_conn)
		mysql_close(db_conn);
	if (cs > 0)
		close(cs);
	if (ss > 0)
		close(ss);
	exit(1);
}

void fatal(const char *op, const char *error)
{
	fprintf(stderr, "%s: %s\n", op, error);
	close_exit();
}

void signal_handler(int signo)
{
	fatal("caught", strsignal(signo));
}

#define IDLE_SECONDS 300

int main(int argc, char *argv[])
{
	bool verbose = false, sock = true, daemon = true, watchdog = true;
	int opt;
	while ((opt = getopt(argc, argv, "vsw")) != -1)
		switch(opt) {
		case 'v':
			verbose = true;
			daemon = false;
			break;
		case 's':
			sock = false;
			break;
		case 'w':
			watchdog = false;
			break;
		default:
			fprintf(stderr, "Usage: %s [-v] [-s] [-w]\n", argv[0]);
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

	if (verbose) 
		printf("MySQL client version: %s\n", mysql_get_client_info());

	db_conn = mysql_init(0);

	if (mysql_real_connect(db_conn, "localhost", USER, PASS, "sensors", 0, NULL, 0) == NULL)
		fatal("mysql_real_connect", mysql_error(db_conn));

	if (sock) {
		ss = socket(AF_INET, SOCK_STREAM, 0);
		if (ss < 0)
			fatal("socket", strerror(errno));

		struct sockaddr_in serv;
		memset(&serv, 0, sizeof(serv));
		serv.sin_family = AF_INET;
		serv.sin_addr.s_addr = htonl(INADDR_ANY);
		serv.sin_port = htons(5555);
		if (0 > bind(ss, (struct sockaddr *)&serv, sizeof(struct sockaddr)))
			fatal("bind", strerror(errno));

		if (0 > listen(ss, 1))
			fatal("listen", strerror(errno));
	}

	RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_26, BCM2835_SPI_SPEED_8MHZ);	
	radio.begin();
	radio.enableDynamicPayloads();
	radio.setAutoAck(true);
	radio.powerUp();

	const uint16_t this_node = 0;
	RF24Network network(radio);
	network.begin(90, this_node);

	time_t last_reading;
	if (watchdog)
		time(&last_reading);
	for (;;) {
		network.update();

		while (network.available()) {
			RF24NetworkHeader header;
			sensor_payload_t payload;
			network.read(header, &payload, sizeof(payload));

			// fixup for negative temperature
			short temp = payload.temperature;
			if ((temp >> 8) == 0x7f)
				temp = temp - 32768;
			float temperature = ((float)temp) / 10;
			float humidity = ((float)payload.humidity) / 10;
			float battery = ((float)payload.battery) * 3.3 / 1023.0;

			if (cs > 0) {
				char buf[1024];
				int n = sprintf(buf, "%d\t%d\t%3.1f\t%3.1f\t%d\t%4.2f\t%u\t%d\t%u\n", 
						header.from_node, payload.light, temperature, humidity, payload.status, 
						battery, header.type, header.id, payload.ms / 1000);
				if (0 > write(cs, buf, n)) {
					perror("write");
					close(cs);
					cs = 0;
				}
			}

			if (header.from_node > 0) {
				char buf[1024];
				sprintf(buf, "INSERT INTO sensor_data (node_id,node_ms,light,humidity,temperature,battery,status,msg_id) VALUES(%d,%d,%d,%.1f,%.1f,%.2f,%d,%d)", 
						header.from_node, payload.ms, payload.light, humidity, temperature, battery, payload.status, header.id);

				if (verbose)
					puts(buf);

				if (mysql_query(db_conn, buf))
					fatal("insert", mysql_error(db_conn));
			}
			if (watchdog)
				time(&last_reading);
		}

		struct timeval timeout;
		timeout.tv_usec = 100000;
		timeout.tv_sec = 0;
		if (!cs) {
			fd_set rd;
			FD_ZERO(&rd);
			if (ss > 0)
				FD_SET(ss, &rd);
			if (select(ss + 1, &rd, 0, 0, &timeout) > 0) {
				struct sockaddr_in client;
				socklen_t addrlen = sizeof(struct sockaddr_in);
				cs = accept(ss, (struct sockaddr *)&client, &addrlen);
				if (cs < 0)
					fatal("accept", strerror(errno));
	
				const char *header = "node\tlight\tdegC\thum%\tstat\tVbatt\ttype\tmsg-id\ttime\n";
				write(cs, header, strlen(header));
			}
		} else {
			// we have a client, just sleep
			usleep(timeout.tv_usec);
		}
		if (watchdog) {
			time_t now;
			time(&now);
			if (now - last_reading > IDLE_SECONDS) {
				// start the destruct sequence
				if (verbose)
					printf("triggering watchdog\n");
				int fd = open("/dev/watchdog", O_RDONLY);
				if (0 > fd)
					perror("opening watchdog");
				else
					close(fd);
				// bail out
				break;
			}
		}
	}
	return 0;
}
