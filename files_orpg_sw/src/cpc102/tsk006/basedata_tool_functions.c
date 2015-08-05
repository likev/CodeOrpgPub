/************************************************************************
 *									*
 *	Module:  basedata_tool_functions.c				*
 *									*
 *	Description:  This module contains a collection of functions	*
 *		      to manipulate the internal basedata message for	*
 *		      the Base Data Display Tool.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/03/21 17:03:31 $
 * $Id: basedata_tool_functions.c,v 1.1 2011/03/21 17:03:31 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci.h>
#include <basedata_tool.h>

#define LOWER_THRESHOLD_INDEX		0
#define UPPER_THRESHOLD_INDEX		1
#define NUM_8BIT_ELEMENTS		256
#define NUM_16BIT_ELEMENTS		65536
#define INDEX_OF_FIRST_DATA		2
#define INDEX_OF_LAST_8BIT_DATA		NUM_8BIT_ELEMENTS-1
#define INDEX_OF_LAST_16BIT_DATA	NUM_16BIT_ELEMENTS-1
#define IS_8BIT_DATA			8
#define IS_16BIT_DATA			16
#define THRESH_ELEMENTS			16

/* Static variables */

static char Data[SIZEOF_BASEDATA]; /* Radial data storage (1 beam) */

static int Init_flag  = 0; /* Initialization flag */
static int Lock_state = 0;
static int  Data_feed = SR_BASEDATA; /* LB to use for basedata */

static float Reflectivity_8bit_LUT[NUM_8BIT_ELEMENTS];
static float Reflectivity_8bit_Thresh[THRESH_ELEMENTS];
static float Prev_reflectivity_8bit_scale = 2.0;
static float Prev_reflectivity_8bit_offset = 66.0;
static float Velocity_low_8bit_LUT[NUM_8BIT_ELEMENTS];
static float Velocity_low_8bit_Thresh[THRESH_ELEMENTS];
static float Prev_velocity_low_8bit_scale = 1.0;
static float Prev_velocity_low_8bit_offset = 129.0;
static float Velocity_high_8bit_LUT[NUM_8BIT_ELEMENTS];
static float Velocity_high_8bit_Thresh[THRESH_ELEMENTS];
static float Prev_velocity_high_8bit_scale = 2.0;
static float Prev_velocity_high_8bit_offset = 129.0;
static float Spectrum_width_8bit_LUT[NUM_8BIT_ELEMENTS];
static float Spectrum_width_8bit_Thresh[THRESH_ELEMENTS];
static float Prev_spectrum_width_8bit_scale = 2.0;
static float Prev_spectrum_width_8bit_offset = 129.0;
static float Diff_reflectivity_8bit_LUT[NUM_8BIT_ELEMENTS];
static float Diff_reflectivity_8bit_Thresh[THRESH_ELEMENTS];
static float Prev_diff_reflectivity_8bit_scale = 16.0;
static float Prev_diff_reflectivity_8bit_offset = 128.0;
static float Diff_phase_16bit_LUT[NUM_16BIT_ELEMENTS];
static float Diff_phase_16bit_Thresh[THRESH_ELEMENTS];
static float Prev_diff_phase_16bit_scale = 2.0;
static float Prev_diff_phase_16bit_offset = 2.8361;
static float Diff_correlation_8bit_LUT[NUM_8BIT_ELEMENTS];
static float Diff_correlation_8bit_Thresh[THRESH_ELEMENTS];
static float Prev_diff_correlation_8bit_scale = 300.0;
static float Prev_diff_correlation_8bit_offset = -60.5;

static float Reflectivity_range[MAX_BINS_ALONG_RADIAL];
static float Prev_reflectivity_first_range = 500.0;
static float Prev_reflectivity_bin_size = 250.0;
static float Velocity_range[MAX_BINS_ALONG_RADIAL];
static float Prev_velocity_first_range = 500.0;
static float Prev_velocity_bin_size = 250.0;
static float Spectrum_width_range[MAX_BINS_ALONG_RADIAL];
static float Prev_spectrum_width_first_range = 500.0;
static float Prev_spectrum_width_bin_size = 250.0;
static float Diff_reflectivity_range[MAX_BINS_ALONG_RADIAL];
static float Prev_diff_reflectivity_first_range = 500.0;
static float Prev_diff_reflectivity_bin_size = 250.0;
static float Diff_phase_range[MAX_BINS_ALONG_RADIAL];
static float Prev_diff_phase_first_range = 500.0;
static float Prev_diff_phase_bin_size = 250.0;
static float Diff_correlation_range[MAX_BINS_ALONG_RADIAL];
static float Prev_diff_correlation_first_range = 500.0;
static float Prev_diff_correlation_bin_size = 250.0;

static Base_data_header *Hdr = NULL;
static Generic_moment_t *Diff_reflectivity_hdr = NULL;
static Generic_moment_t *Diff_phase_hdr = NULL;
static Generic_moment_t *Diff_correlation_hdr = NULL;

static void *Reflectivity_ptr = NULL;
static void *Velocity_ptr = NULL;
static void *Spectrum_width_ptr = NULL;
static void *Diff_reflectivity_ptr = NULL;
static void *Diff_phase_ptr = NULL;
static void *Diff_correlation_ptr = NULL;

void hci_basedata_tool_init();
void build_reflectivity_8bit_LUT( float n_offset, float n_range );
void build_reflectivity_16bit_LUT( float n_offset, float n_range );
void build_velocity_high_8bit_LUT( float n_offset, float n_range );
void build_velocity_high_16bit_LUT( float n_offset, float n_range );
void build_velocity_low_8bit_LUT( float n_offset, float n_range );
void build_velocity_low_16bit_LUT( float n_offset, float n_range );
void build_spectrum_width_8bit_LUT( float n_offset, float n_range );
void build_spectrum_width_16bit_LUT( float n_offset, float n_range );
void build_diff_reflectivity_8bit_LUT( float n_offset, float n_range );
void build_diff_phase_8bit_LUT( float n_offset, float n_range );
void build_diff_phase_16bit_LUT( float n_offset, float n_range );
void build_diff_correlation_8bit_LUT( float n_offset, float n_range );
void build_reflectivity_range( float n_first_range, float n_bin_size );
void build_velocity_range( float n_first_range, float n_bin_size );
void build_spectrum_width_range( float n_first_range, float n_bin_size );
void build_diff_reflectivity_range( float n_first_range, float n_bin_size );
void build_diff_phase_range( float n_first_range, float n_bin_size );
void build_diff_correlation_range( float n_first_range, float n_bin_size );
void reset_radial();
void print_bdh();
void print_gdh();

/************************************************************************
 *	Description: Initialize the base data data source.		*
 *		     Assumes HCI_read_options has been called		*
 *									*
 *	Input:  NONE							*
 *	Return: NONE							*
 *									*
 ************************************************************************/

