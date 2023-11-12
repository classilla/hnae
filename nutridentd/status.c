/* status.c -- routines for extracting status information from the machine
 *
 * Authors: Chris Jalbert
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

#include "tridentd.h"
#include "debug.h"


#ifdef _AIX
#include <fcntl.h>		/* for unbuffered I/O */
#include <utmp.h>		/* for utmp structs and macros */
#include <nlist.h>		/* for struct nlist stuff used by _GetLoads() */

extern int knlist (struct nlist *, int, int) ;

#else	/* _AIX */
#include <Timer.h>
#endif	/* _AIX */


#define	MIN_S	(time_t)60		/* 1 min = MIN_S seconds... */
#define	HOUR_S	(60 * MIN_S)
#define	DAY_S	(24 * HOUR_S)
#define	HOUR_M	(time_t)60		/* 1 hour = HOUR_M minutes */
#define	DAY_M	(24 * HOUR_M)


/******************************************************************************
	==>  Private Globals  <==
******************************************************************************/
static char		_WhoString [BUFSIZ] ;
static int		_NumUsers = 0, _WhoLength ;
#ifdef _AIX
static time_t	_BootTime, _LastMod = 0 ;
#endif	/* _AIX */


/******************************************************************************
	==>  Static Functions  <==
******************************************************************************/
#ifdef _AIX
/*-----------------------------------------------------------------------------
	_CheckUTmp() checks /etc/utmp, a system accounting file, to see if it
	has changed recently. If it has, walks the utmp file looking for users
	and reconstructs the local variables (_NumUsers, _WhoString, _WhoLength)
	that are drawn from it.
-----------------------------------------------------------------------------*/
static void _CheckUTmp (void)
	{
	register char			*szpBuffer ;
	register int			nUsers, nResult ;
	register long			lLength ;
	register struct utmp	*uspEntry ;
	register struct tm		*tmspLocal ;
	struct stat				ssStat ;

	/* Has the utmp file changed since the last time we checked? */
	if (stat (UTMP_FILE, &ssStat))
		return ;
	if (ssStat.st_mtime <= _LastMod)
		return ;
	/* To avoid accessing prematurely, wait a sec before walking the file. */
	if (ssStat.st_mtime == time (NULL))
		return ;
	_LastMod = ssStat.st_mtime ;

#if _DEBUG
	{
	char	szBuffer [40] ;
	tmspLocal = localtime (&(ssStat.st_mtime)) ;
	strftime (szBuffer, sizeof (szBuffer), "%b %e %T", tmspLocal) ;
	DBG ("utmp file changed at %s. Updating globals.\n", szBuffer) ;
	}
#endif

	nUsers = 0 ;
	szpBuffer = _WhoString ;
	lLength = sizeof (_WhoString) ;

	/* Walk the utmp file looking for user entries. */
	while (NULL != (uspEntry = getutent ()))
		if ((uspEntry->ut_type == USER_PROCESS)
			&& uspEntry->ut_user[0])
				{
				nUsers++ ;

				/* Only write the entry if there is enough space. */
				if (lLength < (43 + strlen (uspEntry->ut_host)))
					continue ;

				/* Write the login entry. */
				tmspLocal = localtime (&(uspEntry->ut_time)) ;
				nResult = sprintf (szpBuffer, "%-8.8s    %-12.12s",
										uspEntry->ut_user, uspEntry->ut_line) ;
				lLength -= nResult ;
				szpBuffer += nResult ;

				nResult = strftime (szpBuffer, lLength,
										"%b %e %R    ", tmspLocal) ;
				lLength -= nResult ;
				szpBuffer += nResult ;

				nResult = sprintf (szpBuffer, "%s \n", uspEntry->ut_host) ;
				lLength -= nResult ;
				szpBuffer += nResult ;
				}
	endutent () ;
	_NumUsers = nUsers ;
	_WhoLength = strlen (_WhoString) ;
	}

/*-----------------------------------------------------------------------------
	_GetLoads() makes use of the kernel version of the standard Unix tool,
	nlist, which searches name lists in executables. Although buffered I/O
	does seem to work, the docs say only unbuffered I/O is valid.
-----------------------------------------------------------------------------*/
/*
 * Must agree with SBITS in sys/prod/sched.c
 */
#define FSHIFT	   16
#define FSCALE 	   (1<<FSHIFT)

