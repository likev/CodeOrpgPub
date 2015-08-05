/* 
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:11:13 $
 * $Id: rda_rpg_console_message.h,v 1.5 2002/12/11 22:11:13 nolitam Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  
/************************************************************************

	Header file defining the data structures and constants for
	RDA and RPG Console Message messages (message types 4 and 10)
	as defined in the Interface Control Document (ICD) for the
	RDA/RPG (rev: March 1, 1996).

************************************************************************/


#include <rda_rpg_message_header.h>

#ifndef	RDA_RPG_CONSOLE_MESSAGE_H
#define	RDA_RPG_CONSOLE_MESSAGE_H

#define	MAX_CONSOLE_MESSAGE_LENGTH	404

typedef struct {

    RDA_RPG_message_header_t  msg_hdr; 	/*  Message Header. */

    short                  size;	/*  Console Message Size.  The number 
					    of bytes in the message.  (Range 
           				    2 to 404)
					*/
    char                    message [MAX_CONSOLE_MESSAGE_LENGTH];
    					/*  Message.  2 characters/halfword
					    including non-printing characters.
					*/
} RDA_RPG_console_message_t;

#endif