void hci_basedata_tool_init()
{
  /* Set flag. */

  Init_flag = 1;

  /* Initialize the data lookup tables for non-generic moment
     data, since it is assumed to never change. For generic moment
     data, call appropriate function. */

  build_reflectivity_8bit_LUT( Prev_reflectivity_8bit_offset,
                               Prev_reflectivity_8bit_scale );
  build_velocity_high_8bit_LUT( Prev_velocity_high_8bit_offset,
                               Prev_velocity_high_8bit_scale );
  build_velocity_low_8bit_LUT( Prev_velocity_low_8bit_offset,
                               Prev_velocity_low_8bit_scale );
  build_spectrum_width_8bit_LUT( Prev_spectrum_width_8bit_offset,
                               Prev_spectrum_width_8bit_scale );
  build_diff_reflectivity_8bit_LUT( Prev_diff_reflectivity_8bit_offset,
                               Prev_diff_reflectivity_8bit_scale );
  build_diff_phase_16bit_LUT( Prev_diff_phase_16bit_offset,
                               Prev_diff_phase_16bit_scale );
  build_diff_correlation_8bit_LUT( Prev_diff_correlation_8bit_offset,
                               Prev_diff_correlation_8bit_scale );

  /* Build range lookup tables. */

  build_reflectivity_range( Prev_reflectivity_first_range,
                               Prev_reflectivity_bin_size );
  build_velocity_range( Prev_velocity_first_range,
                               Prev_velocity_bin_size );
  build_spectrum_width_range( Prev_spectrum_width_first_range,
                               Prev_spectrum_width_bin_size );
  build_diff_reflectivity_range( Prev_diff_reflectivity_first_range,
                               Prev_diff_reflectivity_bin_size );
  build_diff_phase_range( Prev_diff_phase_first_range,
                               Prev_diff_phase_bin_size );
  build_diff_correlation_range( Prev_diff_correlation_first_range,
                               Prev_diff_correlation_bin_size );

  /* Initialize the entire data area to zeroes. */

  memset((void*)(&Data[0]), 0, SIZEOF_BASEDATA);

  /* Initialize pointer to basedata. */

  Hdr = (Base_data_header *) Data;
}


/************************************************************************

    Description: The following routine gets the current basedata file
                 descriptor.

    Return:      Basedata file descriptor
 
 ************************************************************************/

int hci_basedata_tool_id()
{
  if( !Init_flag ){ hci_basedata_tool_init(); }
  return ORPGDA_lbfd( Data_feed );
}


/************************************************************************

    Description: The following routine returns the message id of the 
                 previously read message.

    Return:      ID of previously read message.

 ************************************************************************/

int hci_basedata_tool_msgid()
{
  if( !Init_flag ){ hci_basedata_tool_init(); }
  return LB_previous_msgid( hci_basedata_tool_id() );
}


/************************************************************************

    Description: The following routine moves the LB message pointer to 
                 the specified message.

     Input:      msgid - The message to move the pointer to.

    Return:      status - the results of LB_seek(either LB_SUCCESS or an
                          error code. Refer lb.h for error codes.)

 ************************************************************************/

int hci_basedata_tool_seek( int msgid )
{
  if( !Init_flag ){ hci_basedata_tool_init(); }
  return LB_seek( hci_basedata_tool_id(), 0, msgid, NULL );
}

/************************************************************************
 *	Description: This function reads the next radial in the 	*
 *		     basedata LB.					*
 *									*
 *	Input:  msgid - id of message to read				*
 *		partial_read - flag to control whether entire message	*
 *			       is read or just the header.		*
 *	Return: returns the message length on success(status > 0),	*
 *		or error code (refer to lb.h) for status <= 0.		*
 ************************************************************************/

int hci_basedata_tool_read_radial( int msg_id, int partial_read )
{
  int  status;
  char data[SIZEOF_BASEDATA];
  int  start_bytes_to_read = 0;
  int  no_of_bytes_to_read = sizeof(Base_data_header);  

  /* Initialize internal arrays, etc if not done already. */

  if( !Init_flag ){ hci_basedata_tool_init(); }

  /* If the input message ID contains the macro for the latest
     message, then move the file pointer to the latest message. */

  if( msg_id == LB_LATEST )
  {
    status = hci_basedata_tool_seek(LB_LATEST);
    msg_id = LB_ANY;
  }
  else
  {
    status = 0;
  }

  if( status >= 0 )
  {
    /* If a partial read is specified, read the radial header only. */

    if( partial_read == HCI_BASEDATA_PARTIAL_READ )
    {
      status = LB_read_window( hci_basedata_tool_id(),
                               start_bytes_to_read,
                               no_of_bytes_to_read );

      if( status >= 0 )
      {
        status = ORPGDA_read( Data_feed, data, no_of_bytes_to_read, msg_id );
      }
    }
    else
    {
      /* Else, read the entire radial message. */
      status = ORPGDA_read( Data_feed,
                            (char *) data,
                            SIZEOF_BASEDATA,
                            msg_id );
    }
  }

  /* If good data read, copy to static storage and reset radial info. */

  if( status > 0 )
  {
    memcpy( &Data[ 0 ], &data[ 0 ], status );
    reset_radial();
  }

  return status;
}

/************************************************************************
 *	Description: This function resets pointers, range values, etc.	*
 *		     for a new radial of data.				*
 *									*
 *	Input:  NONE							*
 *	Return: NONE							*
 ************************************************************************/

void reset_radial()
{
  int i;
  Generic_moment_t *ghdr = NULL;
  char moment_name[ 5 ];

  Hdr = (Base_data_header *) Data;

  /* Check reflectivity. It is assumed to always be 16-bit data. */

  Reflectivity_ptr = &Data[Hdr->ref_offset];

  if( (float)Hdr->range_beg_surv != Prev_reflectivity_first_range ||
      (float)Hdr->surv_bin_size != Prev_reflectivity_bin_size )
  {
    build_reflectivity_range( (float)Hdr->range_beg_surv,
                              (float)Hdr->surv_bin_size );
  }

  /* Check velocity. It is assumed to always be 16-bit data. */

  Velocity_ptr = &Data[Hdr->vel_offset];

  if( (float)Hdr->range_beg_dop != Prev_velocity_first_range ||
      (float)Hdr->dop_bin_size != Prev_velocity_bin_size )
  {
    build_velocity_range( (float)Hdr->range_beg_dop,
                         (float)Hdr->dop_bin_size );
  }
  
  /* Check spectrum width. It is assumed to always be 16-bit data. */

  Spectrum_width_ptr = &Data[Hdr->spw_offset];

  if( (float)Hdr->range_beg_dop != Prev_spectrum_width_first_range ||
      (float)Hdr->dop_bin_size != Prev_spectrum_width_bin_size )
  {
    build_spectrum_width_range( (float)Hdr->range_beg_dop,
                                (float)Hdr->dop_bin_size );
  }

  /* Check dual-pol. */

  if( Hdr->msg_type & DUALPOL_TYPE )
  {
    Diff_reflectivity_hdr = NULL;
    Diff_reflectivity_ptr = NULL;
    Diff_phase_hdr = NULL;
    Diff_phase_ptr = NULL;
    Diff_correlation_hdr = NULL;
    Diff_correlation_ptr = NULL;

    for( i = 0; i < Hdr->no_moments; i++ )
    {
      ghdr = (Generic_moment_t *) &Data[ Hdr->offsets[ i ] ];
      memcpy( &moment_name[ 0 ], &(ghdr->name[0]), 4 );
      moment_name[ 4 ] = '\0';
      if( strcmp( moment_name, "DZDR" ) == 0 )
      {
        Diff_reflectivity_hdr = (Generic_moment_t *) &Data[Hdr->offsets[i]];

        if( ghdr->data_word_size == IS_8BIT_DATA )
        {
          Diff_reflectivity_ptr = &ghdr->gate;
          if( ghdr->offset != Prev_diff_reflectivity_8bit_offset ||
              ghdr->scale != Prev_diff_reflectivity_8bit_scale )
          {
            build_diff_reflectivity_8bit_LUT( ghdr->offset, ghdr->scale );
          }
        }
        else
        {
          Diff_reflectivity_ptr = &ghdr->gate;
        }
        if( (float)ghdr->first_gate_range != Prev_diff_reflectivity_first_range ||
            (float)ghdr->bin_size != Prev_diff_reflectivity_bin_size )
        {
          build_diff_reflectivity_range( (float)ghdr->first_gate_range,
                                         (float)ghdr->bin_size );
        }
      }
      else if( strcmp( moment_name, "DPHI" ) == 0 )
      {

        Diff_phase_hdr = (Generic_moment_t *) &Data[Hdr->offsets[i]];

        Diff_phase_ptr = &ghdr->gate;
        if( ghdr->offset != Prev_diff_phase_16bit_offset ||
            ghdr->scale != Prev_diff_phase_16bit_scale )
        {
          build_diff_phase_16bit_LUT( ghdr->offset, ghdr->scale );
        }
        if( (float)ghdr->first_gate_range != Prev_diff_phase_first_range ||
            (float)ghdr->bin_size != Prev_diff_phase_bin_size )
        {
          build_diff_phase_range( (float)ghdr->first_gate_range,
                                  (float)ghdr->bin_size );
        }
      }
      else if( strcmp( moment_name, "DRHO" ) == 0 )
      {
        Diff_correlation_hdr = (Generic_moment_t *) &Data[Hdr->offsets[i]];

        if( ghdr->data_word_size == IS_8BIT_DATA )
        {
          Diff_correlation_ptr = &ghdr->gate;
          if( ghdr->offset != Prev_diff_correlation_8bit_offset ||
              ghdr->scale != Prev_diff_correlation_8bit_scale )
          {
            build_diff_correlation_8bit_LUT( ghdr->offset, ghdr->scale );
          }
        }
        else
        {
          Diff_correlation_ptr = &ghdr->gate;
        }
        if( (float)ghdr->first_gate_range != Prev_diff_correlation_first_range ||
            (float)ghdr->bin_size != Prev_diff_correlation_bin_size )
        {
          build_diff_correlation_range( (float)ghdr->first_gate_range,
                                        (float)ghdr->bin_size );
        }
      }
    }
  }
}

