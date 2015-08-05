/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/25 17:22:31 $
 * $Id: rpg_prod_support.c,v 1.32 2014/04/25 17:22:31 steves Exp $
 * $Revision: 1.32 $
 * $State: Exp $
 */  

/********************************************************************

	This module contains basic supporting functions for the 
	old RPG product generaton tasks.

********************************************************************/

#include <rpg.h>
#include <math.h>

#define MAX_VCP_NUMBER            767 /* From RDA/RPG ICD */

/*
  Static global variables.
*/
static In_data_type *Inp_list = NULL;       /* list of inputs */
static int N_inps = 0;                      /* Number of inputs */

static Base_data_header Bd_hd;              /* save the base data header */
static int Bd_hd_set = 0;	            /* flag indicating that Bd_hd is set */

static Prod_header Prod_hdr;                /* save the product header. */
static int Prod_hdr_set = 0;                /* flag indicating that Prod_hdr is set. */

static int Msg_mode = 0;	            /* messaging mode */



/* Function prototypes. */
static int Find_input_buffer( int bufptr, int *ind, int *num );
static int Get_last_elev_index( int vcp_num );
static void Nintd( double *r, int *temp );

/*********************************************************************

   Description:
       This function returns the current target elevation angle (in
       .1 degrees).

   Inputs:
       elev - pointer to int to receive target elevation angle.

   Returns:
       There is no return value currently defined for this function.
       Always returns 0.

*********************************************************************/
int RPG_get_current_target_elev( fint *elev ){

    if( Bd_hd_set )
       *elev = Bd_hd.target_elev;

    else
       *elev = 0;

    return (0);

/* End of RPG_get_current_target_elev */
}

/*********************************************************************

   Description:
       This function returns the elevation angle (in degrees) given
       the elevation angle in BAMS.

   Inputs:
       elev - pointer to int storing elevation angle in BAMS.
       elev_angle_deg - pointer to int to receive elevation angle,
                        in degrees.

   Returns:
       There is no return value currently defined for this function.
       Always returns 0.

*********************************************************************/
fint RPG_elev_angle_BAMS_to_deg( unsigned short *elev, float *elev_angle_deg ){

   *elev_angle_deg = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, *elev );

   return(0);

} /* End of RPG_elev_angle_BAMS_to_deg() */

/***************************************************************
   Description:
      Returns the current volume scan sequence number.

   Inputs:
      bufptr - pointer to algorithm input buffer.

   Outputs: 
      ui_vol_num - positive volume scan sequence number on 
                   success, 0 on error.
   
***************************************************************/
void RPG_get_buffer_vol_seq_num( int *bufptr, 
                                 unsigned int *ui_vol_num ){

   int ret, ind = 0, num = 0;
      
   *ui_vol_num = 0;

   ret = Find_input_buffer( *bufptr, &ind, &num );
   if( ret >= 0 ){ 

      /* Find out what type of input buffer this is.  If radial
         type message, then volume number is stored in radial
         header.  If other type, then volume number is part of
         orpg product header. */
      if( Inp_list[ind].timing == RADIAL_DATA ){

         Base_data_header *bd_hdr =
            (Base_data_header *) Inp_list[ind].buf[num];
         *ui_vol_num = ORPGMISC_vol_seq_num( (int) bd_hdr->vol_num_quotient,
                                             (int) bd_hdr->volume_scan_num );

      }
      else {
      
         Prod_header *phd = (Prod_header *) Inp_list[ind].buf[num];
         *ui_vol_num = (unsigned int) phd->g.vol_num;

      }

   }

/* RPG_get_buffer_vol_seq_num() */
}

/***************************************************************
   Description:
      Returns the current volume scan number (modulo 80).

   Inputs:
      bufptr - pointer to algorithm input buffer.

   Outputs: 
      vol_num - volume scan number on success or -1 on error.

***************************************************************/
void RPG_get_buffer_vol_num( int *bufptr, int *vol_num ){

   unsigned int ui_vol_num;

   *vol_num = -1;

   RPG_get_buffer_vol_seq_num( bufptr, &ui_vol_num );
   if( ui_vol_num == 0xffffffff )
      return;

   /* Verify the volume scan number is within acceptable bounds. */
   *vol_num = ORPGMISC_vol_scan_num( ui_vol_num );

/* RPG_get_buffer_vol_num() */
}

