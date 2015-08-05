
/***********************************************************************

    Description: Wideband (RDA/RPG) comms monitor (mon_wb) header file.

***********************************************************************/

/* 
 * RCS info
 * $Author: jclose $
 * $Locker:  $
 * $Date: 2009/06/29 16:02:05 $
 * $Id: mon_wb_gui_lib.h,v 1.1 2009/06/29 16:02:05 jclose Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#ifndef MON_WB_H
#define MON_WB_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <orpg.h>
#include <orpgrda.h>
#include <infr.h>
#include <comm_manager.h>
#include <rda_rpg_message_header.h>
#include <basedata.h>
#include <rda_rpg_loop_back.h>
#include <rda_status.h>
#include <rda_control.h>
#include <rpg_request_data.h>
#include <rpgcs.h>
#include <misc.h>
#include <math.h>
#include "mon_wb_gui_struct.h"

/* fill values */
#define MONWB_SHORT_FILL			32767


/* Define string and format parameters */
#define MONWB_MAX_STR_LEN			100
#define MONWB_MAX_NUM_CHARS			500
#define MONWB_CHARS_PER_LINE			80
#define MONWB_CHARS_IN_FORMAT_STR		15
#define MONWB_MAX_NUM_CHARS_IN_FORMAT_STR	31
#define MONWB_MAX_NUM_CHARS_IN_ALARM_STR	81

#define MONWB_INT_FMT_STRING			"\n  %s%-24s = %-18d"
#define MONWB_INT_FMT_STRING_NO_NEWLINE		"  %s%-24s = %-18d"
#define MONWB_UINT_FMT_STRING			"\n  %s%-24s = %-18u"
#define MONWB_UINT_FMT_STRING_ARRAY		"\n  %s%-24s %d = %-18u"
#define MONWB_FLOAT_FMT_STRING			"\n  %s%-24s = %-18.3f"
#define MONWB_FLOAT_FMT_STRING_ARRAY		"\n  %s%-24s %d = %-18.3f"
#define MONWB_FLOAT_FMT_STRING_NO_NEWLINE	"  %s%-24s = %-18.3f"
#define MONWB_HEX_FMT_STRING			"\n  %s%-24s = 0x%-x"
#define MONWB_HEX_FMT_STRING_NO_NEWLINE		"  %s%-24s = 0x%-x"
#define MONWB_CHAR_FMT_STRING			"\n  %s%-24s = %-18s"
#define MONWB_CHAR_FMT_STRING_HEX_VAL		"\n  %s%-24s = %-18s (0x%-x)"
#define MONWB_CHAR_FMT_STRING_NOFIELD		"\n  %s%-24s"
#define MONWB_CHAR_FMT_STRING_NOTITLE		"\n = %-18s"
#define MONWB_CHAR_FMT_STRING_NO_NEWLINE	"  %s%-24s = %-18s"
#define MONWB_CHAR_FMT_STRING_NOFIELD_NO_NEWLINE "  %s%-24s"
#define MONWB_CHAR_FMT_STRING_NOTITLE_NO_NEWLINE " = %-18s"
#define MONWB_UNIX_DATE_FMT_STRING      	"\n  %s%-24s = %02d/%02d/%02d (%-5u)"
#define MONWB_UNIX_TIME_WITHOUT_MS_FMT_STRING	"\n  %s%-24s = %02d:%02d:%02d (%-d)"
#define MONWB_UNIX_TIME_WO_SEC_MS_FMT_STRING	"\n  %s%-24s = %02d:%02d (%-d)"
#define MONWB_UNIX_TIME_WITH_MS_FMT_STRING  	"\n  %s%-24s = %02d:%02d:%02d:%02d (%-d)"
#define MONWB_UNIX_DATE_TIME_FMT_STRING       	"\n  %s%-24s = %02d/%02d/%02d %02d:%02d:%02d"
#define MONWB_UNIX_DATE_FMT_STRING_NO_NEWLINE	"  %s%-24s = %02d/%02d/%02d"
#define MONWB_UNIX_TIME_WITHOUT_MS_FMT_STRING_NO_NEWLINE "  %s%-24s = %02d:%02d:%02d"
#define MONWB_UNIX_TIME_WITH_MS_FMT_STRING_NO_NEWLINE "  %s%-24s = %02d:%02d:%02d:%02d"
#define MONWB_UNIX_DATE_TIME_FMT_STRING_NO_NEWLINE "  %s%-24s = %02d/%02d/%02d %02d:%02d:%02d"
#define	MONWB_ALARM_FMT_STRING			"\n  %sAlarm %d: %s"
#define	MONWB_ALARM_FMT_STRING_CLEARED		"\n  %sAlarm %d (Cleared): %s"
#define	MONWB_VCP_ELEV_HDR1_FMT_STRING 		"\n  %s     Angle      Wave  Sup  DP  Az Rate    Ref   Vel   SW    S PRF          Dopp PRF (deg/#/Cnt)       "
#define	MONWB_VCP_ELEV_HDR2_FMT_STRING 		"\n  %s #  BAM/Deg  Ph Form  Res      BAM/DPS   Thrsh Thrsh Thrsh (#/Cnt)     S1          S2          S3     " 
#define	MONWB_VCP_ELEV_HDR3_FMT_STRING 		"\n  %s-- --------- -- ----- --- ---------- ----- ----- ----- ------- ----------- ----------- -----------"
#define	MONWB_VCP_ELEV_DATA_FMT_STRING 		"\n  %s%2d %04x/%-4.1f %2d %5s %3d  %s  %04x/%-5.1f %5.1f %5.1f %5.1f %3d/%-3d %5.1f/%1d/%-3d %5.1f/%1d/%-3d %5.1f/%1d/%-3d"


