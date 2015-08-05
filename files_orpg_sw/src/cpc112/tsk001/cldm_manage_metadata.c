/* 
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2014/09/02 20:44:10 $
 * $Id: cldm_manage_metadata.c,v 1.4 2014/09/02 20:44:10 garyg Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 *
 *
 */


/* cldm_manage_metadata.c - This file contains the routines to store the Metadata
                            messages in the Metadata buffer and write the 
                            Metadata buffer out to the LDM queue as a LDM record
                            when required */


#include "cldm.h"


/* Max # of segments defined per Metadata message (Archive II ICD defined) */

#define MSG_2_MAX_SEGS               1
#define MSG_3_MAX_SEGS               1
#define MSG_5_MAX_SEGS               1
#define MSG_13_MAX_SEGS             49
#define MSG_15_MAX_SEGS             77
#define MSG_18_MAX_SEGS              5

#define LDM_MSG_SEG_LENGTH        2432  /* Arch II ICD Metadata msg segment len */
#define NUMBER_METADATA_ICD_MSGS     6  /* # of Arch II ICD defined Metadata msgs */
#define RDA_RPG_MAX_MSG_SEG_SIZE  2400  /* max rda-rpg msg size (excl. msg hdr) */

/* 134 msgs @ 2432 bytes/msg */

#define LDM_META_DATA_SIZE    LDM_MSG_SEG_LENGTH * (MSG_2_MAX_SEGS  +  \
                                                    MSG_3_MAX_SEGS  +  \
                                                    MSG_5_MAX_SEGS  +  \
                                                    MSG_13_MAX_SEGS +  \
                                                    MSG_15_MAX_SEGS +  \
                                                    MSG_18_MAX_SEGS)

/* define the metadata msgs offsets in the metadata buffer.
   The following order and offsets are ICD defined */

#define MSG_15_BUFFER_OFFSET  0
#define MSG_13_BUFFER_OFFSET  LDM_MSG_SEG_LENGTH *  MSG_15_MAX_SEGS
#define MSG_18_BUFFER_OFFSET  LDM_MSG_SEG_LENGTH * (MSG_13_MAX_SEGS +  \
                                                    MSG_15_MAX_SEGS)
#define MSG_3_BUFFER_OFFSET   LDM_MSG_SEG_LENGTH * (MSG_13_MAX_SEGS +  \
                                                    MSG_15_MAX_SEGS +  \
                                                    MSG_18_MAX_SEGS)
#define MSG_5_BUFFER_OFFSET   LDM_MSG_SEG_LENGTH * (MSG_3_MAX_SEGS +   \
                                                    MSG_13_MAX_SEGS +  \
                                                    MSG_15_MAX_SEGS +  \
                                                    MSG_18_MAX_SEGS)
#define MSG_2_BUFFER_OFFSET   LDM_MSG_SEG_LENGTH * (MSG_3_MAX_SEGS +   \
                                                    MSG_5_MAX_SEGS +   \
                                                    MSG_13_MAX_SEGS +  \
                                                    MSG_15_MAX_SEGS +  \
                                                    MSG_18_MAX_SEGS)

/* this is the static buffer used to store the Metadata messages
   in Archive II/User ICD format */

static char Metadata_buffer [LDM_META_DATA_SIZE];

/* Array that identifies the RDA-RPG msgs by msg type.
   Note: This array is order specific and the Metadata logic 
         will break if the order does not match the Archive II
         Metadata msg order */

static int Metadata_msg_types [NUMBER_METADATA_ICD_MSGS] = 
                                            {CLUTTER_MAP_DATA, 
                                             CLUTTER_FILTER_BYPASS_MAP, 
                                             ADAPTATION_DATA,
                                             PERFORMANCE_MAINTENANCE_DATA,
                                             RDA_RPG_VCP,
                                             RDA_STATUS_DATA};


/* msg type segment counters - used for validation and accounting */

static int Metadata_msgs_seg_cntrs [NUMBER_METADATA_ICD_MSGS] = 
                                                  {0, 0, 0, 0, 0, 0};
static int Num_msg_segs_defined [NUMBER_METADATA_ICD_MSGS] = 
                                                  {0, 0, 0, 0, 0, 0};


/* Local function declarations */

static void Add_msg_to_metadata_buf (int msg_idx, int rda_conf);
static int  Byte_swap_meta_data (int type, int len, char *buf);
static void Clear_metadata_msg_block (ushort msg_type);
static int  Compact_the_msg (char *buf, int msg_type, int bytes_read, int msg_offset);
static int  Compute_number_of_msg_segs (int msg_size);
static void Copy_data_to_buffer (char *msg_buf, RDA_RPG_message_header_t msg_hdr,
                                 int num_segs, int msg_len, int msg_type);
