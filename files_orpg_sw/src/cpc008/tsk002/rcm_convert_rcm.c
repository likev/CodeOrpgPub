/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2003/05/30 14:41:28 $
 * $Id: rcm_convert_rcm.c,v 1.2 2003/05/30 14:41:28 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */  
#include <orpg.h>

#ifdef SLRS_SPK
   #define RCM_strip_off_graphic rcm_strip_off_graphic_
#endif

#ifdef LINUX
   #define RCM_strip_off_graphic rcm_strip_off_graphic__
#endif

/* local functions */
void RCM_strip_off_graphic( char *Rcm, int *status );

/**************************************************************************

   Description: 
      This function converts the pre-edit RCM to the edited
      RCM format.

   Inputs:
      Rcm - pointer to start of RCM product.

   Outputs:
      status - pointer to int to hold status of operation.

   Returns:

   Notes:
      There is no error checking in this function.  It is assumed the 
      legacy code is correct.

**************************************************************************/
void RCM_strip_off_graphic( char *Rcm, int *status ){

   unsigned short *spt, *start, *temp;
   int tab_alpha_off, len_off, len;

   LE_send_msg( GL_INFO,  "Convert RCM to Alphanumeric Only");

   *status = 0;
   spt = (unsigned short *) Rcm;

   /* Gives the offset, in number of shorts, to start of the "-1"
      field of the alphanumeric block (Block ID 3). */
   tab_alpha_off = (spt[OTADLSWOFF - 1] << 16) | spt[OTADLSWOFF];

   /* Gives the offset, in number of shorts, to length of the alphanumeric
      block (from the second product header). */
   len_off = tab_alpha_off + 8;

   /* Gives the length of the alphanumeric block, in shorts. */
   len = (spt[len_off] << 16) | spt[len_off + 1];

   /* Gives the start of the second product header.  It is here that 
      the stand-alone alphanumeric product begins. */
   start = spt + (tab_alpha_off + 4);

   /* malloc a temporary block of memory to hold the alphanumeric data. */
   temp = (unsigned short *) malloc( len*sizeof(unsigned short) );
   if( temp == NULL ){

      LE_send_msg( GL_ERROR, "malloc Failed for %d Bytes\n", len*sizeof(unsigned short) );
      *status = -1;
      return;

   } 

   /* Copy the alphanumeric data to the temporary buffer. */
   memcpy( temp, start, len*sizeof(unsigned short) );

   /* Copy the alphanumeric data back to the original product buffer. */
   memcpy( Rcm, temp, len*sizeof(unsigned short) );

   free( temp );

   return;

}

