/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/03/24 16:54:54 $
 * $Id: rpgcs_rda_status.c,v 1.3 2009/03/24 16:54:54 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <rpgcs.h>

/*\////////////////////////////////////////////////////////////////////////

   Description:
      Returns the CMD status. 

   Returns:
      -1 on error, 0 is CMD is disabled, 1 if CMD is enabled. 

////////////////////////////////////////////////////////////////////////\*/
int RPGCS_get_CMD_status(){

   int cmd_status, cmd = 0;

   /* Read the RDA Status data. */
   cmd_status = ORPGRDA_get_status( RS_CMD );
   if( cmd_status == ORPGRDA_DATA_NOT_FOUND )
      return -1;

   /* Check the CMD status flag. */
   cmd = cmd_status & 0x1;

   /* Return 1 - CMD Enabled, or 2 - CMD Disabled. */
   return( (int) cmd );

/* End of RPGCS_get_CMD_status(). */
}

/*\//////////////////////////////////////////////////////////////////////

   Description: 
      Given VCP and RPG elevation index, checks if CMD is active for
      this elevation.
                                                                       
   Input:  
      vcp_num - Volume coverage pattern. 
                                                   
   Output: 
      rpg_elev_ind - RPG elevation index.

   Return: 
      Return -1 on error, 1 if CMD enabled this cut for this VCP, or
      0 if CMD is disabled.                                                    

************************************************************************/
int RPGCS_is_CMD_applied_this_elev( int vcp_num, int rpg_elev_ind ){

   float elev_ang, seg1lim, seg2lim, seg3lim, seg4lim;
   int cmd_status, elev_ang10, nbr_el_segments;

   /* Read the RDA Status data. */
   cmd_status = ORPGRDA_get_status( RS_CMD );
   if( cmd_status == ORPGRDA_DATA_NOT_FOUND ){

      LE_send_msg( GL_INFO, "Unable to retrieve CMD status.\n" );
      return -1;

   }

   /* Check if CMD is enabled.  No need to proceed any further
      if CMD is disabled. */
   if( (cmd_status & 0x1) == 0 )
      return 0;

   /* Read the RDA adaptation data.  On read error, return error. */
   if( ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_NBR_EL_SEGMENTS, 
                                    &nbr_el_segments ) < 0 ){

      LE_send_msg( GL_INFO, "Unable to get ORPGRDA_ADAPT_NBR_EL_SEGMENTS value\n" );
      return -1;

   }

   /* Validate the number of elevation segments. */
   if( (nbr_el_segments <= 0)
               ||
       (nbr_el_segments  > MAX_ELEVATION_SEGS_ORDA) ){

      LE_send_msg( GL_INFO, "Invalid Number Of Elevation Segments: %d\n",
                   nbr_el_segments );
      return -1;

   }

   /* Get the elevation segment data from RDA adaptation data. */
   if( (ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_SEG1LIM, &seg1lim ) < 0) 
                                   ||
       (ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_SEG2LIM, &seg2lim ) < 0) 
                                   ||
       (ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_SEG3LIM, &seg3lim ) < 0) 
                                   ||
       (ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_SEG4LIM, &seg4lim ) < 0) ){

      LE_send_msg( GL_INFO, "Unable to get ORPGRDA_ADPT_SEGxLIM values\n" );
      return -1;

   }

   /* Get the elevation angle for the specified VCP and elevation index. */
   elev_ang10 = RPGCS_get_target_elev_ang( vcp_num, rpg_elev_ind );
   if( elev_ang10 == RPGCS_ERROR ){

      LE_send_msg( GL_INFO, "Unable to get target elevation for VCP\n" );
      return -1;

   }

   /* Convert to degrees. */
   elev_ang = (float) elev_ang10 / 10.0;

   /* Check which elevation segment this cut belongs to. */
   if( elev_ang < seg1lim ){

      /* In first elevation segment. */

      if( cmd_status & 0x2 )
         return 1;

      return 0;

   }
   else if( (elev_ang >= seg1lim) && (elev_ang < seg2lim) ){

      /* In second elevation segment. */

      if( cmd_status & 0x4 )
         return 1;

      return 0;

   }
   else if( (elev_ang >= seg2lim) && (elev_ang < seg3lim) ){

      /* In third elevation segment. */

      if( cmd_status & 0x8 )
         return 1;

      return 0;

   }
   else if( (elev_ang >= seg3lim) && (elev_ang < seg4lim) ){

      /* In fourth elevation segment. */

      if( cmd_status & 0x10 )
         return 1;

      return 0;

   }
   else{

      /* In last elevation segment. */

      if( cmd_status & 0x20 )
         return 1;

      return 0;

   }

   return 0;

/* End of RPGCS_is_CMD_applied(). */
}
