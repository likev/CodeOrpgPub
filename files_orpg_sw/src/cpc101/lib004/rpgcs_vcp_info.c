/* 
 * RCS info 
 * $Author: steves $ 
 * $Locker:  $ 
 * $Date: 2014/04/25 17:23:55 $ 
 * $Id: rpgcs_vcp_info.c,v 1.25 2014/04/25 17:23:55 steves Exp $ 
 * $Revision: 1.25 $ 
 * $State: Exp $ 
 */ 
#include <rpgcs.h>
#include <rpgc.h>
#include <rdacnt.h>
#include <vcp.h>

/* Static Variables. */
static RDA_rdacnt_t Rda_rdacnt;
static int Read_RDA_RDACNT = 0;
static unsigned int Vol_seq = 0;
static int Vs_num = 0;

/* Function Prototypes. */
void Notify_callback( int fd, LB_id_t msgid, int msg_info, 
                      void *arg );
static int Read_rda_rdacnt();

/***************************************************************
   Description:
      C/C++ algorithm infrastructure support routine.  Returns 
      the number of elevations in the VCP, given the VCP number. 

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.

   Returns: 
      Number of elevations in VCP on success.

   Notes:
      vcp_num should be set to -VCP Number if the user needs 
      to refer to the locally defined version of the VCP.
      Unless SAILS is active, the locally defined version of 
      the VCP is the same as the one provided by the RDA with
      respect to elevations.   If a processs is not configured
      to use SAILS cuts, then vcp_num should be -VCP Number.

***************************************************************/
int RPGCS_get_num_elevations( int vcp_num ){

   int vs_num = 0, ret = sizeof(RDA_rdacnt_t);

   /* If the VCP number is less than 0, use locally defined VCP. */
   if( vcp_num < 0 ){

      vcp_num = -vcp_num;
      return( ORPGVCP_get_num_elevations( vcp_num ) );

   }

   /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
      RDA provided VCP definition.  If available and the vcp 
      number matches, use this.  Otherwise use the information
      provide by RDACNT. */
   if( Read_RDA_RDACNT )
      ret = Read_rda_rdacnt();

   if( ret >= sizeof(RDA_rdacnt_t) ){

      VCP_ICD_msg_t *rdavcp = NULL;

      /* Determine which index to access. */
      if( (Vs_num <= 0) || (Vs_num >= MAX_VSCANS) )
         vs_num = Rda_rdacnt.last_entry;

      else
         vs_num = Vs_num % 2;

      rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];
      if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num ){

         /* Return number of elevations. */
         return( (int) rdavcp->vcp_elev_data.number_cuts );

      }

   }

   /* If it falls through to here, then use the data provided 
      in RDACNT (via ORPGVCP functions). */
   return( ORPGVCP_get_num_elevations( vcp_num ) );

/* End of RPGCS_get_num_elevations() */
}

/***************************************************************
   Description:
      C/C++ algorithm infrastructure support routine.  Returns 
      the RPG elevation number given the VCP number and RDA 
      elevation index.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.
      elev_ind - RDA elevation index within VCP (zero indexed).

   Returns: 
      -1 on error or RPG elevation number on success.

   Notes:
      vcp_num should be set to -VCP Number if the elev_ind passed
      needs to refer to the locally defined version of the VCP.
      Unless SAILS is active, the locally defined version of 
      the VCP is the same as the one provided by the RDA with
      respect to elevations.   If a processs is not configured
      to use SAILS cuts and the elev_ind is passed in as an
      elevation cut counter (vice the elev index one would get 
      from the radial header), then vcp_num should be -VCP Number.

***************************************************************/
int RPGCS_get_rpg_elevation_num( int vcp_num, int elev_ind ){

   int vs_num = 0, ret = sizeof(RDA_rdacnt_t);

   /* If the VCP number is less than 0, use locally defined VCP. */
   if( vcp_num < 0 ){

      vcp_num = -vcp_num;
      return( ORPGVCP_get_rpg_elevation_num( vcp_num, elev_ind ) );

   }

   /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
      RDA provided VCP definition.  If available and the vcp 
      number matches, use this.  Otherwise use the information
      provide by RDACNT. */
   if( Read_RDA_RDACNT )
      ret = Read_rda_rdacnt();

   if( ret >= sizeof(RDA_rdacnt_t) ){

      VCP_ICD_msg_t *rdavcp = NULL;

      /* Determine which index to access. */
      if( (Vs_num <= 0) || (Vs_num >= MAX_VSCANS) )
         vs_num = Rda_rdacnt.last_entry;

      else
         vs_num = Vs_num % 2;

      rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];

      if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num ){

         int num_elevs = RPGCS_get_num_elevations( vcp_num );

         /* Verify we aren't trying to get index for more elevations
            than are in the VCP. */
         if( (num_elevs < 0) || (elev_ind >= num_elevs) )
            return -1;

         /* Return RPG elevation number. */
         return( (int) Rda_rdacnt.data[vs_num].rdccon[elev_ind] );

      }

   }

   /* If it falls through to here, then use the data provided 
      in RDACNT (via ORPGVCP functions). */
   return( ORPGVCP_get_rpg_elevation_num( vcp_num, elev_ind ) );

