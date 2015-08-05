/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/01/14 14:59:59 $
 * $Id: cldm.h,v 1.12 2013/01/14 14:59:59 steves Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

 
# ifndef CLDM_H
# define CLDM_H

#define uchar unsigned char 

#include <orpg.h>
#include <rda_rpg_messages.h>
#include <archive_II.h>

extern char *ghostname (void);

#define CLDM_RECOMBINED_DP          1939
#ifdef LEVEL2_TOOL
#define CLDM_SR_DEALIASED_DP        76  
#endif

/* define the types of LDM data that can be read */

#define CLDM_RADIAL_DATA            1
#define CLDM_META_DATA              2
#define CLDM_NON_ARCHIVE_2_DATA     3

/* miscellaneous macro definitions */

#define MAXLEN_MESSAGE_LEN          128000
#define COMMGR_HDR_SIZE	            sizeof (CM_resp_struct)


/* define structures used to track VCP information and to process
   the messages read from the datastream */

typedef struct {
     int vcp_num;
   float elev;
     int elev_num;
} Elev_stats_t;  

typedef struct {
              Elev_stats_t elev_status;
  RDA_RPG_message_header_t msg_hdr;
                     short msg_size;
                     short radial_status;
                    time_t radial_time;
                       int ldm_msg_type;
                     short rda_status;
                     uchar rda_config;
} msg_data_t;


/* global function declarations */

/* cldm_main.c */

void MA_Abort_Task (char *);
void MA_get_level2_modes (int *local, int *record, int *ldm);
void MA_update_ldm_stats_cntrs (int uncompressed_len, int compressed_len);


/* cldm_init.c */

int INIT_RPG_parameters (char *site_id, int *rda_channel_num);
int INIT_input_lbs (int *recombined_dp_data_id, int *recombined_dp_removed_data_id,
                    int *realtime_data_id);
int INIT_archive_status (int *initialized_status);


/* cldm_manage_metadata.c */

void MM_clear_metadata_buf (void);
void MM_process_metadata_msg (char *msg_buf, msg_data_t msg_data, int ldm_ver);
void MM_write_metadata (msg_data_t *msg_data, int ldm_ver);


/* cldm_read_messages.c */

void RM_determine_msg_hdr_offset (char *buf, int *offset, int *cm_type, 
                                  int input_data_id);
void RM_extract_msg_data (char *buf, msg_data_t *data, short previous_rda_status);
int  RM_read_next_msg (char **buf, int input_data_id, int rda_channel_flag);
void RM_set_recomb_rda_status_updated_flag (void);


/* cldm_write_records.c */

void WR_add_msg_to_radial_buffer (char *msg_buf, msg_data_t *msg_properties, 
                                  int ldm_ver, int previous_rda_status);
void WR_init_record_info (char *site_id);
void WR_process_record (char *buf, int record_len, msg_data_t *msg_data, int ldm_version);

#endif    /* CLDM_H */

