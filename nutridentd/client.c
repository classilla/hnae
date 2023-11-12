/* client.c -- routines for managing client connections.
 *
 * Authors: Chris Jalbert
 * Copyright 1996-1997 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * THIS IS PUBLISHED SAMPLE SOURCE CODE OF APPLE COMPUTER, INC.
 *
 * History:
 * 8/19/96 Chris Jalbert
 *	Initial check in of new sample.
 *	Many ideas and calls taken from original javelind sample.
 */

#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* MacOS based header files. */
#include <Types.h>
#include <AppleEvents.h>

#include "tridentd.h"
#include "AIXAESuite.h"


extern Boolean	TimeToQuit ;


/* timeout values for client HEARTBEAT know-when-to-die scheme
 *
 * we check to see how long it has been since we heard from the client
 * every "HEARTBEAT_CHECK" number of seconds.  This should correspond
 * to how often the we expect the client to send us HEARTBEAT events.
 *
 * We send a HEARTBEAT event to the client after HEARTBEAT_SEND seconds
 * have elapsed since an AppleEvent has successfully been exchanged.
 *
 * If we "miss" MAX_MISSED_HEARTBEATS in a row...
 * we will assume that the client has gone away
 */
#define HEARTBEAT_SEND			120
#define HEARTBEAT_CHECK			 30
#define MAX_MISSED_HEARTBEATS	  3

#define _QItemPlusPlus(pb)	(pb) = (pb)->qLink

#define NOCLIENTID			-1
#define DEFAULT_INTERVAL	30
#define WHO_INTERVAL		10


/******************************************************************************
	==>  Globals  <==
******************************************************************************/
ClientPtr		UptimeClients = NULL ;
ClientPtr		WhoClients = NULL ;


/******************************************************************************
	==>  Private Globals  <==
******************************************************************************/

/******************************************************************************
	==>  Static Functions  <==
******************************************************************************/

/*-----------------------------------------------------------------------------
	_SendMessage() is a wrapper function for simple Apple Events. If the
	message string is not NULL, then the code and string will be added to
	the event as parameters, otherwise it will be left empty.

	NOTE: Most useful routines will have more complicated Apple Event
	structures and would not be able to use a wrapper such as this.
-----------------------------------------------------------------------------*/
static OSErr _SendMessage (
	ClientPtr		conn,
	AEEventID		eventID,
	int				nCode,
	char			*szpString)
	{
	AppleEvent		event, reply ;
	AEDescList		connList ;
	OSErr			result ;

	if (result = AECreateList (NULL, 0, false, &connList))
		return result ;

	/* Note that the target (AEAddressDesc) is cached in the client struct. */
	AECreateAppleEvent (kAEAUXSuite, eventID, &(conn->target),
						kAutoGenerateReturnID, kAnyTransactionID, &event) ;

	/* Parrot back the client's session ID (taken from first NULL msg). */
	AEPutParamPtr (&event, keySessionID, typeInteger,
					&(conn->clientID), sizeof (conn->clientID)) ;

	/* Add the parameters if there are some. */
	if (szpString)
		{
		AEPutParamPtr (&event, keyMessageCode, typeInteger,
						&nCode, sizeof (int)) ;

		/* If this is a WHO connection, break the long string into a list
		 * of strings to avoid nasty crashes.
		 */
		if (conn->serviceType == kWhoType)
			{
			register char	*cpLine ;
			cpLine = strtok (szpString, "\n") ;
			for ( ; cpLine ; cpLine = strtok (NULL, "\n"))
				AEPutPtr (&connList, 0, typeChar, cpLine, strlen (cpLine)) ;
			AEPutParamDesc (&event, keyMsgString, &connList) ;
			}
		else
			{
			AEPutParamPtr (&event, keyMsgString, typeChar,
							szpString, strlen (szpString)) ;
			}
		}

	result = AESend (&event, &reply, kAENoReply, kAENormalPriority,
			kAEDefaultTimeout, NULL, NULL) ;

	AEDisposeDesc (&connList) ;
	AEDisposeDesc (&event) ;

	if (!result && szpString)
		DBGM (tprintf ("code = %d, len = %d, string =%s",
						nCode, strlen (szpString), szpString)) ;
	DBG ("AESend returned %d.\n", result) ;
	return result ;
	}


