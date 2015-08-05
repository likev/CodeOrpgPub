/* 
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:11:14 $
 * $Id: rda_rpg_loop_back.h,v 1.3 2002/12/11 22:11:14 nolitam Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */  
/*********************************************************************

	Header defining the data structures and constants used for
	RDA and RPG Loop Back Test messages (message types 11 and 12)
	as defined in the Interface Control Document (ICD) for the
	RDA/RPG (rev: March 1, 1996).

*********************************************************************/


#ifndef	RDA_RPG_LOOP_BACK_MESSAGE_H
#define	RDA_RPG_LOOP_BACK_MESSAGE_H

#define RDA_RPG_LOOP_BACK_DATA_SIZE		1199

typedef	struct {

    short	size;		/*  Loop Back Message Size.  Number of
				    halfwords in the message (does not
				    include message header).
				    (Range 2 to 1200).
				*/
    short	pattern [RDA_RPG_LOOP_BACK_DATA_SIZE];
    				/*  Bit pattern of 0's and 1's used to
				    test the interface.
				*/

} RDA_RPG_loop_back_message_t;

#endif
