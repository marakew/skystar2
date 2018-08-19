/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#ifndef _debug_h_
#define _debug_h_

#include <sys/syslog.h>

#ifdef DEBUG_MODE
#define DBG_(dbg_, fmt, args...) \
	do { if (debug >= dbg_) printf("%s: " fmt, __FUNCTION__ , ## args); } while(0)
#else
#define DBG_(dbg_, fmt, args...) \
	do { } while(0)
#endif

#define DBG(fmt, args...)	DBG_(1, fmt, ## args)

#define DBG1(fmt, args...)	DBG_(1, fmt, ## args)
#define DBG2(fmt, args...)	DBG_(2, fmt, ## args)
#define DBG3(fmt, args...)	DBG_(3, fmt, ## args)
#define DBG4(fmt, args...)	DBG_(4, fmt, ## args)

#endif
