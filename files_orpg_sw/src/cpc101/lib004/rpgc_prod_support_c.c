/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/05/03 14:43:50 $
 * $Id: rpgc_prod_support_c.c,v 1.26 2012/05/03 14:43:50 steves Exp $
 * $Revision: 1.26 $
 * $State: Exp $
 */

#include <rpgc.h>
#include <rpgcs.h>
#include <orpgsum.h>

#define MAX_VCP_NUMBER          767 		/* From RDA/RPG ICD */
#define IRRE 			(1.2*6371.0) 	/* Effective Earth radius. */
#define CONST 			(2.0*21.21)/IRRE

/*
  Static global variables.
*/
static In_data_type *Inp_list = NULL;       /* list of inputs */
static int N_inps = 0;                      /* Number of inputs */

static Base_data_header Bd_hd;              /* save the base data header */
static int Bd_hd_set = 0;                   /* flag indicating that Bd_hd is set */

static Prod_header Prod_hdr;                /* save the product header. */
static int Prod_hdr_set = 0;                /* flag indicating that Prod_hdr is set. */


/* Function prototypes. */
static int Find_input_buffer( void *bufptr, int *ind, int *num );

/***************************************************************
   Description:
      C/C++ interface routine.  Returns the current volume
      scan number (modulo 80).

***************************************************************/
int RPGC_get_current_vol_num(){

   int vol_num = 0;

   PS_get_current_vol_num( &vol_num );

   return( vol_num );

/* RPGC_get_current_vol_num() */
}

/***************************************************************
   Description:
      C/C++ interface routine.  Returns the current RPG elevation
      index.

***************************************************************/
int RPGC_get_current_elev_index(){

   int elev_index = 0;

   PS_get_current_elev_index( &elev_index );

   return( elev_index );

/* RPGC_get_current_elev_index() */
}

/***************************************************************
   Description:
      C/C++ interface routine.  Returns the current volume 
      scan sequence number.

   Inputs:
      bufptr - pointer to algorithm input buffer.

   Returns: 
      -1 on error or non-negative volume scan sequence number 
      on success.
   
***************************************************************/
unsigned int RPGC_get_buffer_vol_seq_num( void *bufptr ){

   int ret, ind = 0, num = 0;
      
   unsigned int ui_vol_num = 0xffffffff;
         
   ret = Find_input_buffer( bufptr, &ind, &num );
   if( ret >= 0 ){ 

      /* Find out what type of input buffer this is.  If radial
         type message, then volume number is stored in radial
         header.  If other type, then volume number is part of
         orpg product header. */
      if( Inp_list[ind].timing == RADIAL_DATA ){

         Base_data_header *bd_hdr =
            (Base_data_header *) Inp_list[ind].buf[num];
         ui_vol_num = ORPGMISC_vol_seq_num( (int) bd_hdr->vol_num_quotient,
                                            (int) bd_hdr->volume_scan_num );

      }
      else {
      
         Prod_header *phd = (Prod_header *) Inp_list[ind].buf[num];
         ui_vol_num = (unsigned int) phd->g.vol_num;

      }

   }

   return( ui_vol_num );

/* RPGC_get_buffer_vol_seq_num() */
}

/***************************************************************
   Description:
      C/C++ interface routine.  Returns the current volume 
      scan number (modulo 80).

   Inputs:
      bufptr - pointer to algorithm input buffer.

   Returns: 
      -1 on error or non-negative volume scan number on success.

***************************************************************/
int RPGC_get_buffer_vol_num( void *bufptr ){

   int vol_num = -1;
   unsigned int ui_vol_num;

   ui_vol_num = RPGC_get_buffer_vol_seq_num( bufptr );
   if( ui_vol_num == 0xffffffff )
      return( vol_num );

   /* Verify the volume scan number is within acceptable bounds. */
   vol_num = ORPGMISC_vol_scan_num( ui_vol_num );

   return( vol_num );

/* RPGC_get_buffer_vol_num() */
}

