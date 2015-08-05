
/***********************************************************************

    Description: header file defining data structures for product user
		messages. Refer to RPG/APUP ICD.

***********************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/09/05 19:38:00 $
 * $Id: mon_nb.h,v 1.5 2014/09/05 19:38:00 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  

#ifndef MON_NB_H

#define MON_NB_H

/* for c++ compatability */
#ifdef __cplusplus
extern "C"
{
#endif

#define BLACK		0
#define YELLOW		1
#define ORANGE		2
#define RED		3
#define GREEN		4
#define BLUE		5
#define GRAY		6

#define REDTEXT		"\x1B[1m\x1B[38;5;160m"
#define YLWTEXT		"\x1B[1m\x1B[38;5;11m"
#define ORGTEXT		"\x1B[1m\x1B[38;5;202m"
#define GRNTEXT		"\x1B[1m\x1B[38;5;28m"
#define GRYTEXT		"\x1B[1m\x1B[38;5;240m"
#define BLKTEXT		"\x1B[1m\x1B[38;5;16m"
#define BLUTEXT		"\x1B[1m\x1B[38;5;12m"
#define BBLTEXT		"\x1B[1m\x1B[38;5;14m"
#define BCKGRND		"\x1B[1m\x1B[48;5;180m"
#define RESETTC         "\x1B[39m"
#define RESET           "\033[0m"


/* Defines how chatty you want the output to be. */
int Terse_mode;
   
/* Function Prototypes. */
char* Get_date_time_str( int date, int time );
char* Get_product_mnemonic_str( int prod_code );

void Process_incoming_ICD_message( short *msg_data, int size );
void Process_outgoing_ICD_message( CM_req_struct *req, short *msg_data, int size );

void Print_msg( int len, int color );

#ifdef __cplusplus
}
#endif
 
#endif		/* #ifndef MON_NB_H */


