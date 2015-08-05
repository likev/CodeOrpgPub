/********************************************************************************

     Description: Header file for the redundant channel manager
     
 ********************************************************************************/


/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/02/12 16:21:51 $
 * $Id: mngred.h,v 1.10 2013/02/12 16:21:51 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

 
# ifndef MNGRED_H
# define MNGRED_H


#include <infr.h>
#include <orpgcfg.h>
#include <orpgred.h>
#include <orpginfo.h>
#include <orpgerr.h>
#include <orpgmisc.h>
#include <orpgnbc.h>
#include <orpgrda.h>
#include <orpgtask.h>
#include <rda_status.h>


#define   MNGRED_MAX_NAME_LENGTH  256  /* set high because process abort occurs 
                                           if a path length exceeds this value  */
#define   MNGRED_UNINITIALIZED     -1  /* state defining an object is not 
                                           initialized                          */

   /* message log verbosity levels */

#define   MNGRED_OP_VL         LE_VL0  /* Normal operational verbosity level    */
#define   MNGRED_TEST_VL       LE_VL1  /* DT&E verbosity level                  */
#define   MNGRED_DEBUG_VL      LE_VL2  /* Debug/analysis verbosity level        */
#define   MNGRED_DIOM_VL       LE_VL3  /* Debug/analysis verbosity level        */

   /* Warning message text */

#define   MNGRED_WARN_ACTIVE	"REDUNDANT WARNING ACTIVATED: "
#define   MNGRED_WARN_CLEAR	"REDUNDANT WARNING CLEARED: "
#define   MNGRED_WARN		"REDUNDANT WARNING: "

   /* RDA-RPG ICD defined spot blanking states */

#define   MNGRED_SPOT_BLANKING_NOT_INSTALLED  0
#define   MNGRED_SPOT_BLANKING_ENABLED        2
#define   MNGRED_SPOT_BLANKING_DISABLED       4

   /* true/false flags */

enum bool { MNGRED_FALSE = 0,          /* flag specifying a condition is false  */
            MNGRED_TRUE};              /* flag specifying a condition is true   */


   /* type-of-update flags */

enum { MNGRED_UPDATE_ONE_LB = 1,       /* Flag specifying to update one LB on 
                                           redun ch                             */
       MNGRED_UPDATE_ALL_LBS};         /* Flag specifying to update all LBs on 
                                           redun ch                             */

   /* flags specifying type of data (lb type) */

enum { MNGRED_STATE_DAT = 1,           /* Flag specifying State Data LB         */
       MNGRED_ADAPT_DAT};              /* Flag specifying Adaptation Data LB    */

   /* definitions for the different types of LB updates the redundant 
      manager must manage */

enum { MNGRED_TRANSFER_STATE_DATA = 1, /* state data transfers                  */
       MNGRED_TRANSFER_ADAPT_DATA,     /* adaptation data transfers             */
       MNGRED_TRANSFER_AT_SWITCHOVER}; /* any LB data that is transferred at
                                          channel switchover                    */


typedef struct       /* local LB lookup table entry */
{
   int     data_id;              /* id of the LB                                */
   int     local_lb_fd;          /* local channel LB file descriptor            */
   int     redundant_lb_fd;      /* redundant channel LB file descriptor        */
   LB_id_t msg_id;               /* id of the msg we're interested in           */
   int     update_required;      /* flag specifying to update other channel 
                                     with this entry                            */
   int     update_type;          /* specifies what type of an update this lb is:
                                    "state data",
                                    "adaptation data",
                                    "at switchover"                             */
   char    redundant_lb_name[MNGRED_MAX_NAME_LENGTH]; /* path name of the 
                                                         redundant channel LB   */
}   Lb_table_entry_t;

                                       
/* Global Function Prototypes */


   /* mngred_main.c */

void MA_force_adapt_dat_update (void);

   /* mngred_channel_link_services.c */