/***************************************************************
   Description:
      C/C++ interface routine.  Returns the current volume 
      coverage pattern.

   Inputs:
      bufptr - pointer to algorithm input buffer.

   Returns: 
      -1 on error or non-negative volume scan number on success.

***************************************************************/
int RPGC_get_buffer_vcp_num( void *bufptr ){

   int vcp_num = -1;
   int ret, ind = 0, num = 0;

   ret = Find_input_buffer( bufptr, &ind, &num );
   if( ret >= 0 ){

      /* Find out what type of input buffer this is.  If radial
         type message, then volume number is stored in radial
         header.  If other type, then volume number is part of
         orpg product header. */
      if( Inp_list[ind].timing == RADIAL_DATA ){

         Base_data_header *bd_hdr = 
            (Base_data_header *) Inp_list[ind].buf[num];
         vcp_num = bd_hdr->vcp_num;

      }
      else {

         Prod_header *phd = (Prod_header *) Inp_list[ind].buf[num];
         vcp_num = phd->vcp_num;

      }

   }

   /* Verify the volume scan number is within acceptable bounds. */
   if( (vcp_num <= 0) || (vcp_num > MAX_VCP_NUMBER) )
      vcp_num = -1;

   return( vcp_num );

/* RPGC_get_buffer_vcp_num() */
}

/***************************************************************
   Description:
      C/C++ interface routine.  Returns the current RPG elevation 
      index.

   Inputs:
      bufptr - pointer to algorithm input buffer.

   Returns: 
      -1 on error or RPG elevation index on success.

***************************************************************/
int RPGC_get_buffer_elev_index( void *bufptr ){

   int ret, ind = 0, num = 0;
   int elev_index = -1;

   ret = Find_input_buffer( bufptr, &ind, &num );
   if( ret >= 0 ){

      /* Find out what type of input buffer this is.  If radial
         type message, then volume number is stored in radial
         header.  If other type, then volume number is part of
         orpg product header. */
      if( Inp_list[ind].timing == RADIAL_DATA ){

         Base_data_header *bd_hdr = 
            (Base_data_header *) Inp_list[ind].buf[num];
         elev_index = bd_hdr->rpg_elev_ind;

      }
      else {

         Prod_header *phd = (Prod_header *) Inp_list[ind].buf[num];
         elev_index = phd->g.elev_ind;

      }

   }

   /* Verify the elevation index is within acceptable bounds. */
   if( (elev_index < 0) || (elev_index > MAX_ELEVATION_CUTS) )
      elev_index = -1;

   return( elev_index );

/* RPGC_get_buffer_elev_index() */
}

/***************************************************************
   Description:
      C/C++ interface routine.  Returns the RPG elevation
      index of the buffer and whether or not the buffer is 
      from the last elevation index.

   Inputs:
      bufptr - pointer to algorithm input buffer.

   Outputs:
      elev_index - RPG elevation index of buffer.
      last_elev_index - RPG elevation index of last elevation
                        cut in VCP.

   Returns:
      1 if buffer from last elevation index, 0 if not from
      last elevation index, or -1 on error. 

***************************************************************/
int RPGC_is_buffer_from_last_elev( void *bufptr, int *elev_index,
                                   int *last_elev_index ){

   int vcp_num, last_elv_ind;

   /* Get the VCP number for the buffer. */
   vcp_num = RPGC_get_buffer_vcp_num( bufptr );
   if( vcp_num < 0 ) 
      return -1;

   /* Get the RPG elevation index for the buffer. */
   *elev_index = RPGC_get_buffer_elev_index( bufptr );

   /* Get the last RPG elevation index for the VCP. */
   last_elv_ind = RPGCS_get_last_elev_index( vcp_num );
   if( last_elv_ind < 0 )
      return -1;

   /* Check to see if AVSET is active.  If active, then see if
      this is the last elevation index .... may be different
      than VCP definition. */
   if( ORPGINFO_is_avset_enabled() 
                  || 
       (ORPGRDA_get_status( RS_AVSET) == AVSET_ENABLED) ){

      int vol_num = RPGC_get_buffer_vol_num( bufptr );

      if( vol_num > 0 ){

         Scan_Summary *sum = RPGC_get_scan_summary( vol_num );
         if( (sum != NULL) 
                  && 
             ((int) sum->last_rpg_cut != ORPGSUM_UNDEFINED_CUT) )
            last_elv_ind = (int) sum->last_rpg_cut;

      }

   } 

   /* Set the last elevation index. */
   *last_elev_index = last_elv_ind;

   /* Test if last RPG elevation index. */
   if( *elev_index == last_elv_ind )
      return 1;

   return 0;

} /* RPGC_is_buffer_from_last_elev() */

