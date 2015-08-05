/*   @(#) uclid.h 99/12/23 Version 1.3   */
/*<-------------------------------------------------------------------------
| 
|                           Copyright (c) 1991 by
|
|              +++    +++                           +++     +++
|              +++    +++                           +++     +++
|              +++    +++                            +++   +++ 
|              +++    +++   +++++     + +    +++   +++++   +++ 
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
|              +++    +++ ++++++++ ++++ ++++ ++++  ++++++ +++  
|              +++    +++++++   ++ ++++ ++++ ++++  +++ ++++++  
|              +++    ++++++      +++++ ++++++++++ +++ +++++   
|              +++    ++++++      +++++ ++++++++++ +++  +++    
|              +++    ++++++      ++++   +++++++++ +++  +++    
|              +++    ++++++                             +     
|              +++    ++++++      ++++   +++++++ +++++  +++    
|              +++    ++++++      +++++ ++++++++ +++++  +++    
|              +++    ++++++      +++++ ++++++++ +++++ +++++   
|              +++    +++++++   ++ ++++ ++++ +++  ++++ +++++   
|              +++    +++ ++++++++ ++++ ++++ +++  +++++++++++  
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
|               +++  +++    +++++     + +    +++   ++++++  +++ 
|               ++++++++                             +++    +++
|                ++++++         Corporation         ++++    ++++
|                 ++++   All the right connections  +++      +++
|
|
|      This software is furnished  under  a  license and may be used and
|      copied only  in  accordance  with  the  terms of such license and
|      with the inclusion of the above copyright notice.   This software
|      or any other copies thereof may not be provided or otherwise made
|      available to any other person.   No title to and ownership of the
|      program is hereby transferred.
|
|      The information  in  this  software  is subject to change without
|      notice  and  should  not be considered as a commitment by UconX
|      Corporation.
|  
|      UconX Corporation
|      San Diego, California
|                                               
v                                                
-------------------------------------------------------------------------->*/


/*
** Uclid protocol structures and constants
**
** UconX CLient Interface Definition specifies the protcol between the
** host and the server.
*/


#ifndef _uclid_h
#define _uclid_h


/* xSTRa InterFace Header */
typedef struct ucshdr 
{		
   unsigned short mtype;			/* Message type	          */
   unsigned short flags;			/* Flags                  */
   unsigned short csize;			/* Ctrl data size (bytes) */
   unsigned short dsize;			/* Data size (bytes)	  */
} UCSHdr;

#define UHDRLEN	sizeof(UCSHdr)

#define	CLIENT_MAX_DATA	4096		/* Maximum size of data field */

/* 
** Client and Server Message Types
*/

/* Client Message Types */
#define CMTYPE	('c' << 8)

#define COPEN		(CMTYPE|0x01)			/* Open Request    */
#define CDATA		(CMTYPE|0x02)			/* MDATA message   */
#define CPROTO		(CMTYPE|0x03)			/* MPROTO message  */
#define CIOCTL		(CMTYPE|0x04)			/* MIOCTL message  */
#define CCLOSE		(CMTYPE|0x06)			/* Close Request   */
#define CPCPROTO        (CMTYPE|0x07)                   /* PCPROTO message */

/* Server Message Types */
#define SMTYPE		('s' << 8)

#define	SOPENACK	(SMTYPE|0x81) 			/* OPEN Ack         */
#define	SDATA		(SMTYPE|0x82)			/* DATA Message	    */
#define	SPROTO		(SMTYPE|0x83) 			/* PROTO Message    */
#define	SIOCACK		(SMTYPE|0x84) 			/* IOCTL Ack        */
#define SIOCNAK		(SMTYPE|0x85)			/* IOCTL NAK 	    */
#define SOPENNAK        (SMTYPE|0x86)                   /* OPEN NAK         */
#define SPCPROTO        (SMTYPE|0x87)                   /* PCPROTO Message  */
#define	SERROR		(SMTYPE|0x8a)	 		/* Error indication */
#define SPCSIG          (SMTYPE|0x8b)                   /* M_PCSIG msg      */
#define SHANGUP         (SMTYPE|0x8c)                   /* M_HANGUP msg     */

/** The flags field use is dependent on the ucs_mtype field's value: **/

/* mtype: CPROTO | SPROTO */
/*	0				no flags indicated	*/
/*	RS_HIPRI	1	hi-priority message (from xstopts.h) */

/* ucs_mtype: CIOCTL */
/* The ucs_flags field contains one of the valid I_xxx ioctl command types
 * from the stream.h include file; if not recognized as one of the above, the
 * ucs_flags field indicates the ioctl sub-type for an I_STR request.
 */


/*
** xSTRa uses the M_ERROR message type to indicate an asynchronous error has
** occured on a Stream.  Stream modules may use the M_ERROR or a user-defined 
** M_PROTO/PCPROTO message to indicate an the occurrence of an error on a
** Stream.
**
** M_ERROR messages are defined by STREAMS to contain the error indication
** in the first byte of the data.  The error indication will be returned
** to the client.
**
** Errors are defined > 99 to differentiate from the system error codes.
*/

#ifndef ERROR
#define ERROR -1
#endif

#define MAXPROTOLEN	11

typedef struct opendata 
{
	int    slotnum;
	char   protocol[MAXPROTOLEN+1];
	int    dev;
	int    flags;
} OpenData;

typedef struct openreq 
{
        UCSHdr       	uhdr;           		/* API Header */
	OpenData	odata;				/* Open Info  */
} OpenReq;

typedef struct sdata
{
   char *p_mesg;
   int len;
} SaveData;

#endif	/* _uclid_h */

