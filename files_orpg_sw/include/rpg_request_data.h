/* 
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2004/05/27 20:23:13 $
 * $Id: rpg_request_data.h,v 1.6 2004/05/27 20:23:13 ryans Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */  

/**************************************************************************

	Header defining the data structures and constants used for
	RPG Request for Data messages.
	
        6/13/2003 - Comments updated to reflect latest ORDA changes.
        5/27/2004 - Comments updated to reflect latest ORDA changes.
**************************************************************************/


#ifndef RPG_REQUEST_DATA_MESSAGE_H
#define RPG_REQUEST_DATA_MESSAGE_H

typedef	unsigned short RPG_request_data_t;

				/*  bit 0 & 7 set - Request Summary RDA Status
				        1 & 7     - Request RDA Perfomance/
						    Maintenance Data 
				        2 & 7     - Request Clutter Filter
				        	    Bypass Map
				        3 & 7     - Request Clutter Filter Map
				        4 & 7     - Request RDA Adaptation 
						    Data (ORDA only)
				        5 & 7     - Request VCP (ORDA only)
				        	    
				    Note: LSB = 0
				*/


typedef struct {

   unsigned short RPG_request_data;     /* 
                                           This data item is identical to
                                           the data item defined above. This 
                                           structure was provided to
                                           facilitate applications that 
                                           work with structures.
                                           R. Solomon, RSIS, 5/21/03.
                                        */

} RPG_request_data_struct_t;


#endif