/****************************************************************
   Description:
      C/C++ interface routine.  Determines which moments are
      enabled.

   Inputs:
      hdr - pointer to base data radial header.
      ref_flag - pointer to reflectivity enabled flag.
      vel_flag - pointer to velocity enabled flag.
      wid_flag - pointer to spectrum width enabled flag.

   Outputs:
      ref_flag - set to TRUE if reflectivity enabled.
      vel_flag - set to TRUE if velocity enabled.
      wid_flag - set to TRUE if spectrum width enabled.

****************************************************************/
void RPGC_what_moments( Base_data_header *hdr, int *ref_flag,
                        int *vel_flag, int *wid_flag ){

   *ref_flag = FALSE;
   *vel_flag = FALSE;
   *wid_flag = FALSE;

   if( hdr->msg_type & REF_ENABLED_BIT )
      *ref_flag = TRUE;

   if( hdr->msg_type & VEL_ENABLED_BIT )
      *vel_flag = TRUE;

   if( hdr->msg_type & WID_ENABLED_BIT )
      *wid_flag = TRUE;

/* End of RPGC_what_moments( ) */
}

/**************************************************************

   Description:
      Given the bin size, return the number of bins at the 70 
      Kft ceiling.

   Inputs:
      radialptr - pointer to radial.
      bin_size - bin size, in meters. 

   Returns:
      The number of bins to the height ceiling or 0 on error.

**************************************************************/
int RPGC_bins_to_ceiling( void *radialptr, int bin_size ){

    /* Local variables */
    int elang;
    double srto70, sinang;
    int elindx, vcpnum, numbins = 0;

    elindx = RPGC_get_buffer_elev_index( radialptr );
    vcpnum = RPGC_get_buffer_vcp_num( radialptr );

    /* Extract the elevation angle from the VCP data. */
    elang = RPGCS_get_target_elev_ang( vcpnum, elindx );

    /* If elevation angle is found, calculate the number of bins.  
       Assign the minimum of maxbins and the calculated number of bins. */
    if( elang != RPGCS_ERROR ){

        double el = elang / 10.0;
        double bin_size_km = (double) bin_size / 1000.0;

	sinang = sin( el * DEGTORAD );
	srto70 = (sqrt( (sinang * sinang) + CONST) - sinang) * IRRE;

	numbins = srto70 / bin_size_km;

    }
    else{

        RPGC_log_msg( GL_INFO, "RPGCS_get_target_elev_ang() Failed\n" );
        RPGC_log_msg( GL_INFO, "--->vcpnum: %d, elindx: %d\n", vcpnum, elindx );

    }

    return numbins;

}
 
/**************************************************************

   Description:
      C/C++ interface for A3CM58__NUM_RAD_BINS.  

      Given the radial step size and the maximum number of bins
      return the minimum of the number of bins at the 70 Kft 
      ceiling and the maximum number of bins.

   Inputs:
      radialptr - pointer to radial
      maxbins - maximum number of bins
      radstep - radial step size. Can have values 1, 2 or 4 --- 
                corresponds to 1km, 2km and 4km. 
      wave_type - if 1, means surveillance data.

   Returns:
      numbins - number of bins

   Notes:
      This function may need to be modifed with when super
      resolution data becomes available.

**************************************************************/
int RPGC_num_rad_bins( void *radialptr, int maxbins, int radstep, 
                       int wave_type ){

    static double bin_factor[4] = { 4.0,2.0,0.0,1.0 };

    /* Local variables */
    int elang;
    double srto70;
    double sinang;
    int elindx, vcpnum;

    int numbins;

    elindx = RPGC_get_buffer_elev_index( radialptr );
    vcpnum = RPGC_get_buffer_vcp_num( radialptr );

    /* Extract the elevation angle from the VCP data. */
    elang = RPGCS_get_target_elev_ang( vcpnum, elindx );

    /* If elevation angle is found, calculate the number of bins.  Assign the 
       minimum of maxbins and the calculated number of bins. */
    if( elang != RPGCS_ERROR ){

        double el = elang / 10.0;

	sinang = sin( el * DEGTORAD );
	srto70 = (sqrt( (sinang * sinang) + CONST) - sinang) * IRRE;

	if( wave_type == 1 )
	    numbins = srto70 / radstep;

	else
	    numbins = srto70 * bin_factor[radstep-1];
	 
	if( maxbins < numbins )
           numbins = maxbins;

    }
    else{

        RPGC_log_msg( GL_INFO, "RPGCS_get_target_elev_ang() Failed\n" );
        RPGC_log_msg( GL_INFO, "--->vcpnum: %d, elindx: %d\n", vcpnum, elindx );
        numbins = maxbins;

    }

    return numbins;
} 