/***************************************************************
   Description:
      Returns the buffer volume coverage pattern.

   Inputs:
      bufptr - pointer to algorithm input buffer.

   Outputs: 
      vcp_num - volume scan number on success, or -1 on error.

***************************************************************/
void RPG_get_buffer_vcp_num( int *bufptr, int *vcp_num ){

   int ret, ind = 0, num = 0;

   *vcp_num = -1;

   ret = Find_input_buffer( *bufptr, &ind, &num );
   if( ret >= 0 ){

      /* Find out what type of input buffer this is.  If radial
         type message, then volume number is stored in radial
         header.  If other type, then volume number is part of
         orpg product header. */
      if( Inp_list[ind].timing == RADIAL_DATA ){

         Base_data_header *bd_hdr = 
            (Base_data_header *) Inp_list[ind].buf[num];
         *vcp_num = bd_hdr->vcp_num;

      }
      else {

         Prod_header *phd = (Prod_header *) Inp_list[ind].buf[num];
         *vcp_num = phd->vcp_num;

      }

   }

   /* Verify the volume scan number is within acceptable bounds. */
   if( (*vcp_num <= 0) || (*vcp_num > MAX_VCP_NUMBER) )
      *vcp_num = -1;

/* RPG_get_buffer_vcp_num() */
}

/***************************************************************
   Description:
      Returns the current RPG elevation index.

   Inputs:
      bufptr - pointer to algorithm input buffer.

   Outputs: 
      Buffer elevation index on success or -1 on error.

***************************************************************/
void RPG_get_buffer_elev_index( int *bufptr, int *elev_index ){

   int ret, ind = 0, num = 0;

   *elev_index = -1;

   ret = Find_input_buffer( *bufptr, &ind, &num );
   if( ret >= 0 ){

      /* Find out what type of input buffer this is.  If radial
         type message, then volume number is stored in radial
         header.  If other type, then volume number is part of
         orpg product header. */
      if( Inp_list[ind].timing == RADIAL_DATA ){

         Base_data_header *bd_hdr = 
            (Base_data_header *) Inp_list[ind].buf[num];
         *elev_index = bd_hdr->rpg_elev_ind;

      }
      else {

         Prod_header *phd = (Prod_header *) Inp_list[ind].buf[num];
         *elev_index = phd->g.elev_ind;

      }

   }

   /* Verify the elevation index is within acceptable bounds. */
   if( (*elev_index < 0) || (*elev_index > MAX_ELEVATION_CUTS) )
      *elev_index = -1;

/* RPG_get_buffer_elev_index() */
}

/***************************************************************
   Description:
      Returns the RPG elevation index of the buffer and whether 
      or not the buffer is from the last elevation index.

   Inputs:
      bufptr - pointer to algorithm input buffer.

   Outputs:
      elev_index - RPG elevation index on success, -1 on error.
      last_elev_index = 1 if buffer from last elevation index, 
                        0 if not from last elevation index, 
                        or negative error code on error. 

***************************************************************/
void RPG_is_buffer_from_last_elev( int *bufptr, int *elev_index,
                                   int *last_elev_index ){

   int vcp_num;

   *last_elev_index = *elev_index = -1;

   /* Get the VCP number for the buffer. */
   RPG_get_buffer_vcp_num( bufptr, &vcp_num );
   if( vcp_num < 0 ) 
      return;

   /* Get the RPG elevation index for the buffer. */
   RPG_get_buffer_elev_index( bufptr, elev_index );

   /* Get the last RPG elevation index for the VCP. */
   *last_elev_index = Get_last_elev_index( vcp_num );
   if( *last_elev_index < 0 )
      return;

   /* Test if last RPG elevation index. */
   if( *elev_index == *last_elev_index )
      *last_elev_index = 1;

   else
      *last_elev_index = 0;

} /* RPG_is_buffer_from_last_elev() */

/****************************************************************

   Description:
      Given the starting address within the product buffer,
      set the MSW and LSW halfwords (i.e., shorts) for
      data value "value".

   Input:
      loc - starting address within product buffer
      value - address holding data value.

   Output:
      loc - stores the MSW halfword of "value" at
            (unsigned short *) loc and the LSW halfword of
            "value" at ((unsigned short *) loc) + 1.

   Returns:
      Always returns 0.

   Notes:

****************************************************************/
int RPG_set_product_int( void *loc, void *value ){

   return( ORPGMISC_pack_ushorts_with_value( loc, value ));

/* End of RPG_set_product_int() */
}

