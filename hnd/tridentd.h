/* trident.h - structures and #defines for tridentd project.
 *
 * Original Author: Chris Jalbert
 * Copyright 1997 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * THIS IS PUBLISHED SAMPLE SOURCE CODE OF APPLE COMPUTER, INC.
 *
 * History:
 * 1/22/97	Chris Jalbert
 *	Initial check-in for world distribution! Muahaha!
 */


#ifndef _TRIDENT_H_
#define _TRIDENT_H_

#define HEARTBEAT_CHECK 30


/*** Necessary #includes ***/
#include <time.h>
#include <AppleEvents.h>
#include <EPPC.h>

#include <PPCToolBox.h>

#ifdef SIMPLIFIED
# undef SIMPLIFIED
# ifdef _AIX
#  define SIMPLIFIED 1
# else
#  define SIMPLIFIED 0
# endif	/* _AIX */
#else
# define SIMPLIFIED 0
#endif	/* SIMPLIFIED */


/*** Typedefs and Structures ***/
typedef enum
	{
	ClientInvalid = -1,		/* Set before connection is made & after closed. */
	ClientConnected,		/* Set in PPCInform callback before 1st AE. */
	ClientReady,			/* Set after version AE validates protocol. */
	ClientClosed			/* Set when client should be closed. */
	} ClientState ;

typedef struct tagClientStruct *ClientPtr ;
typedef struct tagClientStruct
	{
	ClientPtr		qLink ;				/* Should be unused in multi-threaded version. */
	ClientPtr		*parent ;			/* Modifiable address to head of list. */
	PPCSessRefNum	session ;			/* AppleTalk socket to client. */
	AEAddressDesc	target ;			/* Cached AEDesc target to client. */
	long			serviceType ;		/* Differentiates service type. */
	ClientState		state ;				/* Used between connection and validation AE's. */
	int				clientID ;			/* Client's session ID. (Useless to daemon.) */
	int				interval ;			/* Time between updates. */
	int				missedHeartbeats ;	/* # HBs missed so far. */
	time_t			lastExchange ;		/* Time of last comm to or from client. */
	time_t			lastUpdate ;		/* Time of last update AE to client. */
	time_t			nextUpdate ;		/* Time of next update to client. */
	} ClientStruct ;


/*** Globals ***/
extern ClientPtr	UptimeClients, WhoClients ;


/*** Constants ***/
#define kSignature		'JVLN'	/* Application signature */
#define kUptimeType		'updt'	/* PPC type for update */
#define kWhoType		'who '	/* PPC type for who list */


/*** Macros ***/
#define PStringStr(pstr)	((char *) &(pstr)[1])
#define PStringLen(pstr)	(pstr)[0]


/*** Function prototypes ***/

/* Functions in handlers.c. */
extern OSErr InitAEStuff (long) ;

/* Functions in client.c. */
extern ClientPtr NewClient (long) ;
extern void ClientSetSession (ClientPtr, PPCSessRefNum) ;
extern void ClientSetInterval (ClientPtr, int) ;
extern long UpdateClients (void) ;
extern ClientPtr FindClient (const AppleEvent *) ;
extern void InvalidateClient (ClientPtr) ;
extern void DeleteClient (ClientPtr) ;

/* Functions in status.c. */
extern void InitStatus (void) ;

extern int GetString (char *, long) ;
extern Boolean StringUpdated (time_t) ;

#endif	/* _TRIDENT_H_ */