static int  Get_max_msg_segs (int msg_type);
static int  Get_msg_offset (int msg_type, int msgid);
static void Validate_metadata (int rda_config);
static int  Validate_msg (RDA_RPG_message_header_t msg_hdr, int msg_type, int msg_len);


/********************************************************************************

    Description: Clear the Metadata buffer

           Input:

          Output: Metadata_buffer - the static buffer containing the 
                                    Metadata messages

         Return: 

           Note: Metadata_buffer is globally defined at the beginning of 
                                 this file.
   
 ********************************************************************************/

void MM_clear_metadata_buf (void) {

   int i;

   memset (Metadata_buffer, 0, LDM_META_DATA_SIZE);

      /* reset the msg segment arrays */

   for (i = 0; i < NUMBER_METADATA_ICD_MSGS; i++) {
         Metadata_msgs_seg_cntrs [i] = 0;
         Num_msg_segs_defined [i] = 0;
   }

   return;
}


/********************************************************************************

    Description: Copy all real-time datastream Metadata to the Metadata buffer
                 (ie. non-recombined metadata)

          Input: *msg_buf - buffer pointer to the metadata msg
                 msg_data - message structure containing various message
                            properties
                 ldm_ver  - the current LDM version number

         Output: Metadata_buffer - the metadata buffer updated with the
                                   metadata message

         Return: 

           Note: Metadata_buffer is globally defined at the beginning of 
                                 this file.
   

 ********************************************************************************/

void MM_process_metadata_msg (char *msg_buf, msg_data_t msg_data, int ldm_ver) {

   int buffer_offset = 0;
   int msg_type_offset = 0;
   int msg_upper_boundary = 0;
   int max_num_msg_segs_allowed;
   int msg_len;
   int i;

      /* when the first msg segment is received for each metadata msg type,
         clear the msg block in the metadata buffer for the msg */

   if (msg_data.msg_hdr.seg_num == 1) {
      Clear_metadata_msg_block (msg_data.msg_hdr.type);
      LE_send_msg (GL_INFO, 
                   "Writing real-time stream Metadata msg %d to the Metadata buffer",
                   msg_data.msg_hdr.type);
   }

   switch (msg_data.msg_hdr.type) {

      case RDA_STATUS_DATA:
         msg_type_offset = MSG_2_BUFFER_OFFSET;
         msg_upper_boundary = MSG_2_BUFFER_OFFSET + 
                              (MSG_2_MAX_SEGS * LDM_MSG_SEG_LENGTH);
         max_num_msg_segs_allowed = MSG_2_MAX_SEGS;
      break;

      case PERFORMANCE_MAINTENANCE_DATA:
         msg_type_offset = MSG_3_BUFFER_OFFSET;
         msg_upper_boundary = MSG_3_BUFFER_OFFSET + 
                              (MSG_3_MAX_SEGS * LDM_MSG_SEG_LENGTH);
         max_num_msg_segs_allowed = MSG_3_MAX_SEGS;
      break;

      case RDA_RPG_VCP:
         msg_type_offset = MSG_5_BUFFER_OFFSET;
         msg_upper_boundary = MSG_5_BUFFER_OFFSET + 
                              (MSG_5_MAX_SEGS * LDM_MSG_SEG_LENGTH);
         max_num_msg_segs_allowed = MSG_5_MAX_SEGS;
      break;

      case CLUTTER_FILTER_BYPASS_MAP:
         msg_type_offset = MSG_13_BUFFER_OFFSET; 
         msg_upper_boundary = MSG_13_BUFFER_OFFSET + 
                              (MSG_13_MAX_SEGS * LDM_MSG_SEG_LENGTH);
         max_num_msg_segs_allowed = MSG_13_MAX_SEGS;
      break;

      case CLUTTER_MAP_DATA:
         msg_type_offset = MSG_15_BUFFER_OFFSET; 
         msg_upper_boundary = MSG_15_BUFFER_OFFSET + 
                              (MSG_15_MAX_SEGS * LDM_MSG_SEG_LENGTH);
         max_num_msg_segs_allowed = MSG_15_MAX_SEGS;
      break;

      case ADAPTATION_DATA:
         msg_type_offset = MSG_18_BUFFER_OFFSET;
         msg_upper_boundary = MSG_18_BUFFER_OFFSET + 
                              (MSG_18_MAX_SEGS * LDM_MSG_SEG_LENGTH);
         max_num_msg_segs_allowed = MSG_18_MAX_SEGS;
      break;

      default:
         LE_send_msg (GL_ERROR, 
            "Attempted to add non-Metadata msg (msg_type: %d) to Metadata buffer",
            msg_data.msg_hdr.type);
         return;
      break;
   }

      /* if more msg segments are received than are reserved in the 
         Metadata buffer, then reject the whole msg */

   if (msg_data.msg_hdr.seg_num > max_num_msg_segs_allowed) {
      LE_send_msg (GL_ERROR, 
         "LDM: Too many msg segments read (%d) for msg %d...rejecting msg",
         msg_data.msg_hdr.seg_num, msg_data.msg_hdr.type);
      LE_send_msg (GL_ERROR, "...%d segments allowed for msg %d; %d segments received",
         max_num_msg_segs_allowed, msg_data.msg_hdr.type, msg_data.msg_hdr.seg_num);
      Clear_metadata_msg_block (msg_data.msg_hdr.type);
      return;
   }

      /* increment the segment counter for this msg type */

   for (i = 0; i < NUMBER_METADATA_ICD_MSGS; i++) {
      if (Metadata_msg_types [i] == msg_data.msg_hdr.type) {
         ++Metadata_msgs_seg_cntrs [i];
         if (msg_data.msg_hdr.seg_num == 1)
            Num_msg_segs_defined [i] = msg_data.msg_hdr.num_segs;
         break;
      }
   }   

   buffer_offset = ((msg_data.msg_hdr.seg_num - 1) * LDM_MSG_SEG_LENGTH) +
                     msg_type_offset;
   msg_len = msg_data.msg_size;

      /* perform msg boundary validation */

   if ((buffer_offset + msg_len + CTM_HDRSZE_BYTES) > msg_upper_boundary) {
      int temp;

      LE_send_msg (GL_ERROR, 
         "Metadata msg buffer space violation (msg_type: %d;  seg #: %d; msg_len: %d)",
         msg_data.msg_hdr.type, msg_data.msg_hdr.seg_num, msg_len);
      LE_send_msg (GL_ERROR,
          "...buffer space allocated: %d bytes;  buffer space required: %d bytes",
                 msg_upper_boundary, (buffer_offset + msg_len + CTM_HDRSZE_BYTES));
      temp = (buffer_offset + msg_len + CTM_HDRSZE_BYTES) - msg_upper_boundary;
      msg_len -= temp;
      LE_send_msg (GL_ERROR, 
          "...clipping msg len to preserve boundaries");

      if (msg_len <= 0)
         return;
   }

      /* copy msg to the metadata buffer */

   memcpy (Metadata_buffer + buffer_offset + CTM_HDRSZE_BYTES, msg_buf, msg_len);

   return;
}