/****************************************************************
   Description:
      Searches the registered and acquired input buffers for 
      match on "bufptr".  

   Inputs:
      bufptr - pointer to input buffer to find.

   Outputs:
      ind - index of match in Inp_list array for buffer types.
      num - buffer number in list of types in Inp_list array.

   Returns:
      -1 if no match found, or 0 is match found.

****************************************************************/
static int Find_input_buffer( void *bufptr, int *ind, int *num ){

   char *buffer_addr = NULL;

   static int initialized = FALSE;

   /*
    Get address of input list if not already done so.
   */
   if( initialized == FALSE ){

      N_inps = IB_inp_list( &Inp_list );
      initialized = TRUE;

   }

   /* If bufptr is NULL, return -1. */
   if( bufptr == (void *) NULL )
      return -1;

   /*
     Search the input list for match on buffer pointer.
   */
   for( *ind = 0; *ind < N_inps; (*ind)++ ){
   
      for( *num = 0; *num <= Inp_list[*ind].buffer_count; (*num)++ ){
   
         /* For non-radial type inputs, must account for product header.
            The buffer pointer returned to the application is the malloced
            address plus the size of the product header. */
         buffer_addr = Inp_list[*ind].buf[*num];
         if( Inp_list[*ind].timing != RADIAL_DATA )
            buffer_addr += sizeof(Prod_header);
 
         if( buffer_addr == bufptr )
            return( 0 );

      }
   
   }
  
   /*
     If no match found, return -1.
   */
   return(-1);
  
}

/***************************************************************
                                                                                                      
   Description:
      C/C++ interface routine
                                                                                                      
   Inputs:
      loc - pointer to where data is to be stored.
      value - value to be stored.
                                                                                                      
   Returns:
                                                                                                      
***************************************************************/
int RPGC_set_product_int( void *loc, unsigned int value ){

   return( ORPGMISC_pack_ushorts_with_value( loc, &value ));
                                                                                                      
/* End of PRGC_set_product_int() */
}

/***************************************************************
                                                                                                      
   Description:
      C/C++ interface routine
                                                                                                      
   Inputs:
      loc - pointer to where data is to be retrieved.
      value - pointer to receive the stored value.
                                                                                                      
   Returns:
                                                                                                      
***************************************************************/
int RPGC_get_product_int( void *loc, void *value ){
                                                                                                      
   return( ORPGMISC_unpack_value_from_ushorts( loc, value ));
                                                                                                      
/* End of PRGC_get_product_int() */
}

/***************************************************************
                                                                                                      
   Description:
      C/C++ interface routine
                                                                                                      
   Inputs:
      loc - pointer to where data is to be stored.
      value - value to be stored.
                                                                                                      
   Returns:
                                                                                                      
***************************************************************/
int RPGC_set_product_float( void *loc, float value ){

   return( ORPGMISC_pack_ushorts_with_value( loc, &value ) );
 
/* End of PRGC_set_product_float() */
}

/***************************************************************
                                                                                                      
   Description:
      C/C++ interface routine
                                                                                                      
   Inputs:
      loc - pointer to where data is to be retrieved.
      value - pointer to receive the stored value.
                                                                                                      
   Returns:
                                                                                                      
***************************************************************/
int RPGC_get_product_float( void *loc, void *value ){
                                                                                                      
   return (ORPGMISC_unpack_value_from_ushorts( loc, value ) ); 
/* End of PRGC_get_product_float() */
}

#ifndef LINUX

/*********************************************************************

      Description:
        This function returns the Nearest Integer of a floating point
        number.

      Inputs:
        r - a floating number.

      Outputs:

      Returns:
        This function returns the Nearest Integer of a floating point
        number.

**********************************************************************/
float RPGC_NINT( float r ){

   float temp;

   if ( r >= 0.0 )
      temp = r + .5;
   else
      temp = -((-r) + .5);

   return( temp );

} /* End of RPGC_NINT() */

/*********************************************************************

      Description:
        This function returns the Nearest Integer of a double 
        precision floating point number.

      Inputs:
        r - a double precision floating number.

      Outputs:

      Returns:
        This function returns the Nearest Integer of a double
        precision floating point number.

**********************************************************************/
double RPGC_NINTD( double r ){

   double temp;

   if ( r >= 0.0 )
      temp = r + .5;
   else
      temp = -((-r) + .5);

   return (temp );

} /* End of RPGC_NINTD() */

#endif

