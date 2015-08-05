/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/02/07 22:46:35 $
 * $Id: rda_rpg_message_header.h,v 1.9 2007/02/07 22:46:35 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  
/************************************************************************

	Header file defining the message header data for RDA/RPG
	message types as defined in Table II of the Interface Control
	Document (ICD) for the RDA/RPG (1 March 1996).

************************************************************************/


#ifndef	RDA_RPG_MESSAGE_HEADER_H
#define	RDA_RPG_MESSAGE_HEADER_H

#define	RDA_RPG_MSG_HDR_SIZE		1
#define RDA_RPG_MSG_HDR_CHANNEL		2
#define RDA_RPG_MSG_HDR_TYPE		3
#define RDA_RPG_MSG_HDR_SEQ_NUM		4
#define RDA_RPG_MSG_HDR_DATE		5
#define RDA_RPG_MSG_HDR_TIME		6
#define RDA_RPG_MSG_HDR_NUM_SEGS	7
#define RDA_RPG_MSG_HDR_SEG_NUM		8

/* Typedefs for rda_channel */
#define RDA_RPG_MSG_HDR_LEGACY_CFG      0
#define RDA_RPG_MSG_HDR_ORDA_CFG        8

/* Typedefs for type (see rda_rpg_messages.h) */

typedef	struct {

    unsigned short	size;		/*  Message size in halfwords for
					    this message segment, not for
				   	    the total of all segments in
					    the message.
					*/

    unsigned char       rda_channel;    /*  RDA Redundant channel number for
					    redundant channel configuration.
					    0 = Legacy non-redundant config
					    1 = Legacy RDA1
					    2 = Legacy RDA2
					    8 = ORDA non-redundant config
					    9 = Open RDA1
					    10 = Open RDA2
					*/

    unsigned char	type;		/*  Message type, where:
					     1 = digital radar data
					     2 = rda status data
					     3 = Performance/Maintenance data
					     4 = console message - rda to rpg
					     5 = RDA to RPG VCP message
					     6 = rda control commands
					     7 = RPG to RDA VCP message
					     8 = clutter censor zones
					     9 = request for data
					    10 = console message - rpg to rda
					    11 = loopback test - rda to rpg
					    12 = loopback test - rpg to rda
					    13 = clutter filter bypass map - rda to rpg
					    14 = edited clutter filter bypass map
					    15 = clutter filter notch width map
					    18 = adaptation data
					    31 = generic digital radar data
				 	*/

    unsigned short	sequence_num;	/*  Message Sequence Number from 0 - 65535
					    then to 0
					*/

    unsigned short	julian_date;	/*  Julian date from January 1, 1970 at
					    0000 UT (starting with 1)
					*/

    unsigned int	milliseconds; 	/*  Number of milliseconds past midnight (UT)
					*/ 

    unsigned short	num_segs;	/*  Number of message segments.  Messages
					    larger than 1208 halfwords are segmented
					    and transmitted separately except for Message
                                            31 that has a segment size of 65535 halfwords.
					*/

    unsigned short	seg_num;	/*  Segment number of this message
					*/
} RDA_RPG_message_header_t;

#endif