/********************************************************************************

    Description: Write the Metadata buffer to the LDM queue (or local file)

          Input: radial_stat - the radial status
                 rda_config  - the RDA configuration
                 ldm_ver     - the current LDM version number
                 Metadata_buffer - the metadata buffer

         Output: 

         Return: 

           Note: Metadata_buffer is globally defined at the beginning of 
                                 this file.
   

 ********************************************************************************/

 void MM_write_metadata (msg_data_t *msg_data, int ldm_ver) {

   Validate_metadata (msg_data->rda_config);
   WR_process_record (Metadata_buffer, LDM_META_DATA_SIZE, msg_data, ldm_ver);

   return;
}


/********************************************************************************

    Description: Read the Metadata msg/file from the applicable LB and
                 copy it to the Metadata buffer. This routine is usually called
                 when processing recombined data but it can be called anytime the 
                 Metadata_buffer has not been populated with a particular
                 Metadata message

          Input: msg_idx     - the index to the message arrays
                 rda_config  - the RDA configuration

         Output: Metadata_buffer - the metadata buffer

         Return: 

           Note: Metadata_buffer is globally defined at the beginning of 
                                 this file.
   

 ********************************************************************************/

static void Add_msg_to_metadata_buf (int msg_idx, int rda_conf) {
   int  msg_id;
   int  ret;
   int  msg_offset;
   int  bytes_read;          /* # bytes read from the LB */
   int  msg_data_len;        /* len of msg w/extra data stripped (msg hdr, etc.) */
   int  max_num_msg_segs;    /* Max # of msg segments IAW the Level II ICD */
   int  actual_num_msg_segs; /* # of msg segments computed from the msg len */
   int  msg_type;
   char *tmp_buf = NULL;
   int  msg_hdr_size = sizeof(RDA_RPG_message_header_t);
   RDA_RPG_message_header_t msg_hdr ;
   int rda_data_ids [NUMBER_METADATA_ICD_MSGS] = 
                                       {ORPGDAT_CLUTTERMAP,
                                        ORPGDAT_CLUTTERMAP,
                                        ORPGDAT_RDA_ADAPT_DATA,
                                        ORPGDAT_RDA_PERF_MAIN,
                                        ORPGDAT_RDA_VCP_DATA,
                                        ORPGDAT_RDA_STATUS};

   int  orda_msg_ids [NUMBER_METADATA_ICD_MSGS] = 
                                        {LBID_CLUTTERMAP_ORDA,
                                         LBID_BYPASSMAP_ORDA,
                                         ORPGDAT_RDA_ADAPT_MSG_ID,
                                         LB_NEXT,
                                         ORPGDAT_RDA_VCP_MSG_ID,
                                         RDA_STATUS_ID};

   int  legacy_msg_ids [NUMBER_METADATA_ICD_MSGS] = 
                                        {LBID_CLUTTERMAP_LGCY,
                                         LBID_BYPASSMAP_LGCY,
                                         ORPGDAT_RDA_ADAPT_MSG_ID,
                                         LB_NEXT,
                                         ORPGDAT_RDA_VCP_MSG_ID,
                                         RDA_STATUS_ID};

   if (rda_conf == 0) { /* this is a legacy RDA */
      msg_id = legacy_msg_ids [msg_idx];
   } else {    /* this is an ORDA */
      msg_id = orda_msg_ids [msg_idx];
   }

   msg_type = Metadata_msg_types [msg_idx];

   if (msg_type == PERFORMANCE_MAINTENANCE_DATA)
       ORPGDA_seek (ORPGDAT_RDA_PERF_MAIN, 0, LB_LATEST, NULL);

      /* read the data from the LB */

   bytes_read = ORPGDA_read (rda_data_ids [msg_idx], &tmp_buf, LB_ALLOC_BUF, msg_id);

   if (bytes_read <= 0) {
      LE_send_msg (GL_ERROR, 
            "Error reading metadata LB (data_id: %d; msg_id: %d;  ret: %d)",
            rda_data_ids [msg_idx], msg_id, bytes_read);
      return;
   }

     /* several RPG formatted msgs have fields added to the RDA ICD msg
        and these extra fields must be stripped off, so get the offset
        to the actual msg */

   msg_offset = Get_msg_offset (msg_type, msg_id);

      /* preserve the msg hdr, then byte swap and compact the metadata msg */

   memcpy ((char *) &msg_hdr, tmp_buf + msg_offset, msg_hdr_size);

   
      /* validate the metadata msg read from the RPG LB */

   ret = Validate_msg (msg_hdr, msg_type, bytes_read - msg_offset);

   if (ret < 0) {
      LE_send_msg (GL_ERROR, "LDM: Metadata msg %d not processed",
                   msg_type);
      Clear_metadata_msg_block (msg_type);
      free (tmp_buf);
      return;
   }

      /* compute the msg data len - exclude the msg hdr and msg offsets */

   msg_data_len = bytes_read - msg_offset - msg_hdr_size;

   if (msg_data_len <= 0) {
      LE_send_msg (GL_ERROR, 
         "Invalid msg len (%d) for Metadata msg type %d - msg not added to Metadata buffer",
         msg_data_len, msg_type);
      free (tmp_buf);
      return;
   }

      /* byte swap the data before compacting the msg */

   #ifdef LITTLE_ENDIAN
      ret = Byte_swap_meta_data (msg_type, msg_data_len, tmp_buf + msg_offset);
      if (ret < 0) {
          LE_send_msg (GL_ERROR, "Error Byte swapping metadata (msg type: %d)", 
                       msg_type);
          free (tmp_buf);
          return;
      }
   #endif

      /* compact the msg data and get the msg length after stripping the msg hdr,
         extra structures, etc. from the beginning of the msg data */
         
   msg_data_len = Compact_the_msg (tmp_buf, msg_type, bytes_read, msg_offset);

      /* find the actual and max number of msg segments */

   actual_num_msg_segs = Compute_number_of_msg_segs (msg_data_len);

   max_num_msg_segs = Get_max_msg_segs (msg_type);

   if (max_num_msg_segs <= 0) {
      LE_send_msg (GL_ERROR, 
         "Error obtaining max number Metadata msg segments for msg type %d",
          msg_type);
      LE_send_msg (GL_ERROR, "...Msg Type %d being discarded", msg_type);
      free (tmp_buf);
      return;
   }

   if (actual_num_msg_segs > max_num_msg_segs) {
      LE_send_msg (GL_ERROR, 
         "# of computed msg segs (%d) > max # segs allowed (%d) - msg type %d clipped",
         actual_num_msg_segs, max_num_msg_segs, msg_type);
      actual_num_msg_segs = max_num_msg_segs;
   }

      /* copy the msg to the metadata buffer */

   Copy_data_to_buffer (tmp_buf, msg_hdr, actual_num_msg_segs, msg_data_len,
                        msg_type);

      /* update the msg segment counters array for this msg type */

   Metadata_msgs_seg_cntrs [msg_idx] = actual_num_msg_segs;

   LE_send_msg (GL_INFO, 
      "Reading msg type %d (byte size: %d;  # segments: %d) from LB %d",
      msg_type, msg_data_len, actual_num_msg_segs, rda_data_ids [msg_idx]);

   free (tmp_buf);

   return;
}


