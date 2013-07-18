/*
 * MUSCLE SmartCard Development ( http://www.linuxnet.com )
 *
 * Copyright (C) 1999-2004
 *  David Corcoran <corcoran@linuxnet.com>
 * Copyright (C) 1999-2011
 *  Ludovic Rousseau <ludovic.rousseau@free.fr>
 *
 * $Id: debuglog.h 5854 2011-07-09 11:10:32Z rousseau $
 */

/**
 * @file
 * @brief This handles debugging.
 *
 * @note log message is sent to syslog or stderr depending on --foreground
 * command line argument
 *
 * @code
 * Log1(priority, "text");
 *  log "text" with priority level priority
 * Log2(priority, "text: %d", 1234);
 *  log "text: 1234"
 * the format string can be anything printf() can understand
 * Log3(priority, "text: %d %d", 1234, 5678);
 *  log "text: 1234 5678"
 * the format string can be anything printf() can understand
 * LogXxd(priority, msg, buffer, size);
 *  log "msg" + a hex dump of size bytes of buffer[]
 * @endcode
 */

#ifndef __debuglog_h__
#define __debuglog_h__

#ifndef PCSC_API
#define PCSC_API
#endif

enum {
	DEBUGLOG_NO_DEBUG = 0,
	DEBUGLOG_SYSLOG_DEBUG,
	DEBUGLOG_STDOUT_DEBUG,
	DEBUGLOG_STDOUT_COLOR_DEBUG
};

#define DEBUG_CATEGORY_NOTHING  0
#define DEBUG_CATEGORY_APDU     1
#define DEBUG_CATEGORY_SW       2

enum {
	PCSC_LOG_DEBUG = 0,
	PCSC_LOG_INFO,
	PCSC_LOG_ERROR,
	PCSC_LOG_CRITICAL
};

/* You can't do #ifndef __FUNCTION__ */
#if !defined(__GNUC__) && !defined(__IBMC__)
#define __FUNCTION__ ""
#endif

#ifndef __GNUC__
#define __attribute__(x) /*nothing*/
#endif

#ifdef NO_LOG

#define Log0(priority) do { } while(0)
#define Log1(priority, fmt) do { } while(0)
#define Log2(priority, fmt, data) do { } while(0)
#define Log3(priority, fmt, data1, data2) do { } while(0)
#define Log4(priority, fmt, data1, data2, data3) do { } while(0)
#define Log5(priority, fmt, data1, data2, data3, data4) do { } while(0)
#define Log9(priority, fmt, data1, data2, data3, data4, data5, data6, data7, data8) do { } while(0)
#define LogXxd(priority, msg, buffer, size) do { } while(0)

#define DebugLogA(a) 
#define DebugLogB(a, b) 
#define DebugLogC(a, b,c) 

#else

#define Log0(priority) log_msg(priority, "%s:%d:%s()", __FILE__, __LINE__, __FUNCTION__)
#define Log1(priority, fmt) log_msg(priority, "%s:%d:%s() " fmt, __FILE__, __LINE__, __FUNCTION__)
#define Log2(priority, fmt, data) log_msg(priority, "%s:%d:%s() " fmt, __FILE__, __LINE__, __FUNCTION__, data)
#define Log3(priority, fmt, data1, data2) log_msg(priority, "%s:%d:%s() " fmt, __FILE__, __LINE__, __FUNCTION__, data1, data2)
#define Log4(priority, fmt, data1, data2, data3) log_msg(priority, "%s:%d:%s() " fmt, __FILE__, __LINE__, __FUNCTION__, data1, data2, data3)
#define Log5(priority, fmt, data1, data2, data3, data4) log_msg(priority, "%s:%d:%s() " fmt, __FILE__, __LINE__, __FUNCTION__, data1, data2, data3, data4)
#define Log9(priority, fmt, data1, data2, data3, data4, data5, data6, data7, data8) log_msg(priority, "%s:%d:%s() " fmt, __FILE__, __LINE__, __FUNCTION__, data1, data2, data3, data4, data5, data6, data7, data8)
#define LogXxd(priority, msg, buffer, size) log_xxd(priority, msg, buffer, size)

#define DebugLogA(a) Log1(PCSC_LOG_INFO, a)
#define DebugLogB(a, b) Log2(PCSC_LOG_INFO, a, b)
#define DebugLogC(a, b,c) Log3(PCSC_LOG_INFO, a, b, c)

#endif	/* NO_LOG */

PCSC_API void log_msg(const int priority, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));

PCSC_API void log_xxd(const int priority, const char *msg,
	const unsigned char *buffer, const int size);

void DebugLogSuppress(const int);
void DebugLogSetLogType(const int);
int DebugLogSetCategory(const int);
void DebugLogCategory(const int, const unsigned char *, const int);
PCSC_API void DebugLogSetLevel(const int level);

#endif							/* __debuglog_h__ */

