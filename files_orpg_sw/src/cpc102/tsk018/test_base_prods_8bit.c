
/* RCS info */
/* $Author: cmn $ */
/* $Locker:  $ */
/* $Date: 2011/05/22 17:12:19 $ */
/* $Id: test_base_prods_8bit.c,v 1.1 2011/05/22 17:12:19 cmn Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

/* Include files. */

#include <test_base_prods_8bit.h>

/* Macro definitions. */

#define	INGEST_LB		"RAWDATA"
#define	NUM_LEADING_FLAGS	2 /* 0 = BELOW THRESHOLD, 1 = RF */
#define	DATA_MAX_CHAR		255
#define	DATA_MIN_CHAR		0

/* Global variables. */

static char *BD_buf_ptr = NULL;
static Base_data_header *BD_hdr_ptr = NULL;
static int Elevation_index = -1;
static short Elevation_date = 0;
static unsigned int Elevation_time = 0;
static float Target_elevation_angle = -1;
static float Range_scale_factor = -1;
static int VCP_number = 1;
static int Volume_number = 1;
static int Process_radial = 1;
static int Moments_available[MAX_NUM_MOMENTS_AVAILABLE];

/* Function Prototypes. */

static int Product_generation_control( int );
static int End_of_product_processing( int );
static int Get_radial_data( int );
static void Find_max_min( int, void *, int, int );
static void Initialize_prod_info();
static void Release_outbuf_and_send_product( int );
static void Release_outbuf_and_destroy( int );
static void Abort_product( int, int );
static int Get_input_buffer();
static void Release_input_buffer();
static int Release_all_input_buffers();
static int Initialize_elevation_info();
static void Set_available_data_flags();
static int Data_is_available( int );
static int Get_output_buffer( int );
static int Product_is_requested( int );
static void Fill_data_level_thresholds( int, Graphic_product * );
static void Fill_prod_dep_params( int, Graphic_product * );
static double Get_time_marker();
static void Handle_elevation_restart();
#ifdef PRINT_TEST_PROD
static void Print_product( int );
#endif

/**********************************************************************
  Description: Buffer control module.
**********************************************************************/


int Test_base_prods_8bit_buffer_control()
{
  int pindx, ret, num_prods = 0;
  int elevation_started = 0;

  /* Initialize product info. */

  Initialize_prod_info();

  /* Request first input buffer. */

  if( !Get_input_buffer() ){ RPGC_abort(); }

  /* Initialize elevation info with first radial message. */

  if( !Initialize_elevation_info() ){ Release_input_buffer(); RPGC_abort(); }

  /* Determine which moments are available for this elevation. */

  Set_available_data_flags();

  /* Check if each product is requested and whether it can
     be generated given available data. */ 

  for( pindx = FIRST_PROD_INDEX; pindx < NUM_TEST_BASE_PRODS; pindx++ )
  {
    if( Product_is_requested( pindx ) )
    {
      if( Data_is_available( pindx ) )
      {
        if( !( ret = Get_output_buffer( pindx ) ) )
        {
          Abort_product( pindx, ret );
        }
        else
        {
          /* Flag product to generate and increment number of products. */
          Prod_info[pindx].generate = 1;
          num_prods++;
        }
      }
      else
      {
        /* Data not available for product. */
        Abort_product( pindx, PROD_DISABLED_MOMENT );
      }
    }
  }

  /* If no products to generate, skip to end of cut. */

  if( num_prods < 1 )
  {
    Release_input_buffer();
    if( !Release_all_input_buffers() ){ RPGC_abort(); }
    else{ return 0; }
  }

  /* Loop over input radials until end of cut. */

  while(1)
  {
    /* Note the start of an elevation or volume. If another start
       of elevation or volume is received before finishing the
       current cut, then a volume/elevation restart has occured
       and needs to be handled. */

    if( BD_hdr_ptr->status == GOODBVOL || BD_hdr_ptr->status == GOODBEL )
    {
      if( elevation_started == 0 )
      {
        elevation_started = 1;
      }
      else
      {
        Handle_elevation_restart();
      }
    }

    /* Don't process radials past pseudo end of elevation/volume. */

    if( (BD_hdr_ptr->status == PGENDEL) || (BD_hdr_ptr->status == PGENDVOL) )
    {
      Process_radial = 0;
    }

    /* Loop through each product. If the current radial is to be
       processed and the product is to be generated, then call
       the product generation control routine. */

    for( pindx = FIRST_PROD_INDEX; pindx < NUM_TEST_BASE_PRODS; pindx++ )
    {
      if( Prod_info[pindx].generate && Process_radial )
      {
        if( !Product_generation_control( pindx ) )
        {
          Abort_product( pindx, PGM_PROD_NOT_GENERATED );
        }
      }
    }

    /* If last radial in cut, call special routine to finish creating
       and releasing product. */

    if( (BD_hdr_ptr->status == GENDVOL) || (BD_hdr_ptr->status == GENDEL) )
    {
      for( pindx = FIRST_PROD_INDEX; pindx < NUM_TEST_BASE_PRODS; pindx++ )
      {
        if( Prod_info[pindx].generate )
        {
          if( !End_of_product_processing( pindx ) )
          {
            Abort_product( pindx, PGM_PROD_NOT_GENERATED );
          }
        }
      }
      /* Release last input radial and break out of while loop. */
      Release_input_buffer();
      break;
    }

    /* Release current input radial. */
    Release_input_buffer();

    /* Retrieve next input radial. */
    if( !Get_input_buffer() )
    {
      for( pindx = FIRST_PROD_INDEX; pindx < NUM_TEST_BASE_PRODS; pindx++ )
      {
        Release_outbuf_and_destroy( pindx );
      }
      RPGC_abort();
      break;
    }
  }

  /* Return to wait for activation. */

  return 0;
}
 
