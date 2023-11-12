/******************************************************************************
**
**  Project Name:	AIX AppleEvent Suite
**     File Name:	AIX-AESuite.h
**
**   Description:	AIX AppleEvent Suite Definition
**
**          NOTE:	This copy has been stripped to include only those
**			definitions that are relevent to JAVELIN.  Also, note
**			that these definitions have not been officially blessed
**			by those empowered to do so.
**
*******************************************************************************
**
*******************************************************************************
**
**                     Copyright (c) 1995 Apple Computer, Inc.
**                              ALL RIGHTS RESERVED.
**
**  This program is  confidential  and trade secret of  Apple Computer, Inc.
**  The receipt or possession of this program does not convey any rights to
**  reproduce or disclose  its  contents, or to manufacture,  use,  or sell
**  anything that it may discribe, in whole or in part, without the express
**  written consent of Apple Computer, Inc.
**
*******************************************************************************
**                       A U T H O R   I D E N T I T Y
*******************************************************************************
**
**	Initials	Name
**	--------	-----------------------------------------------
**	JSM		Scott Mulligan
**	CPJ		Chris Jalbert
**
******************************************************************************/

#ifndef __AIXAESuite__
#define __AIXAESuite__

#define kAEAUXSuite 'A/UX'
#define kAEAIXSuite 'AAIX'

/*****************
 *  Event Suite  *
 *****************/

/*-------------------------------------------------------------------------*
 *
 *	AppleEvents for TRIDENT      ***      AppleEvents for TRIDENT
 *
 *-------------------------------------------------------------------------*/

/* * * * * * * * * * * * * *
 * The NULL Event          *
 * * * * * * * * * * * * * */

#define kAENull 'null'			/* Event ID. */
/*
 * Parameters:
 *
 *		None
 *
 * Reply parameters:
 *
 *		keyErrorNumber		// typeChar
 *		keyErrorString  	// typeChar
 */

/* * * * * * * * * * * * * * * *
 * The QUIT Event              *
 * * * * * * * * * * * * * * * */

/* Use kAEQuitApplication from AppleEvents.h!!
#define kAEQuit 'quit' */			/* Event ID. */
/*
 * Parameters:
 *
 *		None
 *
 * Reply parameters:
 *
 *		None.
 */

/* * * * * * * * * * * * * *
 * The Heartbeat Event     *
 * * * * * * * * * * * * * */

#define kAEHeartBeat 'hart'		/* Event ID. */
/*
 * Parameters:
 *
 *		None
 *
 * Reply parameters:
 *
 *		None.
 */

/* * * * * * * * * * * * * *
 * The Version Event       *
 * * * * * * * * * * * * * */

#define kAEMyVersion 'dver'		/* Event ID. */
/*
 * Parameters:
 *
 *		keyAEVersion		// typeChar
 *
 * Reply parameters:
 *
 *		keyErrorNumber		// typeChar
 *		keyErrorString  	// typeChar
 */

/* * * * * * * * * * * * * *
 * The Interval Event       *
 * * * * * * * * * * * * * */

#define kAEInterval 'jint'			/* Event ID. */
/*
 * Parameters:
 *
 *      keyInterval 'tint'			// Integer
 *
 */

/* * * * * * * * * * * * * * *
 * The Trident Message Event *
 * * * * * * * * * * * * * * */

#define kAEJavMsg 'jmsg'			/* Event ID. */
/*
 * Parameters:
 *      keyMessageCode	'msgc'	// typeInteger    : Error Message Number
 *      keyMsgString	'msgs'	// typeChar       : Message String
 *
 *
 */

/* * * * * * * * * * * * * * * * * *
 * Parameter keys used by Trident  *
 * * * * * * * * * * * * * * * * * */

#define keyAEMyVersion	'verx'	/* typeChar       : Software Version         */
#define keySessionID	'ssID'	/* typeInteger    : Session/Document ID      */

#define keyInterval     'tint'  /* typeInteger    : Interval (seconds)       */
#define keyMessageCode	'msgc'	/* typeInteger    : Message Number           */
#define keyMsgString	'msgs'	/* typeChar       : Optional Message String  */

#endif  /* __AIXAESuite__ */
