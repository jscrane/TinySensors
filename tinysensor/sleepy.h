#ifndef __SLEEPY_H__
#define __SLEEPY_H__

class Sleepy {
public:
	static void watchdogEvent(void);
	static void watchdogInterrupts(char mode);

	static void powerDown(void);
	static void loseSomeTime(word ms);
};

#endif
