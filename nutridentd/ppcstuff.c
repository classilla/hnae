/* ppcstuff.c -- PPC Toolbox calls for flexibled sample daemon.
 *
 * Original Author: Chris Jalbert
 * Copyright 1996-1997 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * THIS IS PUBLISHED SAMPLE SOURCE CODE OF APPLE COMPUTER, INC.
 *
 * History:
 * 8/19/96	Chris Jalbert
 *	First checked in. Several ideas borrowed from DTS / Jim Luther's
 *	Remote Control pascal sample.
 * 2/14/97	Chris Jalbert
 *	Added code to clean up properly if a port failed to open. The Mac OS
 *	does not close all ports opened by an application if it exits
 *	abnormally.
 */


#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Includes for MacOS prototypes. */
#include <Types.h>
#include <Errors.h>
#include <Script.h>		/* For smRoman #define. */
#ifdef _AIX
#include <PPCToolBox.h>
#else
#include <PPCToolbox.h>
#endif

#include "tridentd.h"
#include "debug.h"

#define kSigString	"\pJVLN"


/******************************************************************************
	==>  Private Globals  <==
******************************************************************************/
static LocationNameRec	_LocationRecord ;
static PPCPortRec		_UptimeRecord
					= {smRoman, "\pNew Status Demo", ppcByCreatorAndType} ;
static PPCPortRefNum	_UptimePort ;
static PPCInformPBRec	_UptimePB ;
static ClientPtr		_UptimeClient ;
static PPCPortRec		_WhoRecord
					= {smRoman, "\pLogin Demo", ppcByCreatorAndType} ;
static PPCPortRefNum	_WhoPort ;
static PPCInformPBRec	_WhoPB ;
static ClientPtr		_WhoClient ;
static Str32			_NewUser ;


/******************************************************************************
	==>  Static Callback Functions  <==
******************************************************************************/
/*-----------------------------------------------------------------------------
	There is no real reason that the Uptime and Who services shouldn't
	share the same callback function because the services are identical
	with the exception of the command names of their data sources. However,
	most apps would offer services that may use different sets of AppleEvents,
	so they would likely use separate callback functions to simplify coding.
-----------------------------------------------------------------------------*/
static pascal void _InformCallback (
	PPCParamBlockPtr	pbParam)
	{
	PPCInformPBPtr		pb = &pbParam->informParam;
	PPCParamBlockRec	param ;
	OSErr				result ;

	/* These operations are valid for all sub-param blocks. */
	memset (&param, 0, sizeof (PPCParamBlockRec)) ;
	param.endParam.sessRefNum = pb->sessRefNum ;

	/* Bail and close the connection if there was an error. */
	if (pb->ioResult)
		{
		result = PPCEndSync (&param.endParam) ;
		return ;
		}

#if _DEBUG
	{
	time_t				tCurrent = time (NULL) ;
	DBGM (tprintf ("\n------------------- %s"
					"New %s user '%s' on conn #%ld.\n",
				asctime (localtime (&tCurrent)),
				((pb->portRefNum == _WhoPort) ? "WHO" : "UPTIME"),
				(*pb->userName ? (char *) &pb->userName[1] : "<Mac guest>"),
				pb->sessRefNum)) ;
	}
#endif

	/* Tell the client that it has been accepted. */
	if (result = PPCAcceptSync (&param.acceptParam))
		{
		DBG ("Yikes! PPCAccept returned %d!\n", result) ;
		/* Need to issue an end if accept failed. */
		memset (&param, 0, sizeof (PPCParamBlockRec)) ;
		param.endParam.sessRefNum = pb->sessRefNum ;
		result = PPCEndSync (&param.endParam) ;
		return ;
		}

	/* Fill in the connection object. */
	if (pb->portRefNum == _WhoPort)
		{
		ClientSetSession (_WhoClient, pb->sessRefNum) ;
		_WhoClient = NULL ;
		}
	else if (pb->portRefNum == _UptimePort)
		{
		ClientSetSession (_UptimeClient, pb->sessRefNum) ;
		_UptimeClient = NULL ;
		}
	}



/******************************************************************************
	==>  Global (Exported) Functions  <==
******************************************************************************/

/*-----------------------------------------------------------------------------
	PPCTimeSlice really isn't necessary on AIX -- all the necessary
	processing can occur inside the inform callback -- but including it
	allows the same source code to run on both MacOS and AIX. Nifty, huh?
-----------------------------------------------------------------------------*/
OSErr PPCTimeSlice (void)
	{
	OSErr			result ;

	if (_WhoClient == NULL)
		if (NULL == (_WhoClient = NewClient (kWhoType)))
			return memFullErr ;
	if (_UptimeClient == NULL)
		if (NULL == (_UptimeClient = NewClient (kUptimeType)))
			return memFullErr ;

	/* Tell the system that the app is ready to handle connections. */
	if (_UptimePB.ioResult != 1)
		if (result = PPCInformAsync (&_UptimePB))
			return result ;
	if (_WhoPB.ioResult != 1)
		if (result = PPCInformAsync (&_WhoPB))
			return result ;
	return result ;
	}
	
