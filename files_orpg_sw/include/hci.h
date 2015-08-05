/******************************************************************
 *	hci.h This header file contains information for HCI	  *
 *	specific tasks which can be shared amongst any HCI task.  *
 ******************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/12/19 21:04:54 $
 * $Id: hci.h,v 1.66 2012/12/19 21:04:54 ccalvert Exp $
 * $Revision: 1.66 $
 * $State: Exp $
 */

#ifndef HCI_TYPE_H
#define HCI_TYPE_H

/*	System include file definitions					*/

#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>

/*	Motif and X related include file definitions			*/

#ifndef CALLED_FROM_SMIPP
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include <Xm/Xm.h>
#include <Xm/ArrowB.h>
#include <Xm/ComboBox.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/DrawnB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/Notebook.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>

/*	RPG-related local include file definitions			*/

#include <infr.h>
#include <medcp.h>
#include <orpg.h>
#include <orpgedlock.h>
#include <orpgsite.h>
#include <orpgdat.h>
#include <orpgrda.h>
#include <orpgred.h>
#include <prod_distri_info.h>
#include <prod_status.h>
#include <rpgc.h>
#include <rpg_port.h>

/*	HCI-related include file definitions.				*/

#include <hci_color.h>
#include <hci_consts.h>
#include <hci_decode_product.h>
#include <hci_font.h>
#include <hci_le.h>
#include <hci_lock.h>
#include <hci_misc_funcs.h>
#include <hci_nonoperational.h>
#include <hci_options.h>
#include <hci_popup.h>
#include <hci_product_colors.h>
#include <hci_pm.h>
#include <hci_rda_control_functions.h>
#include <hci_rpg_install_info.h>
#include <hci_rpg_options.h>
#include <hci_sails.h>
#include <hci_uipm.h>
#include <hci_validate.h>

/*	Macro for hci task name to pass to ORPGMISC_init		*/

#define	HCI_STRING	"hci"

/*	Macros for various uids						*/

#define	HCI_ROOT_UID	0
#define	HCI_ROOT_USER	"root"
#define	HCI_ROC_UID	20001

/*	Miscellaneous Macros						*/

#define	HCI_DEPTH_3D		8
#define	HCI_ICAO_LEN		5

#define	HCI_BUF_2		2
#define	HCI_BUF_4		4
#define	HCI_BUF_8		8
#define	HCI_BUF_16		16
#define	HCI_BUF_32		32
#define	HCI_BUF_64		64
#define	HCI_BUF_96		96
#define	HCI_BUF_128		128
#define	HCI_BUF_256		256
#define	HCI_BUF_512		512
#define	HCI_BUF_1024		1024
#define	HCI_BUF_2048		2048

#define	HCI_NUM_LE_LOG_MSGS	5000
#define	HCI_SYSTEM_MAX_BUF	HCI_BUF_2048

#define	HCI_CP_MAX_BUF		HCI_BUF_512
#define	HCI_CP_DOWN		MISC_CP_DOWN
#define	HCI_CP_STDOUT		MISC_CP_STDOUT
#define	HCI_CP_STDERR		MISC_CP_STDERR
#define	HCI_CP_MANAGE		MISC_CP_MANAGE

#define	HCI_QUARTER_SECOND	250
#define	HCI_HALF_SECOND		500
#define	HCI_ONE_SECOND		1000
#define	HCI_ONE_AND_HALF_SECOND	1500
#define	HCI_TWO_SECONDS		2000
#define	HCI_THREE_SECONDS	3000

#define	HCI_BYTES_PER_KILOBYTE	1024

#define	HCI_DEG_TO_RAD		(3.14159265/180.0)
#define	HCI_RAD_TO_DEG		(1.0/HCI_DEG_TO_RAD)
#define	HCI_KM_TO_NM		(60.0/111.12)
#define	HCI_NM_TO_KM		(111.12/60.0)
#define	HCI_MPS_TO_KTS		1.9438
#define	HCI_METERS_PER_KM	1000.0
#define	HCI_KM_TO_KFT		3.28084

#define	HCI_MILLISECONDS_PER_SECOND	1000
#define	HCI_HOURS_PER_DAY		24
#define	HCI_MINUTES_PER_HOUR		60
#define	HCI_SECONDS_PER_MINUTE		60
#define	HCI_SECONDS_PER_HOUR	(HCI_SECONDS_PER_MINUTE*HCI_MINUTES_PER_HOUR)
#define	HCI_SECONDS_PER_DAY	(HCI_SECONDS_PER_HOUR*HCI_HOURS_PER_DAY)

enum {HCI_NO_FLAG, HCI_YES_FLAG};
enum {HCI_OFF_FLAG, HCI_ON_FLAG};
enum {HCI_NOT_CHANGED_FLAG, HCI_CHANGED_FLAG};
enum {HCI_CD_MEDIA_FLAG, HCI_FLOPPY_MEDIA_FLAG};
enum {HCI_EXIT_FAIL, HCI_EXIT_SUCCESS};
enum {HCI_RPGA1=1, HCI_RPGA2, HCI_RPGB1, HCI_RPGB2, HCI_MSCF};
enum {HCI_CP_EXIT_SUCCESS,HCI_CP_NOT_STARTED, HCI_CP_STARTED, HCI_CP_FINISHED};
enum {HCI_NOT_MODIFIED_FLAG, HCI_MODIFIED_FLAG};
enum {HCI_WB_CONNECT_FLAG, HCI_WB_DISCONNECT_FLAG};
enum {HCI_OK_TO_EXIT, HCI_NOT_OK_TO_EXIT};

