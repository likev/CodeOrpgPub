/* 
 * RCS info 
 * $Author: steves $ 
 * $Locker:  $ 
 * $Date: 2014/04/25 17:22:32 $ 
 * $Id: rpg_vcp_info.c,v 1.7 2014/04/25 17:22:32 steves Exp $ 
 * $Revision: 1.7 $ 
 * $State: Exp $ 
 */ 
#include <rpg.h>
#include <rdacnt.h>
#include <vcp.h>

#define UNDEFINED_INDEX		-1

/* Static Variables. */
static RDA_rdacnt_t Rda_rdacnt;
static int Read_RDA_RDACNT = 0;
static unsigned int Vol_seq = 0;
static int Vs_num = 0;

/* Function Prototypes. */
void Notify_callback( int fd, LB_id_t msgid, int msg_info, 
                      void *arg );
static int Read_rda_rdacnt();
static Vcp_struct* VI_get_vcp_data( int vcp_num );

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
int RPG_remap_rpg_elev_index( int *vcp_num, int *elev_index ){

    /* Local variables */
    int uniq_cuts, prev_ind, elv, cnelv;
    int lvcp_num = 0;
    int lelev_index = *elev_index;

    Vcp_struct *vcp_data = NULL;
    short *rdccon = NULL;
    unsigned short *suppl = NULL, suppl_flag = 0;

    static short Remapped_rpg_elev_ind[ECUTMAX];

    /* If vcp_num is less than 0, return elev_index. */ 
    lvcp_num = *vcp_num;
    if( lvcp_num < 0 )
       return 0;

    /* Get the VCP data.   This is the RDA version of the 
       VCP definition. */
    vcp_data = VI_get_vcp_data( lvcp_num );
    if( vcp_data == NULL ){

       LE_send_msg( GL_ERROR, "Error Accessing VCP Data\n" );
       return -1;

    }

    cnelv = vcp_data->n_ele;

    /* Get the RDA to RPG elevation mapping table. */
    rdccon = (short *) VI_get_elev_index_table( lvcp_num );
    if( rdccon == NULL ){

       LE_send_msg( GL_ERROR, "Error Accessing RDCCON Data\n" );
       return -1;

    }

    /* Get the Supplemental Flags table. */
    suppl = (unsigned short *) VI_get_suppl_flags_table( lvcp_num );
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
    *elev_index = Remapped_rpg_elev_ind[lelev_index];
    
    return 0;

/* End of RPG_remap_rpg_elev_index() */
}


/***************************************************************
   Description:
      Returns the number of elevations in the VCP, given the 
      VCP number. 

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.

   Returns: 
      Number of elevations in VCP on success.

***************************************************************/
int VI_get_num_elevations( int vcp_num ){

   int ret = sizeof(RDA_rdacnt_t);
   int vs_num = 0;

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

/* End of VI_get_num_elevations() */
}

/***************************************************************
   Description:
      Returns the RPG elevation number given the VCP number and 
      RDA elevation index.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.
      elev_ind - RDA elevation index within VCP (zero indexed).

   Returns: 
      -1 on error or RPG elevation number on success.

***************************************************************/
int VI_get_rpg_elevation_num( int vcp_num, int elev_ind ){

   int ret = sizeof(RDA_rdacnt_t);
   int vs_num = 0;

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

         int num_elev = VI_get_num_elevations( vcp_num );

         /* Verify the elevation index is not larger than 
            the number of elevations in the VCP. */
         if( (num_elev < 0) || (elev_ind >= num_elev) )
            return -1;

         /* Return RPG elevation number. */
         return( (int) Rda_rdacnt.data[vs_num].rdccon[elev_ind] );

      }

   }

   /* If it falls through to here, then use the data provided 
      in RDACNT (via ORPGVCP functions). */
   return( ORPGVCP_get_rpg_elevation_num( vcp_num, elev_ind ) );

/* End of VI_get_rpg_elevation_num() */
}