/************************************************************************
 *	Description: This function returns the range (in meters) to 	*
 *		     the first gate of the selected moment.		*
 *									*
 *	Input:  moment - moment identifier				*
 *	Return: range (meters) to the first gate			*
 ************************************************************************/

int hci_basedata_tool_range_adjust( int moment )
{
  int range = 0;

  /* Get the range adjust data from the header for the specified moment. */

  switch( moment )
  {
    case VELOCITY :
      range = Hdr->range_beg_dop;
      break;

    case SPECTRUM_WIDTH :
      range = Hdr->range_beg_dop;
      break;

    case DIFF_REFLECTIVITY :
      range = Diff_reflectivity_hdr->first_gate_range;
      break;

    case DIFF_PHASE :
      range = Diff_phase_hdr->first_gate_range;
      break;

    case DIFF_CORRELATION :
      range = Diff_correlation_hdr->first_gate_range;
      break;

    case REFLECTIVITY :
    default :
      range = Hdr->range_beg_surv;
      break;
  }

  return range;
}

/************************************************************************
 *	Description: This function returns the gate size (in meters) of	*
 *		     the selected moment.				*
 *									*
 *	Input:  moment - moment identifier				*
 *	Return: Gate size (meters)					*
 ************************************************************************/

int hci_basedata_tool_bin_size( int moment )
{
  int size = 0;

  /* Get the gate size data from the header for the specified moment. */

  switch( moment )
  {
    case VELOCITY :
      size = Hdr->dop_bin_size;
      break;

    case SPECTRUM_WIDTH :
      size = Hdr->dop_bin_size;
      break;

    case DIFF_REFLECTIVITY :
      size = Diff_reflectivity_hdr->bin_size;
      break;

    case DIFF_PHASE :
      size = Diff_phase_hdr->bin_size;
      break;

    case DIFF_CORRELATION :
      size = Diff_correlation_hdr->bin_size;
      break;

    case REFLECTIVITY :
    default :
      size = Hdr->surv_bin_size;
      break;
  }

  return size;
}

/************************************************************************
 *	Description: This function returns the number of gates defined	*
 *		     in the radial for the selected moment.		*
 *									*
 *	Input:  moment - moment identifier				*
 *	Return: Number of gates in radial				*
 ************************************************************************/

int hci_basedata_tool_number_bins( int moment )
{
  /* Get the number of gates from the header for the specified moment. */

  switch( moment )
  {
    case VELOCITY :
      return Hdr->n_dop_bins;
      break;

    case SPECTRUM_WIDTH :
      return Hdr->n_dop_bins;
      break;

    case DIFF_REFLECTIVITY :
      return Diff_reflectivity_hdr->no_of_gates;
      break;

    case DIFF_PHASE :
      return Diff_phase_hdr->no_of_gates;
      break;

    case DIFF_CORRELATION :
      return Diff_correlation_hdr->no_of_gates;
      break;

    case REFLECTIVITY :
    default :
      return Hdr->n_surv_bins;
      break;
  }

  return 0;
}

/************************************************************************
 *	Description: This function returns the time (in milliseconds	*
 *		     past midnight) of the radial.			*
 *									*
 *	Input:  NONE							*
 *	Return: time radial was generated				*
 ************************************************************************/

int hci_basedata_tool_time()
{
  return Hdr->time;
}

/************************************************************************
 *	Description: This function returns the date (Modified Julian	*
 *		     of the radial.					*
 *									*
 *	Input:  NONE							*
 *	Return: date radial was generated				*
 ************************************************************************/

int hci_basedata_tool_date()
{
  return Hdr->date;
}

/************************************************************************
 *	Description: This function returns the unambiguous range (km).	*
 *									*
 *	Input:  NONE							*
 *	Return: the unambiguous range					*
 ************************************************************************/

float hci_basedata_tool_unambiguous_range()
{
  return (float) (Hdr->unamb_range/10.0);
}

/************************************************************************
 *	Description: This function returns the Nyquist velocity (m/s).	*
 *									*
 *	Input:  NONE							*
 *	Return: the Nyquist velocity					*
 ************************************************************************/

float hci_basedata_tool_nyquist_velocity()
{
  return (float) (Hdr->nyquist_vel/100.0);
}

/************************************************************************
 *	Description: This function returns the elevation angle (deg)	*
 *		     of the current radial.				*
 *									*
 *	Input:  NONE							*
 *	Return: the antenna elevation angle.				*
 ************************************************************************/

float hci_basedata_tool_elevation()
{
  return Hdr->elevation;
}

/************************************************************************
 *	Description: This function returns the target elevation angle	*
 *		     (in degrees) of the current radial.		*
 *									*
 *	Input:  NONE							*
 *	Return: the target antenna elevation angle.			*
 ************************************************************************/

float hci_basedata_tool_target_elevation()
{
  return ( Hdr->target_elev / 10.0 ); 
}

/************************************************************************
 *	Description: This function returns the azimuth number of the	*
 *		     current radial.					*
 *									*
 *	Input:  NONE							*
 *	Return: the azimuth number.					*
 ************************************************************************/

int hci_basedata_tool_azimuth_number()
{
  return Hdr->azi_num;
}

/************************************************************************
 *	Description: This function returns the azimuth angle (deg)	*
 *		     of the current radial.				*
 *									*
 *	Input:  NONE							*
 *	Return: the antenna azimuth angle.				*
 ************************************************************************/

float hci_basedata_tool_azimuth()
{
  return Hdr->azimuth;
}