/**********************************************************************
  Description: Process radial of data for given product. Return 0 on
     failure, 1 otherwise.
**********************************************************************/

static int Product_generation_control( int pindx )
{
  /* Get radial data and fill the product buffer. */

  if( Get_radial_data( pindx ) )
  {
    /* Increment the radial count. */
    Prod_info[pindx].num_radials++;
    return 1;
  }

  return 0;
} 

/**********************************************************************
  Description: Find min/max in data array for a given size.
**********************************************************************/

static void Find_max_min( int pindx, void *data, int num_bins, int bit_size )
{
  unsigned int max_value, min_value;
  int i;

  /* Temporary storage for min and max data values. */

  max_value = (unsigned int) Prod_info[pindx].max_data_value;
  min_value = (unsigned int) Prod_info[pindx].min_data_value;

  if( bit_size == RPGC_BYTE_DATA )
  {
    unsigned char *ptr = (unsigned char *) data;
    for( i = 0;  i < num_bins; i++ )
    {
      if( ptr[i] >= (unsigned char) NUM_LEADING_FLAGS )
      {
        /* Find the maximum data level. */
        if( ptr[i] > max_value )
        {
          max_value = (unsigned int) ptr[i];
        }
        /* Find the minimum data level. */
        if( ptr[i] < min_value )
        {
          min_value = (unsigned int) ptr[i];
        }
      }
    }
  }
  else if( bit_size == RPGC_SHORT_DATA )
  {
    unsigned short *ptr = (unsigned short *) data;
    for( i = 0;  i < num_bins; i++ )
    {
      if( ptr[i] >= (unsigned short) NUM_LEADING_FLAGS )
      {
        /* Find the maximum data level. */
        if( ptr[i] > max_value )
        {
          max_value = (unsigned int) ptr[i];
        }
        /* Find the minimum data level. */
        if( ptr[i] < min_value )
        {
          min_value = (unsigned int) ptr[i];
        }
      }
    }
  }

  /* Set the minimum and maximum data levels. */

  Prod_info[pindx].max_data_value = (unsigned int) max_value;
  Prod_info[pindx].min_data_value = (unsigned int) min_value;
} 

/**********************************************************************
  Description: End processing of product for this cut. Finish creating
     product and send. Return 0 on error, 1 otherwise.
**********************************************************************/