/*-----------------------------------------------------------------------------
	_UpdateClient() checks each connection, sending Apple Events and
	updating parameters as necessary.
-----------------------------------------------------------------------------*/
static void _UpdateClient (
	ClientPtr			conn,
	time_t				tCurr)
	{
	register time_t		tCurrent = tCurr ;
	register time_t		tUpdate = conn->lastUpdate + conn->interval ;
	register time_t		tHeartbeat = conn->lastExchange + HEARTBEAT_SEND ;
	register Boolean	bWho = (conn->serviceType == kWhoType) ;

	tHeartbeat += HEARTBEAT_CHECK * (1 + conn->missedHeartbeats) ;

	/* The who block only CHECKS for an update every interval secs. */
	if (bWho)
		tUpdate = tCurrent + (WhoUpdated (conn->lastUpdate) ? 0 : conn->interval) ;

	/* Send status event if necessary. */
	if (tCurrent >= tUpdate)
		{
		char	szBuffer [BUFSIZ] ;
		int		result ;
		int		(*fnpStatus) (char *, long) ;

		fnpStatus = bWho ? GetWhoString : GetUptimeString ;
		result = (*fnpStatus) (szBuffer, sizeof (szBuffer)) ;

		DBGM (tprintf ("\n>>> Sending status event (conn #%ld, client #%d)\n",
						conn->session, conn->clientID)) ;

		if (!_SendMessage (conn, kAEJavMsg, result, szBuffer))
			conn->lastExchange = tCurrent ;
		conn->lastUpdate = tCurrent ;
		tUpdate = tCurrent + conn->interval ;
		}
	/* Send heartbeat event if necessary.
	 * if a heartbeat and a message are due at the same time, the HB
	 * will be skipped. If the immediately previous send failed, it
	 * is highly likely that the HB would anyway, so what's the point?
	 * Besides, it saves updating the registers.
	 */
	else if (tCurrent >= tHeartbeat)
		{
		DBGM (tprintf ("\n>>> Sending heartbeat (conn #%ld, client #%d)\n",
						conn->session, conn->clientID)) ;
		if (!_SendMessage (conn, kAEHeartBeat, 0, NULL))
			conn->lastExchange = tCurrent ;
		}

	/* Check for expired heartbeat receipts. */
	if (tCurrent > (conn->lastExchange + HEARTBEAT_SEND))
		{
		tHeartbeat = conn->lastExchange + HEARTBEAT_SEND ;
		conn->missedHeartbeats = (tCurrent - tHeartbeat) / HEARTBEAT_CHECK ;
		tHeartbeat += HEARTBEAT_CHECK * (1 + conn->missedHeartbeats) ;
		}

	/* Update the timeout info. */
	conn->nextUpdate = (tUpdate < tHeartbeat) ? tUpdate : tHeartbeat ;
	}


/******************************************************************************
	==>  Global (Exported) Functions  <==
******************************************************************************/

/*-----------------------------------------------------------------------------
	NewClient() allocates space for a new client.
-----------------------------------------------------------------------------*/
ClientPtr NewClient (
	long		lType)
	{
	ClientPtr	pNew ;

	/* Allocate a new session object. */
	if (NULL != (pNew = (ClientPtr) calloc (1, sizeof (ClientStruct))))
		{
//		pNew->parent = (lType == kWhoType) ? &WhoClients : &UptimeClients ;
		pNew->parent = &UptimeClients ;
		pNew->state = ClientInvalid ;
		pNew->clientID = NOCLIENTID ;
		pNew->serviceType = lType ;
		}
	else
		DBGM ("Yikes! Can't allocate memory for new client!\n") ;
	return pNew ;
	}

