/******************************************************************

	file: rms_message.h

	This is the main module for processing messages.
	
******************************************************************/
#include <rms_ptmgr.h>
#include <rms_util.h>
#include <orpg.h>
#include <gen_stat_msg.h>
#include <infr.h>
#include <orpgdat.h>
#include <orpgda.h>
#include <orpgevt.h>
#include <stdio.h>
#include <rda_status.h>
#include <rda_control.h>
#include <clutter.h>
#include <rda_rpg_clutter_map.h>
#include <orpggst.h>
#include <rpgc.h>
#include <orpgrda.h>
#include <orpgred.h>
#include <orpgerr.h>
#include <mrpg.h>
#include <rda_rpg_message_header.h>
#include <orpgsite.h>
/* Defines for RMS */

#define MESSAGE_START  		22

#define MAX_RESEND_ARRAY_SIZE 	1500

#define MAX_PAD_SIZE		50

#define MAX_FAA_LINES		47

#define	RMS_LE_LOG_MSG		1

#define	RMS_LE_STATUS		2

#define	RMS_LE_ERROR		3

#define MALRM_SEND_STATUS_MSG	        1

#define MALRM_CHECK_RESEND	        2

#define MALRM_SEND_STATE_MSG	        3

#define MALRM_SEND_ALARM_MSG	        4

#define MALRM_SEND_RMS_UP	        5

#define MALRM_SEND_RMS_STATLOG       6

#define BAD_CHECKSUM		-2

#define RPGOP_CLASS		99

#define RPGOP_CLASS98		98

#define CLASS_I                                 1

#define CLASS_II                                2

#define CLASS_III                               3

#define CLASS_IV                               4

#define CLASS_V_RGDAC                   5

#define CLASS_V_RFC                        6

#define ORPGRED_ERROR		-1

/*
  Define line number for RDA Primary.
*/
#define RDA_PRIMARY       1
#define RDA_SECONDARY     2
#define	WIDEBAND_USER	  3
#define MAX_ERRORS	      3

/* Glogal types for rms message*/

struct header {
	ushort message_size;
	ushort rda_redun;
	ushort rpg_redun;
	ushort seq_num;
	ushort message_type;
	ushort redun_confg;
	ushort julian_date;
	int seconds;
	int checksum;
	};
	
struct resend{
		ushort msg_seq_num;
		LB_id_t lb_location;
		ushort msg_type;
		int time_sent;
		int retrys;
		};

int resend_cnt;

int interface_errors;

enum { RMS_STANDARD, RMS_RETRY, RMS_UP, RMS_ACK };

/* Public functions */

/* rms_header.c */
int strip_header(UNSIGNED_BYTE *msg_buffer, struct header* header_values);
	
int build_header(ushort *message_size, ushort message_type, UNSIGNED_BYTE *msg_buf, ushort ack_seq);

/* rms_ack.c */
int build_ack(struct header *msg_header);

/* rms_rec_ack*/
int receive_ack(UNSIGNED_BYTE *in_buf);

/* rms_send.c*/
int send_message (UNSIGNED_BYTE *write_buf, ushort msg_type, int msg_group);

void add_terminator (UNSIGNED_BYTE *buf_ptr);

/* rms_pad.c */
void pad_message(UNSIGNED_BYTE *buf_ptr, int message_size, int final_length);

/* rms_send_status.c*/
void send_status_msg();

/* rms_check_resend.c*/
void check_resend();

/* rms_checksum.c */
int rms_checksum (UNSIGNED_BYTE *cksum_buf_ptr);

/* rms_up.c */
void send_rms_up_msg();

/* rms_rpg_state.c */
void rms_send_rpg_state();

/* rms_alarm_msg.c */
void rms_send_alarm();

/* rms_rec_wb_command.c */
int rms_rec_wb_command (UNSIGNED_BYTE *rda_wb_buf);

