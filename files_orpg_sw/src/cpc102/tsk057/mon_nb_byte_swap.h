
/***********************************************************************

    Description: header file defining data structures for product user
		messages. Refer to RPG/APUP ICD.

***********************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2004/02/04 14:10:28 $
 * $Id: mon_nb_byte_swap.h,v 1.1 2004/02/04 14:10:28 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#ifndef MON_NB_BYTE_SWAP_H

#define MON_NB_BYTE_SWAP_H

/* for c++ compatability */
#ifdef __cplusplus
extern "C"
{
#endif

   
/* Function Prototypes. */
int From_ICD( short *msg_data, int size );

#ifdef __cplusplus
}
#endif
 
#endif		/* #ifndef MON_NB_BYTE_SWAP_H */


