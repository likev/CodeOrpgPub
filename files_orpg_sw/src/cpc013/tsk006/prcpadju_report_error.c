/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 1999/04/26 21:16:19 $
 * $Id: prcpadju_report_error.c,v 1.1 1999/04/26 21:16:19 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifdef SUNOS
#define prcpadju_report_error prcpadju_report_error_
#endif
#ifdef LINUX
#define prcpadju_report_error prcpadju_report_error__
#endif

#include <orpg.h>

/*******************************************************************    
   This function reports a read/write error occuring with "filename".
 
   Return:  There is no return value.  

********************************************************************/
void prcpadju_report_error( int *error_value, 
                            char *file_name, 
                            int *read_or_write ){


   if( *read_or_write == 0 )
      LE_send_msg( GL_ERROR, "Error %d Occurred During Read of %s\n",
                   *error_value, file_name );
   
   else
      LE_send_msg( GL_ERROR, "Error %d Occurred During Write of %s\n",
                   *error_value, file_name );

   return;
}