/* End of RPGCS_get_rpg_elevation_num() */
}

/***************************************************************
   Description:
      C/C++ algorithm infrastructure support routine.  Returns 
      the elevation waveform given the VCP number and RDA 
      elevation index.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.
      elev_ind - RDA elevation index within VCP (zero indexed).

   Returns: 
      -1 on error or RPG elevation waveform on success.

   Notes:
      vcp_num should be set to -VCP Number if the elev_ind passed
      needs to refer to the locally defined version of the VCP.
      Unless SAILS is active, the locally defined version of 
      the VCP is the same as the one provided by the RDA with
      respect to elevations.   If a processs is not configured
      to use SAILS cuts and the elev_ind is passed in as an
      elevation cut counter (vice the elev index one would get 
      from the radial header), then vcp_num should be -VCP Number.

***************************************************************/
int RPGCS_get_elev_waveform( int vcp_num, int elev_ind ){

   int vs_num = 0, ret = sizeof(RDA_rdacnt_t);

   /* If the VCP number is less than 0, use locally defined VCP. */
   if( vcp_num < 0 ){

      vcp_num = -vcp_num;
      return( ORPGVCP_get_waveform( vcp_num, elev_ind ) );

   }

   /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
      RDA provided VCP definition.  If available and the vcp 
      number matches, use this.  Otherwise use the information
      provide by RDACNT. */
   if( Read_RDA_RDACNT )
      ret = Read_rda_rdacnt();

   if( ret >= sizeof(RDA_rdacnt_t) ){

      VCP_ICD_msg_t *rdavcp = NULL;

      /* Determine which index to access. */
      if( (Vs_num <= 0) || (Vs_num >= MAX_VSCANS) )
         vs_num = Rda_rdacnt.last_entry;

      else
         vs_num = Vs_num % 2;

      rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];

      if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num ){

         int num_elevs = RPGCS_get_num_elevations( vcp_num );

         /* Verify we aren't trying to get index for more elevations
            than are in the VCP. */
         if( (num_elevs < 0) || (elev_ind >= num_elevs) )
            return -1;

         /* Return elevation waveform. */
         return( (int) rdavcp->vcp_elev_data.data[elev_ind].waveform);

      }

   }

   /* If it falls through to here, then use the data provided 
      in RDACNT (via ORPGVCP functions). */
   return( ORPGVCP_get_waveform( vcp_num, elev_ind ) );

/* End of RPGCS_get_elev_waveform() */
}

/***************************************************************
   Description:
      C/C++ algorithm infrastructure support routine.  Returns 
      the elevation azimuth rate given the VCP number and RDA 
      elevation index.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.
      elev_ind - RDA elevation index within VCP (zero indexed).

   Returns: 
      -1 on error or RPG elevation azimuth rate on success.

   Notes:
      vcp_num should be set to -VCP Number if the elev_ind passed
      needs to refer to the locally defined version of the VCP.
      Unless SAILS is active, the locally defined version of 
      the VCP is the same as the one provided by the RDA with
      respect to elevations.   If a processs is not configured
      to use SAILS cuts and the elev_ind is passed in as an
      elevation cut counter (vice the elev index one would get 
      from the radial header), then vcp_num should be -VCP Number.

***************************************************************/
float RPGCS_get_azimuth_rate( int vcp_num, int elev_ind ){

   int vs_num = 0, ret = sizeof(RDA_rdacnt_t);
   float rate;

   /* If the VCP number is less than 0, use locally defined VCP. */
   if( vcp_num < 0 ){

      vcp_num = -vcp_num;
      return( ORPGVCP_get_azimuth_rate( vcp_num, elev_ind ) );

   }

   /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
      RDA provided VCP definition.  If available and the vcp 
      number matches, use this.  Otherwise use the information
      provide by RDACNT. */
   if( Read_RDA_RDACNT )
      ret = Read_rda_rdacnt();

   if( ret >= sizeof(RDA_rdacnt_t) ){

      VCP_ICD_msg_t *rdavcp = NULL;

      /* Determine which index to access. */
      if( (Vs_num <= 0) || (Vs_num >= MAX_VSCANS) )
         vs_num = Rda_rdacnt.last_entry;

      else
         vs_num = Vs_num % 2;

      rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];

      if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num ){

         int num_elevs = RPGCS_get_num_elevations( vcp_num );

         /* Verify we aren't trying to get index for more elevations
            than are in the VCP. */
         if( (num_elevs < 0) || (elev_ind >= num_elevs) )
            return -1.0;

         /* Return elevation azimuth rate. */
         rate = rdavcp->vcp_elev_data.data[elev_ind].azimuth_rate * 
                ORPGVCP_AZIMUTH_RATE_FACTOR;
         return (rate);

      }

   }

   /* If it falls through to here, then use the data provided 
      in RDACNT (via ORPGVCP functions). */
   return( ORPGVCP_get_azimuth_rate( vcp_num, elev_ind ) );