/*	Macros for message elements in ORPGDAT_HCI_DATA			*/

enum {HCI_GUI_INFO_MSG_ID=0,
      HCI_FAA_REDUNDANT_INFO_MSG_ID,
      HCI_TASK_INFO_MSG_ID,
      HCI_RMS_STATUS_MSG_ID,
      HCI_CCZ_TASK_DATA_MSG_ID,
      HCI_PRF_TASK_DATA_MSG_ID,
      HCI_RDA_DEVICE_DATA_MSG_ID,
      HCI_PRECIP_STATUS_MSG_ID,
      HCI_PROD_INFO_STATUS_MSG_ID
};

#define	HCI_MAX_TASK_STATUS_NUM		200

/*	HCI_GUI_INFO_MSG_ID						*/

typedef	struct {

	int	text_fg_color;		/* Normal text foreground color */
	int	text_bg_color;		/* Normal text background color */
	int	button_fg_color;	/* Button types foreground color */
	int	button_bg_color;	/* Button types background color */
	int	edit_fg_color;		/* Edit box (editable) fg color */
	int	edit_bg_color;		/* Edit box (editable) bg color */
	int	canvas_bg1_color;	/* Primary background color */
	int	canvas_bg2_color;	/* Secondary background color */
	int	normal_color;		/* Normal state background color */
	int	warning_color;		/* Warning state background color */
	int	alarm1_color;		/* Alarm 1 state background color */
	int	alarm2_color;		/* Alarm 2 state background color */
	int	loca_fg_color;		/* LOCA objects foreground color */
	int	loca_bg_color;		/* LOCA objects background color */
	int	icon_fg_color;		/* Icon foreground color */
	int	icon_bg_color;		/* Icon background color */
	int	product_fg_color;	/* Product foreground color */
	int	product_bg_color;	/* Product background color */
	int	font_size;		/* Font horizontal and vertical	*
					 * resolution.			*/
	int	font_point;		/* Font point.			*/

} Hci_gui_t;

/*	HCI_RMS_STATUS_MSG_ID					*/

typedef struct {

	int	status;

} Hci_rms_status_t;

/*	The following structure is used for storing information on	*
 *	failed ORPG tasks.  The HCI agent reads the larger task status	*
 *	message and extracts information on failed tasks and writes	*
 *	it out to the HCI_TASK_INFO_MSG_ID.  Other HCI tasks needing	*
 *	this information (ie., RPG Status) can read this message.	*/

typedef	struct	{

	int	instance;
	int	control_task;  /* 1 = control task; 0 = non-control task */
	char	name [ORPG_TASKNAME_SIZ];

} Hci_task_t;

/*	The following structures define the background product info	*
 *	for the HCI Clutter Regions Editor and PRF Selection tasks.	*/

#define	HCI_MAX_CUTS_IN_LIST    VCP_MAXN_CUTS
#define	HCI_MAX_ITEMS_IN_LIST	256

typedef struct {

	short	low_product;	/* low bandwidth background product code */
	short	high_product;	/* high bandwidth background product code */
	short	n_cuts;		/* number of cuts to generate */
	short	cut_list [HCI_MAX_CUTS_IN_LIST];
				/* list of cut angles (*10) */
	short	n_products;	/* number of allowed background product
				   selections */
	short	product_list [HCI_MAX_ITEMS_IN_LIST];
				/* list of selectable background product
				   codes */

} Hci_ccz_data_t; /* Clutter Regions Editor task */

typedef struct {

	short	low_product;	/* low bandwidth background product code */
	short	high_product;	/* high bandwidth background product code */
	short	n_cuts;		/* number of cuts to generate */
	short	cut_list [HCI_MAX_CUTS_IN_LIST];
				/* list of cut angles (*10) */
	short	n_products;	/* number of allowed background product
				   selections */
	short	product_list [HCI_MAX_ITEMS_IN_LIST];
				/* list of selectable background product
				   codes */

} Hci_prf_data_t; /* PRF Selection task */

/*
  The following struct is used to group various pd info into one
  data block to be used by the HCI. Doing this decreases the number
  of LB_read calls from up to LINE_TBL_SIZE+1 to 1.
*/

typedef struct {
    Pd_distri_info pd_info;
    Pd_line_entry pd_line_info[ LINE_TBL_SIZE ];
    Prod_user_status pd_user_status[ LINE_TBL_SIZE ];
} Hci_pd_block_t;

/*	The following functions are used for all I/O to the HCI info	*
 *	messages contained in ORPGDAT_HCI_DATA.				*/

int	hci_read_info_msg  (int msg_id);
int	hci_write_info_msg (int msg_id, void *data_ptr);
void	*hci_get_info_msg  (int msg_id);

int	hci_info_update_status (int msg_id);

int	hci_info_inhibit_RDA_messages ();
void	hci_info_set_inhibit_RDA_messages (int state);

int	hci_info_failed_task_num ();
int	hci_info_failed_task_type (int indx);
char	*hci_info_failed_task_name (int indx);

void	hci_gain_focus_callback( Widget, XtPointer, XtPointer );

Window	hci_window_query (Display *display, Window window, char *name);

void	hci_force_resize_callback (Widget w, XtPointer client_data,
				   XtPointer call_data);

void	hci_display_radial_product( Display *, Drawable, GC, int, int, int, int, int, float, float, int );
void	hci_display_color_bar( Display *, Drawable, GC, int, int, int, int, int, int );
float	hci_find_azimuth( int, int, int, int );


#endif
