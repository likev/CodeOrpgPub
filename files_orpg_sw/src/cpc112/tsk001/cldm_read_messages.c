/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/09 22:40:07 $
 * $Id: cldm_read_messages.c,v 1.13 2014/12/09 22:40:07 steves Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 *
 */


/* cldm_read_messages.c - This file contains the routines that reads messages
                          from the datastream and extracts data of interest from
                          the message. If the message read is in msg 1 format, 
                          then the msg is converted to msg 31.
*/
                          

#include "cldm.h"
#include <a309.h>
#include <orpggdr.h>


#define MAX_SEQUENCE_NUM     65535
#define	NUM_DATA_BLOCK_PTRS	 9


static int   Recomb_rda_status_updated = 0; /* Recombined RDA status updated 
                                               flag */

   /* function declarations */

static int Convert_to_msg31 (char **buf_ptr, int rda_channel);
static int Convert_to_new_scale (char *inp, char *out, float out_scale, 
                                 float out_offset, unsigned char out_word_size);
static short Get_rda_status (char *buf, uchar rda_conf);
static int Get_recombined_rda_status_msg (char **status_msg, int *byte_cnt); 


/********************************************************************************

    Description: Determine the message header offset based on the data source 
                 (real-time datastream or recombined data)

          Input: *buf          - pointer to the message
                 input_data_id - ID of the data source  LB (global var)

         Output: offset  - offset in bytes from the beginning of the message 
                           to the message header
                 cm_type - the comm manager msg type if a comm manager
                           msg is prepended to the msg. 

         Return: 

           Note: 

 ********************************************************************************/

void RM_determine_msg_hdr_offset (char *buf, int *offset, int *cm_type, 
                                 int input_data_id) {

   int first_word;
   int msg_len;

   if ((input_data_id == ORPGDAT_RECOMBINED_RAWDATA) ||
#ifdef LEVEL2_TOOL
       (input_data_id == CLDM_SR_DEALIASED_DP) ||
#endif
       (input_data_id == CLDM_RECOMBINED_DP)) {
        *offset = CTM_HDRSZE_BYTES;
        *cm_type = CM_DATA;
   } else {
      first_word = *((int *) buf);
      msg_len = (first_word >> 16) & 0xffff;
        
      if (msg_len == 0) {  /* data is real time RDA data */
         *offset = COMMGR_HDR_SIZE + CTM_HDRSZE_BYTES;
         *cm_type = first_word;
      } else {  /* data is playback data (i.e. doesn't have a comm_mgr hdr) */
         *offset = 0;
        *cm_type = CM_DATA;
      }
   }

   return;
}


/********************************************************************************

    Description: Extract the data of interest from the message

          Input: *buf - pointer to the message

         Output: *data - pointer to the message data structure that contains
                         fields-of-interest for processing the message

         Return: 

           Note: 

 ********************************************************************************/

#define SECONDS_PER_DAY       86400
#define MILLISECS_PER_SECOND  1000