static int End_of_product_processing( int pindx )
{
  Graphic_product *phd;
  Symbology_block *sym;
  Packet_16_hdr_t *pkt16;
  int data_len = 0;
  int result;
  double t1, t2;

  /* Point headers to appropriate places in memory. */

  phd = (Graphic_product *) Prod_info[pindx].outbuf;
  sym = (Symbology_block *) ( Prod_info[pindx].outbuf + GP_SIZE );
  pkt16 = (Packet_16_hdr_t *) ( Prod_info[pindx].outbuf + GP_SIZE + SYMB_SIZE );

  /* Fill in Product Description Block. */

  RPGC_prod_desc_block( Prod_info[pindx].outbuf,
                        Prod_info[pindx].id,
                        Volume_number );

  RPGC_set_product_int( (void *) &phd->msg_time, Elevation_time );
  phd->msg_date = Elevation_date;

  RPGC_set_prod_block_offsets( phd, GP_SIZE/sizeof(short), 0, 0 );

  Fill_data_level_thresholds( pindx, phd );

  Fill_prod_dep_params( pindx, phd );

  /* Fill in Symbology Block. */

  sym->divider = (short) -1;
  sym->block_id = (short) 1;
  sym->n_layers = (short) 1;
  sym->layer_divider = (short) -1;

  data_len = Prod_info[pindx].data_index - ( GP_SIZE + SYMB_SIZE );
  RPGC_set_product_int( (void *) &sym->data_len, data_len );
  data_len += SYMB_SIZE;
  RPGC_set_product_int( (void *) &sym->block_len, data_len );

  /* Fill in Packet 16 header. */

  RPGC_digital_radial_data_hdr( 0, Prod_info[pindx].num_bins, 0, 0,
        Range_scale_factor, Prod_info[pindx].num_radials, (void *) pkt16 );

  /* Fill in Message Header Block. */

  result = RPGC_prod_hdr( Prod_info[pindx].outbuf, Prod_info[pindx].id, &data_len );

  /* If product created, release buffer and send product. */

  if( result == 0 )
  {
    /* Print log message of product info. */
    sprintf( Msg_buf, "Id: %d size: %d", Prod_info[pindx].id, Prod_info[pindx].data_index );
    RPGC_log_msg( GL_INFO, "%s", Msg_buf );

#ifdef PRINT_TEST_PROD
    /* Print product header info for validation. */
    Print_product( pindx );
#endif

    /* Get time marker. */
    t1 = Get_time_marker();

    /* Release outbuf and send product. */
    Release_outbuf_and_send_product( pindx );

    /* Get end time and log compression time. */
    t2 = Get_time_marker();
    sprintf( Msg_buf, "Compression time: %g", t2 - t1 );
    RPGC_log_msg( GL_INFO, "%s", Msg_buf );
  }
  else
  {
    if ( Prod_info[pindx].outbuf != NULL )
    {
      Release_outbuf_and_destroy( pindx );
    }
    return 0;
  }

  return 1;
} 

#define RHO_SCALE       300.0f
#define RHO_OFFSET      -60.5f
#define PHI_SCALE      2.8361f
#define PHI_OFFSET        2.0f

/**********************************************************************
  Description: Read radial data from input buffer and write to the
     product's buffer.
**********************************************************************/

