/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/04/03 16:12:10 $
 * $Id: prcprate_RateAlg_buffctrl.c,v 1.2 2006/04/03 16:12:10 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_RateAlg_buffctrl.c

   Description
   ===========
       This function performs buffer control functions including getting and 
    releasing both input and output buffers as needed.

   Change History
   ==============
   10/01/88      0000      Greg Umstead         SPR# 80390
   04/02/90      0001      Dave Hozlock         SPR# 90697
   06/20/90      0001      Paul Pisani          SPR# 90775
   08/06/90      0002      Edward Wendowski     SPR# 90842
   01/08/91      0003      Paul Jendrowski      SPR# 90888
   02/13/91      0003      Bayard Johnston      SPR # 91254
   12/03/91      0004      Steve Anderson       SPR # 92740
   12/10/91      0005      Ed Nichlas           SPR 92637 PDL Removal
   04/24/92      0006      Toolset              SPR 91895
   03/25/93      0007      Toolset              SPR NA93-06801
   01/28/94      0008      Toolset              SPR NA94-01101
   03/03/94      0009      Toolset              SPR NA94-05501
   04/01/95      0010      Toolset              CCR NA95-11802
   08/25/95      0011      Dennis Miller        CCR NA94-08459
   12/23/96      0012      Toolset              CCR NA95-11807
   03/16/99      0013      Toolset              CCR NA98-23803
   01/15/05      0012      Cham Pham            CCR NA05-01303
 
*****************************************************************************/
/* Global include files */
#include <a309.h>
#include <a313hbuf.h>
#include <a313h.h>
#include <prcprtac_main.h>
#include <epre_main.h>

/* Local include file */
#include "prcprtac_Constants.h"

/* Declare function prototypes */
void copy_input_buffer( EPRE_buf_t *in_buf );
int rate_scan_ctrl( void );

void Rate_Buffer_Control( int *abort_flg )
{

int iostat, status;		/* Variables to hold function results         */
EPRE_buf_t *InPtr = NULL;	/* Pointer to input buffer structure          */

/* Begin executable code...*/
   if ( DEBUG ) 
   {
     fprintf(stderr," ***** BEGIN MODULE A31340__BUFFER_CONTROLLER\n");
   }
   *abort_flg = FLAG_SET; 

/* Get input buffer...*/
   InPtr = (EPRE_buf_t *)RPGC_get_inbuf( HYBRSCAN, &iostat );

/* Check the iostat for normal condition...*/
   if ( ( iostat != NORMAL ) || ( InPtr == NULL ) )
   {
     RPGC_log_msg( GL_INFO, "RPGC_get_inbuf HYBRSCAN (%d)\n",iostat);

     if ( InPtr != NULL ) 
     {
       RPGC_rel_inbuf( (void*)InPtr );
       InPtr = NULL;

       if ( DEBUG )  
         {fprintf(stderr," Cannot get input buffer, status=%d\n",iostat);}

     }

     RPGC_cleanup_and_abort( iostat );
   }
   else 
   {
/* Copy input buffer from HYBRSCAN to local structure */
     copy_input_buffer( InPtr );

/* Run Rate algorithm */
     status = rate_scan_ctrl( );

/* Check for successful i/o...*/
     if ( status == IO_OK ) 
     {
       if (DEBUG) {fprintf(stderr," RATE output buffer===============\n");}
       *abort_flg = FLAG_CLEAR;
       RPGC_rel_inbuf( (void*)InPtr );
     } 
     else 
     {
/* Bad i/o, notify statmon and abort hydromet (via hari kiri)...*/
        fprintf(stderr,"***** a31340: i/o error=%d\n",status);
        RPGC_rel_inbuf( (void*)InPtr );
        fprintf(stderr,"calling RPGC_hari_kiri()\n");
        RPGC_hari_kiri( );
     } 

   }/* End else for get input buffer */

   if ( DEBUG )
     {fprintf(stderr," ***** END OF MODULE A31340__BUFFER_CONTROLLER\n");}
}
