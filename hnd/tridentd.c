/* tridentd.c -- main routine for sample service daemons.
 *
 * Cameron Kaiser's note: this still has remnants of the old uptime daemon
 * mostly because I'm too lazy to remove them all, but the only thing that
 * actually generates data is the old "who" view, which is now the String
 * viewer. Also, this version is strictly AIX only, and mostly assumes gcc,
 * though it should still build with xlC.
 *
 * My changes, Copyright 2023 Cameron Kaiser. All rights reserved.
 *
 * Original Author: Chris Jalbert
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

/* none */

/******************************************************************************
	==>  Main Function  <==
******************************************************************************/
void main (
	int			argc,
	char		**argv)
	{
	OSErr 		result ;

	chdir (_P_tmpdir) ;	/* so core files end up in /tmp */

	DBGSetup ((SIMPLIFIED ? "/tmp/simpled.debug" : "/tmp/flexibled.debug")) ;
	DBGM ("Sleeping 15 seconds to allow dbx to attach...\n") ;
	DBGPause (15) ;
	DBGM ("Starting initialization procedures.\n") ;

	/* Install Apple event handlers. */
	if (result = InitAEStuff (0L))
		{
		DBG ("Error %d during InitAEStuff().\n", result) ;
		DBGClose () ;
		exit (result) ;
		}

	InitStatus () ;

	{
	ClientPtr	cspInherit ;
	/* Create the initial, inherited client connection. */
	if (NULL != (cspInherit = NewClient (kWhoType)))
		ClientSetSession (cspInherit, 0) ;
	}

	while (!TimeToQuit && (result >= 0))
		{
		long		lTimeOut ;
		EventRecord	event ;

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

	DBGM ("Shutting down...\n") ;
	DBGClose () ;
	exit (0) ;
	}