/* End of RPGCS_get_azimuth_rate() */
}

/***************************************************************
   Description:
      C/C++ algorithm infrastructure support routine.  Returns 
      the elevation super res flag given the VCP number and RDA 
      elevation index.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.
      elev_ind - RDA elevation index within VCP (zero indexed).

   Returns: 
      -1 on error or RPG elevation super res flag on success.

   Notes:
      vcp_num should be set to -VCP Number if the elev_ind passed
      needs to refer to the locally defined version of the VCP.
      Unless SAILS is active, the locally defined version of 
      the VCP is the same as the one provided by the RDA with
      respect to elevations.   If a processs is not configured
      to use SAILS cuts and the elev_ind is passed in as an
      elevation cut counter (vice the elev index one would get 
      from the radial header), then vcp_num should be -VCP Number.

***************************************************************/
int RPGCS_get_super_res( int vcp_num, int elev_ind ){

   int vs_num = 0, ret = sizeof(RDA_rdacnt_t);
   int super_res = 0;

   /* If the VCP number is less than 0, use locally defined VCP. */
   if( vcp_num < 0 ){

      vcp_num = -vcp_num;
      return( ORPGVCP_get_super_res( vcp_num, elev_ind ) );

   }

   /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
      RDA provided VCP definition.  If available and the vcp 
      number matches, use this.  Otherwise use the information
      provide by RDACNT. */
   if( Read_RDA_RDACNT )
      ret = Read_rda_rdacnt();

   if( ret >= sizeof(RDA_rdacnt_t) ){

      VCP_ICD_msg_t *rdavcp = NULL;

      /* Determine which index to access. */
      if( (Vs_num <= 0) || (Vs_num >= MAX_VSCANS) )
         vs_num = Rda_rdacnt.last_entry;

      else
         vs_num = Vs_num % 2;

      rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];

      if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num ){

         int num_elevs = RPGCS_get_num_elevations( vcp_num );

         /* Verify we aren't trying to get index for more elevations
            than are in the VCP. */
         if( (num_elevs < 0) || (elev_ind >= num_elevs) )
            return -1.0;

         /* Return elevation super res flag. */
         super_res = rdavcp->vcp_elev_data.data[elev_ind].super_res;
         super_res &= (0xff & VCP_SUPER_RES_MASK);

         return (super_res);

      }

   }

   /* If it falls through to here, then use the data provided 
      in RDACNT (via ORPGVCP functions). */
   return( ORPGVCP_get_super_res( vcp_num, elev_ind ) );

/* End of RPGCS_get_super_res() */
}

/***************************************************************
   Description:
      C/C++ algorithm infrastructure support routine.  Returns 
      the elevation angle, in degrees, given the VCP number 
      and elevation index.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.
      elev_ind - RDA elevation index within VCP (zero indexed).

   Returns: 
      -99.9 on error or elevation angle on success.

   Notes:
      vcp_num should be set to -VCP Number if the elev_ind passed
      needs to refer to the locally defined version of the VCP.
      Unless SAILS is active, the locally defined version of 
      the VCP is the same as the one provided by the RDA with
      respect to elevations.   If a processs is not configured
      to use SAILS cuts and the elev_ind is passed in as an
      elevation cut counter (vice the elev index one would get 
      from the radial header), then vcp_num should be -VCP Number.

***************************************************************/
float RPGCS_get_elevation_angle( int vcp_num, int elev_ind ){

   int target_elev, vs_num = 0, ret = sizeof(RDA_rdacnt_t);
   float elev_angle = -99.9;

   /* If the VCP number is less than 0, use locally defined VCP. */
   if( vcp_num < 0 ){

      vcp_num = -vcp_num;
      return( ORPGVCP_get_elevation_angle( vcp_num, elev_ind ) );

   }

   /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
      RDA provided VCP definition.  If available and the vcp 
      number matches, use this.  Otherwise use the information
      provide by RDACNT. */
   if( Read_RDA_RDACNT )
      ret = Read_rda_rdacnt();

   if( ret >= sizeof(RDA_rdacnt_t) ){

      VCP_ICD_msg_t *rdavcp = NULL;

      /* Determine which index to access. */
      if( (Vs_num <= 0) || (Vs_num >= MAX_VSCANS) )
         vs_num = Rda_rdacnt.last_entry;

      else
         vs_num = Vs_num % 2;

      rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];

      if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num ){

         int num_elevs = RPGCS_get_num_elevations( vcp_num );

         /* Verify we aren't trying to get index for more elevations
            than are in the VCP. */
         if( (num_elevs < 0) || (elev_ind >= num_elevs) )
            return elev_angle;

         target_elev = rdavcp->vcp_elev_data.data[elev_ind].angle;
         elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE,
                                           target_elev );
         /* Return elevation angle. */
         return( (float) elev_angle );

      }

   }

   /* If it falls through to here, then use the data provided 
      in RDACNT (via ORPGVCP functions). */
   return( ORPGVCP_get_elevation_angle( vcp_num, elev_ind ) );

