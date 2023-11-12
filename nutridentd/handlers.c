/* handlers.c -- Apple Event Handlers for Trident UNIX daemon.
 *
 * Original Author: Chris Jalbert
 * Copyright 1993-1997 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * THIS IS PUBLISHED SAMPLE SOURCE CODE OF APPLE COMPUTER, INC.
 *
 * History:
 * 8/19/96	Chris Jalbert
 *	Initial check in of new sample.
 *	Note the similarities between this and the Javelin daemon stuff.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* MacOS Universal headers for compatability. */
#include <Types.h>
#include <AppleEvents.h>
#include <AERegistry.h>

#include "tridentd.h"
#include "AIXAESuite.h" 
#include "debug.h"


extern Boolean TimeToQuit ;


/******************************************************************************
	==>  Private Globals  <==
******************************************************************************/
static char	*_ServerVersion = "2.0" ;
static char *_UnknownClient = "Client shouldn't be connected to this service!?!" ;


/******************************************************************************
	==>  Static Functions  <==
******************************************************************************/
static OSErr _StuffResultInReply (
	AppleEvent	*reply,
	char		cNumber,
	const char	*szpErr)
	{
	OSErr				result ;

	if (cNumber != 'S')
		DBGM (tprintf ("status code = '%c', string = %s",
					cNumber, (szpErr ? szpErr : "Success\n"))) ;

	result = AEPutParamPtr (reply, keyErrorNumber, typeChar,
							&cNumber, sizeof (char)) ;
	if (szpErr != NULL)
		result = AEPutParamPtr (reply, keyErrorString, typeChar,
								szpErr, strlen (szpErr)) ;
	return result ;
	}


/*-----------------------------------------------------------------------------
	_HandleNullEvent() handles the first event from the client, which
	should only be sent once. It define's the client's ID number for
	this connection.
-----------------------------------------------------------------------------*/
static pascal OSErr _HandleNullEvent (
	const AppleEvent	*msg,
	AppleEvent			*reply,
	long				refcon)
	{
//	ClientPtr			conn ;

	DBGM ("\n<<< HandleNullEvent()\n") ;

	/* Find the client this request belongs to. */
//	if (NULL == (conn = FindClient (Clients, msg)))
//		return _StuffResultInReply (reply, 'F', _UnknownClient) ;
	return _StuffResultInReply (reply, 'S', NULL) ;
	}

/*-----------------------------------------------------------------------------
	_HandleVersionEvent() handles the version event from the client, which
	should only be sent once. If the major numbers don't match (and hence
	the messaging protocol doesn't either), failure will be returned and
	the connection dropped.
	The client record should be freed (or marked to be freed).
-----------------------------------------------------------------------------*/
static pascal OSErr _HandleVersionEvent (
	const AppleEvent	*msg,
	AppleEvent			*reply,
	long				refcon)
	{
	OSErr				result ;
	ClientPtr			conn ;
	DescType			returnedType ;
	Size				returnedSize ;
	char				szClientVer [BUFSIZ], szServerVer [16] ;

	DBGM ("\n<<< HandleVersionEvent()\n") ;

	/* Pull out the version parameter. */
	if (result = AEGetParamPtr (msg, keyAEMyVersion, typeChar, &returnedType,
								szClientVer,
								(sizeof (szClientVer) - 1), &returnedSize))
		{
		_StuffResultInReply (reply, 'F',
							"Required parameter (keyAEMyVersion) not found!\n") ;
		return result ;
		}
	szClientVer[returnedSize] = '\0' ;
	DBGM (tprintf ("Server v%s, client v%s\n", _ServerVersion, szClientVer)) ;

	/* The major version number defines the protocol. */
	strcpy (szServerVer, _ServerVersion) ;
	strtok (szServerVer, ".") ;
	strtok (szClientVer, ".") ;

	/* If the major versions don't match, puke and close the client. */
	if (strcmp (szServerVer, szClientVer))
		{
		DBGM ("Protocol versions don't match!\n") ;
		_StuffResultInReply (reply, 'F', "Protocol versions don't match!\n") ;

		/* It is bad news to close a client connection from a handler
		 * because the reply will not be sent back, hanging or
		 * crashing the Mac.
		 * The update code will properly close the client later.
		 */
		if (NULL != (conn = FindClient (msg)))
			conn->state = ClientClosed ;
		return noErr ;
		}

	/* Find the client this request belongs to. */
	if (NULL == (conn = FindClient (msg)))
		return _StuffResultInReply (reply, 'F', _UnknownClient) ;

	/* There might be a reason to send the OS version, but I can't figure why. */
	result = AEPutParamPtr (reply, keyAEMyVersion, typeChar,
							_ServerVersion, strlen (_ServerVersion)) ;
	return _StuffResultInReply (reply, 'S', NULL) ;
	}

