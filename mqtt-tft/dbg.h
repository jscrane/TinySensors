#ifndef __DBG_H__
#define __DBG_H__

extern bool debugging;
#define OUT(x) Serial.x
#define DBG(x) if (debugging) { OUT(x); }
#define ERR(x) OUT(x)

#endif