/* Define LB message buffer sizes */
#define MONWB_REQ_LB_BUF_SIZE			128
#define MONWB_RESP_LB_BUF_SIZE			128


/* Misc defines */
#define MONWB_COMMS_HDR_SIZE_BYTES		24
#define MONWB_MSG_HDR_SIZE_SHORTS		8
#define MONWB_MSG_HDR_SIZE_BYTES		16
#define MONWB_ADAPT_DATA_MSG_TYPE_NUMBER	18
#define MONWB_NUM_MESSAGE_TYPES			31
#define MONWB_MINS_TO_MILLISECS			60000
#define MONWB_ZERO_FLOAT_COMPARE		0.000001 /* used for doing an absolute error comparison in
                                                            determining whether or not a floating point value
                                                            is equal to zero */
#define MONWB_SIZEOF_GEN_RAD_MOMENT             28   /* bytes, does not include union at end of struct */
#define MONWB_SIZEOF_VCP_ELEV_HDR               16   /* size (bytes) of VCP_elevation_header_t BEFORE cut data */
#define MONWB_VCP_HI_DOPP_RES                    2   /* VCP msg - High doppler res code */
#define MONWB_VCP_LO_DOPP_RES                    4   /* VCP msg - Low doppler res code */
#define MONWB_VCP_SHORT_PULSE                    2   /* VCP msg - Short pulse width code */
#define MONWB_VCP_LONG_PULSE                     4   /* VCP msg - Long pulse width code */

/* Define radial categories */
#define MONWB_RADIAL_STAT_ELEV_START		0
#define MONWB_RADIAL_STAT_INTERMED		1
#define MONWB_RADIAL_STAT_ELEV_END		2
#define MONWB_RADIAL_STAT_VOLSCAN_START		3
#define MONWB_RADIAL_STAT_VOLSCAN_END		4


/* Define Request for Data types */
#define MONWB_REQDAT_RDA_STATUS			129
#define MONWB_REQDAT_RDA_PERFMAINT		130
#define MONWB_REQDAT_CLTR_FILTER_BYPASS_MAP	132
#define MONWB_REQDAT_CLTR_FILTER_NOTCHWIDTH_MAP	136
#define MONWB_REQDAT_RDA_ADAPTATION_DATA	144
#define MONWB_REQDAT_VOLUME_COVERAGE_PATTERN	160