void RM_extract_msg_data (char *buf, msg_data_t *data, short previous_rda_status) {

   RDA_RPG_message_header_t *msg;
     static short previous_radial_status = END_VOL;
            short radial_status = -1;
   unsigned short mod_jul_date;
   unsigned long  radial_time;


      /* Extract message header and RDA configuration. */

   msg = (RDA_RPG_message_header_t *) buf;
   memcpy (&data->msg_hdr, msg, sizeof (RDA_RPG_message_header_t));

   #ifdef LITTLE_ENDIAN  /* byte swap msg hdr if necessary */
      UMC_RDAtoRPG_message_header_convert ((char *)&data->msg_hdr);
   #endif

   data->msg_size = data->msg_hdr.size * sizeof (unsigned short);
   data->rda_config = (msg->rda_channel & RDA_RPG_MSG_HDR_ORDA_CFG);

   switch (data->msg_hdr.type) {

      case DIGITAL_RADAR_DATA: {

         RDA_basedata_header *rad_header;
         ushort elevation;

         rad_header = (RDA_basedata_header *) msg;
         radial_status = SHORT_BSWAP_L (rad_header->status);
         data->elev_status.vcp_num = SHORT_BSWAP_L (rad_header->vcp_num);

            /* get the radial time in seconds since the Epoch */

         mod_jul_date = (SHORT_BSWAP_L (rad_header->date)) - 1; /* RDA starts on day 1 instead of day 0 */
         radial_time = INT_BSWAP_L (rad_header->time);
         data->radial_time = (time_t) ((mod_jul_date * SECONDS_PER_DAY) + 
                                       (radial_time / MILLISECS_PER_SECOND));

         elevation = SHORT_BSWAP_L (rad_header->elevation);

         data->elev_status.elev_num = (int) SHORT_BSWAP_L (rad_header->elev_num) - 1;
         data->elev_status.elev = ORPGVCP_get_elevation_angle (data->elev_status.vcp_num | ORPGVCP_RDAVCP,
                                                              data->elev_status.elev_num);
         if (data->elev_status.elev < -90.0)
            data->elev_status.elev = ORPGVCP_BAMS_to_deg (ORPGVCP_ELEVATION_ANGLE,
                                                          elevation);
         data->ldm_msg_type = CLDM_RADIAL_DATA;
      }   
      break;

      case GENERIC_DIGITAL_RADAR_DATA: {
         Generic_basedata_t *rad_header;
         Generic_vol_t *v_hdr = NULL;
         char *tmp_buf;

            /* Copy the radial so we can byte-swap it, then convert to local Endianess. */

         tmp_buf = malloc (data->msg_size);

         if (tmp_buf == NULL) {
            LE_send_msg (GL_ERROR, "malloc failure");
            ORPGTASK_exit (GL_EXIT_FAILURE);
         }

         memcpy (tmp_buf, msg, data->msg_size);

         UMC_RDAtoRPG_message_convert_to_internal (
                               MESSAGE_TYPE_GENERIC_DIGITAL_RADAR_DATA,
                               (char *) tmp_buf);

         rad_header = (Generic_basedata_t *) tmp_buf;
         radial_status = rad_header->base.status;

            /* Get the volume data block so we can extract the VCP number. */ 

         if ((v_hdr = (Generic_vol_t *) ORPGGDR_get_data_block ((char *) tmp_buf,
                                                 ORPGGDR_RVOL)) != NULL)
            data->elev_status.vcp_num = v_hdr->vcp_num;

         else
            LE_send_msg( GL_INFO, "ORPGGDR_get_data_block( ORPGGDR_RVOL ) Failed\n" );

         data->elev_status.elev_num = rad_header->base.elev_num - 1;
         data->elev_status.elev = ORPGVCP_get_elevation_angle (data->elev_status.vcp_num | ORPGVCP_RDAVCP,
                                                data->elev_status.elev_num);

         if (data->elev_status.elev < -90.0)
            data->elev_status.elev = rad_header->base.elevation;

            /* get the radial time in seconds since the Epoch */

         mod_jul_date = rad_header->base.date - 1; /* RDA starts on day 1 instead of day 0 */
         radial_time = rad_header->base.time;
         data->radial_time = (time_t) ((mod_jul_date * SECONDS_PER_DAY) + 
                                       (radial_time / MILLISECS_PER_SECOND));

         data->ldm_msg_type = CLDM_RADIAL_DATA;
         free (tmp_buf);

      }
      break;

      case RDA_STATUS_DATA: 
         data->rda_status = Get_rda_status (buf, data->rda_config);

         if (data->radial_status != END_VOL)
            data->ldm_msg_type = CLDM_RADIAL_DATA;
         else
            data->ldm_msg_type = CLDM_META_DATA;

         if (data->rda_status != RS_OPERATE) {
             previous_radial_status = END_VOL;
             data->radial_status = END_VOL;
         }

      break;

      case PERFORMANCE_MAINTENANCE_DATA:
      case RDA_RPG_VCP:
      case CLUTTER_FILTER_BYPASS_MAP:
      case CLUTTER_MAP_DATA:
      case ADAPTATION_DATA:
         data->ldm_msg_type = CLDM_META_DATA;
      break;

      default:
         data->ldm_msg_type = CLDM_NON_ARCHIVE_2_DATA;
      break;
   }

      /* Do not process any radials that are received after end-of-volume flag is
         set and before beginning-of-volume status is received */

   if ((data->msg_hdr.type == DIGITAL_RADAR_DATA) || 
       (data->msg_hdr.type == GENERIC_DIGITAL_RADAR_DATA)) {
      if ((previous_radial_status == END_VOL && radial_status != BEG_VOL)  || 
          (data->rda_status != RS_OPERATE)) {
         previous_radial_status = END_VOL;
         data->ldm_msg_type = CLDM_NON_ARCHIVE_2_DATA;
      } else {
         previous_radial_status = radial_status;
         data->radial_status = radial_status;
      }
         /* Log beginning of each elevation */

      if ((data->radial_status == BEG_ELEV) || (data->radial_status == BEG_VOL))
         LE_send_msg (GL_INFO, 
                "Beginning of Elevation Cut %d (%4.1f deg) Detected.\n",
                data->elev_status.elev_num + 1, data->elev_status.elev);
   } else {
         /* the following condition can happen when the BEG_VOL radial msg is 
            read then the next msg (i.e. 2nd msg in the volume scan) is a 
            non-radial msg. If this occurs and the radial_status variable is 
            not forced off BEG_VOL, then beginning of volume logic will execute 
            twice which is not desirable */

      if ((previous_radial_status == BEG_VOL) && (data->radial_status == BEG_VOL))
              data->radial_status = INT_ELEV;
   }
   return;
}


