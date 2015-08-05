
/***********************************************************************

    Description: Internal include file for play_a2.

***********************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/10 21:11:05 $
 * $Id: pa_def.h,v 1.13 2014/04/10 21:11:05 steves Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */  

#ifndef PA_DEF_H

#define PA_DEF_H

#include <time.h>
#include <rda_rpg_message_header.h>
#include <rpg_vcp.h>

#define LOCAL_NAME_SIZE 200		/* maximum name size */
#define STR_SIZE 128			/* max size for input strings */

enum {AP_COMP_BZ2, AP_COMP_Z, AP_COMP_GZ};	/* compression types */
enum {AP_LOCAL_TYPE, AP_NCDC_TYPE, AP_TAPE_TYPE, AP_LDM_TYPE};	
					/* file types */

typedef struct {			/* volume file */
    char *path;				/* full path of the file */
    char *prefix;			/* file name prefix */
    char *name;				/* pointer to the file name */
    int compress_type;			/* compression type */
    int content_type;			/* file format type */
    time_t time;			/* file time */
    int size;				/* file size */
    short selected;			/* this item is selected */
    short session;			/* session number */
} Ap_vol_file_t;

enum {OPTION_ENV, TAPE_DEVICE, CDW_DEVICE, CDW_SPEED, CONVERT_TO_BZ2, PLAYBACK_LB};
			/* for argument option_name of PAM_get_options */


typedef struct {

   RDA_RPG_message_header_t msg_hdr;	/* Message header. */
   VCP_ICD_msg_t            vcp_data;	/* VCP message. */

} VCP_t;


int PAF_search_volume_files (char *d_name, Ap_vol_file_t **vol_files);
int PAP_playback_volume (Ap_vol_file_t *vf);
char *PAF_get_full_path (char *dir, char *name);
int PAP_initialize (double speed, int verbose, int interactive_mode,
                    int ignore_sails);
int PAF_get_listed_file_name (char *list_file_name, 
					char **dir, char **name);
int PAW_write_cd (Ap_vol_file_t *vol_files, int n_vol_files);
char *PAP_get_work_dir ();
void PAI_main_loop (char *dir_name);
void PAP_set_speed (double speed);
int PAI_pause (int radial_status, time_t data_time);
int PAF_get_listed_files (char *dir_name, 
			char *list_file_name, Ap_vol_file_t **vol_files);
char *PAI_ascii_time (time_t t);
int PAR_pause (int radial_status, time_t data_time);
int PAR_read_tape_data (Ap_vol_file_t *vf, char *buffer, int buffer_size);
int PAR_main_loop ();
time_t PAP_convert_time (int julian_date, int millisecs_past_midnight);
int PAW_execute_command (char *cmd);
void PAM_get_signal (int *flag);
char *PAM_get_options (int option_name);
int PAI_set_sessions (Ap_vol_file_t *vol_files, 
				int n_vol_files, int vol_time_gap);
void PAI_print_session (Ap_vol_file_t *vol_files, int n_vol_files);
int PAP_open_playback_lb ();
char *PAI_gets ();
double PAP_get_speed ();
time_t PAM_get_time_by_seconds_in_a_day (time_t st_time, int seconds);
int PMI_parse_input_time (char *in, time_t *t_in);
int PAM_reset_data_time ();
void PAP_session_start (int true);
int PAM_full_speed_simulation ();
int PAM_convert_to_31 ();

void PAP_byte_swap_vol_title (void *vol_header);


#endif		/* #ifndef PA_DEF_H */