/********************************************************************************

    Description: Byte swap the Metadata message

          Input: msg_type    - the RDA-RPG ICD msg type
                 len         - the msg length
                 *buf        - pointer to the msg buffer

         Output: 

         Return: 0 on success; -1 on error

 ********************************************************************************/

 static int Byte_swap_meta_data(int type, int len, char *buf) {
   int len_hw;
   int ret_val = 0;

   switch (type) {

      case ADAPTATION_DATA:
         UMC_RPGtoRDA_message_convert_to_external( type, buf);
      break;

      case CLUTTER_MAP_DATA:
      case CLUTTER_FILTER_BYPASS_MAP:

            /* swap on short boundaries. Note that we need to swap only the 
               data, not the header. */

         len_hw = len / sizeof (short);

         if (len % sizeof (short) != 0)
            ++len_hw;

         MISC_swap_shorts(len_hw, (short *)(buf + sizeof(RDA_RPG_message_header_t)));
      break;

      case RDA_STATUS_DATA:
         UMC_RPGtoRDA_message_convert_to_external( type, buf);
      break;

      case PERFORMANCE_MAINTENANCE_DATA:
         UMC_RPGtoRDA_message_convert_to_external( type, buf);
      break;

      case RDA_RPG_VCP:
         UMC_RPGtoRDA_message_convert_to_external( type, buf);
      break;

      default:
         ret_val = -1;
      break;
   }

   return (ret_val);
}