static void _GetLoads (
	double				*dpArray)
	{
	static struct nlist	_nl = { "avenrun" } ;
	unsigned long		runarray [3] = {0, 0, 0} ;
	register int		kmem ;

	/* Find the "avenrun" entry in the kernel. */
	if (-1 == (kmem = open ("/dev/kmem", O_RDONLY)))
		return ;
	knlist (&_nl, 1, sizeof (struct nlist)) ;
	if (_nl.n_value == lseek (kmem, _nl.n_value, SEEK_SET))
		read (kmem, runarray, sizeof (runarray)) ;
	close (kmem) ;

	/* Convert the avenrun to doubles. */
	*dpArray++ = ((double) runarray[0]) / FSCALE ;
	*dpArray++ = ((double) runarray[1]) / FSCALE ;
	*dpArray++ = ((double) runarray[2]) / FSCALE ;
	}

#else	/* _AIX */
/*-----------------------------------------------------------------------------
	The Mac version is extremely lazy: It recognizes only one user, set to
	the default user name.
-----------------------------------------------------------------------------*/
static void _CheckUTmp (void)
	{
	unsigned long	ulUser ;
	Str32			sUser ;

	if (_NumUsers)
	  return ;

	if (GetDefaultUser (&ulUser, sUser))
		PStringLen (sUser) = 0 ;
	strncpy (_WhoString, PStringStr (sUser), PStringLen (sUser)) ;
	_WhoLength = PStringLen (sUser) ;
	_NumUsers = 1 ;
	}

#define _GetLoads(dp)	dp[0] = dp[1] = dp[2] = 0.5
#endif	/* _AIX */


/******************************************************************************
	==>  Global (Exported) Functions  <==
******************************************************************************/

int GetUptimeString (
	char				*szpBuf,
	long				lLength)
	{
	register char		*szpBuffer = szpBuf ;
	register time_t		tUptime = time (NULL) ;
	register long		lDays, lHours ;
	time_t				tCurrent = tUptime ;
	register struct tm	*tmCurrent = localtime (&tCurrent) ;
	UnsignedWide		uptime ;
	double				daLoads [3] ;

#ifdef _AIX
	tUptime -= _BootTime ;
	tUptime += 30 ;
	tUptime /= MIN_S ;				/* Convert to minutes. */
#else
	Microseconds (&uptime) ;
	tUptime = uptime.lo / 60000 ;	/* Convert to minutes. */
#endif	/* _AIX */

	szpBuffer += strftime (szpBuffer, lLength, "%I:%M%p", tmCurrent) ;

	/* Convert uptime to readable format. */
	lDays = tUptime / DAY_M ;
	tUptime %= DAY_M ;
	lHours = tUptime / HOUR_M ;
	tUptime %= HOUR_M ;
	if (lDays > 1)
		szpBuffer += sprintf (szpBuffer, "  up %ld days,", lDays) ;
	else if (lDays == 1)
		szpBuffer += sprintf (szpBuffer, "  up 1 day,") ;
	szpBuffer += sprintf (szpBuffer, "  %02d:%02d,", lHours, (long) tUptime) ;

	/* Count the number of users. */
	_CheckUTmp () ;
	if (_NumUsers == 1)
		szpBuffer += sprintf (szpBuffer, "  1 user") ;
	else
		szpBuffer += sprintf (szpBuffer, "  %d users", _NumUsers) ;

	/* Determine the load average. */
	_GetLoads (daLoads) ;
	sprintf (szpBuffer, ",  load average: %.2f, %.2f, %.2f\n",
						daLoads[0], daLoads[1], daLoads[2]) ;

	return noErr ;
	}

int GetWhoString (
	char			*szpBuf,
	long			lLength)
	{
	_CheckUTmp () ;
	strncpy (szpBuf, _WhoString, lLength) ;
	return noErr ;
	}

#ifdef _AIX
void InitStatus (void)
	{
	struct utmp	sFilter, *uspEntry ;

	/* Get bootime from utmp file. */
	sFilter.ut_type = BOOT_TIME ;
	if (NULL == (uspEntry = getutid (&sFilter)))
		{
		DBGM ("Couldn't find boot time!\n") ;
		exit (1) ;
		}

	_BootTime = uspEntry->ut_time ;
	DBG ("Boot time was %lu.\n", _BootTime) ;
	endutent () ;
	}

Boolean WhoUpdated (
	time_t	tLast)
	{
	_CheckUTmp () ;
	return (tLast <= _LastMod) ;
	}
#else
Boolean WhoUpdated (
	time_t	tCurrent)
	{
	return false ;
	}
#endif	/* _AIX */