/************************************************************************
 *	Description: This function returns the VCP associated with the	*
 *		     current radial.					*
 *									*
 *	Input:  NONE							*
 *	Return: VCP number.						*
 ************************************************************************/

int hci_basedata_tool_vcp_number()
{
  return Hdr->vcp_num;
}

/************************************************************************
 *	Description: This function returns the message type field from	*
 *		     the current radial.				*
 *									*
 *	Input:  NONE							*
 *	Return: message type.						*
 ************************************************************************/

int hci_basedata_tool_msg_type()
{
  return Hdr->msg_type;
}

/************************************************************************
 *      Description: This function returns the radial status of the     *
 *                   current radial.                                    *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Return: radial status.                                          *
 ************************************************************************/

int hci_basedata_tool_radial_status()
{
  return Hdr->status;
}

/************************************************************************
 *	Description: This function returns the elevation number from	*
 *		     the current radial.				*
 *									*
 *	Input:  NONE							*
 *	Return: elevation number.					*
 ************************************************************************/

int hci_basedata_tool_elevation_number()
{
  return Hdr->elev_num;
}

/************************************************************************
 *	Description: This function returns the velocity resolution of	*
 *		     the Velocity data in the current radial.		*
 *									*
 *	Input:  NONE							*
 *	Return: resolution code						*
 ************************************************************************/

int hci_basedata_tool_velocity_resolution()
{
  switch( Hdr->dop_resolution )
  {
    case 1 :
    default :
      return DOPPLER_RESOLUTION_HIGH;

    case 2 :
      return DOPPLER_RESOLUTION_LOW;
  }
}

/************************************************************************
 *	Description: This function returns the scaled value for a	*
 *		     specified unscale value and moment.	 	*
 *									*
 *	Input:  u_value - unscaled value				*
 *		moment - Moment ID					*
 *	Return: scaled value						*
 ************************************************************************/

float hci_basedata_tool_value( int u_value, int moment )
{
  float value = 0.0;

  switch( moment )
  {
    case SPECTRUM_WIDTH :
      {
        value = Spectrum_width_8bit_LUT[ u_value ];
      }
      break;

    case VELOCITY :
      switch( hci_basedata_tool_velocity_resolution() )
      {
        case DOPPLER_RESOLUTION_LOW :
          {
            value = Velocity_low_8bit_LUT[ u_value ];
          }
          break;

        default :
          {
            value = Velocity_high_8bit_LUT[ u_value ];
          }
          break;
      }
      break;

    case DIFF_REFLECTIVITY :
      {
        value = Diff_reflectivity_8bit_LUT[ u_value ];
      }
      break;

    case DIFF_PHASE :
      {
        value = Diff_phase_16bit_LUT[ u_value ];
      }
      break;

    case DIFF_CORRELATION :
      {
        value = Diff_correlation_8bit_LUT[ u_value ];
      }
      break;

    case REFLECTIVITY:
    default :
      {
        value = Reflectivity_8bit_LUT[ u_value ];
      }
      break;
  }

  return value;
}

/************************************************************************
 *	Description: This function returns the scaled value for a	*
 *		     specified gate and moment.			 	*
 *									*
 *	Input:  indx   - range gate index				*
 *		moment - Moment ID					*
 *	Return: unscaled value						*
 ************************************************************************/

float hci_basedata_tool_value_index( int indx, int moment )
{
  unsigned char *value_c = NULL;
  unsigned short *value_s = NULL;
  int value_index = 0;

  if( hci_basedata_tool_data_word_size( moment ) == IS_16BIT_DATA )
  {
    value_s = (unsigned short *)hci_basedata_tool_data_ptr( moment );
    if( value_s == NULL ){ value_index = LOWER_THRESHOLD_INDEX; }
    else{ value_index = value_s[ indx ]; }
  }
  else
  {
    value_c = (unsigned char *)hci_basedata_tool_data_ptr( moment );
    if( value_c == NULL ){ value_index = LOWER_THRESHOLD_INDEX; }
    else{ value_index = (int)value_c[ indx ]; }
  }

  return (hci_basedata_tool_data_table_ptr( moment ))[ value_index ];
}

/************************************************************************
 *      Description: This function returns the index of the specified	*
 *                   specified range and moment.			*
 *                                                                      *
 *      Input:  indx   - range gate index                               *
 *              moment - Moment ID                                      *
 *      Return: range to center of gate                                 *
 ************************************************************************/

int hci_basedata_tool_range_index( float range, int moment )
{
  int index = 0;
  float *range_array = NULL;
  int num_ranges = 0;
  int bin_size = 0;
  int i = 0;


  num_ranges = hci_basedata_tool_number_bins( moment );
  range_array = hci_basedata_tool_range_ptr( moment );
  bin_size = hci_basedata_tool_bin_size( moment );

  if( range < range_array[0] ||
      range >= (range_array[num_ranges-1]+bin_size) )
  {
    return -1;
  }
  else
  {
    for( i=1; i<num_ranges; i++ )
    {
      if( range_array[i] > range )
      {
        index = i-1;
        break;
      }
    }
  }

  return index;
}

/************************************************************************
 *	Description: This function returns the range (in km) of a	*
 *		     specified gate index  and moment.			*
 *									*
 *	Input:  indx   - range gate index				*
 *		moment - Moment ID					*
 *	Return: range to center of gate					*
 ************************************************************************/

float hci_basedata_tool_range( int indx, int moment )
{
  float range = 0.0;

  switch( moment )
  {
    case VELOCITY :
      range = Velocity_range[ indx ];
      break;

    case SPECTRUM_WIDTH :
      range = Spectrum_width_range[ indx ];
      break;

    case DIFF_REFLECTIVITY :
      range = Diff_reflectivity_range[ indx ];
      break;

    case DIFF_PHASE :
      range = Diff_phase_range[ indx ];
      break;

    case DIFF_CORRELATION :
      range = Diff_correlation_range[ indx ];
      break;

    case REFLECTIVITY :
    default :
      range = Reflectivity_range[indx];
      break;
  }

  return range;
}

/************************************************************************
 *	Description: This function returns a flag indicating whether 	*
 *		     another function has locked the basedata LB.  This	*
 *		     must be done in instances where the file pointer	*
 *		     is changed to look at a previous message and no	*
 *		     timer updates are allowed. It is assumed that	*
 *		     whenever a function is done, it will restore the	*
 *		     file pointer to its state before the lock and 	*
 *		     reset the flag.					*
 *									*
 *	Input:	NONE							*
 *	Return: The lock state						*
 ************************************************************************/

int hci_basedata_tool_get_lock_state()
{
  return Lock_state;
}

/************************************************************************
 *	Description: This function stes the basedata lock flag. Note: 	*
 *		     See hci_basedata_tool_get_lock_state description	*
 *		     for further information.				*
 *									*
 *	Input:	state - the new lock state				*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_tool_set_lock_state( int state )
{
  Lock_state = state;
}

/************************************************************************
 *	Description: This function changes the LB to use for basedata.	*
 *									*
 *	Input:	feed_type - New feed type to use for basedata		*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_tool_set_data_feed( int feed_type )
{
  Data_feed = feed_type;
}

/************************************************************************
 *      Description: This function builds the reflectivity lookup table *
 *                                                                      *
 *      Input:  n_offset - new offset                                   *
 *              n_scale - new scale                                     *
 *      Return: NONE                                                    *
 ************************************************************************/