/* End of RPGCS_get_elevation_angle() */
}


/***************************************************************
   Description:
      C/C++ algorithm infrastructure support routine.  Returns 
      the target elevation angle, in degrees*10, given the 
      VCP number and elevation index.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.
      elev_ind - RPG elevation index within VCP.

   Returns: 
      Error code (defined in rpgcs.h) on error or elevation
      angle*10 on success.

   Notes:
      vcp_num should be set to -VCP Number if the elev_ind passed
      needs to refer to the locally defined version of the VCP.
      Unless SAILS is active, the locally defined version of 
      the VCP is the same as the one provided by the RDA with
      respect to elevations.   If a processs is not configured
      to use SAILS cuts and the elev_ind is passed in as an
      elevation cut counter (vice the elev index one would get 
      from the radial header), then vcp_num should be -VCP Number.

***************************************************************/
int RPGCS_get_target_elev_ang( int vcp_num, int elev_ind ){

   int num_elevs, ind, target_elev = RPGCS_ERROR, i;
   int vs_num = 0, ret = sizeof(RDA_rdacnt_t);
   float elev_angle = -99.9;

   /* If the VCP number is greater than 0, use the VCP provide 
      by the RDA. */
   if( vcp_num > 0 ){

      /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
         RDA provided VCP definition.  If available and the vcp 
         number matches, use this.  Otherwise use the information
         provide by RDACNT. */
      if( Read_RDA_RDACNT )
         ret = Read_rda_rdacnt();

      if( ret >= sizeof(RDA_rdacnt_t) ){

         VCP_ICD_msg_t *rdavcp = NULL;

         /* Determine which index to access. */
         if( (Vs_num <= 0) || (Vs_num >= MAX_VSCANS) )
            vs_num = Rda_rdacnt.last_entry;

         else
            vs_num = Vs_num % 2;

         rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];

         if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num ){

            /* Get target elevation for the requested elevation index . */
            num_elevs = rdavcp->vcp_elev_data.number_cuts;
            for( i = 0; i < num_elevs; i++ ){

               if( Rda_rdacnt.data[vs_num].rdccon[i] == elev_ind ){

                  target_elev = rdavcp->vcp_elev_data.data[i].angle;
                  elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, 
                                                    target_elev ); 
                  target_elev = (int) RPGC_NINTD( elev_angle*10.0 );

                  /* Return elevation angle. */
                  return( target_elev );

               }

            }

         }

      }

   }

   /* If it falls through to here, then use the data provided 
      in RDACNT (via ORPGVCP functions). */

   /* If the VCP number is less than 0, change its sign. */
   if( vcp_num < 0 )
      vcp_num = -vcp_num;

   /* Find the index into the VCP table corresponding to 
      vcp_num. */
   if( (ind = ORPGVCP_index( vcp_num )) < 0 )
      return( target_elev );

   /* Index of VCP has been found... now find the elevation angle
      initialize elevation index to zero. */
   num_elevs = ORPGVCP_get_num_elevations( vcp_num );
   if( (elev_ind < 0) || (elev_ind > num_elevs) ){

      LE_send_msg( GL_ERROR, "Elev Index < 0 or > %d\n", num_elevs );
      return( target_elev );
 
   }
   
   /* Get target elevation for the requested elevation index . */
   for( i = 0; i < num_elevs; i++ ){

      /* Pass in the RDA elevation index, i, and get the RPG elevation
         index, ind. */
      ind = ORPGVCP_get_rpg_elevation_num( vcp_num, i );
      if( ind == elev_ind ){

         elev_angle = ORPGVCP_get_elevation_angle( vcp_num, i ); 
         target_elev = (int) RPGC_NINTD( elev_angle*10.0 );
         return( target_elev );

      }

   }
   
   /* If here, the target elevation was not found so return an
      error. */
   return( target_elev );

/* End of RPGCS_get_target_elev_ang() */
}