static int Get_radial_data( int pindx )
{
  Generic_moment_t *ghdr;
  void *data = NULL, *obuf = NULL;
  short *tmp_array = NULL;
  int bit_size = RPGC_SHORT_DATA;
  int num_orig_bins = 0;
  int num_bins = 0;
  int num_bytes = 0;
  int i, j;

  if( pindx == REF_PROD_INDEX && !Moments_available[DRF2_MOMENT_INDEX] )
  {
    num_orig_bins = MAX_460_NUM_BINS;
    num_bins = BD_hdr_ptr->n_surv_bins;
    data = (void *) &(BD_buf_ptr[BD_hdr_ptr->ref_offset]);
  }
  else if( pindx == VEL_PROD_INDEX )
  {
    num_orig_bins = MAX_300_NUM_BINS;
    num_bins = BD_hdr_ptr->n_dop_bins;
    data = (void *) &(BD_buf_ptr[BD_hdr_ptr->vel_offset]);
  }
  else if( pindx == SPW_PROD_INDEX )
  {
    num_orig_bins = MAX_300_NUM_BINS;
    num_bins = BD_hdr_ptr->n_dop_bins;
    data = (void *) &(BD_buf_ptr[BD_hdr_ptr->spw_offset]);
  }
  else
  {
    for( i = 0; i < BD_hdr_ptr->no_moments; i++ )
    {
      ghdr = (Generic_moment_t *) (BD_buf_ptr+BD_hdr_ptr->offsets[i]);
      if( ( pindx == ZDR_PROD_INDEX &&
            strncmp(ghdr->name, Prod_info[ZDR_PROD_INDEX].moment, 4) == 0 ) ||
          ( pindx == PHI_PROD_INDEX &&
            strncmp(ghdr->name, Prod_info[PHI_PROD_INDEX].moment, 4) == 0 ) ||
          ( pindx == RHO_PROD_INDEX &&
            strncmp(ghdr->name, Prod_info[RHO_PROD_INDEX].moment, 4) == 0 ) ||
          ( pindx == REF_PROD_INDEX &&
            strncmp(ghdr->name, "DRF2", 4) == 0 ) )
      {
        num_orig_bins = MAX_300_NUM_BINS;
        num_bins = ghdr->no_of_gates;
        if( ghdr->data_word_size == RPGC_BYTE_DATA )
        {
          bit_size = RPGC_BYTE_DATA;
          data = (void *) &(ghdr->gate.b[0]);
        }
        else
        {
          if( (pindx == PHI_PROD_INDEX) || (pindx == RHO_PROD_INDEX) )
          {
            /* Convert 10-bit PHI to 8-bit by dividing by 4. */
            tmp_array = calloc( 1, num_orig_bins * sizeof(short) );
            if( pindx == PHI_PROD_INDEX )
            {
                float scale = ghdr->scale;
                float f, offset = ghdr->offset;
                
                for( j = 0; j < num_bins; j++ )
                {
                  if( ghdr->gate.u_s[j] >= NUM_LEADING_FLAGS )
                  {
                    f = ((float) ghdr->gate.u_s[j] - offset)/scale;
                    tmp_array[j] = ((short) (roundf( (f*PHI_SCALE) + PHI_OFFSET )))/4;
                    if( tmp_array[j] < NUM_LEADING_FLAGS )
                    {
                      tmp_array[j] = NUM_LEADING_FLAGS;
                    }
                  }
                  else
                  {
                    tmp_array[j] = ghdr->gate.u_s[j];
                  }
                }
                data = (void *) &(tmp_array[0]);
             }
             else if( pindx == RHO_PROD_INDEX )
             {
                float scale = ghdr->scale;
                float f, offset = ghdr->offset;
                for( j = 0; j < num_bins; j++ )
                {
                   if( ghdr->gate.u_s[j] > BASEDATA_RDRNGF )
                   {
                     f = ((float) ghdr->gate.u_s[j] - offset)/scale;
                     tmp_array[j] = roundf( (f*RHO_SCALE) + RHO_OFFSET );
                   }
                   else
                   {
                      tmp_array[j] = ghdr->gate.u_s[j];
                   }
                }
                data = (void *) &(tmp_array[0]);
             }
          }
          else
          {
            data = (void *) &(ghdr->gate.u_s[0]);
          }
        }
      }
    }
  }

  if( data != NULL )
  {
    obuf = (void *) (Prod_info[pindx].outbuf + Prod_info[pindx].data_index);
    num_bytes = RPGC_digital_radial_data_array( data, bit_size,
                      0, num_bins-1, 0, num_orig_bins, 1,
                      BD_hdr_ptr->start_angle, BD_hdr_ptr->delta_angle, obuf );
    if( num_bytes > 0 )
    {
      Prod_info[pindx].data_index += num_bytes;
      Find_max_min( pindx, data, num_bins, bit_size );
      Prod_info[pindx].num_bins = num_orig_bins;
    }
  }

  if( tmp_array != NULL ){ free( tmp_array ); }

  if( num_bytes ){ return 1; }

  return 0;
}

/**********************************************************************
  Description: Release memory previously allocated to products output 
     buffer. The product is sent.
**********************************************************************/

static void Release_outbuf_and_send_product( int pindx )
{
  RPGC_rel_outbuf( Prod_info[pindx].outbuf, FORWARD|EXTENDED_ARGS_MASK, Prod_info[pindx].data_index );
  Prod_info[pindx].outbuf = NULL;
}

/**********************************************************************
  Description: Release memory previously allocated to products output 
     buffer. The product is destroyed and not sent.
**********************************************************************/

static void Release_outbuf_and_destroy( int pindx )
{
  RPGC_rel_outbuf( Prod_info[pindx].outbuf, DESTROY );
  Prod_info[pindx].outbuf = NULL;
}