/***************************************************************************
  
   Description: Read the next message

         Input: Recomb_rda_status_updated - Recombined RDA status updated flag
                input_data_id             - Data ID of the LB to read from
                rda_channel_flag          - the RDA channel to convert msg 31 to

        Output: **buf - pointer to the message that was read

        Return: bytes_read - the number of bytes read in the message

         Notes: Recomb_rda_status_updated is a global variable that is 
                defined at the beginning of this file.
  
 **************************************************************************/

int RM_read_next_msg (char **buf, int input_data_id, int rda_channel_flag) {

   int ret;
   int bytes_read;
   char *tmp_buf;


      /* If Recombined RDA Status Data Updated Event Received, 
         process it now. */

   if (Recomb_rda_status_updated) {

      Recomb_rda_status_updated = 0;

      ret = Get_recombined_rda_status_msg (buf, &bytes_read);

      if (ret < 0) {
         LE_send_msg (GL_ERROR, "Error reading RPG formatted RDA Status msg");
         return (-1);
      } 

   } else {

         /* Read the next message from the input LB and convert the msg
            to msg 31 format if the data is recombined data */

      ret = ORPGDA_read (input_data_id, (char *)&tmp_buf, LB_ALLOC_BUF, LB_NEXT);

      *buf = tmp_buf;

      if ((ret > 0) && 
          ((input_data_id == ORPGDAT_RECOMBINED_RAWDATA) ||
#ifdef LEVEL2_TOOL
           (input_data_id == CLDM_SR_DEALIASED_DP) ||
#endif
          (input_data_id == CLDM_RECOMBINED_DP)))
         ret = Convert_to_msg31 (buf, rda_channel_flag);

      bytes_read = ret;
   }

   return (bytes_read);
}


/***************************************************************************
  
   Description: Set the recombined RDA Status msg updated flag

         Input: 

        Output: Recomb_rda_status_updated - recombined RDA status updated flag


        Return:

          Note:  Recomb_rda_status_updated is a global variable defined 
                 at the beginning of this file

  
 **************************************************************************/

void RM_set_recomb_rda_status_updated_flag (void) {
      Recomb_rda_status_updated = 1;
   return;
}


/***************************************************************************
  
   Description: Convert internal RPG format to message 31 format.

         Input: *buf_ptr  - The RPG formatted msg buffer

        Output: **buf_ptr - The buffer pointer re-assigned to the msg 31 buffer

        Return: the number of bytes in the converted msg
  
 **************************************************************************/