/***************************************************************
   Description:
      C/C++ algorithm infrastructure support routine.  Returns 
      the index of the last elevation angle in the supplied VCP.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.

   Returns: 
      Error code (defined in rpgcs.h) on error or elevation
      index.

***************************************************************/
int RPGCS_get_last_elev_index( int vcp_num ){

   int num_elevs;

   /* If the VCP number is less than 0, change its sign. */
   if( vcp_num < 0 )
      vcp_num = -vcp_num;

   /* Index of VCP has been found... now get the number of 
      elevation scans for this vcp. */
   num_elevs = RPGCS_get_num_elevations( vcp_num );
   if( (num_elevs <= 0) || (num_elevs > ECUTMAX) ){

      LE_send_msg( GL_ERROR, "Number of Elevs Incorrect In VCP %d: %d\n",
                   vcp_num, num_elevs );
      return( RPGCS_ERROR );

   }
    
   /* Get last elevation index for this VCP. */
   return( (int) RPGCS_get_rpg_elevation_num( vcp_num, num_elevs-1 ) );

/* End of RPGCS_get_last_elev_index() */
}

/***************************************************************
   Description:
      C/C++ algorithm infrastructure support routine.  Returns 
      the VCP data associated with vcp_num.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.

   Returns: 
      NULL on error or VCP data on success.  User is responsible 
      for freeing the VCP data.

   Notes:  If vcp_num is < 0, then return the local VCP 
           definition.

***************************************************************/
Vcp_struct* RPGCS_get_vcp_data( int vcp_num ){

   int ind, ret = sizeof(RDA_rdacnt_t);
   int vs_num, use_local = 0;
   short *vcp_array = NULL;
   Vcp_struct *vcp_struct = NULL;

   /* If the VCP number is less than 0, use locally defined VCP. */
   use_local = 0;
   if( vcp_num < 0 ){

      vcp_num = -vcp_num;
      use_local = 1;

   }

   /* Even if the data is stored in RDA_RDACNT, we still require
      a VCP of the same number to be stored in RDACNT. */
   if( (ind = ORPGVCP_index( vcp_num )) < 0 ){

      LE_send_msg( GL_INFO, "Invalid or Unknown VCP: %d\n", vcp_num );
      return NULL;

   }

   /* If not use_local, use the RDA provided VCP. */
   if( !use_local ){

      /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
         RDA provided VCP definition.  If available and the vcp 
         number matches, use this.  Otherwise use the information
         provide by RDACNT. */
      if( Read_RDA_RDACNT )
         ret = Read_rda_rdacnt();

      if( ret >= sizeof(RDA_rdacnt_t) ){

         VCP_ICD_msg_t *rdavcp = NULL;

         /* Determine which index to access. */
         if( (Vs_num <= 0) || (Vs_num >= MAX_VSCANS) )
            vs_num = Rda_rdacnt.last_entry;

         else
            vs_num = Vs_num % 2;

         rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];

         if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num )
            vcp_array = &Rda_rdacnt.data[vs_num].rdcvcpta[0];

      }

   }

   /* If vcp_array is NULL, then there must have not been a match
      of VCP number with RDA_RDACNT. */
   if( vcp_array == NULL )
      vcp_array = ORPGVCP_ptr( ind );

   /* We have VCP data! */
   if( vcp_array != NULL ){

      vcp_struct = malloc( sizeof( Vcp_struct ) );
      if( vcp_struct == NULL )
         return NULL;

      memcpy( (void *) vcp_struct, (void *) vcp_array, sizeof( Vcp_struct ) );
      return vcp_struct;

   }

   return NULL;

/* End of RPGCS_get_vcp_data() */
}

/***************************************************************
   Description:
      C/C++ algorithm infrastructure support routine.  Returns 
      the weather mode associated with vcp_num.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.

   Returns: 
      NULL on error or VCP data on success.  User is responsible 
      for freeing the VCP data.

***************************************************************/
int RPGCS_get_wxmode_for_vcp( int vcp_num ){

   int mode, pos, vcp;

   /* If the VCP number is less than 0, make it positive. */
   if( vcp_num < 0 )
      vcp_num = -vcp_num;

   /* Validate the VCP number. */
   if( vcp_num > 255 )
      return -1;

   /* Cycle through weather mode table. */
   for( mode = CLEAR_AIR_MODE; mode <= PRECIPITATION_MODE; mode++ ){

      for( pos = 0; pos < WXVCPMAX; pos++ ){

         if( (vcp = ORPGVCP_get_wxmode_vcp( mode, pos )) == vcp_num )
            return mode;

      }

   }

   return -1;

/* End of RPGCS_get_wxmode_for_vcp() */
}

