#include <Arduino.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include "wdt.h"

static void enable(uint8_t e) {
	noInterrupts();
	uint8_t b = _BV(WDIE) | e;
	WDTCSR = _BV(WDCE) | _BV(WDE);
	WDTCSR = b;
	interrupts();
}

ISR(WDT_vect) {
	enable(0);
}

void wdt_sleep(uint8_t wdto, unsigned n) {
	while (n--) {
		enable(wdto);
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();
		interrupts();
		sleep_cpu();
		sleep_disable();
	}
}