static int Convert_to_msg31 (char **buf_ptr, int rda_channel) {

   char  *msg31_buf;
   char  *new_msg_buf;
   char  *buf;
   int   msg_size = 0;
   int   temp_size = 0;
   int   no_of_datum = 0;
   int   word_align_bytes = 0;
   int   ref_bytes = 0;
   int   vel_bytes = 0;
   int   spw_bytes = 0;
   int   zdr_bytes = 0;
   int   phi_bytes = 0;
   int   rho_bytes = 0;
   int   dp_bytes = 0;
   int   bytes_per_gate;
   int   waveform;
   int   num_elev_cuts;
   int   include_dref2_dp_data;
   int   cut;
   int   ret;
   int   i;
   float output_scale;
   float output_offset;
   uchar output_word_sz;
   static int sequence_num = 0;
   Moment_t *data = NULL;
   Generic_vol_t    *vol_data;
   Generic_elev_t   *elev_data;
   Generic_rad_t    *rad_data;
   Generic_moment_t *ref_data;
   Generic_moment_t *vel_data;
   Generic_moment_t *spw_data;
   Generic_moment_t *zdr_data = NULL;
   Generic_moment_t *phi_data = NULL;
   Generic_moment_t *rho_data = NULL;
   Base_data_header *m;
   RDA_RPG_message_header_t  *msg_hdr;
   Generic_basedata_header_t *base_hdr;

   buf = *buf_ptr;

      /* create the msg 31 buffer and reserve CTM HDR space */

   new_msg_buf = (char *) calloc ((size_t) MAXLEN_MESSAGE_LEN, 1);

   if (new_msg_buf == NULL) {
      LE_send_msg (GL_ERROR, "calloc failure");
      MA_Abort_Task (NULL);
   }

   msg31_buf = new_msg_buf + CTM_HDRSZE_BYTES;

     /* Cast buf to Base_data_header struct. */

  m = (Base_data_header *) buf;

     /* Fill msg31_buf with message header values. */

  msg_hdr = (RDA_RPG_message_header_t *) &msg31_buf[0];
  msg_hdr->rda_channel = rda_channel;
  msg_hdr->type = MESSAGE_TYPE_GENERIC_DIGITAL_RADAR_DATA;
  msg_hdr->sequence_num = sequence_num;
  msg_hdr->julian_date = m->date;
  msg_hdr->milliseconds = m->time;
  msg_hdr->num_segs = 1;
  msg_hdr->seg_num = 1;
  msg_size += sizeof(RDA_RPG_message_header_t);

     /* Increment sequence counter. */

  sequence_num = ((sequence_num+1)%MAX_SEQUENCE_NUM);

     /* Fill msg31_buf with generic basedata header values. */

  base_hdr = (Generic_basedata_header_t *) &msg31_buf[msg_size];
  strncpy (base_hdr->radar_id, m->radar_name, 4);
  base_hdr->time = m->time;
  base_hdr->date = m->date;
  base_hdr->azi_num = m->azi_num;
  base_hdr->azimuth = m->azimuth;
  base_hdr->compress_type = 0;
  base_hdr->spare_17 = 0;
  base_hdr->azimuth_res = m->azm_reso;
  base_hdr->elev_num = m->elev_num;
  base_hdr->sector_num = m->sector_num;
  base_hdr->elevation = m->elevation;
  base_hdr->spot_blank_flag = m->spot_blank_flag;
  base_hdr->azimuth_index = m->azm_index;
  msg_size += sizeof(Generic_basedata_header_t);

     /* Determine the ICD compliant radial status */

  num_elev_cuts = ORPGVCP_get_num_elevations (m->vcp_num | ORPGVCP_RDAVCP);

     /* Set the status to "good beginning elevation last cut" if the
        VCP is being terminated before all cuts have been generated */

  if ((m->status == GOODBEL) && (m->last_ele_flag == 1) && (m->elev_num < num_elev_cuts))
     base_hdr->status = GOODBELLC;
     
     /* Check for psuedo end-of-elevation, end-of-volume conditions */

  else if ((m->status == PGENDEL) || (m->status == PGENDVOL))
     base_hdr->status = GOODINT;
  else
     base_hdr->status = m->status;

     /* Initialize data block pointers to zero. This will
        be filled in later with actual values. */

  memset (msg31_buf+msg_size, 0, NUM_DATA_BLOCK_PTRS*sizeof(int));
  msg_size += NUM_DATA_BLOCK_PTRS*sizeof(int);

     /* Fill msg31_buf with volume data. */

  vol_data = (Generic_vol_t *) &msg31_buf[msg_size];
  memcpy (vol_data->type, "RVOL", 4);
  vol_data->len = sizeof (Generic_vol_t);
  vol_data->major_version = 1;
  vol_data->minor_version = 0;
  vol_data->lat = m->latitude;
  vol_data->lon = m->longitude;
  vol_data->height = m->height;
  vol_data->feedhorn_height = m->feedhorn_height;
  vol_data->calib_const = m->calib_const;
  vol_data->horiz_shv_tx_power = m->horiz_shv_tx_power;
  vol_data->vert_shv_tx_power = m->vert_shv_tx_power;
  vol_data->sys_diff_refl = m->sys_diff_refl;
  vol_data->sys_diff_phase = m->sys_diff_phase;
  vol_data->vcp_num = m->vcp_num;
  vol_data->sig_proc_states = m->sig_proc_states;
  msg_size += sizeof(Generic_vol_t);

     /* Fill msg31_buf with elevation data. */

  elev_data = (Generic_elev_t *) &msg31_buf[msg_size];
  memcpy (elev_data->type, "RELV", 4);
  elev_data->len = sizeof (Generic_elev_t);
  elev_data->atmos = m->atmos_atten;
  elev_data->calib_const = m->calib_const;
  msg_size += sizeof (Generic_elev_t);

     /* Since this is recombined data, if the radial status is GOODBVOL
        and the elevation number is not 1, set the radial status to
        GOODBEL. */

  if ((base_hdr->status == GOODBVOL) && (base_hdr->elev_num != 1))
     base_hdr->status = GOODBEL;

     /* Fill msg31_buf with radial data. */

  rad_data = (Generic_rad_t *) &msg31_buf[msg_size];
  memcpy (rad_data->type, "RRAD", 4);
  rad_data->len = sizeof (Generic_rad_t);
  rad_data->unamb_range = m->unamb_range;
  rad_data->horiz_noise = m->horiz_noise;
  rad_data->vert_noise = m->vert_noise;
  rad_data->nyquist_vel = m->nyquist_vel;
  rad_data->spare = 0;

  msg_size += sizeof(Generic_rad_t);

     /* Get the waveform type. */

  cut = (int) base_hdr->elev_num - 1;
  waveform = ORPGVCP_get_waveform ((int) vol_data->vcp_num | ORPGVCP_RDAVCP, cut);

     /* if this radial is split cut Continuous Doppler data, remove
        DREF2 and Dual Pol data from the radial */

  if (waveform != VCP_WAVEFORM_CD)
     include_dref2_dp_data = 1;
  else
     include_dref2_dp_data = 0;

     /* Fill msg31_buf with reflectivity data (if present). */

  if ((m->n_surv_bins > 0) && include_dref2_dp_data) {
    ref_data = (Generic_moment_t *) &msg31_buf[msg_size];
    memcpy (ref_data->name, "DREF", 4);
    ref_data->info = 0;
    ref_data->no_of_gates = m->n_surv_bins;
    ref_data->first_gate_range = m->range_beg_surv + (m->surv_bin_size / 2);
    ref_data->bin_size = m->surv_bin_size;
    ref_data->tover = 0;
    ref_data->SNR_threshold = m->surv_snr_thresh;
    ref_data->control_flag = 3;
    ref_data->data_word_size = BYTE_MOMENT_DATA;
    ref_data->scale = 2.0;
    ref_data->offset = 66.0;

    data = (Moment_t *) (buf+m->ref_offset);
    for (i = 0; i < m->n_surv_bins; i++)
      ref_data->gate.b[i] = data[i];

    word_align_bytes = m->n_surv_bins % 4;
    if (word_align_bytes) {
      word_align_bytes = 4 - word_align_bytes;
      memset (&ref_data->gate.b[m->n_surv_bins], 0, word_align_bytes);
    }
       /* Determine number of bytes in data block. */
    ref_bytes = sizeof(Generic_moment_t) + m->n_surv_bins + word_align_bytes;
    msg_size += ref_bytes;
  }

     /* Fill msg31_buf with velocity data (if present). */

  if ((m->n_dop_bins > 0) && (m->vel_offset != 0)) {
    vel_data = (Generic_moment_t *) &msg31_buf[msg_size];
    memcpy (vel_data->name, "DVEL", 4);
    vel_data->info = 0;
    vel_data->no_of_gates = m->n_dop_bins;
    vel_data->first_gate_range = m->range_beg_dop+(m->dop_bin_size / 2);
    vel_data->bin_size = m->dop_bin_size;
    vel_data->tover = m->vel_tover;
    vel_data->SNR_threshold = m->vel_snr_thresh;
    vel_data->control_flag = 1;
    vel_data->data_word_size = BYTE_MOMENT_DATA;
    if (m->dop_resolution == 1){ vel_data->scale = 2.0; }
    else {vel_data->scale = 1.0;}
    vel_data->offset = 129.0;

    data = (Moment_t *) (buf+m->vel_offset);
    for (i = 0; i < m->n_dop_bins; i++) {
      if (data[i] > 255)
        vel_data->gate.b[i] = 255;
      else
        vel_data->gate.b[i] = data[i];
    }

    word_align_bytes = m->n_dop_bins%4;
    if (word_align_bytes) {
      word_align_bytes = 4 - word_align_bytes;
      memset (&vel_data->gate.b[m->n_dop_bins], 0, word_align_bytes);
    }
       /* Determine number of bytes in data block. */

    vel_bytes = sizeof(Generic_moment_t) + m->n_dop_bins + word_align_bytes;
    msg_size += vel_bytes;
  }

     /* Fill msg31_buf with spectrum data (if present). */

  if ((m->n_dop_bins > 0) && (m->spw_offset != 0)) {
    spw_data = (Generic_moment_t *) &msg31_buf[msg_size];
    memcpy (spw_data->name, "DSW ", 4);
    spw_data->info = 0;
    spw_data->no_of_gates = m->n_dop_bins;
    spw_data->first_gate_range = m->range_beg_dop+(m->dop_bin_size / 2);
    spw_data->bin_size = m->dop_bin_size;
    spw_data->tover = m->spw_tover;
    spw_data->SNR_threshold = m->spw_snr_thresh;
    spw_data->control_flag = 1;
    spw_data->data_word_size = BYTE_MOMENT_DATA;
    spw_data->scale = 2.0;
    spw_data->offset = 129.0;

    data = (Moment_t *) (buf+m->spw_offset);
    for (i = 0; i < m->n_dop_bins; i++)
      spw_data->gate.b[i] = data[i];

    word_align_bytes = m->n_dop_bins % 4;
    if (word_align_bytes) {
      word_align_bytes = 4 - word_align_bytes;
      memset (&spw_data->gate.b[m->n_dop_bins], 0, word_align_bytes);
    }
       /* Determine number of bytes in data block. */

    spw_bytes = sizeof(Generic_moment_t) + m->n_dop_bins + word_align_bytes;
    msg_size += spw_bytes;
  }

     /* Process the Dual Pol data */

     /* check for DP moments in the input buffer */

  for (i = 0; i < m->no_moments; i++) {
     Generic_moment_t *gen_moment;

     gen_moment = (Generic_moment_t *)((char *)m + m->offsets[i]);

     if (strncmp (gen_moment->name, "DZDR", 4) == 0) 
        zdr_data = gen_moment;

     else if (strncmp (gen_moment->name, "DPHI", 4) == 0) 
        phi_data = gen_moment;

     else if (strncmp (gen_moment->name, "DRHO", 4) == 0) 
        rho_data = gen_moment;
  }

     /* Re-scale and copy ZDR data */

  if ((zdr_data != NULL) && include_dref2_dp_data) {

     output_scale = 16.0;               /* agreed upon ICD interpretation */
     output_offset = 128.0;             /* agreed upon ICD interpretation */
     output_word_sz = BYTE_MOMENT_DATA; /* agreed upon ICD interpretation */

        /* Rescale the data from RPG to ICD format */

     ret = Convert_to_new_scale ((char *)zdr_data, (char *) &msg31_buf[msg_size],
                                 output_scale, output_offset, output_word_sz);

     if (ret < 0)
        LE_send_msg (GL_ERROR, "Error processing DP moment %4s", zdr_data->name);
     else {

           /* reset the zdr data ptr to the msg 31 buffer */

        zdr_data = (Generic_moment_t *) &msg31_buf[msg_size];

        bytes_per_gate = output_word_sz / BYTE_MOMENT_DATA;
        dp_bytes = zdr_data->no_of_gates * bytes_per_gate;
        word_align_bytes = dp_bytes % 4;

        if (word_align_bytes) {
          word_align_bytes = 4 - word_align_bytes;
          memset (&zdr_data->gate.b[dp_bytes], 0, word_align_bytes);
        }
           /* Determine number of bytes in data block. */

        zdr_bytes = sizeof(Generic_moment_t) + dp_bytes + word_align_bytes;
        msg_size += zdr_bytes;
     }
  }

     /* Re-scale and copy PHI data */

  if ((phi_data != NULL) && include_dref2_dp_data) {

     output_scale = 2.8361;              /* agreed upon ICD interpretation */
     output_offset = 2.0;                /* agreed upon ICD interpretation */
     output_word_sz = SHORT_MOMENT_DATA; /* agreed upon ICD interpretation */

        /* Rescale the data from RPG to ICD format */

     ret = Convert_to_new_scale ((char *)phi_data, (char *)&msg31_buf[msg_size],
                                 output_scale, output_offset, output_word_sz);

     if (ret < 0)
        LE_send_msg (GL_ERROR, "Error processing DP moment %4s", phi_data->name);
     else {

           /* reset the phi data ptr to the msg 31 buffer */

        phi_data = (Generic_moment_t *) &msg31_buf[msg_size];

        bytes_per_gate = output_word_sz / BYTE_MOMENT_DATA;
        dp_bytes = phi_data->no_of_gates * bytes_per_gate;
        word_align_bytes = dp_bytes % 4;

        if (word_align_bytes) {
          word_align_bytes = 4 - word_align_bytes;
          memset (&phi_data->gate.b[dp_bytes], 0, word_align_bytes);
        }
           /* Determine number of bytes in data block. */

        phi_bytes = sizeof(Generic_moment_t) + dp_bytes + word_align_bytes;
        msg_size += phi_bytes;
     }
  }

     /* Re-scale and copy RHO data */

  if ((rho_data != NULL) && include_dref2_dp_data) {

     output_scale = 300.0;              /* agreed upon ICD interpretation */
     output_offset = -60.5;             /* agreed upon ICD interpretation */
     output_word_sz = BYTE_MOMENT_DATA; /* agreed upon ICD interpretation */

        /* Rescale the data from RPG to ICD format */

     ret = Convert_to_new_scale ((char *)rho_data, (char *)&msg31_buf[msg_size],
                                 output_scale, output_offset, output_word_sz);

     if (ret < 0)
        LE_send_msg (GL_ERROR, "Error processing DP moment %4s", rho_data->name);
     else {

           /* reset the rho data ptr to the msg 31 buffer */

        rho_data = (Generic_moment_t *) &msg31_buf[msg_size];

        bytes_per_gate = output_word_sz / BYTE_MOMENT_DATA;
        dp_bytes = rho_data->no_of_gates * bytes_per_gate;
        word_align_bytes = dp_bytes % 4;

        if (word_align_bytes) {
          word_align_bytes = 4 - word_align_bytes;
          memset (&rho_data->gate.b[dp_bytes], 0, word_align_bytes);
        }
           /* Determine number of bytes in data block. */

        rho_bytes = sizeof(Generic_moment_t) + dp_bytes + word_align_bytes;
        msg_size += rho_bytes;
     }
  }

     /* Set variables we didn't know until now. */

  msg_hdr->size = msg_size/sizeof( short );
  base_hdr->radial_length = msg_size - sizeof(RDA_RPG_message_header_t);

     /* Set data block pointers. */

  temp_size = sizeof(Generic_basedata_header_t)+(NUM_DATA_BLOCK_PTRS*sizeof (int));
  base_hdr->data[0] = temp_size;
  temp_size += sizeof (Generic_vol_t);
  base_hdr->data[1] = temp_size;
  temp_size += sizeof (Generic_elev_t);
  base_hdr->data[2] = temp_size;
  temp_size += sizeof (Generic_rad_t);

     /* Reset value to account for variable number of moments. */

  no_of_datum = 3; /* vol, elev, rad */
  if (ref_bytes > 0) {
    base_hdr->data[no_of_datum] = temp_size;
    no_of_datum++;
    temp_size += ref_bytes;
  }

  if (vel_bytes > 0) {
    base_hdr->data[no_of_datum] = temp_size;
    no_of_datum++;
    temp_size += vel_bytes;
  }

  if (spw_bytes > 0) {
    base_hdr->data[no_of_datum] = temp_size;
    no_of_datum++;
    temp_size += spw_bytes;
  }

  if (zdr_bytes > 0) {
    base_hdr->data[no_of_datum] = temp_size;
    no_of_datum++;
    temp_size += zdr_bytes;
  }

  if (phi_bytes > 0) {
    base_hdr->data[no_of_datum] = temp_size;
    no_of_datum++;
    temp_size += phi_bytes;
  }

  if (rho_bytes > 0) {
    base_hdr->data[no_of_datum] = temp_size;
    no_of_datum++;
    temp_size += rho_bytes;
  }

  base_hdr->no_of_datum = no_of_datum;

     /* Byte-swap message header. */

  UMC_RDAtoRPG_message_header_convert (msg31_buf);

     /* Byte-swap rest of message. */

  UMC_RPGtoRDA_message_convert_to_external (GENERIC_DIGITAL_RADAR_DATA, msg31_buf);

     /* free the RPG formatted msg buffer then re-assign it to the msg 31 buffer */

  free (*buf_ptr); 
  *buf_ptr = new_msg_buf;

  return (msg_size);
}