/***************************************************************
   Description:
      C/C++ algorithm infrastructure support routine.  Returns 
      the RPG elevation index table associated with vcp_num.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.

   Returns: 
      NULL on error or RPG elevation index data on success.  
      User is responsible for freeing the RPG elevation index
      data.

   Notes:
      vcp_num should be set to -VCP Number if the elev_ind passed
      needs to refer to the locally defined version of the VCP.
      Unless SAILS is active, the locally defined version of 
      the VCP is the same as the one provided by the RDA with
      respect to elevations.   If a processs is not configured
      to use SAILS cuts and the elev_ind is passed in as an
      elevation cut counter (vice the elev index one would get 
      from the radial header), then vcp_num should be -VCP Number.

***************************************************************/
short* RPGCS_get_elev_index_table( int vcp_num ){

   int vs_num = 0, ind, use_local = 0, ret = sizeof(RDA_rdacnt_t);
   short *rdccon = NULL;
   short *rpg_index_table = NULL;

   /* If the VCP number is less than 0, use locally defined VCP. */
   use_local = 0;
   if( vcp_num < 0 ){

      use_local = 1;
      vcp_num = -vcp_num;

   }

   /* Even if the data is stored in RDA_RDACNT, we still require
      a VCP of the same number to be stored in RDACNT. */
   if( (ind = ORPGVCP_index( vcp_num )) < 0 ){

      LE_send_msg( GL_INFO, "Invalid or Unknown VCP: %d\n", vcp_num );
      return NULL;

   }

   /* If not using locally defined VCP .... */
   if( !use_local ){ 

      /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
         RDA provided VCP definition.  If available and the vcp 
         number matches, use this.  Otherwise use the information
         provide by RDACNT. */
      if( Read_RDA_RDACNT )
         ret = Read_rda_rdacnt();

      if( ret >= sizeof(RDA_rdacnt_t) ){

         VCP_ICD_msg_t *rdavcp = NULL;

         /* Determine which index to access. */
         if( (Vs_num <= 0) || (Vs_num >= MAX_VSCANS) )
            vs_num = Rda_rdacnt.last_entry;

         else
            vs_num = Vs_num % 2;

         rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];

         if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num )
            rdccon = &Rda_rdacnt.data[vs_num].rdccon[0];

      }

   }

   /* If rdccon is NULL, then there must have not been a match
      of VCP number with RDA_RDACNT. */
   if( rdccon == NULL )
      rdccon = ORPGVCP_elev_indicies_ptr( ind );

   if( rdccon != NULL ){

      rpg_index_table = malloc( ECUTMAX*sizeof(short) );
      if( rpg_index_table == NULL )
         return NULL;

      memcpy( (void *) rpg_index_table, (void *) rdccon, ECUTMAX*sizeof(short) );
      return rpg_index_table;

   }

   return NULL;

/* End of RPGCS_get_elev_index_table() */
}


#define UNDEFINED_INDEX		-1

/***************************************************************

   Description:
      Given VCP number and RPG elevation index, returns the
      remapped elevation index based on no SAILS cuts.  

      Elevation index is assumed to come from the radial 
      header.  This value is based on the VCP definition
      provided by the RDA.  If SAILS is active, the RPG 
      elevation index is relative to the RDA definition of 
      the VCP.  If the index relative to the RPG definition 
      is needed, this function should be called.
      
   Inputs:
      vcp_num - VCP Number
      elev_index - Elevation index

   Returns:
      Always returns 0.  If vcp_num is negative, returns
      the value for elev_index provided as input.  Otherwise
      the remapped value is returned.

***************************************************************/
int RPGCS_remap_rpg_elev_index( int vcp_num, int elev_index ){

    /* Local variables */
    int uniq_cuts, prev_ind, elv, cnelv;

    Vcp_struct *vcp_data = NULL;
    short *rdccon = NULL;
    unsigned short *suppl = NULL, suppl_flag = 0;

    static short Remapped_rpg_elev_ind[ECUTMAX];

    /* If vcp_num is less than 0, return elev_index. */ 
    if( vcp_num < 0 )
       return elev_index;

    /* Get the VCP data.   This is the RDA version of the 
       VCP definition. */
    vcp_data = RPGCS_get_vcp_data( vcp_num );
    if( vcp_data == NULL ){

       LE_send_msg( GL_ERROR, "Error Accessing VCP Data\n" );
       return -1;

    }

    cnelv = vcp_data->n_ele;

    /* Get the RDA to RPG elevation mapping table. */
    rdccon = (short *) RPGCS_get_elev_index_table( vcp_num );
    if( rdccon == NULL ){

       LE_send_msg( GL_ERROR, "Error Accessing RDCCON Data\n" );
       return -1;

    }

    /* Get the Supplemental Flags table. */
    suppl = (unsigned short *) RPGCS_get_suppl_flags_table( vcp_num );
    if( suppl == NULL )
       LE_send_msg( GL_ERROR, "Error Accessing Supplemental Flags Data\n" );

    /* Initialize the RPG elevation index remapping. */
    for( elv = 0; elv < cnelv; ++elv )
       Remapped_rpg_elev_ind[elv] = UNDEFINED_INDEX;

    /* Do For All Elevations in the current volume coverage pattern */
    uniq_cuts = 0;
    prev_ind = 0;
    for( elv = 0; elv < cnelv; ++elv ){

        suppl_flag = 0;
        if( suppl != NULL )
           suppl_flag = suppl[elv];

        /* If the RPG elevation index is different than the previous
           and the current cut is not a SAILS cut, then ... */
        if( (rdccon[elv] != prev_ind)
                         &&
            !(suppl_flag & RDACNT_SUPPL_SCAN) ){

            /* Track the remapped RPG elevation index.  Need to add one since 
               elevation index is unit indexed.  Rpg_elev_ind maps internal cut
               number, which is sequential, into actual rpg cut number for this
               VCP.  */
            ++uniq_cuts;
            Remapped_rpg_elev_ind[rdccon[elv]] = uniq_cuts;

            /* Prepare for next pass. */
            prev_ind = rdccon[elv];

        }

    }

#ifdef DEBUG
    LE_send_msg( GL_INFO, "RPG Elevation Index Remapping Table\n" );
    for( elv = 0; elv < cnelv; ++elv ){
       i = Remapped_rpg_elev_ind[rdccon[elv]];
       if( i != UNDEFINED_INDEX )
          LE_send_msg( GL_INFO, "rdccon[%d]: %d, remap_cut: %d, elang: %6.2f\n",
                       elv, rdccon[elv], i, elang[i-1] );
    }

#endif

    /* Free memory associated with VCP data and RDCCON data. */
    free( vcp_data );
    free( rdccon );
    if( suppl != NULL )
       free( suppl );

    /* Return the remapped elevation index. */
    elev_index = Remapped_rpg_elev_ind[elev_index];
    
    return elev_index;

/* End of RPGCS_remap_rpg_elev_index() */
}