void build_reflectivity_8bit_LUT( float n_offset, float n_scale )
{
  int i = 0, j;
  int start, end;

  Prev_reflectivity_8bit_offset = n_offset;
  Prev_reflectivity_8bit_scale = n_scale;

  Reflectivity_8bit_Thresh[0] = NO_LABEL_FLAG;
  Reflectivity_8bit_Thresh[1] = 5.0;
  Reflectivity_8bit_Thresh[2] = 10.0;
  Reflectivity_8bit_Thresh[3] = 15.0;
  Reflectivity_8bit_Thresh[4] = 20.0;
  Reflectivity_8bit_Thresh[5] = 25.0;
  Reflectivity_8bit_Thresh[6] = 30.0;
  Reflectivity_8bit_Thresh[7] = 35.0;
  Reflectivity_8bit_Thresh[8] = 40.0;
  Reflectivity_8bit_Thresh[9] = 45.0;
  Reflectivity_8bit_Thresh[10] = 50.0;
  Reflectivity_8bit_Thresh[11] = 55.0;
  Reflectivity_8bit_Thresh[12] = 60.0;
  Reflectivity_8bit_Thresh[13] = 65.0;
  Reflectivity_8bit_Thresh[14] = 70.0;
  Reflectivity_8bit_Thresh[15] = NO_LABEL_FLAG;

  /* Compute LUT based on threshold values. */
  start = 2;
  for( i = 1; i < PRODUCT_COLORS-1; i++ ){

     /* Get the threshold value. */
     float z = Reflectivity_8bit_Thresh[i];

     /* Convert it to ICD units.   if the value is the NO_LABEL_FLAG,
        set the end to the highest possible ICD value. */
     if( z == NO_LABEL_FLAG )
        end = 255;

     else
        end = (int) roundf( ((float) z + 32.0) * 2.0f ) + 2;

     for(j=start;j<end;j++)
        Reflectivity_8bit_LUT[j]=i;

     start = end;

  }

  Reflectivity_8bit_LUT[0]=0;
  Reflectivity_8bit_LUT[1]=15;

}

/************************************************************************
 *      Description: This function builds the velocity lookup table *
 *                                                                      *
 *      Input:  n_offset - new offset                                   *
 *              n_scale - new scale                                     *
 *      Return: NONE                                                    *
 ************************************************************************/

void build_velocity_high_8bit_LUT( float n_offset, float n_scale )
{
  int i = 0, j;
  int start, end;

  Prev_velocity_high_8bit_offset = n_offset;
  Prev_velocity_high_8bit_scale = n_scale;


  /* Same 16 data levels used by V products (Res 2). */
  Velocity_high_8bit_Thresh[0] = NO_LABEL_FLAG;
  Velocity_high_8bit_Thresh[1] = -64;
  Velocity_high_8bit_Thresh[2] = -50;
  Velocity_high_8bit_Thresh[3] = -36;
  Velocity_high_8bit_Thresh[4] = -26;
  Velocity_high_8bit_Thresh[5] = -20;
  Velocity_high_8bit_Thresh[6] = -10;
  Velocity_high_8bit_Thresh[7] = 0;
  Velocity_high_8bit_Thresh[8] = 10;
  Velocity_high_8bit_Thresh[9] = 20;
  Velocity_high_8bit_Thresh[10] = 26;
  Velocity_high_8bit_Thresh[11] = 36;
  Velocity_high_8bit_Thresh[12] = 50;
  Velocity_high_8bit_Thresh[13] = 64;
  Velocity_high_8bit_Thresh[14] = NO_LABEL_FLAG;
  Velocity_high_8bit_Thresh[15] = NO_LABEL_FLAG;
  
  /* Compute LUT based on threshold values. */
  start = 2;
  for( i = 1; i < PRODUCT_COLORS-1; i++ ){

     /* Get the threshold value. */
     float v = Velocity_high_8bit_Thresh[i];

     /* Convert it to ICD units.   if the value is the NO_LABEL_FLAG,
        set the end to the highest possible ICD value. */
     if( v == NO_LABEL_FLAG )
        end = 255;

     else
        end = (int) roundf( (((float) v / MPS_TO_KTS) + 63.5)*2.0f ) + 2;

     for(j=start;j<end;j++)
        Velocity_high_8bit_LUT[j]=i;            

     start = end;

  }

  Velocity_high_8bit_LUT[0] = 0;              
  Velocity_high_8bit_LUT[1] = 15;             

}

/************************************************************************
 *      Description: This function builds the reflectivity lookup table *
 *                                                                      *
 *      Input:  n_offset - new offset                                   *
 *              n_scale - new scale                                     *
 *      Return: NONE                                                    *
 ************************************************************************/

void build_velocity_low_8bit_LUT( float n_offset, float n_scale )
{
  int i = 0, j;
  int start, end;

  Prev_velocity_low_8bit_offset = n_offset;
  Prev_velocity_low_8bit_scale = n_scale;

  /* Same 16 data levels used by V products (Res 1). */
  Velocity_low_8bit_Thresh[0] = NO_LABEL_FLAG;
  Velocity_low_8bit_Thresh[1] = -64;
  Velocity_low_8bit_Thresh[2] = -50;
  Velocity_low_8bit_Thresh[3] = -36;
  Velocity_low_8bit_Thresh[4] = -26;
  Velocity_low_8bit_Thresh[5] = -20;
  Velocity_low_8bit_Thresh[6] = -10;
  Velocity_low_8bit_Thresh[7] = 0;
  Velocity_low_8bit_Thresh[8] = 10;
  Velocity_low_8bit_Thresh[9] = 20;
  Velocity_low_8bit_Thresh[10] = 26;
  Velocity_low_8bit_Thresh[11] = 36;
  Velocity_low_8bit_Thresh[12] = 50;
  Velocity_low_8bit_Thresh[13] = 64;
  Velocity_low_8bit_Thresh[14] = NO_LABEL_FLAG;
  Velocity_low_8bit_Thresh[15] = NO_LABEL_FLAG;

  /* Compute LUT based on threshold values. */
  start = 2;
  for( i = 1; i < 15; i++ ){

     /* Get the threshold value. */
     float v = Velocity_low_8bit_Thresh[i];

     /* Convert it to ICD units.   if the value is the NO_LABEL_FLAG,
        set the end to the highest possible ICD value. */
     if( v == NO_LABEL_FLAG )
        end = 255;

     else
        end = (int) roundf( (((float) v / MPS_TO_KTS) + 127.0f) ) + 2;

     for(j=start;j<end;j++)
        Velocity_low_8bit_LUT[j]=i;            

     start = end;

  }

  Velocity_low_8bit_LUT[0] = 0;              
  Velocity_low_8bit_LUT[1] = 15;             

}

/************************************************************************
 *      Description: This function builds the reflectivity lookup table *
 *                                                                      *
 *      Input:  n_offset - new offset                                   *
 *              n_scale - new scale                                     *
 *      Return: NONE                                                    *
 ************************************************************************/