/* rms_rec_rda_state_command.c */
int rms_rec_rda_state_command (UNSIGNED_BYTE *rda_change_buf);

/* rms_rec_rda_mode_command.c */
int rms_rec_rda_mode_command (UNSIGNED_BYTE *rda_mode_buf);

/* rms_rec_rda_channel_command.c */
int rms_rec_rda_channel_command (UNSIGNED_BYTE *rda_channel_buf);

/* rms_rec_clutter_command */
int rms_rec_clutter_command (UNSIGNED_BYTE *rda_clutter_buf);

/* rms_rec_download_clutter_command.c */
int rms_rec_download_clutter_command (UNSIGNED_BYTE *rda_clutter_buf);

/* rms_rec_bypass_command.c */
int rms_rec_bypass_command (UNSIGNED_BYTE *rda_bypass_buf);

/* rms_download_bypass_map.c */
int rms_rec_download_bypass_command (UNSIGNED_BYTE *rda_bypass_buf);

/* rms_rec_arch_command.c */
int rms_rec_arch_command (UNSIGNED_BYTE *rda_arch_buf);

/* rms_loopback_msg.c */
int rms_rec_loopback_command (UNSIGNED_BYTE *loopback_buf);

/* rms_force_adaptation.c */
int rms_rec_adaptation_command (UNSIGNED_BYTE *adaptation_buf);

/* rms_handle_msg.c */
void rms_handle_msg();

/* rms_download_auth_user.c */
int rms_rec_download_auth_user_command (UNSIGNED_BYTE *auth_user_buf);

/* rms_edit_auth_user.c */
int rms_rec_auth_user_command (UNSIGNED_BYTE *auth_user_buf);

/* rms_edit_load_shed.c */
int rms_rec_load_shed_command (UNSIGNED_BYTE *load_shed_buf);

/* rms_download_load_shed.c */
int rms_rec_download_load_shed_command (UNSIGNED_BYTE *auth_user_buf);

/* rms_edit_pup_id.c */
int rms_rec_pup_id_command (UNSIGNED_BYTE *pup_id_buf);

/* rms_download_pup_id.c */
int rms_rec_download_pup_id_command (UNSIGNED_BYTE *pup_id_buf);

/* rms_edit_nb_cfg.c */
int rms_rec_nb_cfg_command (UNSIGNED_BYTE *nb_cfg_buf);

/* rms_download_nb_cfg.c */
int rms_rec_download_nb_cfg_command (UNSIGNED_BYTE *nb_cfg_buf);

/* rms_nb_cfg.c */
int rms_get_cfg_ptr (Nb_cfg* cfg_ptr,short line_num, short uid);

/* rms_nb_status.c */
int rms_get_nb_status(Nb_status *nb_ptr);

/* rms_edit_dial_cfg.c */
int rms_rec_dial_cfg_command (UNSIGNED_BYTE *dial_cfg_buf);

/* rms_download_dial_cfg.c */
int rms_rec_download_dial_cfg_command (UNSIGNED_BYTE *dial_cfg_buf);

/* rms_rec_free_text.c */
int rms_rec_free_text_command(UNSIGNED_BYTE *free_text_buf);

/* rms_send_free_text.c */
void rms_send_free_text_msg();

/* rms_send_status_log.c */
void rms_send_status_log_msg();

/* rms_rpg_control.c */
int rms_rec_rpg_control_command (UNSIGNED_BYTE *rpg_change_buf);
 
/* rms_send_rpg_msg.c */
int rms_send_RPG_command ();

/* rms_inhibit.c */
int rms_inhibit (int seconds);

/* rms_init.c */
int init_rms_interface();

int reset_rms_interface();

void close_rms_interface();

/*rms_send_record_log.c */
void rms_send_record_log_msg (int fd, LB_id_t msgid, int msg_info, void *arg);

void rms_clear_record_log();

/* rms_narrowband_interface.c */
int rms_rec_nb_inter_command (UNSIGNED_BYTE *nb_inter_buf);