/**********************************************************************
  Description: Abort product creation for given request and reason.
**********************************************************************/

static void Abort_product( int pindx, int reason_code )
{
  RPGC_abort_request( Prod_info[pindx].request, reason_code );
  Prod_info[pindx].generate = 0;
}

/**********************************************************************
  Description: Initialize product-specific info for this cut.
**********************************************************************/

static void Initialize_prod_info()
{
  int pindx;

  Process_radial = 1;
  for( pindx = FIRST_PROD_INDEX; pindx < NUM_TEST_BASE_PRODS; pindx++ )
  {
    Prod_info[pindx].generate = 0;
    Prod_info[pindx].num_bins = 0;
    Prod_info[pindx].num_radials = 0;
    Prod_info[pindx].data_index = OFFSET_TO_DATA;
    Prod_info[pindx].max_data_value = DATA_MIN_CHAR;
    Prod_info[pindx].min_data_value = DATA_MAX_CHAR;
    if( Prod_info[pindx].outbuf != NULL )
    {
      Release_outbuf_and_destroy( pindx );
    }
    memset( &Prod_info[pindx].request, 0, sizeof(User_array_t) );
  }
}

/**********************************************************************
  Description: Obtain input buffer. Return 0 on error, 1 otherwise.
**********************************************************************/

static int Get_input_buffer()
{
  int ret;

  BD_buf_ptr = RPGC_get_inbuf_by_name( INGEST_LB, &ret );

  /* Check the status of the operation. */

  if( (ret != NORMAL) || (BD_buf_ptr == NULL) )
  {
    if( ret != NORMAL )
    {
      sprintf( Msg_buf, "Request input buffer failed (%d)", ret );
    }
    else
    {
      sprintf( Msg_buf, "Requested input buffer is NULL" );
    }
    RPGC_log_msg( GL_INFO, "%s", Msg_buf );
    BD_hdr_ptr = NULL;
    return 0;
  }

  BD_hdr_ptr = (Base_data_header *) BD_buf_ptr;

  return 1;
}

/**********************************************************************
  Description: Release previously obtained input buffer.
**********************************************************************/

static void Release_input_buffer()
{
  RPGC_rel_inbuf( BD_buf_ptr );
}

/**********************************************************************
  Description: Get elevation index for given current cut. Return 0 on
     error, 1 otherwise.
**********************************************************************/

static int Initialize_elevation_info()
{
  int tmp_int;

  Elevation_date = BD_hdr_ptr->date;
  Elevation_time = BD_hdr_ptr->time / 1000;

  Elevation_index = RPGC_get_buffer_elev_index( BD_buf_ptr );
  if( Elevation_index < 0 )
  {
    sprintf( Msg_buf, "Negative elev index (%d)", Elevation_index );
    RPGC_log_msg( GL_INFO, "%s", Msg_buf );
    return 0;
  }

  VCP_number = RPGC_get_buffer_vcp_num( BD_buf_ptr );
  if( VCP_number < 0 )
  {
    sprintf( Msg_buf, "Negative VCP number (%d)", VCP_number );
    RPGC_log_msg( GL_INFO, "%s", Msg_buf );
    return 0;
  }

  Volume_number = RPGC_get_buffer_vol_num( BD_buf_ptr );
  if( Volume_number < 0 )
  {
    sprintf( Msg_buf, "Negative volume number (%d)", Volume_number );
    RPGC_log_msg( GL_INFO, "%s", Msg_buf );
    return 0;
  }


  tmp_int = RPGCS_get_target_elev_ang( VCP_number, Elevation_index );
  if( tmp_int == RPGCS_ERROR )
  {
    sprintf( Msg_buf, "Error retrieving target elevation angle" );
    RPGC_log_msg( GL_INFO, "%s", Msg_buf );
    return 0;
  }
  else
  {
    Target_elevation_angle = tmp_int / 10.0;
  }

  Range_scale_factor = cos( Target_elevation_angle * DEGTORAD ) * 1e3f;

  return 1;
}

/**********************************************************************
  Description: Determine what moments are available for a given radial.
**********************************************************************/

