/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/12/17 16:14:53 $
 * $Id: translate_main.h,v 1.4 2012/12/17 16:14:53 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/**********************************************************************

	Header file for the translation task.

**********************************************************************/



# ifndef TRNS_MAIN_H
# define TRNS_MAIN_H

#include <vcp.h>
#include <basedata.h>
#include <orpg.h>
#include <orpgrda.h>
#include <translate.h>

#define MAX_RADIAL_SIZE			(MAX_GENERIC_BASEDATA_SIZE*sizeof(short))
#define TRNS_MAX_SCANS           	80 	/* Maximum volume scan number for
                                      		   volume scan number. */
#define CTM_HEADER_SIZE                 12
#define CM_REQ_HEADER_SIZE              (sizeof(CM_req_struct))
#define CM_RESP_HEADER_SIZE             (sizeof(CM_resp_struct))
#define RESP_OFFSET                     (CTM_HEADER_SIZE+CM_RESP_HEADER_SIZE)
#define REQ_OFFSET                      (CTM_HEADER_SIZE+CM_REQ_HEADER_SIZE)
#define TRNS_INCOMING_VCP               1
#define TRNS_TRNS_TO_VCP                2
#define RESTART_VOLUME			32768

#ifdef GLOBAL_DEFINED
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN int TRNS_vcp_number;	 /* Current volume coverage pattern. */
EXTERN int TRNS_current_elev_num;   /* The current RDA elevation number. */
EXTERN int TRNS_response_in_lb;
EXTERN int TRNS_response_out_lb;
EXTERN int TRNS_request_in_lb;
EXTERN int TRNS_request_out_lb;
EXTERN int TRANS_user_commanded_vcp;
EXTERN int TRNS_verbose;
EXTERN Trans_tbl_t *Current_table;
EXTERN Trans_info_t Trans_info;

/* Function Prototypes. */
int Read_translation_table( );
Trans_tbl_t* Find_in_translation_table( int vcp_num, int entry );
int Is_translation_table_installed();
char* Process_radial( char *rda_msg, int offset, int has_supple_cuts );
int Process_status ( char *rda_msg );
int Process_vcp ( char *rda_msg, int *size );
#ifdef BUILD13_OR_EARLIER
int Remove_suppl_cuts_from_vcp ( char *rda_msg, int *size );
#endif
int Process_rda_control ( char *rda_msg );
int Process_RPG_RDA_vcp ( char *rda_msg, int *size );
void Write_vcp_data( char *rda_msg );
#endif