/**************************************************************

   Description:
      Return in "value", an integer or unsigned integer value
      which is packed in 2 unsigned shorts.

   Inputs:
      loc - address to the MSW of the value in product
      value - address to receive the unpacked value.

   Outputs:
      value - address where the unpacked value is stored.

   Returns:
      Always returns 0.

**************************************************************/
int RPG_get_product_int( void *loc, void *value ){

   return( ORPGMISC_unpack_value_from_ushorts( loc, value ));

/* End of RPG_get_product_int() */
}

/****************************************************************

   Description:
      Given the starting address within the product buffer,
      set the MSW and LSW halfwords (i.e., shorts) for
      data value "value".

   Input:
      loc - starting address within product buffer
      value - address holding data value.

   Output:
      loc - stores the MSW halfword of "value" at
            (unsigned short *) loc and the LSW halfword of
            "value" at ((unsigned short *) loc) + 1.

   Returns:
      Always returns 0.

   Notes:

****************************************************************/
int RPG_set_product_float( void *loc, void *value ){

   return( ORPGMISC_pack_ushorts_with_value( loc, value ) );

/* End of RPG_set_product_float() */
}

/**************************************************************

   Description:
      Return in "value", a float value which is packed in
      2 unsigned shorts.  The value is assumed in IEEE 754
      format.

   Inputs:
      loc - address to the MSW of the value in product
      value - address to receive the unpacked value.

   Outputs:
      value - address where the unpacked value is stored.

   Returns:
      Always returns 0.

**************************************************************/
int RPG_get_product_float( void *loc, void *value ){

   return (ORPGMISC_unpack_value_from_ushorts( loc, value ) );

/* End of RPG_get_product_float() */
}

/**************************************************************

   Description:
      Takes the Most Significant Short Word starting at
      address "loc" and packs into an unsigned integer.

   Inputs:
      loc - address to the MSW of the value to pack.

   Outputs:
      value - address where the MSW of loc is stored.

   Returns:
      Always returns 0.

**************************************************************/
int RPG_set_mssw_to_uint( void *loc, unsigned int *value ){

   unsigned int uint_value = *((unsigned int *) loc);

   *value = (unsigned int) (uint_value >> 16) & 0xffff;

   return 0;

/* End of RPG_set_mssw_to_uint() */
}

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
int RPG_set_veldeal_bit( fint2 *field_value ){

   /* Set the velocity dealiasing bit in radial header. */
   *field_value |= VEL_DEALIASED_BIT;

   return (0);

/* End of RPG_set_veldeal_bit */
}

/****************************************************************
   Description:
      Determines which moments are enabled.

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
void RPG_what_moments( Base_data_header *hdr, int *ref_flag,
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

/* End of RPG_what_moments( ) */
}