void CLS_check_channel_link (void);
void CLS_update_ping_response (unsigned int seq_num);

   /* mngred_channel_status.c */
   
void CST_print_channel_status (char *msg);
void CST_print_previous_state (Previous_channel_state_t previous_state);
void CST_save_channel_state ();
void CST_set_misc_state_data_flag (void);
int  CST_transmit_channel_status (void);
int  CST_update_channel_status (void);

   /* mngred_dio_cntlr.c */

int DIO_acquire_comms_relay (void);
int DIO_init_dio_module (void);
int DIO_reset_dos (void);
Comms_relay_state_t DIO_read_comms_relay_state (void);

   /* mngred_comms_relay.c */

Comms_relay_state_t CR_acquire_comms_relay (void);
Comms_relay_state_t CR_read_comms_relay_state (void);
int CR_open_dio_card (void);


   /* mngred_download_commands.c */

int  DC_are_cmds_pending (int data_type);
void DC_clear_channel_cmds ();
void DC_process_download_commands (void);
int  DC_send_IPC_cmds ();
void DC_set_download_cmd (Redundant_channel_msg_t channel_cmd, LB_id_t msg_id);
void DC_set_IPC_cmd (int cmd, int parameter1, int parameter2, 
                     int parameter3, int parameter4, int parameter5);
void DC_set_redun_rda_dnld_cmd (Redundant_cmd_t cmd_msg);


   /* mngred_initialize.c */

int IC_initialize_channel(void);


   /* mngred_mng_lookup_table.c */

int               MLT_add_table_entry (int data_id, int lbfd, int redun_lbfd, 
                             LB_id_t msg_id, char *lb_path, int lbtype);
int               MLT_are_updates_pending (int data_type); 
void              MLT_clear_update_required_flags ();                            
Lb_table_entry_t *MLT_find_table_entry (int dataid, LB_id_t msg_id);
int               MLT_get_number_of_entries (void);
void             *MLT_get_table_id (void);
Lb_table_entry_t *MLT_get_table_ptr (void);
int               MLT_open_lookup_table (void);
void              MLT_reset_redun_ch_lbds (void);
int               MLT_set_update_required_flag (int data_id, int local_lbd, 
                             LB_id_t msg_id, int lb_update_type, int lb_type);
void              MLT_update_table_redun_ch_lbd (int data_id, int new_redun_lbd);


   /* mngred_process_callbacks.c */

void PC_process_adapt_dat_update_event (int lbd, LB_id_t msgid, int msg_len,
                                        void *data_id);
void PC_process_at_switchover_event (int lbd, LB_id_t msgid, int msg_len, 
                                     void *data_id);
void PC_process_channel_cmd (int lbd, LB_id_t msg_id, int msg_len, 
                             void *data_id);
void PC_process_channel_msg (int lbd, LB_id_t msg_id, int msg_len, 
                             void *data_id);
void PC_process_on_update_event (int lbd, LB_id_t msgid, int msg_len, 
                                 void *dataid);
void PC_process_on_demand_event (int lbd, LB_id_t msgid, int msg_len,
                                 void *data_id);
void PC_process_timer_expired_event (void);


   /* mngred_processing_states.c */

void PS_process_active_state (void);
void PS_process_inactive_state (void);
void PS_xsition_to_active (void);
void PS_xsition_to_inactive (void);


   /* mngred_write_channel_data.c */

int  WCD_clear_chan_err_alarm (void);
void WCD_set_adapt_data_updated_time (void);
void WCD_update_redun_ch (void);
void WCD_update_redundant_lb_on_demand (int lbd, LB_id_t msgid, int msg_len,
                                        void *data_id);
int  WCD_write_redundant_lb_data (int data_id, char *buffer, int msg_length,
                                  LB_id_t msg_id, 
                                  Lb_table_entry_t *lb_table_entry, 
                                  void *tag_address);
#endif    /* MNGRED_H */