/*-----------------------------------------------------------------------------
	ClientSetSession and ClientSetInterval are just accessor functions.
-----------------------------------------------------------------------------*/
void ClientSetSession (
	ClientPtr		conn,
	PPCSessRefNum	session)
	{
	if (conn == NULL)
		return ;

	/* Fill in some of the structure fields. */
	conn->session = session ;
	conn->state = ClientConnected ;
	time (&conn->lastExchange) ;

	/* Put the new item at the head of it's parent's list. */
	conn->qLink = *(conn->parent) ;
	*(conn->parent) = conn ;

	/* To save the overhead of malloc/free for each AESend call, the target
	 * descriptor is cached in the client struct.
	 * IMPORTANT:
	 * All apps should include one of the following calls to define the
	 * event's target (Mac client app).
	 * Using typeSessionID is more reliable for two reasons:
	 *	o The AIX AE library supports it. (Big reason!)
	 *	o Session IDs are unique; target IDs may not be.
	 */
	AECreateDesc (typeSessionID, &session, sizeof (PPCSessRefNum), &(conn->target)) ;
//	AECreateDesc (typeTargetID, &target, sizeof (TargetID), &(conn->target)) ;
	}

void ClientSetInterval (
	ClientPtr		conn,
	int				nInterval)
	{
	if (conn == NULL)
		return ;
	if (conn->serviceType == kWhoType)
		conn->interval = nInterval ;
	time (&conn->lastExchange) ;
	conn->interval = nInterval ;
	if (conn->nextUpdate)
		conn->nextUpdate = conn->lastUpdate + (nInterval <= HEARTBEAT_SEND)
							? nInterval : HEARTBEAT_SEND ;
	}

/*-----------------------------------------------------------------------------
	FindClient() walks thru the list of connections looking for a match
	to the given Apple Event.
-----------------------------------------------------------------------------*/
ClientPtr FindClient (
	const AppleEvent	*aeMsg)
	{
	register ClientPtr	pTemp ;
	ClientPtr			pConnList, pMultiplex = NULL ;
	OSErr				result ;
	DescType			returnedType ;
	Size				returnedSize ;
	TargetID			target ;
	int					i, clientID ;

	if (result = AEGetAttributePtr (aeMsg, keyAddressAttr,
									typeTargetID, &returnedType,
									(Ptr) &target,
									sizeof (TargetID), &returnedSize))
		return NULL ;
	if (result = AEGetParamPtr (aeMsg, keySessionID, typeInteger, &returnedType,
						(Ptr) &clientID, sizeof (int), &returnedSize))
		return NULL ;

	for (pTemp = UptimeClients, i = 2 ; i-- && !pMultiplex ; pTemp = WhoClients)
		for ( ; pTemp ; _QItemPlusPlus (pTemp))
			/* Look for matching session ID's. */
			if (target.sessionID == pTemp->session)
				{
				/* If the client ID has not been assigned (this is the first
				 * connection), update the struct and return it.
				 */
				if (pTemp->clientID == NOCLIENTID)
					{
					DBG ("client's session ID = %d\n", clientID) ;
					pTemp->clientID = clientID ;
					pTemp->state = ClientReady ;
					ClientSetInterval (pTemp, DEFAULT_INTERVAL) ;
					return pTemp ;
					}
				/* If the ID's match, return it. */
				if (clientID == pTemp->clientID)
					return pTemp ;
				/* A client might be trying to multiplex a new connection. */
				pMultiplex = pTemp ;
				}

	/* If the client is trying to multiplex, do the right thing. */
	if (!pMultiplex)
		return pTemp ;
	/* Who can not be multiplexed. */
	if (pMultiplex->serviceType == kWhoType)
		return pTemp ;
	/* Multiplex an uptime client. */
	if (pTemp = NewClient (pMultiplex->serviceType))
		{
		DBG ("New multiplex client's session ID = %d\n", clientID) ;
		ClientSetSession (pTemp, pMultiplex->session) ;
		pTemp->clientID = clientID ;
		pTemp->state = ClientReady ;
		ClientSetInterval (pTemp, DEFAULT_INTERVAL) ;
		}
	return pTemp ;
	}

