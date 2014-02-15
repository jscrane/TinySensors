#include <RF24Network.h>
#include <RF24.h>
#include <my_global.h>
#include <mysql.h>
#include <getopt.h>
#include <unistd.h>

struct payload_t
{
	uint32_t ms;
	uint8_t light, status;
	int16_t humidity, temperature;
	uint16_t battery;
};

static MYSQL *con;

void signal_handler(int signo)
{
	fprintf(stderr, "Caught %d\n", signo);
	if (con)
		mysql_close(con);
	exit(1);
}

void close_exit(const char *msg, MYSQL *con)
{
	fprintf(stderr, "%s: %s\n", msg, mysql_error(con));
	mysql_close(con);
	exit(1);
}

int main(int argc, char *argv[])
{
	bool verbose = false;
	int opt;
	while ((opt = getopt(argc, argv, "v")) != -1)
		switch(opt) {
		case 'v':
			verbose = true;
			break;
		default:
			fprintf(stderr, "Usage: %s [-v]\n", argv[0]);
			exit(1);
		}

	signal(SIGINT, signal_handler);

	if (verbose) 
		printf("MySQL client version: %s\n", mysql_get_client_info());
	MYSQL *con = mysql_init(0);

	if (mysql_real_connect(con, "localhost", "sensors", "s3ns0rs", "sensors", 0, NULL, 0) == NULL)
		close_exit("connect", con);

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

			char buf[1024];
			float humidity = ((float)payload.humidity)/10;
			float temperature = ((float)payload.temperature)/10;
			float battery = ((float)payload.battery) * 3.3 / 1023.0;

			if (header.from_node > 0) {
				sprintf(buf, "INSERT INTO sensordata VALUES(null,%d,%d,%d,%.1f,%.1f,%.2f,%d,%d,null)", header.from_node, payload.ms, payload.light, humidity, temperature, battery, payload.status, header.id);
				if (mysql_query(con, buf))
					close_exit("insert", con);
			}

			if (verbose) {
				puts(buf);
				printf("#%d from %d at %d: %d %d %3.1f %3.1f %4.2f\n", 
					header.id, header.from_node, payload.ms, 
					payload.status, payload.light, 
					humidity, temperature, battery);
			}
		}
		usleep(100000);
	}
	return 0;
}