/********************************************************************

     Description: 
         Sets the velocity dealiased bit in the orpg radial header.

     Inputs:
         field_value - pointer to short to receive modified value.
                       Value has VEL_DEALIASED_BIT set.

     Returns:
         There is no return value defined for this function.  Always
         returns 0.

********************************************************************/
int RPGC_set_veldeal_bit( short *field_value ){

   /* Set the velocity dealiasing bit in radial header. */
   *field_value |= VEL_DEALIASED_BIT;

   return (0);

/* End of RPGC_set_veldeal_bit */
}

/********************************************************************

   Description:
       This function converts the OS32 data/time into the UNIX time.

   Inputs:
       date - modified Julian date (from radial header).
       time_ms - radial time (milliseconds since midnight) .. most
                 significant 16 bits.
       time_ls - radial time (milliseconds since midnight) .. least
                 significant 16 bits.

   Return:
       UNIX time is seconds since reference Julian date.

   Note:
       The RDA modified Julian date has 1/1/70 as reference day 1 
       whereas UNIX reference day 0 is 1/1/70.
        
*********************************************************************/
time_t PS_convert_to_unix_time (short date, short time_ms, short time_ls){

    time_t tm;

    tm = (date - 1) * 86400 + ((time_ms << 16) + time_ls) / 1000;
    return (tm);

/* End of PS_convert_to_unix_time */
}

/*********************************************************************

    Description: 
        This function receives the basedata header and stores
	it for use by other functions in this module.

    Input:
	bd_hd - current base data header.

    Return:
        There is no return value defined for this function.

*********************************************************************/
void PS_register_bd_hd (Base_data_header *bd_hd){

    memcpy ((char *)&Bd_hd, (char *)bd_hd, sizeof (Base_data_header));
    Bd_hd_set = 1;
    return;

/* End of PS_register_bd_hd */
}

/*********************************************************************

    Description: 
        This function receives the product header and stores
	it for use by other functions in this module.

    Input:
	phd - current product header.

    Return:
        There is no return value defined for this function.

*********************************************************************/
void PS_register_prod_hdr (Prod_header *phd){

    memcpy ((char *)&Prod_hdr, (char *)phd, sizeof (Prod_header));
    Prod_hdr_set = 1;
    return;

/* End of PS_register_prod_hdr */
}

/********************************************************************

    Description: 
        This function returns the volume time of this radial.

    Inputs:
 	bhd - the base data header.

    Return:
 	Returns the volume time of this radial in modified 
        Julian seconds (i.e., UNIX time).

********************************************************************/
time_t PS_get_volume_time (Base_data_header *bhd){

    int date;
    time_t tm;

    if (bhd->begin_vol_time > bhd->time)
	date = bhd->date - 1;
    else
	date = bhd->date;
    tm = bhd->begin_vol_time / 1000;
    
    return (tm + (date - 1) * 86400);

/* End of PS_get_volume_time */
}

/*********************************************************************

   Description:
       This function returns the current volume scan sequence number. 

   Inputs:
       vol_num - pointer to int to receive volume scan number.

   Outputs:
       vol_num - pointer to int receiving volume scan number (1-80)

   Return:
      Returns the volume sequence number.

*********************************************************************/
unsigned int PS_get_current_vol_num (int *vol_num){

    unsigned int vol_seq_num = 0xffffffff;

    if (Bd_hd_set){

       int vol_quotient;

        /* Get volume scan number from base data header. */
	*vol_num = Bd_hd.volume_scan_num;
	vol_quotient = Bd_hd.vol_num_quotient;
        vol_seq_num = ORPGMISC_vol_seq_num( vol_quotient, *vol_num);

        if( (*vol_num >= 0) && (*vol_num < MAX_VSCANS) )
           return vol_seq_num;

        LE_send_msg( GL_INFO, "Invalid Vol Scan Num in Basedata Hdr (%d)\n",
                     *vol_num );

    }

    /* Volume sequence number is stored in product header.
       Must convert to volume scan number in range 1-80. */
    if( !Bd_hd_set )
       vol_seq_num = OB_vol_number ();

    if( vol_seq_num != 0xffffffff )
       *vol_num = ORPGMISC_vol_scan_num( vol_seq_num );

    else{

        /* Get the volume sequence number from the volume status data. */
        *vol_num = PS_get_vol_stat_vol_num( &vol_seq_num );

    }

    return (vol_seq_num );

/* End of PS_get_current_vol_num */
}