/***************************************************************
   Description:
      C/C++ algorithm infrastructure support routine.  Returns 
      the RPG supplemental flags table associated with vcp_num.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.

   Returns: 
      NULL on error or RPG supplemental flags data on success.  
      User is responsible for freeing the RPG supplmental flags
      data.

***************************************************************/
unsigned short* RPGCS_get_suppl_flags_table( int vcp_num ){

   int vs_num = 0, ind, use_local = 0, ret = sizeof(RDA_rdacnt_t);
   unsigned short *suppl = NULL;
   unsigned short *rpg_suppl_flags_table = NULL;

   /* If the VCP number is less than 0, use locally defined VCP. */
   use_local = 0;
   if( vcp_num < 0 ){

      use_local = 1;
      vcp_num = -vcp_num;

   }

   /* Even if the data is stored in RDA_RDACNT, we still require
      a VCP of the same number to be stored in RDACNT. */
   if( (ind = ORPGVCP_index( vcp_num )) < 0 ){

      LE_send_msg( GL_INFO, "Invalid or Unknown VCP: %d\n", vcp_num );
      return NULL;

   }

   /* If not using locally define VCP ... */
   if( !use_local ){

      /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
         RDA provided VCP definition.  If available and the vcp 
         number matches, use this.  Otherwise return an zero'd out
         table. */
      if( Read_RDA_RDACNT )
         ret = Read_rda_rdacnt();

      if( ret >= sizeof(RDA_rdacnt_t) ){

         VCP_ICD_msg_t *rdavcp = NULL;

         /* Determine which index to access. */
         if( (Vs_num <= 0) || (Vs_num >= MAX_VSCANS) )
            vs_num = Rda_rdacnt.last_entry;

         else
            vs_num = Vs_num % 2;

         rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];

         if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num )
            suppl = &Rda_rdacnt.data[vs_num].suppl[0];

      }

   }

   /* If suppl is NULL, try the locally define version of this table. */
   if( use_local )
      suppl = (unsigned short *) ORPGVCP_vcp_flags_ptr( vcp_num );

   /* If suppl is NULL, then there must have not been a match
      of VCP number with RDA_RDACNT. */
   if( suppl != NULL ){

      rpg_suppl_flags_table = calloc( ECUTMAX*sizeof(short), 1 );
      if( rpg_suppl_flags_table == NULL )
         return NULL;

      memcpy( (void *) rpg_suppl_flags_table, (void *) suppl, 
              ECUTMAX*sizeof(short) );
      return rpg_suppl_flags_table;

   }

   return NULL;

/* End of RPGCS_get_vcp_suppl_flags_table() */
}


/*****************************************************************

   Description:
      Initializes the Volume Information (VI) module. 

*****************************************************************/
void VI_initialize(){

   int ret;

   /* Open ORPGDAT_ADAPTATION for write permission. */
   ORPGDA_write_permission( ORPGDAT_ADAPTATION );

   /* Register for updates. */
   if( (ret = ORPGDA_UN_register( ORPGDAT_ADAPTATION, RDA_RDACNT,
                                  Notify_callback )) < 0 )
      PS_task_abort( "LB Notification Registration Failed for RDA_RDACNT\n" );

   /* Perform an initial read of this data. */
   if( (ret = ORPGDA_read( ORPGDAT_ADAPTATION, &Rda_rdacnt, 
                           sizeof(RDA_rdacnt_t), RDA_RDACNT )) <= 0 ){

      if( ret != LB_NOT_FOUND ) 
         PS_task_abort( "ORPGDA_read(RDA_RDACNT) Failed: %d\n", ret );

   }

/* End of VI_initialize(). */
}