/* Define Basedata Radial Status types */
#define MONWB_BD_RADIAL_STAT_BEG_ELEV		0
#define MONWB_BD_RADIAL_STAT_INTERMED		1
#define MONWB_BD_RADIAL_STAT_END_ELEV		2
#define MONWB_BD_RADIAL_STAT_BEG_VOL		3
#define MONWB_BD_RADIAL_STAT_END_VOL		4


/* Define RDA Status Data Transmission Enabled types */
#define MONWB_RDASTAT_DATATRANSEN_NONE		0x0002
#define MONWB_RDASTAT_DATATRANSEN_R		0x0004
#define MONWB_RDASTAT_DATATRANSEN_V		0x0008
#define MONWB_RDASTAT_DATATRANSEN_RV		0x000C
#define MONWB_RDASTAT_DATATRANSEN_W		0x0010
#define MONWB_RDASTAT_DATATRANSEN_RW		0x0014
#define MONWB_RDASTAT_DATATRANSEN_VW		0x0018
#define MONWB_RDASTAT_DATATRANSEN_RVW		0x001C


/* Define RDA Status RMS Control Status types */
#define MONWB_RDASTAT_RMS_CNTRL_NON_RMS_SYSTEM	0
#define MONWB_RDASTAT_RMS_CNTRL_RMS_IN_CONTROL	2
#define MONWB_RDASTAT_RMS_CNTRL_HCI_IN_CONTROL	4


/* Define RDA Control Cmd Data Transmission Enabled types */
#define MONWB_RDACNTRL_DATATRANSEN_NOCHANGE	0x0000
#define MONWB_RDACNTRL_DATATRANSEN_NONE		0x8000
#define MONWB_RDACNTRL_DATATRANSEN_R		0x8001
#define MONWB_RDACNTRL_DATATRANSEN_V		0x8002
#define MONWB_RDACNTRL_DATATRANSEN_W		0x8004
#define MONWB_RDACNTRL_DATATRANSEN_RV (MONWB_RDACNTRL_DATATRANSEN_R | MONWB_RDACNTRL_DATATRANSEN_V)
#define MONWB_RDACNTRL_DATATRANSEN_RW (MONWB_RDACNTRL_DATATRANSEN_R | MONWB_RDACNTRL_DATATRANSEN_W)
#define MONWB_RDACNTRL_DATATRANSEN_VW (MONWB_RDACNTRL_DATATRANSEN_V | MONWB_RDACNTRL_DATATRANSEN_W)
#define MONWB_RDACNTRL_DATATRANSEN_RVW (MONWB_RDACNTRL_DATATRANSEN_R | MONWB_RDACNTRL_DATATRANSEN_V | MONWB_RDACNTRL_DATATRANSEN_W)



/* enum for handling message name display */
enum
{
   MONWB_IS_NOT_DISPLAYED,
   MONWB_IS_DISPLAYED
};


/* enum for handling verbose levels */
enum
{
   MONWB_NONE,        /* 0 */
   MONWB_LOW,         /* 1 */
   MONWB_MODERATE,    /* 2 */
   MONWB_HIGH,        /* 3 */
   MONWB_EXTREME,     /* 4 */
   MONWB_TESTING,     /* 5 */
   MONWB_NEVER = 1000
};

/* for c++ compatability */
#ifdef __cplusplus
extern "C"
{
#endif


/* For convenience, define TRUE and FALSE */
#define MONWB_FALSE				0
#define MONWB_TRUE				1


/* For convenience, define SUCCEED and FAIL for use in returning a 
   status from a function. NOTE: these follow standard C conventions. */
#define MONWB_SUCCESS				0
#define MONWB_FAIL				-1
#define MONWB_FLOAT_FAIL			-999.9

int lb_main(void* args);
int mon_wb_lib_main(int argc, char** argv);
int _get_request_lb_id();
int _get_response_lb_id();
int get_message_contents(int msg_id_num, char **buf);
void ProcessReadRequestWrapper(char* responseLbBufPtr, int outsideMsgLen, int msgID);
char* _get_message_output_buf();
int _get_verbose_level();
void _set_verbose_level(int verb);

#ifdef __cplusplus
}
#endif
 
#endif		/* #ifndef MON_WB_H */
