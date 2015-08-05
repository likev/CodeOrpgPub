/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/04/06 21:33:53 $
 * $Id: a3053b.c,v 1.7 2006/04/06 21:33:53 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#include <prfselect.h>

/* This includes EPWR_SIZE floats to hold epwr data and EPWR_SIZE floats to 
   hold pwr_lookup data. */
#define SCRSIZE    (2*EPWR_SIZE)

/* Static Function prototypes. */
static int Prfselect_get_prfselect();

/*\///////////////////////////////////////////////////////////////////////////////////

   Description:
      Acquires all output buffers needed by the PRF Selection function.

   Outputs:
      outbuf - holds the address of the PRFSEL buffer.
      scratchbuf - holds the address of the SCRATCH buffer.
      autoprf - flag, if set, indicates Auto PRF is active

///////////////////////////////////////////////////////////////////////////////////\*/
int A3053B_get_buffers( char **outbuf, char **scratchbuf, int *autoprf ){

   int ostat = NORMAL, scratchstat = NORMAL; 

  
   /* Initialize flags for which buffers are obtained. */
   *autoprf = 0;

   /* Initialize the scratch buffer pointer. */
   *scratchbuf = NULL;

   /* Get the output buffer. */
   *outbuf = (char *) RPGC_get_outbuf_by_name( "PRFSEL", sizeof(Prfselect_t), &ostat );
   if( ostat == NORMAL ){

      /* Get output buffer for PRF selection information. */
      *autoprf = Prfselect_get_prfselect();

      if( *autoprf ){
 
         /* AUTOPRF turned on, get scratch buffer and PRFSEL output buffer. */
         *scratchbuf = (char *) RPGC_get_outbuf( SCRATCH, SCRSIZE*sizeof(float), &ostat );

         if( scratchstat != NORMAL )
            *autoprf = 0;

      }
      else
         RPGC_log_msg( GL_INFO, "The Auto PRF Selection Flag is Not Set\n" );

   }

   /* Check for ABORT condition. */
   if( (ostat == NO_MEM) || (scratchstat == NO_MEM) ){

      RPGC_log_msg( GL_INFO, "RPGC_get_outbuf() Returned NO_MEM status\n" );
      RPGC_abort_because( PROD_MEM_SHED );

   }
   else if( (ostat != NORMAL) || (scratchstat != NORMAL) ){

      RPGC_log_msg( GL_INFO, "RPGC_get_outbuf Returned non-NORMAL status\n" );
      RPGC_abort();

   }

   return 0;

} /* End of A3053B_get_buffers() */

/*\////////////////////////////////////////////////////////////////////

   Description:
      Returns the value of Auto PRF flag. 

   Returns:
      Auto PRF ON == 1 and OFF == 0.

////////////////////////////////////////////////////////////////////\*/
static int Prfselect_get_prfselect( ){

   unsigned char flag = 0, paused = 0;

   /* Get the PRFSELECT flag from the state information. */
   flag = ORPGINFO_is_prf_select();

   /* Check if the paused flag is set.  If set, clear it. */
   paused = ORPGINFO_is_prf_select_paused();
   if( paused ){

      paused = ORPGINFO_clear_prf_select_paused();
      RPGC_log_msg( GL_INFO, "!!! Resuming Auto PRF from Pause\n" );

   }

   if( flag )
      return 1;

   else 
      return 0;

} /* End of Prfselect_get_prfselect() */
