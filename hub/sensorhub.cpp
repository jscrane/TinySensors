#include <RF24Network.h>
#include <RF24.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <tinysensor.h>
#include "sensorlib.h"

static int ss = -1, cs = -1;

void close_exit()
{
	if (cs >= 0)
		close(cs);
	if (ss >= 0)
		close(ss);
}

void signal_handler(int signo)
{
	fatal("Caught: %s\n", strsignal(signo));
}

#define IDLE_SECONDS 3600

int main(int argc, char *argv[])
{
	bool verbose = false, daemon = true, watchdog = true;
	int opt;
	atexit(close_exit);
	while ((opt = getopt(argc, argv, "vw")) != -1)
		switch(opt) {
		case 'v':
			verbose = true;
			daemon = false;
			break;
		case 'w':
			watchdog = false;
			break;
		default:
			fatal("Usage: %s [-v] [-w]\n", argv[0]);
		}

	if (daemon)
		daemon_mode();

	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	ss = socket(AF_INET, SOCK_STREAM, 0);
	if (ss < 0)
		fatal("socket: %s\n", strerror(errno));

	int reuse = 1;
	if (0 > setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)))
		fatal("setsockopt: %s\n", strerror(errno));

	struct sockaddr_in serv;
	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	serv.sin_port = htons(5555);
	if (0 > bind(ss, (struct sockaddr *)&serv, sizeof(struct sockaddr)))
		fatal("bind: %s\n", strerror(errno));

	if (0 > listen(ss, 1))
		fatal("listen: %s\n", strerror(errno));

	// because I wired up the CSN and CE pins backwards on the
	// "Slice of Pi" proto-board...
	//RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_26, BCM2835_SPI_SPEED_8MHZ);	
	RF24 radio(RPI_V2_GPIO_P1_26, RPI_V2_GPIO_P1_15, BCM2835_SPI_CLOCK_DIVIDER_32);	
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

			sensor s;
			s.temperature = ((float)temp) / 10;
			s.humidity = ((float)payload.humidity) / 10;
			s.battery = ((float)payload.battery) * 3.3 / 1023.0;
			s.light = payload.light;
			s.node_id = header.from_node;
			s.node_status = payload.status;
			s.msg_id = header.id;
			s.node_time = payload.ms;
			s.node_type = 0;
			s.domoticz_id = 0;

			if (cs >= 0) {
				char buf[1024];
				int n = s.to_csv(buf, sizeof(buf));
				if (0 > write(cs, buf, n)) {
					perror("write");
					close(cs);
					cs = -1;
				}
			}
			if (watchdog)
				time(&last_reading);
		}

		struct timeval timeout;
		timeout.tv_usec = 100000;
		timeout.tv_sec = 0;
		if (cs < 0) {
			fd_set rd;
			FD_ZERO(&rd);
			FD_SET(ss, &rd);
			if (select(ss + 1, &rd, 0, 0, &timeout) > 0) {
				struct sockaddr_in client;
				socklen_t addrlen = sizeof(struct sockaddr_in);
				cs = accept(ss, (struct sockaddr *)&client, &addrlen);
				if (cs < 0)
					fatal("accept: %s\n", strerror(errno));
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
