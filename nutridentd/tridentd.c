/* tridentd.c -- main routine for Status and Status-Who sample service daemons.
 *
 * Author: Chris Jalbert
 * Copyright 1996-1997 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * THIS IS PUBLISHED SAMPLE SOURCE CODE OF APPLE COMPUTER, INC.
 *
 * History:
 * 8/19/96	Chris Jalbert
 *	Initial check in of new sample.
 * 1/3/96	Chris Jalbert
 *	Added code to test new AIX library function, PPCGetSelectMask.
 * 1/22/97	Chris Jalbert
 *	Moved select mask stuff to inactive section to clarify main().
 *	Moved simplified model connection init to main().
 * 2/14/97	Chris Jalbert
 *	Setting a negative timeout on the Mac is BAAAD.
 */


#include <stdio.h>		/* required for _P_tmpdir */
#include <time.h>		/* required for struct timeval */
#include <stdlib.h>
#include <unistd.h>		/* required for chdir() */

/* MacOS header files. */
#include <Types.h>
#include <AppleEvents.h>

#ifndef _AIX
# define _P_tmpdir "Desktop"
#endif	/* _AIX */

#include "tridentd.h"
#include "debug.h"


/******************************************************************************
	==>  Globals  <==
******************************************************************************/
Boolean TimeToQuit = false ;

#if _DEBUG
FILE *__DebugFile ;
#endif


/******************************************************************************
	==>  Global Functions  <==
******************************************************************************/
#ifndef _AIX
int WaitNextAppleEvent (
	EventRecord	*event,
	long		*lpTimeOut)
	{
	long		lTimeOut = (*lpTimeOut == -1) ? 20 : *lpTimeOut ;

	WaitNextEvent (highLevelEventMask, event, (60 * lTimeOut), NULL) ;
	switch (event->what)
		{
		case nullEvent:
			/* Might need to do some processing here. */
			return 0 ;
		case kHighLevelEvent:
			return 1 ;
		}
	return 0 ;
	}
#elif 0 && !SIMPLIFIED
/*-----------------------------------------------------------------------------
	The following block of code (between the SNIPs) should be placed as the
	FIRST directives in any file that will make use of select() of its
	associated structures.
	Because this service daemon does not make use of select(), this header
	info is not compiled.
	This code is included for completeness. Move this block wherever it
	makes the most sense, probably in the file that handles the stuff
	that makes this necessary.
-----------------------------------------------------------------------------*/
				----- 8< *SNIP* >8 -----
/* A lot of the following stuff works only when _ALL_SOURCE is defined
 * because it is only available on BSD compatable systems.
 */

#ifdef _AIX
# ifndef _ALL_SOURCE
#  define _ALL_SOURCE
#  define USING_ALL_SOURCE
# endif	/* _ALL_SOURCE */

# include <sys/select.h>	/* required for fd_set and select() */

# ifdef USING_ALL_SOURCE
#  undef _ALL_SOURCE
#  undef USING_ALL_SOURCE
# endif	/* USING_ALL_SOURCE */
#endif	/* _AIX */

/* It doesn't really matter at this point, but the rest can be compiled
 * as strict POSIX or X/OPEN.
 */
				----- 8< *SNIP* >8 -----

/*-----------------------------------------------------------------------------
	HandleSelect() demonstrates how to make a select call manually.
	As near as I can tell, it is not useful to select on a file
	descriptor that references a regular file because it will
	always be ready for I/O.
	NB: This function is not currently used by the sample.
-----------------------------------------------------------------------------*/
long HandleSelect (
	long	lTimeOut)
	{
	extern Boolean GetMySelectMask (fd_set *) ;
	register int	nResult ;
	fd_set			fdsMask ;
	struct timeval	tsTimeout = { 0, 0 } ;

	PPCGetSelectMask (&fdsMask) ;
	tsTimeout.tv_sec = lTimeOut ;
	if (GetMySelectMask (&fdsMask))
		{
		DBGM ("main(): Making select call manually.\n") ;
		nResult = select (FD_SETSIZE, &fdsMask, 0, 0, &tsTimeout) ;
		if (nResult > 0)
			nResult = CheckMySelectMask (&fdsMask) ;
		return 0 ;
		}
	return lTimeOut ;
	}
#endif	/* _AIX && !SIMPLIFIED */


/******************************************************************************
	==>  Main Function  <==
******************************************************************************/
void main (
	int			argc,
	char		**argv)
	{
	OSErr 		result ;

//fprintf(stderr, "%d\n", __LINE__);
	chdir (_P_tmpdir) ;	/* so core files end up in /tmp */

	DBGSetup ((SIMPLIFIED ? "/tmp/simpled.debug" : "/tmp/flexibled.debug")) ;
	DBGM ("Sleeping 15 seconds to allow dbx to attach...\n") ;
	DBGPause (15) ;
	DBGM ("Starting initialization procedures.\n") ;

//fprintf(stderr, "%d\n", __LINE__);
	/* Install Apple event handlers. */
	if (result = InitAEStuff (0L))
		{
		DBG ("Error %d during InitAEStuff().\n", result) ;
		DBGClose () ;
		exit (result) ;
		}

//fprintf(stderr, "%d\n", __LINE__);
	/*** Stub or delete this call for simplified service daemons!! ***/
	/* Register services with PPC Toolbox. */
	if (result = InitPPCStuff ())
		{
		DBG ("Error %d during InitPPCStuff().\n", result) ;
		DBGClose () ;
		exit (result) ;
		}

//fprintf(stderr, "%d\n", __LINE__);
	InitStatus () ;

#if SIMPLIFIED
	{
	ClientPtr	cspInherit ;
	/* Create the initial, inherited client connection. */
	if (NULL != (cspInherit = NewClient (kUptimeType)))
		ClientSetSession (cspInherit, 0) ;
	}
#endif	/* SIMPLIFIED */

	while (!TimeToQuit && (result >= 0))
		{
		long		lTimeOut ;
		EventRecord	event ;

		/*** Stub or delete this call for simplified service daemons!! ***/
		/* Give the Mac OS version some time to deal with PPC overhead. */
//fprintf(stderr, "%d\n", __LINE__);
		result = PPCTimeSlice () ;

		/* Update the timers in the list of connections. */
		lTimeOut = UpdateClients () ;

		DBG ("main(): setting timeout of %ld seconds.\n", lTimeOut) ;

		/*** HandleSelect() call would go here. ***/

		/* Wait for an Apple Event or a new connection. */
		if (0 < (result = WaitNextAppleEvent (&event, &lTimeOut)))
			{
			result = AEProcessAppleEvent (&event) ;
			DBG ("main(): AEProcessAppleEvent() returned (%d)\n", result) ;
			}
		}

	/*** Stub or delete this call for simplified service daemons!! ***/
fprintf(stderr, "%d\n", __LINE__);
	PPCShutDown () ;
	DBGM ("Shutting down...\n") ;
	DBGClose () ;
	exit (0) ;
	}
