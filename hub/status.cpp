#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <bcm2835.h>

#include "sensorlib.h"

int mux = -1;
bool verbose = false;
int idle_secs = 300;
int pin = RPI_GPIO_P1_07;

void close_sockets() {
	if (mux >= 0)
		close(mux);

	bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);
	bcm2835_close();
}

void signal_handler(int signo) {
	fatal("Caught: %s\n", strsignal(signo));
}

void set_led(bool on) {
}

int main(int argc, char *argv[]) {
	int opt;
	bool daemon = true;

	atexit(close_sockets);
	while ((opt = getopt(argc, argv, "m:vf")) != -1)
		switch (opt) {
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
			fatal("Usage: %s: -m mux:port [-v] [-f]\n", argv[0]);
		}
	if (mux < 0)
		mux = connect_socket("localhost", 5678);
		
	if (daemon)
		daemon_mode();

	if (!bcm2835_init())
		fatal("initialising bcm2835\n");

	bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);

	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	struct timeval last, now;
	last.tv_sec = 0;

	char buf[256];
	for (;;) {
		fd_set rd;
		FD_ZERO(&rd);
		FD_SET(mux, &rd);

		gettimeofday(&now, 0);
		if (now.tv_sec - last.tv_sec > idle_secs)
			bcm2835_gpio_write(pin, LOW);

		struct timeval dt;
		dt.tv_sec = idle_secs;
		dt.tv_usec = 0;
		if (0 > select(mux+1, &rd, 0, 0, &dt))
			fatal("select: %s\n", strerror(errno));

		if (FD_ISSET(mux, &rd)) {
			int n = sock_read_line(mux, buf, sizeof(buf));
			if (verbose)
				printf("%d: %d [%s]\n", mux, n, buf);
			if (n > 0) {
				sensor_t s;
				parse_sensor_data(buf, &s);

				// FIXME: check it's a wireless one
				last.tv_sec = now.tv_sec;
				bcm2835_gpio_write(pin, LOW);
				bcm2835_gpio_write(pin, HIGH);
			} else if (n == 0)
				fatal("Mux died\n");
		}
	}
}