/********************************************************************************

    Description: Converts generic moment data of "inp" to a new generic moment data 
                 of new scale, offset and word size of respectively "out_scale", 
                 "out_offset" and "out_word_size" and puts it in buffer "out". The 
                 "out" buffer must be sufficiently large to hold the new generic 
                 moment and the pointer of "out" must be 4-byte-aligned.

          Input: *inp          - internally formatted generic moment data
                 out_scale     - scaling factor to use to convert the generic data
                 out_offset    - the offset to use to convert the generic data
                 out_word_size - the data size (8 bit or 16 bit) to convert the 
                                 data to

         Output: *out - msg 31 data converted from the internally formatted 
                        generic data

         Return: size of the output moment on success, or -1 on error.


    Note: This routine only processes 8 bit or 16 bit word sizes.

********************************************************************************/

static int Convert_to_new_scale (char *inp, char *out, float out_scale, 
                                 float out_offset, unsigned char out_word_size) {
                                 
    Generic_moment_t *gmin, *gmout;
    int upper_bndry, lower_bndry, i;
    float one_over_iscale;
    unsigned short *isp, *osp;
    unsigned char *ibp, *obp;

    gmin = (Generic_moment_t *)inp;
    gmout = (Generic_moment_t *)out;
    memcpy (out, inp, sizeof (Generic_moment_t));

    if ((gmin->scale == out_scale && gmin->offset == out_offset && 
                                gmin->data_word_size == out_word_size) 
                ||
        (gmin->scale == 0.0))   {

            /* no rescaling is necessary - copy data */
            
        if (gmin->data_word_size == 16) {
            memcpy (gmout->gate.u_s, gmin->gate.u_s, 
                    gmin->no_of_gates * sizeof (unsigned short));
            return ((char *)(gmout->gate.u_s + gmout->no_of_gates) - (char *)gmout);
        }
        else if (gmin->data_word_size == 8) {
            memcpy (gmout->gate.b, gmin->gate.b, 
                    gmin->no_of_gates * sizeof (unsigned char));
            return ((char *)(gmout->gate.b + gmout->no_of_gates) - (char *)gmout);
        }
        else 
            return (-1);
    }

    gmout->scale = out_scale;
    gmout->offset = out_offset;
    gmout->data_word_size = out_word_size;

       /* initialize the input pointer based on data type */

    isp = NULL;
    ibp = NULL;

    if (gmin->data_word_size == 16)
        isp = gmin->gate.u_s;
    else if (gmin->data_word_size == 8)
        ibp = gmin->gate.b;
    else
        return (-1);

       /* initialize the output pointer based on data type */

    osp = NULL;
    obp = NULL;

    if (gmout->data_word_size == 16)
        osp = gmout->gate.u_s;
    else if (gmout->data_word_size == 8)
        obp = gmout->gate.b;
    else
        return (-1);

       /* set the upper and lower boundaries based on data type */

    if (out_word_size == 16) {
        upper_bndry = 0xffff;
        lower_bndry = 2;
    } else {
        upper_bndry = 0xff;
        lower_bndry = 2;
    }

    one_over_iscale = 1.f / gmin->scale;

    for (i = 0; i < gmin->no_of_gates; i++) {
        int odata, idata;
        float f, v;

        if (isp != NULL)
            idata = isp[i];     /* scaled input data */
        else
            idata = ibp[i];

        if (idata <= 1) {       /* special values */
            if (osp != NULL)
                osp[i] = idata;
            else
                obp[i] = idata;
            continue;
        }

        v = (idata - gmin->offset) * one_over_iscale; /* float value */
        f = v * out_scale + out_offset; /* scaled output value */

        if (f >= 0.f)                   /* rounding */
            odata = (int)(f + .5f);
        else
            odata = (int)(-(-f + .5f));

        if (odata > upper_bndry)        /* saturate at value boundaries */
            odata = upper_bndry;
        if (odata < lower_bndry)
            odata = lower_bndry;

        if (osp != NULL)                /* convert to output type */
            osp[i] = (unsigned short)odata;
        else
            obp[i] = (unsigned char)odata;
    }

    if (gmout->data_word_size == 16)
        return ((char *)(gmout->gate.u_s + gmout->no_of_gates) - (char *)gmout);
    else
        return ((char *)(gmout->gate.b + gmout->no_of_gates) - (char *)gmout);
}