/********************************************************************************

    Description: Clear the reserved space in the Metadata buffer for the 
                 Metadata message

          Input: msg_type - the RDA-RPG ICD msg type

         Output: 

         Return: 

 ********************************************************************************/

static void Clear_metadata_msg_block (ushort msg_type) {
   int i;

/*   LE_send_msg (GL_INFO, "Clearing Metadata msg block for msg type %d", msg_type); */

   switch (msg_type) {

      case RDA_STATUS_DATA:
         memset (Metadata_buffer + MSG_2_BUFFER_OFFSET, 0, MSG_2_MAX_SEGS *
                 LDM_MSG_SEG_LENGTH);
      break;

      case PERFORMANCE_MAINTENANCE_DATA:
         memset (Metadata_buffer + MSG_3_BUFFER_OFFSET, 0, MSG_3_MAX_SEGS *
                 LDM_MSG_SEG_LENGTH);
      break;

      case RDA_RPG_VCP:
         memset (Metadata_buffer + MSG_5_BUFFER_OFFSET, 0, MSG_5_MAX_SEGS *
                 LDM_MSG_SEG_LENGTH);
      break;

      case CLUTTER_FILTER_BYPASS_MAP:
         memset (Metadata_buffer + MSG_13_BUFFER_OFFSET, 0, MSG_13_MAX_SEGS *
                 LDM_MSG_SEG_LENGTH);
      break;

      case CLUTTER_MAP_DATA:
         memset (Metadata_buffer + MSG_15_BUFFER_OFFSET, 0, MSG_15_MAX_SEGS *
                 LDM_MSG_SEG_LENGTH);
      break;

      case ADAPTATION_DATA:
         memset (Metadata_buffer + MSG_18_BUFFER_OFFSET, 0, MSG_18_MAX_SEGS *
                 LDM_MSG_SEG_LENGTH);
      break;
   }

      /* reset the msg segment arrays */

   for (i = 0; i < NUMBER_METADATA_ICD_MSGS; i++) {
      if (Metadata_msg_types [i] == msg_type) {
         Metadata_msgs_seg_cntrs [i] = 0;
         Num_msg_segs_defined [i] = 0;
         break;
      }
   }   

   return;
}