static void Set_available_data_flags()
{
  int r_flag, v_flag, w_flag, i;
  Generic_moment_t *ghdr = NULL;

  for( i = 0; i < MAX_NUM_MOMENTS_AVAILABLE; i++ ){ Moments_available[i] = 0; }

  RPGC_what_moments( BD_hdr_ptr, &r_flag, &v_flag, &w_flag );

  if( r_flag ){ Moments_available[REF_MOMENT_INDEX] = 1; }
  if( v_flag ){ Moments_available[VEL_MOMENT_INDEX] = 1; }
  if( w_flag ){ Moments_available[SPW_MOMENT_INDEX] = 1; }

  for( i = 0; i < BD_hdr_ptr->no_moments; i++ )
  {
    ghdr = (Generic_moment_t *) (BD_buf_ptr+BD_hdr_ptr->offsets[i]);
    if( strcmp( ghdr->name, Prod_info[ZDR_PROD_INDEX].moment ) == 0 )
    {
      Moments_available[ZDR_MOMENT_INDEX] = 1;
    }
    if( strcmp( ghdr->name, Prod_info[PHI_PROD_INDEX].moment ) == 0 )
    {
      Moments_available[PHI_MOMENT_INDEX] = 1;
    }
    if( strcmp( ghdr->name, Prod_info[RHO_PROD_INDEX].moment ) == 0 )
    {
      Moments_available[RHO_MOMENT_INDEX] = 1;
    }
    if( strcmp( ghdr->name, "DRF2" ) == 0 )
    {
      Moments_available[DRF2_MOMENT_INDEX] = 1;
    }
  }
}

/**********************************************************************
  Description: Determine if radial data is available to produce given
     product. Return 1 if available, 0 otherwise.
**********************************************************************/

static int Data_is_available( int pindx )
{
  if( pindx == REF_PROD_INDEX &&
      ( Moments_available[REF_MOMENT_INDEX] || Moments_available[DRF2_MOMENT_INDEX] ) )
  {
    return 1;
  }
  else if( pindx == VEL_PROD_INDEX && Moments_available[VEL_MOMENT_INDEX] )
  {
    return 1;
  }
  else if( pindx == SPW_PROD_INDEX && Moments_available[SPW_MOMENT_INDEX] )
  {
    return 1;
  }
  else if( pindx == ZDR_PROD_INDEX && Moments_available[ZDR_MOMENT_INDEX] )
  {
    return 1;
  }
  else if( pindx == PHI_PROD_INDEX && Moments_available[PHI_MOMENT_INDEX] )
  {
    return 1;
  }
  else if( pindx == RHO_PROD_INDEX && Moments_available[RHO_MOMENT_INDEX] )
  {
    return 1;
  }

  return 0;
}

/**********************************************************************
  Description: Get output buffer for given product. Return 0 on error,
     1 otherwise.
**********************************************************************/

static int Get_output_buffer( int pindx )
{
  int ret;

  Prod_info[pindx].outbuf = RPGC_get_outbuf_by_name_for_req(
            Prod_info[pindx].name, Prod_info[pindx].max_prod_size,
            Prod_info[pindx].request, &ret );

  /* Check the status of the operation. */

  if( ret != NORMAL || Prod_info[pindx].outbuf == NULL )
  {
    if( ret != NORMAL )
    {
      sprintf( Msg_buf, "Requested output buffer failed (%d)", ret );
    }
    else
    {
      sprintf( Msg_buf, "Requested output buffer is NULL" );
    }
    RPGC_log_msg( GL_INFO, "%s", Msg_buf );
    return 0;
  }

  return 1;
}

/**********************************************************************
  Description: Cycle through and release input buffers until reaching
     end of elevation or volume. Return 0 on error, 1 otherwise.
**********************************************************************/

static int Release_all_input_buffers()
{
  int radial_status;

  while(1)
  {
    /* Get the next radial in the elevation. */
    if( !Get_input_buffer() ){ return 0; }

    /* Get radial status. */
    radial_status = BD_hdr_ptr->status;

    /* Release buffer. */
    Release_input_buffer();
          
    /* Return at end of volume or elevation. */
    if( (radial_status == GENDEL) || (radial_status == GENDVOL) ){ break; }
  }

  return 1;
}

/**********************************************************************
  Description: Return 1 if product is requested for this elevation,
     return 0 if not.
**********************************************************************/

