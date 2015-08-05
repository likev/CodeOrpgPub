/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2002/12/06 13:49:16 $
 * $Id: radcdmsg_read_rdastatus_lb.c,v 1.8 2002/12/06 13:49:16 christie Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

#include <string.h>
#include<gen_stat_msg.h>
#include <orpgda.h>
#include<orpgdat.h>

#ifdef SUNOS
#define rcm_read_rdastatus_lb rcm_read_rdastatus_lb_
#endif

#ifdef LINUX
#define rcm_read_rdastatus_lb rcm_read_rdastatus_lb__
#endif

void rcm_read_rdastatus_lb( short *rda_status, int *status ){

   int size;
   RDA_status_t buff;

   /*
     Set the rda status data size, in bytes.
   */
   size = sizeof( RDA_status_msg_t ) - 
          sizeof( RDA_RPG_message_header_t );

   /*
     Read the RDA status data.  If read fails, set the RDA
     operability status to INOPERABLE and return.
   */
   if( (*status = ORPGDA_read( ORPGDAT_RDA_STATUS, (char*) &buff, 
                               sizeof(RDA_status_t),
                               RDA_STATUS_ID )) < 0 ){

      rda_status[RS_OPERABILITY_STATUS] = OS_INOPERABLE; 
      return;

   }

   /*
     Copy RDA status data to rda_status buffer.
   */
   memcpy( rda_status, &buff.status_msg.rda_status, size );
      
}