/*****************************************************************

   Description:
      Set the volume sequence number .... 

   Note:
      MAX_VSCAN has a value of 80, MAX_VSCANS has a value of 81.
      Poor choice of macro names .... easy to mix up.

*****************************************************************/
void VI_set_vol_seq_num( unsigned int vol_seq ){

   Vol_seq = vol_seq;
   Vs_num = Vol_seq % MAX_VSCAN;
   if( Vs_num == 0 )
      Vs_num = MAX_VSCAN;

/* End of VI_set_vol_seq_num() */
}


/*****************************************************************

   Description:
      Returns 1 if volume scan and elevation index are a
      supplemental scan. 

   Inputs:
      vol_num - Volume scan number [0,80].
      elev_index - RPG elevation index.

   Returns:
      1 is volume scan/elevation is supplemental scan or 0 
      otherwise.

*****************************************************************/
int VI_is_supplemental_scan( int vcp_num, int vol_num, 
                             int elev_index ){

   unsigned short *suppl = NULL; 
   short *rdccon = NULL; 
   int i, rda_ind, retval = 0;

   /* Verify the Vs_num and vol_num are consistent. */
   if( vol_num != Vs_num ){

      LE_send_msg( GL_INFO, "VI_is_supplemental_scan() Failed\n" );
      LE_send_msg( GL_INFO, "--->vol_num: %d Does Not Match Vs_num: %d\n",
                   vol_num, Vs_num );
      return 0;

   }

   /* Get the supplemental flags table. */
   suppl = RPGCS_get_suppl_flags_table( vcp_num );
   if( suppl == NULL ){

      LE_send_msg( GL_INFO, "VI_is_supplemental_scan() Failed\n" );
      LE_send_msg( GL_INFO, "--->RPGCS_get_suppl_flags_table(%d) Failed\n",
                   vcp_num );
      return 0;

   }

   /* Get the RDA/RPG elevation mapping table. */
   rdccon = RPGCS_get_elev_index_table( vcp_num );
   if( rdccon == NULL ){

      LE_send_msg( GL_INFO, "VI_is_supplemental_scan() Failed\n" );
      LE_send_msg( GL_INFO, "--->RPGCS_get_elev_index_table(%d) Failed\n",
                   vcp_num );

      /* Free suppl flags table. */
      if( suppl != NULL )
         free( suppl );

      return 0;

   }

   /* Find match on elevation index. */
   rda_ind = -1;
   for( i = 0; i < ECUTMAX; i++ ){

      if( rdccon[i] == elev_index ){

         /* Match found.  Break out of loop. */
         rda_ind = i;
         break;

      }

   }
 
   /* Validate the RDA elevation index. */
   if( rda_ind < 0 ){

      LE_send_msg( GL_INFO, "VI_is_supplemental_scan() Failed\n" );
      LE_send_msg( GL_INFO, "--->RDA Elevation Index Not Set.  Value Passed: %d\n",
                   elev_index );

      /* Free suppl flags table. */
      if( suppl != NULL )
         free( suppl );

      /* Free suppl flags table. */
      if( rdccon != NULL )
         free( rdccon );

      return 0;

   }

   /* So far so good. Mask off the Supplemental Scan bit.  If
      bit is set, return 1 (True) */
   retval = 0;
   if( suppl[rda_ind] & RDACNT_SUPPL_SCAN )
      retval = 1;

   /* Free suppl flags table. */
   if( suppl != NULL )
      free( suppl );

   /* Free suppl flags table. */
   if( rdccon != NULL )
      free( rdccon );
    
   /* Return */
   return retval;

/* End of VI_is_supplemental_scan() */
}


/**********************************************************************

   Description:
      Callback function used for LB notification.

   Inputs:
      fd - adaptation data file LB fd.
      msgid - message id withing LB which was updated.
      msg_info - length of the message (not used).
      arg - pointer to Adapt_block structure for the adaptation block
            which was updated.

   Outputs:
      None.

   Notes:

**********************************************************************/
void Notify_callback( int fd, LB_id_t msgid, int msg_info, void *arg ){

   /* Set flag to read RDA_RDACNT. */
   Read_RDA_RDACNT = 1;
   
/* End of Notify_callback() */
}

/*********************************************************************

   Description:
      Reads the RDA_RDACNT message from ORPGDAT_ADAPTATION.

   Returns:
      The return value from ORPGDA_read().

*********************************************************************/
static int Read_rda_rdacnt(){

   /* Reset update flag and read LB. */
   Read_RDA_RDACNT = 0;
   return( ORPGDA_read( ORPGDAT_ADAPTATION, (void *) &Rda_rdacnt, 
                        sizeof(RDA_rdacnt_t), RDA_RDACNT ) );

/* End of Read_rda_rdacnt(). */
}

