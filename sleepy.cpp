// Jeelib no longer compiles under Arduino (for me anyway) so
// this is a cut-down version.
// See https://github.com/jcw/jeelib/blob/master/Ports.cpp for (c)

#include <Arduino.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include "sleepy.h"

static volatile byte watchdogCounter;
static byte backupMode = 0;

void Sleepy::watchdogEvent(void) {
	++watchdogCounter;
}

void Sleepy::watchdogInterrupts (char mode) {
#ifndef WDTCSR
#define WDTCSR WDTCR
#endif
	// correct for the fact that WDP3 is *not* in bit position 3!
	if (mode & bit(3))
		mode ^= bit(3) | bit(WDP3);
	// pre-calculate the WDTCSR value, can't do it inside the timed sequence
	// we only generate interrupts, no reset
	byte wdtcsr = mode >= 0 ? bit(WDIE) | mode : backupMode;
	if(mode>=0) backupMode = WDTCSR;
	MCUSR &= ~(1<<WDRF);
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		WDTCSR |= (1<<WDCE) | (1<<WDE); // timed sequence
		WDTCSR = wdtcsr;
	}
}

/// @see http://www.nongnu.org/avr-libc/user-manual/group__avr__sleep.html
void Sleepy::powerDown (void) {
	byte adcsraSave = ADCSRA;
	ADCSRA &= ~ bit(ADEN); // disable the ADC
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		sleep_enable();
		// sleep_bod_disable(); // can't use this - not in my avr-libc version!
#ifdef BODSE
		MCUCR = MCUCR | bit(BODSE) | bit(BODS); // timed sequence
		MCUCR = (MCUCR & ~ bit(BODSE)) | bit(BODS);
#endif
	}
	sleep_cpu();
	sleep_disable();
	// re-enable what we disabled
	ADCSRA = adcsraSave;
}

void Sleepy::loseSomeTime(word msecs) {
	word msleft = msecs;
	// only slow down for periods longer than the watchdog granularity
	while (msleft >= 16) {
		char wdp = 0; // wdp 0..9 corresponds to roughly 16..8192 ms
		// calc wdp as log2(msleft/16), i.e. loop & inc while next value is ok
		for (word m = msleft; m >= 32; m >>= 1)
			if (++wdp >= 9)
				break;
		watchdogCounter = 0;
		watchdogInterrupts(wdp);
		powerDown();
		watchdogInterrupts(-1); // off
		// when interrupted, our best guess is that half the time has passed
		word halfms = 8 << wdp;
		msleft -= halfms;
		if (watchdogCounter == 0) {
			break;
		}
		msleft -= halfms;
	}
	// adjust the milli ticks, since we will have missed several
	extern volatile unsigned long timer0_millis;
	timer0_millis += msecs - msleft;
}