/*-----------------------------------------------------------------------------
	UpdateClients() actually does the bulk of the work. It services all
	connections whose heartbeat or info timeouts have expired and reorders
	the list so the first item is always the next to be serviced.
	The returned value is the time remaining until servicing is required.
	If there are currently no connections, a timeout of -1 is returned.
-----------------------------------------------------------------------------*/
long UpdateClients (void)
	{
	ClientPtr			*ppConnList = &UptimeClients ;
	register ClientPtr	pPrev, pNext, pTemp ;
	int					i ;
	time_t				tCurrent ;

	if (UptimeClients == NULL)
		{
#if SIMPLIFIED
		TimeToQuit = true ;
		return 0 ;
#else	/* SIMPLIFIED */
		return -1 ;
#endif	/* SIMPLIFIED */
		}

	time (&tCurrent) ;

	/* Check and process all expired timeouts.
	 * Processing stops when all items have been looped over or when an
	 * item is reached that is not yet due for processing.
	 */
//	for (ppConnList = &UptimeClients, i = 2 ; i-- ; ppConnList = &WhoClients)
		for (pTemp = *ppConnList ; (pTemp && (tCurrent >= pTemp->nextUpdate)) ;
				pTemp = pNext)
			{
			pNext = pTemp->qLink ;

			/* Until stuff has been defined, skip this client. */
			switch (pTemp->state)
				{
				case ClientInvalid:
					DBGM ("Yikes! Invalid client in the list!\n") ;
					continue ;
				case ClientConnected:
					/* If the client hasn't been validated in a resonable
					 * amount of time, punt it!
					 */
					if ((tCurrent - pTemp->lastExchange) > HEARTBEAT_CHECK)
						CloseClient (pTemp) ;
					continue ;
				case ClientClosed:
					CloseClient (pTemp) ;
					continue ;
				}

			/* Maintain the head of the list correctly. */
			if (pTemp == *ppConnList)
				*ppConnList = pNext ;

			/* Separate the item from the list. */
			pTemp->qLink = NULL ;

			_UpdateClient (pTemp, tCurrent) ;

			/* If it's time to dispose of a dead client, then do so. */
			if (pTemp->missedHeartbeats >= MAX_MISSED_HEARTBEATS)
				{
				CloseClient (pTemp) ;
				continue ;
				}

			/* Place the item into the list ordered correctly. */
			pPrev = (ClientPtr) ppConnList ;
			for ( ; pPrev->qLink ; _QItemPlusPlus (pPrev))
				if (pPrev->qLink->nextUpdate > pTemp->nextUpdate)
					break ;
			pTemp->qLink = pPrev->qLink ;
			pPrev->qLink = pTemp ;
			}

	/* Find a valid item in the list. */
	for (pTemp = UptimeClients ; pTemp ; _QItemPlusPlus (pTemp))
		if (pTemp->nextUpdate)
			break ;

	/* Now determine the timeout. */
	time (&tCurrent) ;
#if SIMPLIFIED
	if (UptimeClients)
		return (long) (pTemp ? (pTemp->nextUpdate - tCurrent) : -1) ;
	TimeToQuit = true ;
	return 0 ;
#else	/* SIMPLIFIED */
	return (long) (pTemp ? (pTemp->nextUpdate - tCurrent) : -1) ;
#endif	/* SIMPLIFIED */
	}

/*-----------------------------------------------------------------------------
	InvalidateClient() prepares the client for close and deletion.
-----------------------------------------------------------------------------*/
void InvalidateClient (
	ClientPtr			pClient)
	{
	pClient->state = ClientClosed ;
	pClient->nextUpdate = 0 ;
	}

/*-----------------------------------------------------------------------------
	DeleteClient() removes the client from its parent list.
-----------------------------------------------------------------------------*/
void DeleteClient (
	ClientPtr			pClient)
	{
	register ClientPtr	pPrev, pTemp = pClient ;

	DBG ("\n>>> Deleting conn #%ld.\n", pClient->session) ;

	pPrev = (ClientPtr) pClient->parent ;
	for (pTemp = pPrev->qLink ; pTemp ; _QItemPlusPlus (pTemp))
		{
		if (pTemp == pClient)
			{
			pPrev->qLink = pTemp->qLink ;
			pTemp->state = ClientInvalid ;
			free (pTemp) ;
			return ;
			}
		pPrev = pTemp ;
		}
	}