/********************************************************************************

    Description: Compact the msg by stripping the RPG specific data and msg header
                 from the beginning of the msg buffer

          Input: *buf       - pointer to the msg buffer
                 msg_type   - the RDA-RPG ICD msg type
                 bytes_read - # bytes read from the LB
                 hdr_offset - the byte offset from the beginning of the msg buffer
                              to the beginning of the msg header
         Output: 

         Return: msg_size - the byte size of the msg after compaction

 ********************************************************************************/

static int Compact_the_msg (char *buf, int msg_type, int bytes_read, int hdr_offset) {
   int msg_size;
   int msg_hdr_size = sizeof(RDA_RPG_message_header_t);

      /* compact the msg to the beginning of the buffer by stripping the msg
         header and any extra data that is not part of the Metadata msg */

   msg_size = bytes_read - msg_hdr_size - hdr_offset;

   if (msg_size <= 0) {
      LE_send_msg (GL_ERROR, 
         "Metadata msg size (%d) for msg type (%d) < msg_hdr + computed offset (%d)",
         msg_size, msg_type, msg_hdr_size + hdr_offset);
      return (-1);
   }   

   memmove (buf, buf + msg_hdr_size + hdr_offset, msg_size);

      /* clear the unused section of the msg buffer */

   memset (buf + msg_size, 0, bytes_read - msg_size);

   return (msg_size);
}


/********************************************************************************

    Description: Compute the number of msg segments using the msg size and the
                 max number of bytes in a RDA-RPG ICD msg (excluding the msg hdr)

          Input: msg_size - the msg size in bytes

         Output: 

         Return: number_of_segs - the number of segments computed

 ********************************************************************************/

static int Compute_number_of_msg_segs (int msg_size) {

   int number_of_segs = 0;

   number_of_segs = msg_size / RDA_RPG_MAX_MSG_SEG_SIZE;

   if ((msg_size % RDA_RPG_MAX_MSG_SEG_SIZE) != 0)
      ++number_of_segs;

   return (number_of_segs);
}


/********************************************************************************

    Description: Add msg headers to each msg segment and copy the msg to 
                 the Metadata buffer 

          Input: *msg_buf - pointer to the msg buffer
                 msg_hdr  - the msg header obtained from the msg when read from
                            the LB. This header is used to construct the other
                            msg headers for each msg segment
                 num_segs - number of msg segments computed for this msg
                 msg_len  - msg length in bytes
                 msg_type - the RDA-RPG ICD msg type

         Output: Metadata_buffer - the buffer that stores all the Metadata msgs

         Return:

           Note: Metadata_buffer is globally defined at the beginning of 
                                 this file.

 ********************************************************************************/

