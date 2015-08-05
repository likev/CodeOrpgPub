/****************************************************************
		
    Module: hci_options.h
				
    Description: This is the header file the read_options functions
    		 for hci tasks

****************************************************************/

/*
 * RCS info 
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2014/07/21 20:05:29 $
 * $Id: hci_options.h,v 1.40 2014/07/21 20:05:29 ccalvert Exp $
 * $Revision: 1.40 $
 * $State: Exp $
 * $Log: hci_options.h,v $
 * Revision 1.40  2014/07/21 20:05:29  ccalvert
 * 4212 - MESO SAILS into HCI
 *
 * Revision 1.39  2013/05/30 19:29:02  steves
 * CCR NA13-00075
 *
 * Revision 1.38  2011/10/03 21:47:48  ccalvert
 * allow HCI to write to status/error log
 *
 * Revision 1.37  2011/06/17 17:37:31  ccalvert
 * Only check adapt/hardware if HCI initially fails
 *
 * Revision 1.36  2011/06/08 21:33:19  ccalvert
 * add -S option
 *
 * Revision 1.35  2011/01/04 23:34:50  ccalvert
 * remove references to BDDS and wbserver
 *
 * Revision 1.34  2010/04/05 22:13:06  ccalvert
 * fix compiler warning in CPCI-32
 *
 * Revision 1.33  2010/03/17 22:59:48  ccalvert
 * fix R2359
 *
 * Revision 1.32  2010/03/16 14:37:48  ccalvert
 * fix for R2359
 *
 * Revision 1.30  2009/09/21 16:53:02  ccalvert
 * implement MRPG button on MSCF
 *
 * Revision 1.29  2009/07/02 20:53:08  ccalvert
 * add console msg gui tool (tsk014)
 *
 * Revision 1.28  2009/04/15 19:25:12  ccalvert
 * rm HCI passwords msg in HCI_DATA LB (use DEAU)
 *
 * Revision 1.26  2009/02/27 22:26:12  ccalvert
 * consolidate HCI code
 *
 * Revision 1.25  2008/05/21 15:16:17  ccalvert
 * add new fx to get channel for GUI title
 *
 * Revision 1.24  2005/12/30 16:51:15  steves
 * issue 2-744
 *
 * Revision 1.23  2004/11/23 14:55:12  jing
 * Update
 *
 * Revision 1.22  2004/01/22 22:14:18  steves
 * issue 2-282
 *
 * Revision 1.21  2002/07/23 18:45:15  davep
 * Issue 2-022
 *
 * Revision 1.20  2002/03/01 15:04:02  eddief
 * Pass child options correctly
 *
 * Revision 1.19  2001/01/24 16:13:29  eforren
 * Make hci re-start whenever linear buffers are re-created
 *
 * Revision 1.18  2001/01/09 15:09:09  eforren
 * Restart on network disconnections
 *
 * Revision 1.17  2000/05/26 17:07:23  eforren
 * Implement shutdown of bdds hci when network connectivity is lost
 *
 * Revision 1.16  2000/05/22 21:38:50  eforren
 * Add a feature to let the listener know when disconnects occur
 *
 * Revision 1.15  2000/05/05 13:41:41  eforren
 * Add -u option for updating configuration files
 *
 * Revision 1.14  2000/05/04 22:23:00  eforren
 * Add code to copy the system configuration file when running remotely
 *
 * Revision 1.13  2000/05/03 18:23:23  eforren
 * Move/enhance monitor network stuff
 *
 * Revision 1.12  2000/04/18 13:29:56  eforren
 * Add function to monitor for network connectivity changes
 *
 * Revision 1.11  2000/01/02 00:59:54  eforren
 * Add hci_read_custom_options function
 *
 * Revision 1.10  1999/05/12 18:20:03  priegni
 * Remove compiler warnings
 *
 * Revision 1.9  1999/05/07 18:26:02  priegni
 * Clean up compiler warnings and add ORPGRDA function support
 *
 * Revision 1.8  1999/04/27 20:55:37  eforren
 * Add machine name to the mscf option
 *
 * Revision 1.7  1999/04/23 15:28:36  eforren
 * Change name of HCI_reg_term_hdlr function
 *
 * Revision 1.6  1999/04/23 14:43:09  eforren
 * Fix options
 *
 * Revision 1.5  1999/04/22 20:26:24  priegni
 * update
 *
 * Revision 1.3  1999/04/19 15:52:30  eforren
 * Add simulation options and other changes related to cancelling I/O operations
 *
 * Revision 1.2  1998/09/10 14:22:18  eforren
 * Add watch cursors to hci
 *
 * Revision 1.1  1998/07/06 19:08:50  eforren
 * Initial revision
 *
*/

#ifndef HCI_OPTIONS_H
#define HCI_OPTIONS_H