void build_spectrum_width_8bit_LUT( float n_offset, float n_scale )
{
  int i = 0, j;
  int start, end;

  Prev_spectrum_width_8bit_offset = n_offset;
  Prev_spectrum_width_8bit_scale = n_scale;

  Spectrum_width_8bit_Thresh[0] = 0;
  Spectrum_width_8bit_Thresh[1] = 2;
  Spectrum_width_8bit_Thresh[2] = 4;
  Spectrum_width_8bit_Thresh[3] = 6;
  Spectrum_width_8bit_Thresh[4] = 8;
  Spectrum_width_8bit_Thresh[5] = 10;
  Spectrum_width_8bit_Thresh[6] = 12;
  Spectrum_width_8bit_Thresh[7] = 14;
  Spectrum_width_8bit_Thresh[8] = 16;
  Spectrum_width_8bit_Thresh[9] = 18;
  Spectrum_width_8bit_Thresh[10] = 20;
  Spectrum_width_8bit_Thresh[11] = 22;
  Spectrum_width_8bit_Thresh[12] = 24;
  Spectrum_width_8bit_Thresh[13] = NO_LABEL_FLAG;
  Spectrum_width_8bit_Thresh[14] = NO_LABEL_FLAG;
  Spectrum_width_8bit_Thresh[15] = NO_LABEL_FLAG;

  /* Compute LUT based on threshold values. */
  start = 2;
  for( i = 1; i < 15; i++ ){

     /* Get the threshold value. */
     float w = Spectrum_width_8bit_Thresh[i];

     /* Convert it to ICD units.   if the value is the NO_LABEL_FLAG,
        set the end to the highest possible ICD value. */
     if( w == NO_LABEL_FLAG )
        end = 255;

     else
        end = (int) roundf( (((float) w / MPS_TO_KTS) + 63.5)*2.0f ) + 2;

     for(j=start;j<end;j++)
        Spectrum_width_8bit_LUT[j]=i;            

     start = end;

  }

  Spectrum_width_8bit_LUT[0]=0;
  Spectrum_width_8bit_LUT[1]=15;

}

/************************************************************************
 *	Description: This function builds the differential reflectivity	*
 *		     lookup table.					*
 *									*
 *	Input:	n_offset - new offset					*
 *		n_scale - new scale					*
 *	Return: NONE							*
 ************************************************************************/

void build_diff_reflectivity_8bit_LUT( float n_offset, float n_scale )
{

  int i = 0, j;
  int start, end;

  Prev_diff_reflectivity_8bit_offset = n_offset;
  Prev_diff_reflectivity_8bit_scale = n_scale;

  Diff_reflectivity_8bit_Thresh[0] = NO_LABEL_FLAG;
  Diff_reflectivity_8bit_Thresh[1] = -4.0;
  Diff_reflectivity_8bit_Thresh[2] = -2.0;
  Diff_reflectivity_8bit_Thresh[3] = -0.5;
  Diff_reflectivity_8bit_Thresh[4] = 0.0;
  Diff_reflectivity_8bit_Thresh[5] = 0.25;
  Diff_reflectivity_8bit_Thresh[6] = 0.5;
  Diff_reflectivity_8bit_Thresh[7] = 1.0;
  Diff_reflectivity_8bit_Thresh[8] = 1.5;
  Diff_reflectivity_8bit_Thresh[9] = 2.0;
  Diff_reflectivity_8bit_Thresh[10] = 2.5;
  Diff_reflectivity_8bit_Thresh[11] = 3.0;
  Diff_reflectivity_8bit_Thresh[12] = 4.0;
  Diff_reflectivity_8bit_Thresh[13] = 5.0;
  Diff_reflectivity_8bit_Thresh[14] = 6.0;
  Diff_reflectivity_8bit_Thresh[15] = NO_LABEL_FLAG;

  /* Compute LUT based on threshold values. */
  start = 2;
  for( i = 1; i < 15; i++ ){

     /* Get the threshold value. */
     float z = Diff_reflectivity_8bit_Thresh[i];

     /* Convert it to ICD units.   if the value is the NO_LABEL_FLAG,
        set the end to the highest possible ICD value. */
     if( z == NO_LABEL_FLAG )
        end = 255;

     else
        end = (int) roundf( (((float) z) * 16.0 ) + 128.0 );

     for(j=start;j<end;j++)
        Diff_reflectivity_8bit_LUT[j]=i;

     start = end;

  }

  Diff_reflectivity_8bit_LUT[0] = 0;
  Diff_reflectivity_8bit_LUT[1] = 15;

}

/************************************************************************
 *	Description: This function builds the differential phase	*
 *		     lookup table.					*
 *									*
 *	Input:	n_offset - new offset					*
 *		n_scale - new scale					*
 *	Return: NONE							*
 ************************************************************************/

void build_diff_phase_16bit_LUT( float n_offset, float n_scale )
{
  int i = 0, j;
  int start, end;

  Prev_diff_phase_16bit_offset = n_offset;
  Prev_diff_phase_16bit_scale = n_scale;

  Diff_phase_16bit_Thresh[0] = 0.0;
  Diff_phase_16bit_Thresh[1] = 25.0;
  Diff_phase_16bit_Thresh[2] = 50.0;
  Diff_phase_16bit_Thresh[3] = 75.0;
  Diff_phase_16bit_Thresh[4] = 100.0;
  Diff_phase_16bit_Thresh[5] = 125.0;
  Diff_phase_16bit_Thresh[6] = 150.0;
  Diff_phase_16bit_Thresh[7] = 175.0;
  Diff_phase_16bit_Thresh[8] = 200.0;
  Diff_phase_16bit_Thresh[9] = 225.0;
  Diff_phase_16bit_Thresh[10] = 250.0;
  Diff_phase_16bit_Thresh[11] = 275.0;
  Diff_phase_16bit_Thresh[12] = 300.0;
  Diff_phase_16bit_Thresh[13] = 325.0;
  Diff_phase_16bit_Thresh[14] = 350.0;
  Diff_phase_16bit_Thresh[15] = NO_LABEL_FLAG;

  /* Compute LUT based on threshold values. */
  start = 2;
  for( i = 1; i < 15; i++ ){

     /* Get the threshold value. */
     float c = Diff_phase_16bit_Thresh[i];

     /* Convert it to ICD units.   if the value is the NO_LABEL_FLAG,
        set the end to the highest possible ICD value. */
     if( c == NO_LABEL_FLAG )
        end = 65535;

     else
        end = (int) roundf( (((float) c) * 2.8361 ) + 2.0 );

     for(j=start;j<end;j++)
        Diff_phase_16bit_LUT[j]=i;

     start = end;

  }

  Diff_phase_16bit_LUT[0] = 0;
  Diff_phase_16bit_LUT[1] = 15;

}

/************************************************************************
 *      Description: This function builds the differential correlation  *
 *                   lookup table.                                      *
 *                                                                      *
 *      Input:  n_offset - new offset                                   *
 *              n_scale - new scale                                     *
 *      Return: NONE                                                    *
 ************************************************************************/

void build_diff_correlation_8bit_LUT( float n_offset, float n_scale )
{

  int i = 0, j;
  int start, end;

  Prev_diff_correlation_8bit_offset = n_offset;
  Prev_diff_correlation_8bit_scale = n_scale;

  Diff_correlation_8bit_Thresh[0] = 0.2;
  Diff_correlation_8bit_Thresh[1] = 0.45;
  Diff_correlation_8bit_Thresh[2] = 0.65;
  Diff_correlation_8bit_Thresh[3] = 0.75;
  Diff_correlation_8bit_Thresh[4] = 0.80;
  Diff_correlation_8bit_Thresh[5] = 0.85;
  Diff_correlation_8bit_Thresh[6] = 0.90;
  Diff_correlation_8bit_Thresh[7] = 0.93;
  Diff_correlation_8bit_Thresh[8] = 0.95;
  Diff_correlation_8bit_Thresh[9] = 0.96;
  Diff_correlation_8bit_Thresh[10] = 0.97;
  Diff_correlation_8bit_Thresh[11] = 0.98;
  Diff_correlation_8bit_Thresh[12] = 0.99;
  Diff_correlation_8bit_Thresh[13] = 1.0;
  Diff_correlation_8bit_Thresh[14] = 1.05;
  Diff_correlation_8bit_Thresh[15] = NO_LABEL_FLAG;

  /* Compute LUT based on threshold values. */
  start = 2;
  for( i = 1; i < 15; i++ ){

     /* Get the threshold value. */
     float c = Diff_correlation_8bit_Thresh[i];

     /* Convert it to ICD units.   if the value is the NO_LABEL_FLAG,
        set the end to the highest possible ICD value. */
     if( c == NO_LABEL_FLAG )
        end = 255;

     else
        end = (int) roundf( (((float) c) * 300.0 ) - 60.5 );

     for(j=start;j<end;j++)
        Diff_correlation_8bit_LUT[j]=i;

     start = end;

  }

  Diff_correlation_8bit_LUT[0] = 0;
  Diff_correlation_8bit_LUT[1] = 15;

}