/***************************************************************
   Description:
      Returns the elevation angle, in degrees, given the VCP 
      number and elevation index.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.
      elev_ind - RDA elevation index within VCP (zero indexed).

   Returns: 
      -99.9 on error or elevation angle on success.

***************************************************************/
float VI_get_elevation_angle( int vcp_num, int elev_ind ){

   int target_elev;
   int vs_num = 0, ret = sizeof(RDA_rdacnt_t);
   float elev_angle = -99.9;

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

         int num_elev = VI_get_num_elevations( vcp_num );

         /* Verify the elevation index is not larger than 
            the number of elevations in the VCP. */
         if( (num_elev < 0) || (elev_ind >= num_elev) )
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

/* End of VI_get_elevation_angle() */
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
      Set the volume scan sequence number. 

*****************************************************************/
void VI_set_vol_seq_num( unsigned int vol_seq ){

   Vol_seq = vol_seq;
   Vs_num = Vol_seq % MAX_VSCAN;
   if( Vs_num == 0 )
      Vs_num = MAX_VSCAN;

/* End of VI_set_vol_seq_num(). */
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
   int i, rda_ind;

   /* Verify the Vs_num and vol_num are consistent. */
   if( vol_num != Vs_num ){

      LE_send_msg( GL_INFO, "VI_is_supplemental_scan() Failed\n" );
      LE_send_msg( GL_INFO, "--->vol_num: %d Does Not Match Vs_num\n",
                   vol_num, Vs_num );
      return 0;

   }

   /* Get the supplemental flags table. */
   suppl = VI_get_suppl_flags_table( vcp_num );
   if( suppl == NULL ){

      LE_send_msg( GL_INFO, "VI_is_supplemental_scan() Failed\n" );
      LE_send_msg( GL_INFO, "--->RPG_get_suppl_flags_table(%d) Failed\n",
                   vcp_num );
      return 0;

   }

   /* Get the RDA/RPG elevation mapping table. */
   rdccon = VI_get_elev_index_table( vcp_num );
   if( rdccon == NULL ){

      LE_send_msg( GL_INFO, "VI_is_supplemental_scan() Failed\n" );
      LE_send_msg( GL_INFO, "--->RPG_get_elev_index_table(%d) Failed\n",
                   vcp_num );
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
      return 0;

   }

   /* So far so good. Mask off the Supplemental Scan bit.  If
      bit is set, return 1 (True) */
   if( suppl[rda_ind] & RDACNT_SUPPL_SCAN )
      return 1;

   /* Return 0 (False) */
   return 0;

/* End of VI_is_supplemental_scan() */
}

/***************************************************************
   Description:
      Returns the RPG supplemental flags table associated with 
      vcp_num.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.

   Returns: 
      NULL on error or RPG supplemental flags data on success.  
      User is responsible for freeing the RPG supplmental flags
      data.

***************************************************************/
unsigned short* VI_get_suppl_flags_table( int vcp_num ){

   int vs_num = 0, ind, ret = sizeof(RDA_rdacnt_t);
   unsigned short *suppl = NULL;
   unsigned short *rpg_suppl_flags_table = NULL;

   /* Even if the data is stored in RDA_RDACNT, we still require
      a VCP of the same number to be stored in RDACNT. */
   if( (ind = ORPGVCP_index( vcp_num )) < 0 ){

      LE_send_msg( GL_INFO, "Invalid or Unknown VCP: %d\n", vcp_num );
      return NULL;

   }

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

/* End of VI_get_vcp_suppl_flags_table() */
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

***************************************************************/
short* VI_get_elev_index_table( int vcp_num ){

   int vs_num = 0, ind, ret = sizeof(RDA_rdacnt_t);
   short *rdccon = NULL;
   short *rpg_index_table = NULL;

   /* Even if the data is stored in RDA_RDACNT, we still require
      a VCP of the same number to be stored in RDACNT. */
   if( (ind = ORPGVCP_index( vcp_num )) < 0 ){

      LE_send_msg( GL_INFO, "Invalid or Unknown VCP: %d\n", vcp_num );
      return NULL;

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

      if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num )
         rdccon = &Rda_rdacnt.data[vs_num].rdccon[0];

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

/* End of VI_get_vcp_elev_index_table() */
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
   
/* End of Nnotify_callback() */
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

/***************************************************************
   Description:
      Returns the VCP data associated with vcp_num.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.

   Returns: 
      NULL on error or VCP data on success.  User is responsible 
      for freeing the VCP data.

   Notes:  If vcp_num is < 0, then return the local VCP 
           definition.

***************************************************************/
static Vcp_struct* VI_get_vcp_data( int vcp_num ){

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

/* End of VI_get_vcp_data() */
}
 