#ifdef __cplusplus
extern "C"
{
#endif

/* System include files */

#include <stdlib.h>
#include <string.h>

/* X/Motif include files */

#include <X11/cursorfont.h>
#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include <Xm/MwmUtil.h>
#include <Xm/Protocols.h>

/* Macros */

#define	HCI_AGENCY_PWD_ID	"site_info.hci_password_agency"
#define	HCI_ROC_PWD_ID		"site_info.hci_password_roc"
#define	HCI_URC_PWD_ID		"site_info.hci_password_urc"

/* Enumerations */

enum { NO_RESIZE_HCI, RESIZE_HCI };
enum { HCI_UNKNOWN_NODE, HCI_RPGA_NODE, HCI_RPGB_NODE, HCI_MSCF_NODE };
enum { HCI_UNKNOWN_SYSTEM, HCI_NWS_SYSTEM, HCI_NWSR_SYSTEM, HCI_FAA_SYSTEM, HCI_DODAN_SYSTEM, HCI_DODFR_SYSTEM };
enum { HCI_TASK, HCI_PROD_TASK, HCI_PERF_TASK, HCI_WIND_TASK,
       HCI_CCZ_TASK, HCI_VCP_TASK, HCI_BASEDATA_TASK, HCI_PRF_TASK,
       HCI_CBM_TASK, HCI_ALT_TASK, HCI_PSTAT_TASK, HCI_STATUS_TASK,
       HCI_ARCHII_TASK, HCI_PROD_PRIORITY_TASK, HCI_NB_TASK,
       HCI_SPP_TASK, HCI_HUB_RTR_LOAD_TASK, HCI_CONFIGURE_HUB_RTR_TASK,
       HCI_PASSWD_TASK, HCI_PRECIP_STATUS_TASK, HCI_RDA_LINK_TASK,
       HCI_RDC_TASK, HCI_RDA_TASK, HCI_RPC_TASK, HCI_LOAD_TASK,
       HCI_PDC_TASK, HCI_APPS_ADAPT_TASK, HCI_ORDA_PMD_TASK,
       HCI_MODE_STATUS_TASK, HCI_MISC_TASK, HCI_RESTORE_ADAPT_TASK,
       HCI_SAVE_ADAPT_TASK, HCI_MERGE_TASK, HCI_BLOCKAGE_TASK,
       HCI_HARDWARE_CONFIG_TASK, HCI_LOG_TASK,
       HCI_BASEDATA_TOOL, HCI_STATUS_PRINT_TOOL, HCI_TRD_TOOL,
       HCI_RDASIM_GUI_TOOL, HCI_ENVIROVIEWER_TASK, HCI_SAILS_TASK,
       NUM_HCI_TASKS };

/* Typedefs */

/* Main functions. */

void HCI_init( int, char **, int );
void HCI_partial_init( int, char **, int );
void HCI_init_non_RPG( int, char ** );
void HCI_start( void (*)(), int, int );
void HCI_finish();
void HCI_task_exit( int );
void HCI_set_check_install (void (*check_install)());

/* Set functions. */

int HCI_register_term_hdlr( void (*)(int) );
void HCI_set_destroy_callback( int (*)() );
void HCI_set_timer_interval( int );
void HCI_set_custom_args( const char *, int(*)(char, char *), void(*)() );

/* Access functions. */

char* HCI_orpg_machine_name();
int HCI_is_low_bandwidth();
int HCI_is_satellite_connection();
int HCI_is_minimized();
int HCI_simulation_speed();
char* HCI_child_options_string();
int HCI_get_node();
int HCI_get_config_type();
int HCI_get_channel_number();
int HCI_default_mode_A_vcp();
int HCI_default_mode_B_vcp();
int HCI_default_wx_mode();
int HCI_bkg_product_code();
int HCI_has_bdds();
int HCI_has_ldm();
int HCI_has_mlos();
int HCI_has_rms();
int HCI_rda_elevation();
int HCI_rda_latitude();
int HCI_rda_longitude();
int HCI_rpg_id();
int HCI_is_orda();
int HCI_agency_password( char ** );
int HCI_roc_password( char ** );
int HCI_urc_password( char ** );
int HCI_set_agency_password( char * );
int HCI_set_roc_password( char * );
int HCI_set_urc_password( char * );
int HCI_rpg_name( char * );
void HCI_Shell_init( Widget *, char * );
void HCI_Shell_start( Widget, int );
void HCI_Shell_popup( Widget );
void HCI_Shell_popdown( Widget );
void HCI_flush_X_events();


Widget HCI_get_top_widget();
XtAppContext HCI_get_appcontext();
Display *HCI_get_display();
Screen *HCI_get_screen();
Window HCI_get_window();
Colormap HCI_get_colormap();
Dimension HCI_get_depth();
int HCI_get_system();
void HCI_default_cursor();
void HCI_busy_cursor();
void HCI_selectable_cursor();
void HCI_LE_log(const char *,...);
void HCI_LE_status(const char *,...);
void HCI_LE_error(const char *,...);
void HCI_LE_status_log(const char *,...);
void HCI_LE_error_log(const char *,...);
void HCI_display_feedback( char * );
char **HCI_get_months();
char *HCI_get_month(int);
char *HCI_get_task_name();

#ifdef __cplusplus
}
#endif

#endif

