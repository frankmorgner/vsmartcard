/*
 * MUSCLE SmartCard Development ( http://www.linuxnet.com )
 *
 * Copyright (C) 1999-2002
 *  David Corcoran <corcoran@linuxnet.com>
 * Copyright (C) 2002-2011
 *  Ludovic Rousseau <ludovic.rousseau@free.fr>
 *
 * $Id: debug.c 6551 2013-03-06 13:44:17Z rousseau $
 */

/**
 * @file
 * @brief This handles debugging for libpcsclite.
 */

#include "config.h"
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "debuglog.h"
#include "strlcpycat.h"

#define DEBUG_BUF_SIZE 2048

#ifdef NO_LOG

void log_msg(const int priority, const char *fmt, ...)
{
	(void)priority;
	(void)fmt;
}

#else

/** default level is quiet to avoid polluting fd 2 (possibly NOT stderr) */
static char LogLevel = 20;

static signed char LogDoColor = 0;	/**< no color by default */

static void log_init(void)
{
	char *e;

#ifdef LIBPCSCLITE
	e = getenv("PCSCLITE_DEBUG");
#else
	e = getenv("MUSCLECARD_DEBUG");
#endif
	if (e)
		LogLevel = atoi(e);

	/* log to stderr and stderr is a tty? */
	if (isatty(fileno(stderr)))
	{
		char *term;

		term = getenv("TERM");
		if (term)
		{
			const char *terms[] = { "linux", "xterm", "xterm-color", "Eterm", "rxvt", "rxvt-unicode" };
			unsigned int i;

			/* for each known color terminal */
			for (i = 0; i < sizeof(terms) / sizeof(terms[0]); i++)
			{
				/* we found a supported term? */
				if (0 == strcmp(terms[i], term))
				{
					LogDoColor = 1;
					break;
				}
			}
		}
	}
} /* log_init */

void log_msg(const int priority, const char *fmt, ...)
{
	char DebugBuffer[DEBUG_BUF_SIZE];
	va_list argptr;
	static int is_initialized = 0;

	if (!is_initialized)
	{
		log_init();
		is_initialized = 1;
	}

	if (priority < LogLevel) /* log priority lower than threshold? */
		return;

	va_start(argptr, fmt);
	(void)vsnprintf(DebugBuffer, DEBUG_BUF_SIZE, fmt, argptr);
	va_end(argptr);

	{
		if (LogDoColor)
		{
			const char *color_pfx = "", *color_sfx = "\33[0m";

			switch (priority)
			{
				case PCSC_LOG_CRITICAL:
					color_pfx = "\33[01;31m"; /* bright + Red */
					break;

				case PCSC_LOG_ERROR:
					color_pfx = "\33[35m"; /* Magenta */
					break;

				case PCSC_LOG_INFO:
					color_pfx = "\33[34m"; /* Blue */
					break;

				case PCSC_LOG_DEBUG:
					color_pfx = ""; /* normal (black) */
					color_sfx = "";
					break;
			}
			fprintf(stderr, "%s%s%s\n", color_pfx, DebugBuffer, color_sfx);
		}
		else
			fprintf(stderr, "%s\n", DebugBuffer);
	}
} /* log_msg */

#endif

