/* debug.h -- header for simplifying debug output.
 *
 * Author: Chris Jalbert
 * Copyright 1996-1997 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * THIS IS PUBLISHED SAMPLE SOURCE CODE OF APPLE COMPUTER, INC.
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifndef _DEBUG
#define _DEBUG 0
#endif

#if _DEBUG

#include <stdio.h>
#include <stdarg.h>

extern FILE *__DebugFile;
static char *tprintf (char *format, ...)
{
static char	_buffer[512];
va_list		args;

	va_start(args, format);
	vsprintf(_buffer, format, args);
	va_end(args);
	return _buffer;
}

#define DBGSetup(path) __DebugFile = fopen (path, "w")
#define DBGPause(secs)	sleep (secs)
#define DBGClose()		fclose (__DebugFile)
#define DBG(x,a)		(__DebugFile == NULL) ? 0	\
							: fprintf (__DebugFile, x, a), fflush (__DebugFile)
#define DBGM(x)			(__DebugFile == NULL) ? 0	\
							: fputs (x, __DebugFile), fflush (__DebugFile)

#else	/* !_DEBUG */

#define DBGSetup(path)
#define DBGPause(secs)
#define DBGClose()
#define DBG(x,a)
#define DBGM(x)

#endif	/* _DEBUG */

#endif	/* _DEBUG_H_ */
