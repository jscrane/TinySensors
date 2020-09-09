#include <Arduino.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include "wdt.h"

static void enable(uint8_t e) {
	cli();
	uint8_t b = e? _BV(WDIE) | e: 0;
	WDTCSR = _BV(WDCE) | _BV(WDE);
	WDTCSR = b;
	sei();
}

ISR(WDT_vect) {
	enable(0);
}

void wdt_sleep(unsigned secs) {
	power_usi_disable();

	uint8_t adcsra = ADCSRA;
	ADCSRA &= ~_BV(ADEN);
	power_adc_disable();

	while (secs) {
		uint8_t e = 0;
		if (secs >= 8) {
			e = _BV(WDP3) | _BV(WDP0);
			secs -= 8;
		} else if (secs >= 4) {
			e = _BV(WDP3);
			secs -= 4;
		} else if (secs >= 2) {
			e = _BV(WDP2) | _BV(WDP1) | _BV(WDP0);
			secs -= 2;
		} else {
			e = _BV(WDP2) | _BV(WDP1);
			secs = 0;
		}
		enable(e);
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		cli();
		sleep_enable();
		sleep_bod_disable();
		sei();
		sleep_cpu();
		sleep_disable();
	}

	ADCSRA = adcsra;
	power_adc_enable();
	power_usi_enable();
}