/*-----------------------------------------------------------------------------
	InitPPCStuff initializes the PPC library, opens (registers) two
	listening ports, one for 'uptime' and one for 'who' connections, and
	notifies the PPC TB that it is ready to accept connections.
-----------------------------------------------------------------------------*/
OSErr InitPPCStuff (void)
	{
	PPCParamBlockRec	param ;
	PPCOpenPBPtr		open = &param.openParam ;
	PPCClosePBPtr		close = &param.closeParam ;
	OSErr				err ;

	/* Init the library (and stream to ppcd on AIX). */
	PPCInit () ;


	/*** Define and open two ports, one for uptime and one for who. ***/

	/* Fill in location record. */
	_LocationRecord.locationKindSelector = ppcNBPTypeLocation ;
	strncpy ((char *) _LocationRecord.u.nbpType, (char *) kSigString, 5) ;

	/* Define the open param block. */
	memset (open, 0, sizeof (PPCOpenPBRec)) ;
	open->ioCompletion = NULL ;
	open->serviceType = ppcServiceRealTime ;
	open->locationName = &_LocationRecord ;
	open->networkVisible = true ;

	/* Fill in and register port info for uptime command. */
	/* Unions can't be filled statically. */
#ifdef _AIX
	/* The Universal Headers guys changed the struct field names
	 * between versions!!!!
	 */
	_UptimeRecord.u.port.creator = kSignature ;
	_UptimeRecord.u.port.type = kUptimeType ;
#else
	_UptimeRecord.u.port.portCreator = kSignature ;
	_UptimeRecord.u.port.portType = kUptimeType ;
#endif	/* _AIX */
	open->portName = &_UptimeRecord ;
	if (err = PPCOpenSync (open))
		return err ;
	_UptimePort = open->portRefNum ;

	/* Fill in and register port info for who command. */
#ifdef _AIX
	_WhoRecord.u.port.creator = kSignature ;
	_WhoRecord.u.port.type = kWhoType ;
#else
	_WhoRecord.u.port.portCreator = kSignature ;
	_WhoRecord.u.port.portType = kWhoType ;
#endif	/* _AIX */
	open->locationName = NULL ;
	open->portName = &_WhoRecord ;
	if (err = PPCOpenSync (open))
		{
		/* To avoid leaving a defined port lying around, close the uptime
		 * port if an error was encountered.
		 * This is not an issue on AIX because the OS closes the ports
		 * automatically in the case of a crash.
		 */
		memset (&param, 0, sizeof (PPCParamBlockRec)) ;
		close->ioCompletion = NULL ;
		close->portRefNum = _UptimePort ;
		PPCCloseSync (close) ;
		return err ;
		}
	_WhoPort = open->portRefNum ;

	/* Fill in the global structures this one time and never again! */
	_UptimePB.ioCompletion = NewPPCCompProc (_InformCallback) ;
	_UptimePB.portRefNum = _UptimePort ;
	_UptimePB.autoAccept = false ;
	_UptimePB.portName = &_UptimeRecord ;
	_UptimePB.locationName = &_LocationRecord ;
	_UptimePB.userName = _NewUser ;

	_WhoPB.ioCompletion = NewPPCCompProc (_InformCallback) ;
	_WhoPB.portRefNum = _WhoPort ;
	_WhoPB.autoAccept = false ;
	_WhoPB.portName = &_WhoRecord ;
	_WhoPB.locationName = &_LocationRecord ;
	_WhoPB.userName = _NewUser ;

	/* The inform calls will be made by PPCTimeSlice when the daemon enters
	 * its main loop. It does not make sense to tell ppcd now that it is
	 * ready to accept connections if there is more startup overhead.
	 */
	return err ;
	}


OSErr CloseClient (
	ClientPtr	conn)
	{
	PPCEndPBRec	end ;

	memset (&end, 0, sizeof (PPCEndPBRec));
	end.sessRefNum = conn->session ;
	DeleteClient (conn) ;
	return PPCEndSync (&end) ;
	}


/*-----------------------------------------------------------------------------
	PPCShutDown closes (unregisters) the two listening ports. The Toolbox
	could close all outstanding connections, but this functions closes
	them before to free memory and be a nice client.
-----------------------------------------------------------------------------*/
OSErr PPCShutDown (void)
	{
	PPCParamBlockRec	param ;
	PPCClosePBPtr		close = &param.closeParam ;
	PPCEndPBPtr			end = &param.endParam ;
	ClientPtr			last ;
	OSErr				err, err2 ;

	memset (&param, 0, sizeof (PPCParamBlockRec));
	while (UptimeClients)
		{
		end->sessRefNum = UptimeClients->session ;
		err = PPCEndSync (end) ;
		last = UptimeClients ;
		UptimeClients = UptimeClients->qLink ;
		free (last) ;
		}
	while (WhoClients)
		{
		end->sessRefNum = WhoClients->session ;
		err = PPCEndSync (end) ;
		last = WhoClients ;
		WhoClients = WhoClients->qLink ;
		free (last) ;
		}

	/* Define the close param block. */
	memset (&param, 0, sizeof (PPCParamBlockRec)) ;
	close->ioCompletion = NULL ;
	close->portRefNum = _UptimePort ;
	err = PPCCloseSync (close) ;
	close->portRefNum = _WhoPort ;
	err2 = PPCCloseSync (close) ;
	return (err ? err : err2) ;
	}