/************************************************************************
 *	Description: This function builds the reflectivity range	*
 *		     lookup table.					*
 *									*
 *	Input:	n_first_range - new distance to first bin		*
 *		n_bin_size - new bin size				*
 *	Return: NONE							*
 ************************************************************************/

void build_reflectivity_range( float n_first_range, float n_bin_size )
{
  int i = 0;

  Prev_reflectivity_first_range = n_first_range;
  Prev_reflectivity_bin_size = n_bin_size;

  Reflectivity_range[0] = n_first_range;
  for( i=1; i<MAX_BINS_ALONG_RADIAL; i++ )
  {
    Reflectivity_range[i] = Reflectivity_range[i-1]+n_bin_size;
  }
}

/************************************************************************
 *      Description: This function builds the Velocity range lookup     *
 *                   table.                                             *
 *                                                                      *
 *      Input:  n_first_range - new distance to first bin               *
 *              n_bin_size - new bin size                               *
 *      Return: NONE                                                    *
 ************************************************************************/

void build_velocity_range( float n_first_range, float n_bin_size )
{
  int i = 0;

  Prev_velocity_first_range = n_first_range;
  Prev_velocity_bin_size = n_bin_size;

  Velocity_range[0] = n_first_range;
  for( i=1; i<MAX_BINS_ALONG_RADIAL; i++ )
  {
    Velocity_range[i] = Velocity_range[i-1]+n_bin_size;
  }
}

/************************************************************************
 *	Description: This function builds the Spectrum Width range	*
 *		     lookup table.					*
 *									*
 *	Input:	n_first_range - new distance to first bin		*
 *		n_bin_size - new bin size				*
 *	Return: NONE							*
 ************************************************************************/

void build_spectrum_width_range( float n_first_range, float n_bin_size )
{
  int i = 0;

  Prev_spectrum_width_first_range = n_first_range;
  Prev_spectrum_width_bin_size = n_bin_size;

  Spectrum_width_range[0] = n_first_range;
  for( i=1; i<MAX_BINS_ALONG_RADIAL; i++ )
  {
    Spectrum_width_range[i] = Spectrum_width_range[i-1]+n_bin_size;
  }
}

/************************************************************************
 *	Description: This function builds the differential reflectivity	*
 *		     range lookup table.				*
 *									*
 *	Input:	n_first_range - new distance to first bin		*
 *		n_bin_size - new bin size				*
 *	Return: NONE							*
 ************************************************************************/

void build_diff_reflectivity_range( float n_first_range, float n_bin_size )
{
  int i = 0;

  Prev_diff_reflectivity_first_range = n_first_range;
  Prev_diff_reflectivity_bin_size = n_bin_size;

  Diff_reflectivity_range[0] = n_first_range-(n_bin_size/2.0);
  for( i=1; i<MAX_BINS_ALONG_RADIAL; i++ )
  {
    Diff_reflectivity_range[i] = Diff_reflectivity_range[i-1]+n_bin_size;
  }
}

/************************************************************************
 *	Description: This function builds the differential phase	*
 *		     range lookup table.				*
 *									*
 *	Input:	n_first_range - new distance to first bin		*
 *		n_bin_size - new bin size				*
 *	Return: NONE							*
 ************************************************************************/

void build_diff_phase_range( float n_first_range, float n_bin_size )
{
  int i = 0;

  Prev_diff_phase_first_range = n_first_range;
  Prev_diff_phase_bin_size = n_bin_size;

  Diff_phase_range[0] = n_first_range-(n_bin_size/2.0);
  for( i=1; i<MAX_BINS_ALONG_RADIAL; i++ )
  {
    Diff_phase_range[i] = Diff_phase_range[i-1]+n_bin_size;
  }
}

/************************************************************************
 *	Description: This function builds the differential correlation	*
 *		     range lookup table.				*
 *									*
 *	Input:	n_first_range - new distance to first bin		*
 *		n_bin_size - new bin size				*
 *	Return: NONE							*
 ************************************************************************/

void build_diff_correlation_range( float n_first_range, float n_bin_size )
{
  int i = 0;

  Prev_diff_correlation_first_range = n_first_range;
  Prev_diff_correlation_bin_size = n_bin_size;

  Diff_correlation_range[0] = n_first_range-(n_bin_size/2.0);
  for( i=1; i<MAX_BINS_ALONG_RADIAL; i++ )
  {
    Diff_correlation_range[i] = Diff_correlation_range[i-1]+n_bin_size;
  }
}

/************************************************************************
 *      Description: This function returns a pointer to the threshold   *
 *                   table corresponding to the moment.                 *
 *                                                                      *
 *      Input: Integer representing moment                              *
 *      Return: NONE                                                    *
 ************************************************************************/

float *hci_basedata_tool_data_thresh_ptr( int moment )
{
  switch( moment )
  {
    case VELOCITY :
      switch( hci_basedata_tool_velocity_resolution() )
      {
        case DOPPLER_RESOLUTION_LOW :
          {
            return &Velocity_low_8bit_Thresh[0];
          }
          break;

        default:
          {
            return &Velocity_high_8bit_Thresh[0];
          }
          break;
      }
      break;

    case SPECTRUM_WIDTH :
      {
        return &Spectrum_width_8bit_Thresh[0];
      }
      break;

    case DIFF_REFLECTIVITY :
      {
        return &Diff_reflectivity_8bit_Thresh[0];
      }
      break;

    case DIFF_PHASE :
      {
        return &Diff_phase_16bit_Thresh[0];
      }
      break;

    case DIFF_CORRELATION :
      {
        return &Diff_correlation_8bit_Thresh[0];
      }
      break;

    case REFLECTIVITY :
      {
        return &Reflectivity_8bit_Thresh[0];
      }
      break;
  }

  return NULL;
}
                                                                                                                                    
/************************************************************************
 *	Description: This function returns a pointer to the lookup	*
 *		     table corresponding to the moment.			*
 *									*
 *	Input: Integer representing moment				*
 *	Return: NONE							*
 ************************************************************************/

float *hci_basedata_tool_data_table_ptr( int moment )
{
  switch( moment )
  {
    case VELOCITY :
      switch( hci_basedata_tool_velocity_resolution() )
      {
        case DOPPLER_RESOLUTION_LOW :
          {
            return &Velocity_low_8bit_LUT[0];
          }
          break;

        default:
          {
            return &Velocity_high_8bit_LUT[0];
          }
          break;
      }
      break;

    case SPECTRUM_WIDTH :
      {
        return &Spectrum_width_8bit_LUT[0];
      }
      break;

    case DIFF_REFLECTIVITY :
      {
        return &Diff_reflectivity_8bit_LUT[0];
      }
      break;

    case DIFF_PHASE :
      {
        return &Diff_phase_16bit_LUT[0];
      }
      break;

    case DIFF_CORRELATION :
      {
        return &Diff_correlation_8bit_LUT[0];
      }
      break;

    case REFLECTIVITY :
    default :
      {
        return &Reflectivity_8bit_LUT[0];
      }
      break;
  }

  return NULL;
}