static void Copy_data_to_buffer (char *msg_buf, RDA_RPG_message_header_t msg_hdr,
                                 int num_segs, int msg_len, int msg_type) {
   int  i;
   int  metadata_buf_offset;
   int  msg_buf_offset = 0;
   char *msg_seg = NULL;
   int  msg_seg_hw_size;   /* msg seg size in H/Ws including the msg hdr */
   int  msg_seg_byte_size; /* msg seg size in bytes excluding the msg hdr */
   int  msg_hdr_size = sizeof (RDA_RPG_message_header_t);

   switch (msg_type) {
      case RDA_STATUS_DATA:
         metadata_buf_offset = MSG_2_BUFFER_OFFSET;
      break;

      case PERFORMANCE_MAINTENANCE_DATA:
         metadata_buf_offset = MSG_3_BUFFER_OFFSET;
      break;

      case RDA_RPG_VCP:
         metadata_buf_offset = MSG_5_BUFFER_OFFSET;
      break;

      case CLUTTER_FILTER_BYPASS_MAP:
         metadata_buf_offset = MSG_13_BUFFER_OFFSET; 
      break;

      case CLUTTER_MAP_DATA:
         metadata_buf_offset = MSG_15_BUFFER_OFFSET; 
      break;

      case ADAPTATION_DATA:
         metadata_buf_offset = MSG_18_BUFFER_OFFSET;
      break;

      default:
         LE_send_msg (GL_ERROR, 
             "Attempting to copy invalid msg type (%d) to Metadata buffer",
             msg_type);
          return;
       break;
   }

      /* pad each msg segment with a CTM hdr */

   metadata_buf_offset += CTM_HDRSZE_BYTES;

      /* check the number of msg segments specified in the msg hdr obtained from 
         the LB against the number of msg segments computed from the msg size.
         print a log error if they're not the same and use the computed # of
         segments */

   if (msg_hdr.num_segs != num_segs) {
      LE_send_msg (GL_ERROR, 
        "# msg segs (%d) defined in the LB != # segs computed (%d) for msg type (%d)",
         msg_hdr.num_segs, num_segs, msg_type);
      LE_send_msg (GL_ERROR, 
         "...using the # of computed msg segments for msg processing");
      msg_hdr.num_segs = num_segs;
   }

      /* alloc mem space for a msg segment */

   msg_seg = calloc ((size_t) MAX_RDA_MESSAGE_SIZE, 1);

   if (msg_seg == NULL)
      MA_Abort_Task ("calloc failure");

   msg_seg_hw_size = MAX_RDA_MESSAGE_SIZE / sizeof (short);
   msg_seg_byte_size = RDA_RPG_MAX_MSG_SEG_SIZE;

      /* construct the msg header for each msg segment and copy each
         segment to the metadata buffer */

   for (i = 1; i <= num_segs; i++) {

         /* if this is the last msg segment, compute the msg size of the 
            last segment */

      if (i == num_segs) {
         int last_seg_size;

         msg_seg_byte_size = msg_len - msg_buf_offset;

         if (msg_seg_byte_size > RDA_RPG_MAX_MSG_SEG_SIZE) {
            LE_send_msg (GL_ERROR, 
                "Msg segment is too large (type %d), clipping size by %d bytes",
                 msg_seg_byte_size - RDA_RPG_MAX_MSG_SEG_SIZE);
            msg_seg_byte_size = RDA_RPG_MAX_MSG_SEG_SIZE;
         }

         last_seg_size = msg_seg_byte_size + msg_hdr_size;

         msg_seg_hw_size = last_seg_size / sizeof (short);

         if (last_seg_size % sizeof (short) != 0)
            ++msg_seg_hw_size;
      }

      msg_hdr.seg_num = i;
      msg_hdr.size = msg_seg_hw_size;

         /* copy the msg hdr to the msg segment buffer */

      memcpy (msg_seg, &msg_hdr, msg_hdr_size);

      #ifdef LITTLE_ENDIAN  /* byte swap msg hdr if necessary */
         UMC_RDAtoRPG_message_header_convert ((char *)msg_seg);
      #endif

         /* copy a msg data block of msg segment size to the msg segment buffer */

      memcpy (msg_seg + msg_hdr_size, msg_buf + msg_buf_offset, msg_seg_byte_size);

         /* copy the msg segment buffer to the metadata buffer */

      memcpy (Metadata_buffer + metadata_buf_offset, msg_seg, MAX_RDA_MESSAGE_SIZE);

         /* adjust the buffer offsets */

      metadata_buf_offset += LDM_MSG_SEG_LENGTH;
      msg_buf_offset += RDA_RPG_MAX_MSG_SEG_SIZE;  /* the msg buffer does not 
                                                      contain hdrs */

         /* clear the msg_seg buffer */

      memset (msg_seg, 0, MAX_RDA_MESSAGE_SIZE);
   }

   free (msg_seg);

   return;
}


/********************************************************************************

    Description: Get the max number of msg segments defined in the Archive II/user
                 ICD for the Metadata msg

          Input: msg_type - the RDA-RPG ICD msg type

         Output: 

         Return: max_segs - the max # of msg segments defined for the Metadata msg

           Note: 

 ********************************************************************************/

static int Get_max_msg_segs (int msg_type) {
   int max_segs;

   switch (msg_type) {
      case RDA_STATUS_DATA:
         max_segs = MSG_2_MAX_SEGS;
      break;

      case PERFORMANCE_MAINTENANCE_DATA:
         max_segs = MSG_3_MAX_SEGS;
      break;

      case RDA_RPG_VCP:
         max_segs = MSG_5_MAX_SEGS;
      break;

      case CLUTTER_FILTER_BYPASS_MAP:
         max_segs = MSG_13_MAX_SEGS;
      break;

      case CLUTTER_MAP_DATA:
         max_segs = MSG_15_MAX_SEGS;
      break;

      case ADAPTATION_DATA:
         max_segs = MSG_18_MAX_SEGS;
      break;

      default:
         LE_send_msg (GL_ERROR, "Max # msg segments not defined for msg type %d",
                      msg_type);
         max_segs = 0;
      break;
   }
   return (max_segs);
}


/********************************************************************************

    Description: Get the msg offset from the beginning of the msg read to the
                 beginning of the msg header

          Input: msg_type - the RDA-RPG ICD msg type
                 msg_id   - the LB msg ID of the msg

         Output: 

         Return: msg_offset - the msg offset in bytes

           Note: 

 ********************************************************************************/