/*-----------------------------------------------------------------------------
	_HandleIntervalEvent() handles the interval event from the client, which
	will be sent immediately following the version event to signal that the
	client is ready to accept messages.
	Multiple interval events are allowed to modify the update frequency.
-----------------------------------------------------------------------------*/
static pascal OSErr _HandleIntervalEvent (
	const AppleEvent	*msg,
	AppleEvent			*reply,
	long				refcon)
	{
	OSErr				result ;
	ClientPtr			conn ;
	DescType			returnedType ;
	Size				returnedSize ;
	int					nInterval ;

	DBGM ("\n<<< HandleInterval()\n") ;

	/* Pull out the interval parameter. */
	if (result = AEGetParamPtr (msg, keyInterval, typeInteger, &returnedType,
								&nInterval, sizeof (nInterval), &returnedSize))
		{
		_StuffResultInReply (reply, 'F',
							"Required parameter (keyAEInteger) not found!\n") ;
		return result ;
		}

	/* Find the client this request belongs to. */
	if (NULL == (conn = FindClient (msg)))
		return _StuffResultInReply (reply, 'F', _UnknownClient) ;

	/* Set the stuff. */
	DBGM (tprintf ("New interval for client %ld is %d seconds.\n",
					conn->session, nInterval)) ;
	ClientSetInterval (conn, nInterval) ;

	return _StuffResultInReply (reply, 'S', NULL) ;
	}

/*-----------------------------------------------------------------------------
	_HandleHeartbeatEvent() handles heartbeat events from the client,
	which can be sent multiple times at random intervals. Currently, the
	client only sends heartbeat events after receiving them from the
	daemon.
-----------------------------------------------------------------------------*/
static pascal OSErr _HandleHeartbeatEvent (
	const AppleEvent	*msg,
	AppleEvent			*reply,
	long				refcon)
	{
	ClientPtr			conn ;

	DBGM ("\n<<< HandleHeartBeat ") ;

	if (NULL == (conn = FindClient (msg)))
		return _StuffResultInReply (reply, 'F', _UnknownClient) ;

	time (&conn->lastExchange) ;
	conn->missedHeartbeats = 0 ;
	DBG ("(conn #%ld)\n", conn->session) ;
	return _StuffResultInReply (reply, 'S', NULL) ;
	}

/*-----------------------------------------------------------------------------
	_HandleQuit() handles the quit event from the client, which will be
	sent once as the connection is closed. The client connection is invalid
	after receiving this event.
-----------------------------------------------------------------------------*/
static pascal OSErr _HandleQuit (
	const AppleEvent	*msg,
	AppleEvent			*reply,
	long				refcon)
	{
	ClientPtr			conn ;

	DBGM ("\n<<< HandleQuit()\n") ;

	if (NULL != (conn = FindClient (msg)))
		InvalidateClient (conn) ;

	return noErr ;
	}

/*-----------------------------------------------------------------------------
	The following four functions handle the four required Apple Events.
	They are mostly useless for a background app like this one.
-----------------------------------------------------------------------------*/
static pascal OSErr _HandleOpenApp (
	const AppleEvent	*msg,
	AppleEvent			*reply,
	long				refcon)
	{
	DBGM ("\n<<< HandleOpenApp()\n") ;
	return noErr ;
	}

static pascal OSErr _HandleOpenDocs (
	const AppleEvent	*msg,
	AppleEvent			*reply,
	long				refcon)
	{
	DBGM ("\n<<< HandleOpenDocs()\n") ;
	return errAEEventNotHandled ;
	}

static pascal OSErr _HandlePrintDocs (
	const AppleEvent	*msg,
	AppleEvent			*reply,
	long				refcon)
	{
	DBGM ("\n<<< HandlePrintDocs()\n") ;
	return errAEEventNotHandled ;
	}

static pascal OSErr _HandleQuitApp (
	const AppleEvent	*msg,
	AppleEvent			*reply,
	long				refcon)
	{
	DBGM ("\n<<< HandleQuitApp()\n") ;
	TimeToQuit = true ;
	return noErr ;
	}


/******************************************************************************
	==>  More Private Globals  <==
******************************************************************************/
typedef struct _tagAEHandler
	{
	AEEventID				eventID ;
	AEEventHandlerProcPtr	handler ;
	} _AEHandler ;

static _AEHandler _AIXHandlerTable [] = {
	{ kAENull,				_HandleNullEvent },
	{ kAEMyVersion,			_HandleVersionEvent },
	{ kAEInterval,			_HandleIntervalEvent },
	{ kAEHeartBeat,			_HandleHeartbeatEvent },
	{ kAEQuitApplication,	_HandleQuit }
} ;
static _AEHandler _RequiredHandlerTable [] = {
	{ kAEOpenApplication,	_HandleOpenApp },
	{ kAEOpenDocuments,		_HandleOpenDocs },
	{ kAEPrintDocuments,	_HandlePrintDocs },
	{ kAEQuitApplication,	_HandleQuitApp }
} ;


/******************************************************************************
	==>  Global (Exported) Functions  <==
******************************************************************************/
/*-----------------------------------------------------------------------------
	InitAEStuff() installs all the handlers defined in the handler tables.
-----------------------------------------------------------------------------*/
OSErr InitAEStuff (
	long			refcon)
	{
	register OSErr	result = noErr;
	register int	i ;

#ifdef _AIX
	AEInit () ;
#endif

	/* Install required event handlers. */
	i = sizeof (_RequiredHandlerTable) / sizeof (_AEHandler) ;
	while (!result && i--)
		result = AEInstallEventHandler (kCoreEventClass, 
						_RequiredHandlerTable[i].eventID,
						NewAEEventHandlerProc (_RequiredHandlerTable[i].handler),
						refcon, false) ;

	/* Install AIX (OK, A/UX), app-specific event handlers. */
	i = sizeof (_AIXHandlerTable) / sizeof (_AEHandler) ;
	while (!result && i--)
		result = AEInstallEventHandler (kAEAUXSuite, 
						_AIXHandlerTable[i].eventID,
						NewAEEventHandlerProc (_AIXHandlerTable[i].handler),
						refcon, false) ;
	return result ;
	}