/************************************************************************
 *	Description: This function returns a pointer to the array of	*
 *		     range values corresponding to the moment.		*
 *									*
 *	Input: Integer representing moment				*
 *	Return: NONE							*
 ************************************************************************/

float *hci_basedata_tool_range_ptr( int moment )
{
  switch( moment )
  {
    case VELOCITY :
      return &Velocity_range[0];
      break;

    case SPECTRUM_WIDTH :
      return &Spectrum_width_range[0];
      break;

    case DIFF_REFLECTIVITY :
      return &Diff_reflectivity_range[0];
      break;

    case DIFF_PHASE :
      return &Diff_phase_range[0];
      break;

    case DIFF_CORRELATION :
      return &Diff_correlation_range[0];
      break;

    case REFLECTIVITY :
    default :
      return &Reflectivity_range[0];
      break;
  }

  return NULL;
}

/************************************************************************
 *	Description: This function sets the static memory associated	*
 *		     with a radial of data to the passed in data.	*
 *									*
 *	Input: Pointer to data and integer representing data length	*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_tool_set_data_ptr( char *new_data, int data_len )
{
  if( new_data != NULL )
  {
    memcpy( &Data[ 0 ], &new_data[ 0 ], data_len );
    reset_radial();
  }
}

/************************************************************************
 *	Description: This function returns the word size associated	*
 *		     with a given moment.				*
 *									*
 *	Input: Integer representing moment				*
 *	Return: Integer of word size in bytes				*
 ************************************************************************/

int hci_basedata_tool_data_word_size( moment )
{
  switch( moment )
  {
    case VELOCITY :
      return IS_16BIT_DATA;
      break;

    case SPECTRUM_WIDTH :
      return IS_16BIT_DATA;
      break;

    case DIFF_REFLECTIVITY :
      return Diff_reflectivity_hdr->data_word_size;
      break;

    case DIFF_PHASE :
      return Diff_phase_hdr->data_word_size;
      break;

    case DIFF_CORRELATION :
      return Diff_correlation_hdr->data_word_size;
      break;

    case REFLECTIVITY :
    default :
      return IS_16BIT_DATA;
      break;
  }

  return -1;
}

/************************************************************************
 *	Description: This function returns a pointer to the data array	*
 *		     associated with the given moment.			*
 *									*
 *	Input: Integer representing moment				*
 *	Return: Pointer to data array					*
 ************************************************************************/

void *hci_basedata_tool_data_ptr( int moment )
{
  switch( moment )
  {
    case VELOCITY :
      return Velocity_ptr;
      break;

    case SPECTRUM_WIDTH :
      return Spectrum_width_ptr;
      break;

    case DIFF_REFLECTIVITY :
      return Diff_reflectivity_ptr;
      break;

    case DIFF_PHASE :
      return Diff_phase_ptr;
      break;

    case DIFF_CORRELATION :
      return Diff_correlation_ptr;
      break;

    case REFLECTIVITY :
    default :
      return Reflectivity_ptr;
      break;
  }

  return NULL;
}

/************************************************************************
 *	Description: This function returns azimuth resolution.		*
 *									*
 *	Input: NONE							*
 *	Return: Integer of azimuth resolution				*
 ************************************************************************/

int hci_basedata_tool_azimuth_resolution()
{
  return Hdr->azm_reso;
}

/************************************************************************
 *	Description: This function sets LB id of radar data to read to	*
 *		     given LB_id.					*
 *									*
 *	Input: Integer LB id of radar data to read			*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_tool_set_basedata_LB_id( int LB_id )
{
  Data_feed = LB_id;
}

/************************************************************************
 *	Description: This function returns the azimuth width.		*
 *									*
 *	Input: Integer LB id of radar data to read			*
 *	Return: NONE							*
 ************************************************************************/

float hci_basedata_tool_azimuth_width()
{
  if( hci_basedata_tool_azimuth_resolution() == BASEDATA_HALF_DEGREE )
  {
    return 0.5;
  }

  return 1.0;
}

/************************************************************************
 *	Description: This function returns 1 if the moment is available.*
 *									*
 *	Input: Moment							*
 *	Return: NONE							*
 ************************************************************************/

int hci_basedata_tool_data_available( int moment )
{
  switch( moment )
  {
    case VELOCITY :
      return Hdr->msg_type & VEL_ENABLED_BIT;
      break;

    case SPECTRUM_WIDTH :
      return Hdr->msg_type & WID_ENABLED_BIT;
      break;

    case DIFF_REFLECTIVITY :
      if( Diff_reflectivity_hdr != NULL ){ return Hdr->msg_type & DUALPOL_TYPE;}
      break;

    case DIFF_PHASE :
      if( Diff_phase_hdr != NULL ){ return Hdr->msg_type & DUALPOL_TYPE;}
      break;

    case DIFF_CORRELATION :
      if( Diff_correlation_hdr != NULL ){ return Hdr->msg_type & DUALPOL_TYPE;}
      break;

    case REFLECTIVITY :
    default :
      return Hdr->msg_type & REF_ENABLED_BIT;
      break;
  }

  return 0;
}

/************************************************************************
 *	Description: Print contents of base data header.		*
 *									*
 *	Input: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void print_bdh()
{
  printf("BASE\n");
  printf("TIME:    %d\n",Hdr->time);
  printf("DATE:    %d\n",Hdr->date);
  printf("AZ:      %f\n",Hdr->azimuth);
  printf("AZ#:     %d\n",Hdr->azi_num);
  printf("STAT:    %d\n",Hdr->status);
  printf("ELEV:    %f\n",Hdr->elevation);
  printf("ELEV#:   %d\n",Hdr->elev_num);
  printf("VCP:     %d\n",Hdr->vcp_num);
  printf("REF RANGE:  %d\n",Hdr->surv_range);
  printf("DOP RANGE:  %d\n",Hdr->dop_range);
  printf("REF SIZE:   %d\n",Hdr->surv_bin_size);
  printf("DOP SIZE:   %d\n",Hdr->dop_bin_size);
  printf("# REF BINS: %d\n",Hdr->n_surv_bins);
  printf("# DOP BINS: %d\n\n",Hdr->n_dop_bins);
}

/************************************************************************
 *	Description: Print contents of generic data header.		*
 *									*
 *	Input: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void print_gdh()
{
  printf("GENERIC\n");
  printf("TIME:    %d\n",Hdr->time);
  printf("DATE:    %d\n",Hdr->date);
  printf("AZ:      %f\n",Hdr->azimuth);
  printf("AZ#:     %d\n",Hdr->azi_num);
  printf("STAT:    %d\n",Hdr->status);
  printf("ELEV:    %f\n",Hdr->elevation);
  printf("ELEV#:   %d\n",Hdr->elev_num);
  printf("VCP:     %d\n",Hdr->vcp_num);
  printf("REF RANGE:  %d\n",Hdr->surv_range);
  printf("DOP RANGE:  %d\n",Hdr->dop_range);
  printf("REF SIZE:   %d\n",Hdr->surv_bin_size);
  printf("DOP SIZE:   %d\n",Hdr->dop_bin_size);
  printf("# REF BINS: %d\n",Hdr->n_surv_bins);
  printf("# DOP BINS: %d\n\n",Hdr->n_dop_bins);
}