static int Product_is_requested( int pindx )
{
  if( RPGC_get_request_by_name( Elevation_index, Prod_info[pindx].name,
                                Prod_info[pindx].request, MAX_REQUESTS ) > 0 )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

/**********************************************************************
  Description: Fill data level thresholds in PDB.
**********************************************************************/

static void Fill_data_level_thresholds( int pindx, Graphic_product *phd )
{
  if( pindx == REF_PROD_INDEX )
  {
    phd->level_1 = -320;
    phd->level_2 = 5;
    phd->level_3 = 254;
  }
  else if( pindx == VEL_PROD_INDEX )
  {
    phd->level_1 = -635 * BD_hdr_ptr->dop_resolution;
    phd->level_2 = 5 * BD_hdr_ptr->dop_resolution;
    phd->level_3 = 254;
  }
  else if( pindx == SPW_PROD_INDEX )
  {
    phd->level_1 = 0;
    phd->level_2 = 5;
    phd->level_3 = 40;
  }
  else
  {
    /* Fill Data Levels according to new scale/offset methodology. */

    RPGC_set_product_float( (void *) &phd->level_1, Prod_info[pindx].data_scale );
    RPGC_set_product_float( (void *) &phd->level_3, Prod_info[pindx].data_offset );
    phd->level_6 = (short) DATA_MAX_CHAR;
    phd->level_7 = (short) NUM_LEADING_FLAGS;
    phd->level_8 = (short) 0;
  }
}


/**********************************************************************
  Description: Fill product dependent parameters in PDB.
**********************************************************************/

static void Fill_prod_dep_params( int pindx, Graphic_product *phd )
{
  short params[10] = {0};
  int elev_index;
  float max_value, min_value;

  max_value = Prod_info[pindx].max_data_value - Prod_info[pindx].data_offset;
  max_value /= Prod_info[pindx].data_scale;
  max_value /= Prod_info[pindx].data_precision;

  min_value = Prod_info[pindx].min_data_value - Prod_info[pindx].data_offset;
  min_value /= Prod_info[pindx].data_scale;
  min_value /= Prod_info[pindx].data_precision;

  params[3] = (short) RPGC_NINT( min_value );
  params[4] = (short) RPGC_NINT( max_value );

  if( ( elev_index = ORPGPAT_get_elevation_index( Prod_info[pindx].id ) ) >= 0 )
  {
    params[elev_index] = (short) ( Target_elevation_angle * 10 );
  }

  RPGC_set_dep_params( (char *) phd, params );
}

/**********************************************************************
  Description: Generate timer marker to use in calculating time spans.
**********************************************************************/

static double Get_time_marker()
{
  struct timeval tv;

  gettimeofday( &tv, NULL );
  return (double)( (double)tv.tv_sec + (tv.tv_usec/1000000.0) );
}

/**********************************************************************
  Description: Handle elevation restart by reinitializing variables.
**********************************************************************/

static void Handle_elevation_restart()
{
  int pindx = 0;

  Process_radial = 1;
  for( pindx = FIRST_PROD_INDEX; pindx < NUM_TEST_BASE_PRODS; pindx++ )
  {
    Prod_info[pindx].num_bins = 0;
    Prod_info[pindx].num_radials = 0;
    Prod_info[pindx].data_index = OFFSET_TO_DATA;
    Prod_info[pindx].max_data_value = DATA_MIN_CHAR;
    Prod_info[pindx].min_data_value = DATA_MAX_CHAR;
  }
}

#ifdef PRINT_TEST_PROD
/**********************************************************************
  Description: Print product info to validate/verify contents.
**********************************************************************/

static void Print_product( int pindx )
{
  Graphic_product *phd;
  Symbology_block *sym;
  Packet_16_hdr_t *pkt16;
  int i1, i2, i3;
  float f1, f2;

  phd = (Graphic_product *) Prod_info[pindx].outbuf;
  sym = (Symbology_block *) (Prod_info[pindx].outbuf+GP_SIZE);
  pkt16 = (Packet_16_hdr_t *) (Prod_info[pindx].outbuf+GP_SIZE+SYMB_SIZE);

  printf("\n\nPRODUCT: %s\n",Prod_info[pindx].moment);

  printf("\nPRODUCT DESCRIPTION BLOCK\n");
  /* Header stuff. */
  RPGC_get_product_int( (void *) &phd->msg_time, &i1 );
  RPGC_get_product_int( (void *) &phd->msg_len, &i2 );
  printf("MSG CODE: %d MSG DATE: %d MSG TIME: %d MSG LEN: %d\nSRC_ID: %d DEST_ID: %d N_BLKS: %d\n",
         phd->msg_code,phd->msg_date,i1,i2,phd->src_id,phd->dest_id,phd->n_blocks);
  RPGC_get_product_int( (void *) &phd->latitude, &i1 );
  RPGC_get_product_int( (void *) &phd->longitude, &i2 );
  printf("DIV: %d LAT: %d LON: %d HGT: %d\nPROD CODE: %d OP MODE: %d VCP: %d SEQ: %d\n",
         phd->divider,i1,i2,phd->height,phd->prod_code,phd->op_mode,phd->vcp_num,phd->seq_num);
  RPGC_get_product_int( (void *) &phd->gen_time, &i1 );
  printf("VOL NUM: %d VOL DATE: %d VOL TIME MSB: %d VOL TIME LSB: %d\nGEN DATE: %d GEN TIME: %d ELEV IND: %d\n",
         phd->vol_num,phd->vol_date,phd->vol_time_ms,phd->vol_time_ls,phd->gen_date,i1,phd->elev_ind);

  /* Product dependent stuff. */
  printf("P1: %d P2: %d P3: %d P4: %d P5: %d\nP6: %d P7: %d P8: %d P9: %d P10: %d\n(MIN: %f  MAX: %f)\n",
         phd->param_1,phd->param_2,phd->param_3,phd->param_4,phd->param_5,phd->param_6,phd->param_7,phd->param_8,phd->param_9,phd->param_10,phd->param_4*Prod_info[pindx].data_precision,phd->param_5*Prod_info[pindx].data_precision);

  /* Data level stuff. */
  if( pindx == REF_PROD_INDEX || pindx == VEL_PROD_INDEX || pindx == SPW_PROD_INDEX )
  {
    /* This section is for legacy data level methodology. */
    printf("L1: %d L2: %d L3: %d L4: %d L5: %d L6: %d L7: %d L8: %d\nL9: %d L10: %dL11: %d L12: %d L13: %d L14: %d L15: %d L16: %d\n",
         phd->level_1,phd->level_2,phd->level_3,phd->level_4,phd->level_5,phd->level_6,phd->level_7,phd->level_8,phd->level_9,phd->level_10,phd->level_11,phd->level_12,phd->level_13,phd->level_14,phd->level_15,phd->level_16);
  }
  else
  {
    /* This section is for new scale/offset data level methodology. */
    RPGC_get_product_float( (void *) &phd->level_1, &f1 );
    RPGC_get_product_float( (void *) &phd->level_3, &f2 );
    printf("Scale: %f Offset: %f Max lvl: %d Lead flags: %d End flags: %d\n",
         f1,f2,phd->level_6,phd->level_7,phd->level_8);
  }

  /* Offset stuff. */
  RPGC_get_product_int( (void *) &phd->sym_off, &i1 );
  RPGC_get_product_int( (void *) &phd->gra_off, &i2 );
  RPGC_get_product_int( (void *) &phd->tab_off, &i3 );
  printf("NMAPS: %d SYMOFF: %d GABOFF: %d TABOFF: %d\n",phd->n_maps,i1,i2,i3);

  /* Product Symbology blocks stuff. */
  printf("\nSYMBOLOGY BLOCK\n");
  RPGC_get_product_int( (void *) &sym->block_len, &i1 );
  RPGC_get_product_int( (void *) &sym->data_len, &i2 );
  printf("DIV: %d BLKID: %d BLKLEN: %d NLAY: %d LAYDIV: %d DATALEN: %d\n",
         sym->divider,sym->block_id,i1,sym->n_layers,sym->layer_divider,i2);

  /* Packet 16 header stuff. */
  printf("\nPKT16\n");
  printf("CODE: %d 1STBIN: %d NBIN: %d\nICTR: %d JCTR: %d SCALE: %d NUMRAD: %d\n\n",
         pkt16->code,pkt16->first_bin,pkt16->num_bins,pkt16->icenter,pkt16->jcenter,pkt16->scale_factor,pkt16->num_radials);

}
#endif