static int Get_msg_offset (int msg_type, int msgid) {
   int msg_offset = 0;

   switch (msg_type) {
      case RDA_STATUS_DATA:
      msg_offset = sizeof (RDA_RPG_comms_status_t);
      break;

      case CLUTTER_FILTER_BYPASS_MAP:
         if (msgid == LBID_BYPASSMAP_LGCY)
            msg_offset = 4; /* strip the embedded date and time fields */
      break;

   }

   return (msg_offset);
}


/********************************************************************************

    Description: Validates the Metadata buffer by ensuring that all required
                 Metadata msgs have be stored in the Metadata buffer. Validation
                 is performed by checking the Metadata msg segment counter array, and
                 verifying at least one segment for this msg type has been written 
                 to the Metadata buffer. If the validation check fails, then the 
                 Metadata buffer is populated with the msg by reading the msg 
                 from its respective LB.
                 
                 The validation is performed just before writing the Metadata 
                 buffer out as a LDM record to the LDM queue

          Input: rda_config - the RDA configuration (legacy od orda)
                 Metadata_msgs_seg_cntrs [] - the Metadata msg types segment
                                              counter array 
                    Num_msg_segs_defined [] - The number of msg segments defined
                                              in msg segment 1 of the RDA msg header
                 
         Output: 

         Return: 

           Note: Metadata_msgs_seg_cntrs [] is globally defined at the beginning 
                                            of this file.
                    Num_msg_segs_defined [] is globally defined at the beginning 
                                            of this file.

 ********************************************************************************/

static void Validate_metadata (int rda_config) {
   int idx;

      /* compare the number of msg segments processed against the number
         of msg segments defined in the RDA msg header to ensure every msg segment
         has been populated in the Metadata buffer. If it has not, then
         populate the buffer with the msg from its LB */

   for (idx = 0; idx < NUMBER_METADATA_ICD_MSGS; idx++) {
      if ((Metadata_msgs_seg_cntrs [idx] == 0) || 
          (Metadata_msgs_seg_cntrs [idx] != Num_msg_segs_defined [idx])) {
/*          LE_send_msg (GL_ERROR, 
              "Metadata msg %d validation failed...re-read from RPG LB",
              Metadata_msg_types [idx]); */
          Clear_metadata_msg_block (Metadata_msg_types [idx]);
          Add_msg_to_metadata_buf (idx, rda_config);
      }
   }

   return;
}


/********************************************************************************

    Description: Validates the Metadata msg before storing it in the Metadata
                 buffer. If validation fails then the msg is discarded.
                 
          Input: msg_hdr  - the msg header
                 msg_type - the RDA-RPG ICD msg type
                 msg_len  - msg length in bytes

         Output: 

         Return: 0 on successful msg validation; -1 if msg validation fails

           Note: 

 ********************************************************************************/

static int Validate_msg (RDA_RPG_message_header_t msg_hdr, int msg_type, int msg_len) {

   int max_msg_size_allowed;
   int max_msg_segments_allowed;

      /* check the msg type read from the LB */

   switch (msg_hdr.type) {
      case ADAPTATION_DATA:
      case CLUTTER_MAP_DATA:
      case CLUTTER_FILTER_BYPASS_MAP:
      case RDA_STATUS_DATA:
      case PERFORMANCE_MAINTENANCE_DATA:
      case RDA_RPG_VCP:
      break;

      default:
         LE_send_msg (GL_ERROR, 
             "LDM: Error reading Metadata msg %d", msg_type);
         return (-1);
      break;
   }

      /* check the number of msg segments defined in the LB msg hdr */

   max_msg_segments_allowed = Get_max_msg_segs (msg_type);

   if (msg_hdr.num_segs > max_msg_segments_allowed) {
         LE_send_msg (GL_ERROR, 
             "LDM Error: Too many msg segments defined (%d) for msg %d", 
             msg_hdr.num_segs, msg_type);
         return (-1);
   }

      /* lastly, check the msg size returned by the LB read */
   
   max_msg_size_allowed = (max_msg_segments_allowed * RDA_RPG_MAX_MSG_SEG_SIZE) +
                           sizeof (RDA_RPG_message_header_t); 

   if (msg_len > max_msg_size_allowed) {
      LE_send_msg (GL_ERROR,
          "LDM Error: Metadata msg size (%d) > max size allocated (%d) for msg %d",
          msg_len - sizeof (RDA_RPG_message_header_t),
          max_msg_size_allowed - sizeof (RDA_RPG_message_header_t),
          msg_type);
      return (-1);
   }

   return (0);
}