/********************************************************************************

    Description: Get RDA status from the RDA Status msg

          Input: *buf     - pointer to the message
                 rda_conf - RDA configuration (ORDA or legacy)

         Output: None

         Return: rda_status - status of the RDA

           Note:

 ********************************************************************************/

static short Get_rda_status (char *buf, uchar rda_conf) {

   short rda_status;
   
   if (!rda_conf) {
         /* Legacy */
      RDA_status_msg_t *status_l = (RDA_status_msg_t *) buf;
      rda_status = SHORT_BSWAP_L(status_l->rda_status);
   } else {
        /* ORDA */
      ORDA_status_msg_t *status_o = (ORDA_status_msg_t *) buf;
      rda_status = SHORT_BSWAP_L(status_o->rda_status);
   }
   return (rda_status);
}


/********************************************************************************

    Description: This routine is called when a RDA Status Update Event 
                 Notification is received when processing recombined data

          Input: None

         Output: *status_msg - pointer to the RDA Status msg
                 byte_cnt    - The number of bytes read 

         Return: 0 on success; -1 on error
   
 ********************************************************************************/

static int Get_recombined_rda_status_msg (char **status_msg, int *byte_cnt) {
   char *tmp_buf;
   char *msg_ptr;

      /* Read the RDA Status msg from the LB */

   *byte_cnt = ORPGDA_read (ORPGDAT_RDA_STATUS, &tmp_buf, LB_ALLOC_BUF, 
                            RDA_STATUS_ID);
   
   if (*byte_cnt <= 0) {
      *status_msg = NULL;
      return (-1);

   } else { 
        /* Adjust the msg size for the RPG specific info at the beginning
           of the msg and alloc mem space for the msg and CTM_HDR */

      *byte_cnt -= sizeof (RDA_RPG_comms_status_t);
      *status_msg = calloc ((size_t)(*byte_cnt + CTM_HDRSZE_BYTES), 1);

      if (*status_msg == NULL) {
         LE_send_msg (GL_ERROR, "calloc failure");
         MA_Abort_Task (NULL);
      }

         /* Copy the msg to the status msg buffer and free the tmp buffer */

      memcpy (*status_msg + CTM_HDRSZE_BYTES, 
              tmp_buf + sizeof (RDA_RPG_comms_status_t), *byte_cnt);

      free (tmp_buf);

         /* byte swap msg if necessary */

      #ifdef LITTLE_ENDIAN  
         msg_ptr = *status_msg + CTM_HDRSZE_BYTES;
         UMC_RDAtoRPG_message_header_convert ((char *)msg_ptr);
         UMC_RPGtoRDA_message_convert_to_external(RDA_STATUS_DATA, (char *)msg_ptr);
      #endif
   }
   return (0);
}