/*********************************************************************

      Description:
        This function returns the Nearest Integer of a floating point
        number.

      Inputs:
        r - a floating number.

      Outputs:
        temp - the Nearest Integer to r, the floating point

      Returns:
        There is no return value defined for this function.

**********************************************************************/
void RPG_NINT( float *r, int *temp ){

#ifdef LINUX
   float result;

   result = roundf( *r );
   *temp = (int) result;
   return;
#else
    if ( *r >= 0.0 )
        *temp = (int)(*r + .5);
    else
        *temp = -(int)((-(*r)) + .5);

    return;
#endif

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

   int num_elevs, ind, use_local = 0, i;
   double elev_angle_d;

   int lvcp_num = *vcp_num;
   int lelev_ind = *elev_ind;

   /* Initialize the elevation angle to -999. */
   *elev_angle = -999;
   *found = 0;

   /* If vcp number is less than 0, use local VCP definition.
      That is, use ORPGVCP functions directly.  This assumes
      elevation index is relative to the RPG definition of the 
      VCP. */
   if( lvcp_num < 0 ){

      use_local = 1;
      lvcp_num = -lvcp_num;

   }

   /* Find the index into the VCP table corresponding to
      vcp_num. */
   if( (ind = ORPGVCP_index( lvcp_num )) < 0 )
      return -1;
 
   /* Index of VCP has been found... now find the elevation angle
      initialize elevation index to zero. */
   if( use_local )
      num_elevs = ORPGVCP_get_num_elevations( lvcp_num );
   else
      num_elevs = VI_get_num_elevations( lvcp_num );

   if( (lelev_ind < 0) || (lelev_ind > num_elevs) ){

      LE_send_msg( GL_ERROR, "Elev Index: %d < 0 or > %d for VCP %d\n", 
                   lelev_ind, num_elevs, lvcp_num );
      return -1;

   }

   /* Get target elevation for the requested elevation index . */
   for( i = 0; i < num_elevs; i++ ){

      /* Pass in the RDA elevation index, i, and get the RPG elevation
         index, ind. */
      if( use_local )
         ind = ORPGVCP_get_rpg_elevation_num( lvcp_num, i );
      else
         ind = VI_get_rpg_elevation_num( lvcp_num, i );

      if( ind == lelev_ind ){

         if( use_local )
            elev_angle_d = ORPGVCP_get_elevation_angle( lvcp_num, i );
         else 
            elev_angle_d = VI_get_elevation_angle( lvcp_num, i );

         elev_angle_d *= 10.0;
         Nintd( &elev_angle_d, elev_angle );
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

/********************************************************************

    Description: 
        This function sends a log message to the LE module
	if the process runs in additional message mode.

    Input:
	format - message format string;
	... - list of variables whose values are to be printed.

    Returns:
        There is no return value defined for this function.

********************************************************************/
void PS_message (char *format, ... ){

    char buf [MAX_MSG_SIZE];
    va_list args;

    if (!Msg_mode)
	return;

    if (format != NULL && *format != '\0') {
	va_start (args, format);
	vsprintf (buf, format, args);
	va_end (args);
    }
    else
	buf [0] = '\0';

    LE_send_msg (GL_INFO, buf);

    return;

/* End of PS_message */
}


/********************************************************************

    Description: 
        This function is called if the process is to run in
	additional message mode.

    Returns:
        There is no return value define for this function.

********************************************************************/
void PS_message_mode (){

    Msg_mode = 1;
    return;

/* End of PS_message_mode */
}

/********************************************************************

    Description: 
        Returns whether process is to run in additional message mode.

    Returns:
        Msg_mode

********************************************************************/
int PS_in_message_mode (){

    return (Msg_mode);

/* End of PS_in_message_mode */
}

/***************************************************************
   Description:
      Returns the index of the last elevation angle in the
      supplied VCP.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.

   Returns:
      Negative error code on error or RPG elevation index.

***************************************************************/
static int Get_last_elev_index( int vcp_num ){

   int num_elevs;

   /* Index of VCP has been found... now get the number of
      elevation scans for this vcp. */
   num_elevs = VI_get_num_elevations( vcp_num );
   if( (num_elevs <= 0) || (num_elevs > ECUTMAX) ){

      LE_send_msg( GL_ERROR, "Number of Elevs Incorrect In VCP %d: %d\n",
                   vcp_num, num_elevs );
      return -1;

   }

   /* Get last elevation index for this VCP. */
   return( (int) VI_get_rpg_elevation_num( vcp_num, num_elevs-1 ) );

/* End of Get_last_elev_index() */
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
static int Find_input_buffer( int bufptr, int *ind, int *num ){

   static int initialized = FALSE;

   /*
    Get address of input list if not already done so.
   */
   if( initialized == FALSE ){

      N_inps = IB_inp_list( &Inp_list );
      initialized = TRUE;

   }

   /*
     Search the input list for match on buffer pointer.
   */
   for( *ind = 0; *ind < N_inps; (*ind)++ ){

      for( *num = 0; *num <= Inp_list[*ind].buffer_count; (*num)++ ){

         /* For non-radial type inputs, must account for product header.
            The buffer pointer returned to the application is the malloced
            address plus the size of the product header. */
         if( bufptr == Inp_list[*ind].bufptr[*num] )
            return( 0 );

      }

   }

   /*
     If no match found, return -1.
   */
   return(-1);

}

/*********************************************************************

      Description:
        This function returns the Nearest Integer of a double precision
        floating point number.

      Inputs:
        r - a double precision floating number.

      Outputs:
        temp - the Nearest Integer to r, the floating point

      Returns:
        There is no return value defined for this function.

**********************************************************************/
static void Nintd( double *r, int *temp ){

#ifdef LINUX
   double result;

   result = round( *r );
   *temp = (int) result;
   return;
#else
    if ( *r >= 0.0 )
        *temp = (int)(*r + .5);
    else
        *temp = -(int)((-(*r)) + .5);

    return;
#endif

}