/*********************************************************************************
   
   Description:
      Gets the volume sequence number from volume status.

   Inputs:
      vol_seq - pointer to unsigned int to hold volume sequence number.

   Outputs:

   Returns:
      volume scan number associated with volume sequence number, or 0.

*********************************************************************************/
int PS_get_vol_stat_vol_num( unsigned int *vol_seq ){

   int volume_sequence, vol_num;

   *vol_seq = vol_num = 0;

   /* Get the volume sequence number from the volume status data. */
   volume_sequence = ORPGVST_get_volume_number();
   if( volume_sequence != ORPGVST_DATA_NOT_FOUND ){

      /* Extract the volume sequence number and convert to 
         volume scan number. */
      *vol_seq = (unsigned int) volume_sequence;
      vol_num = ORPGMISC_vol_scan_num( (unsigned int) volume_sequence );

   }

   return( vol_num );

}

/*********************************************************************

   Description:
       This function returns the current RPG elevation index. 

   Inputs:
       index - pointer to int to receive RPG elevation index.

   Returns:
       There is no return value defined for this function.  Always
       returns 0.

*********************************************************************/
int PS_get_current_elev_index (int *index){

    if( Bd_hd_set )
       *index = Bd_hd.rpg_elev_ind;

    else if( Prod_hdr_set )
       *index = Prod_hdr.g.elev_ind;

    else
       *index = 0;

    return (0);

/* End of PS_get_current_elev_index */
}

/***************************************************************
   Description:
      Returns the elevation angle, in degrees*10, given the
      VCP number and elevation index.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.
      elev_ind - RPG elevation index within VCP.

   Outputs:
      elev_angle - Negative -999 on error or elevation 
                   angle*10 on success.
      found = 1 if successful, 0 otherwise.

   Returns:
      -1 on error, 0 on success.

***************************************************************/
int PS_get_elev_angle( int *vcp_num, int *elev_ind, 
                       int *elev_angle, int *found ){

   int num_elevs, ind, i;
   double elev_angle_d;

   int lvcp_num = *vcp_num;
   int lelev_ind = *elev_ind;

   /* Initialize the elevation angle to -999. */
   *elev_angle = -999;
   *found = 0;

   /* Find the index into the VCP table corresponding to
      vcp_num. */
   if( (ind = ORPGVCP_index( lvcp_num )) < 0 )
      return -1;

   /* Index of VCP has been found... now find the elevation angle
      initialize elevation index to zero. */
   num_elevs = RPGCS_get_num_elevations( lvcp_num );
   if( (lelev_ind < 0) || (lelev_ind > num_elevs) ){

      LE_send_msg( GL_ERROR, "Elev Index: %d < 0 or > %d for VCP %d\n", 
                   lelev_ind, num_elevs, lvcp_num );
      return -1;

   }

   /* Get target elevation for the requested elevation index . */
   for( i = 0; i < num_elevs; i++ ){

      /* Pass in the RDA elevation index, i, and get the RPG elevation
         index, ind. */
      ind = RPGCS_get_rpg_elevation_num( lvcp_num, i );
      if( ind == lelev_ind ){

         elev_angle_d = RPGCS_get_elevation_angle( lvcp_num, i );
         elev_angle_d *= 10.0;
         *elev_angle = (int) RPGC_NINTD( elev_angle_d );
         *found = 1;

         return 0;

      }

   }

   /* If here, the target elevation was not found so return an
      error. */
   return -1;

/* End of PS_get_elev_ang() */
}

/********************************************************************

    Description: 
        This function is called when a process wants to stop
	execution because of a fatal error condition. It 
	delivers a message and then calls exit.

    Input:
 	format - message format string;
	... - list of variables whose values are to be printed.

    Notes:
	This function does not return.

********************************************************************/

#define MAX_MSG_SIZE	256

void PS_task_abort (char *format, ... ){

    char buf [MAX_MSG_SIZE];
    va_list args;

    if (format != NULL && *format != '\0') {
	va_start (args, format);
	vsprintf (buf, format, args);
	va_end (args);
    }
    else
	buf [0] = '\0';

    LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, buf);

    /* Set the abort reason code. */
    AP_set_abort_reason( PGM_TASK_SELF_TERMINATED );
 
    /* It is assumed that if this flag is set, then ORPGTASK_exit 
       has already been called. */
    if( !INIT_task_terminating() )
       ORPGTASK_exit (GL_EXIT_FAILURE);

/* End of PS_task_abort */
}
