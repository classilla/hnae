/* status.c -- routines for extracting status information from the machine
 *
 * Cameron Kaiser: This has been commandeered to run a Perl script and use its
 * output.
 * My changes, Copyright 2023 Cameron Kaiser, All rights reserved.
 *
 * Original Authors: Chris Jalbert
 * Copyright 1996-1997 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * THIS IS PUBLISHED SAMPLE SOURCE CODE OF APPLE COMPUTER, INC.
 *
 * History:
 * 8/27/96 Chris Jalbert
 *	Initial check in of new sample.
 * 10/9/96 Chris Jalbert
 *	Changed AIX code from using pipes to accessing services directly.
 * 1/21/97 Chris Jalbert
 *	Cleaned up code to parse user info file only when necessary.
 *	This file has been modularized and can be used without external
 *	dependencies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
/* ?! */
#ifndef NBBY
#define NBBY 8
#endif
#include <sys/select.h>

#include "tridentd.h"
#include "debug.h"

#include <fcntl.h>		/* for unbuffered I/O */


/******************************************************************************
	==>  Private Globals  <==
******************************************************************************/

static int		_NumUsers = 0;
static time_t		_LastMod = 0 ;


/******************************************************************************
	==>  Static Functions  <==
******************************************************************************/

/* none */

/******************************************************************************
	==>  Global (Exported) Functions  <==
******************************************************************************/

/* Cameron's note: BUFSIZ is usually 1024 bytes. Not a lot. */

int GetString (
	char			*szpBuf,
	long			lLength)
	{
#if 0
	char teststring[] = "hello world\nknow your daddy\npeople will ask\n";
	strncpy (szpBuf, teststring, strlen(teststring));
#else
	FILE *f;
	int fd, off, c;
	fd_set ff;
	struct timeval tv;

	f = popen("/usr/bin/perl /home/spectre/src/hnd/hn.pl", "r");
	if (!f || ((fd = fileno(f)) < 1)) {
		sprintf(szpBuf, "popen: %s\n", strerror(errno));
		return noErr; /* well, _we_ didn't do it */
	}
	tv.tv_sec = HEARTBEAT_CHECK - 2;
	tv.tv_usec = 0;
	FD_ZERO(&ff);
	FD_SET(fd, &ff);
	if (select(fd+1, &ff, NULL, NULL, &tv) < 1) {
		if (!tv.tv_sec) {
			sprintf(szpBuf, "timeout waiting for script\n");
		} else {
			sprintf(szpBuf, "select: %s\n", strerror(errno));
		}
		return noErr;
	}
	/* Block-read the rest. We ass-U-me that the dependent script will
	 * timeout before we need to reply to another heartbeat. */
	off = 0;
	while (off < (BUFSIZ-3) && (c = fgetc(f)) != EOF) 
		szpBuf[off++] = (char)c;
	(void)pclose(f);
	szpBuf[off++] = '\n';
	szpBuf[off] = '\0';
#endif
	return noErr ;
	}

void InitStatus (void)
	{
/* insert joke here */
	}

Boolean StringUpdated (
	time_t	tLast)
	{
	return true;
/*
	return (tLast <= _LastMod) ;
*/
	}
