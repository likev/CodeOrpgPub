/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2004/02/05 22:38:12 $
 * $Id: orpgrat.h,v 1.3 2004/02/05 22:38:12 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef ORPGRAT_H
#define ORPGRAT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <rss_replace.h>
#include <rda_alarm_data.h>

#define MAX_RDA_ALARM_TEXT_SIZE	  80
#define ORPGRAT_MAX_RDA_ALARMS   800

#define ORPGRAT_NUM_TABLES         2
#define ORPGRAT_RDA_TABLE          0
#define ORPGRAT_ORDA_TABLE         1

/* The following structure defines an RDA Alarms Table entry.		   */
typedef struct {

	short code;		/* Alarm code.				   */
	short state;		/* Alarm state: Secondary, Maintenance     *
				 * Required, Maintenance Mandatory, or	   *
				 * Inoperable, Not Applicable.		   */
	short type;		/* Alarm type: N/A, Edge Detected, 	   *
				 * Filtered, Occurrence.		   */
	short device;		/* Alarm Device Category: Transmitter,     *
				 * Utility, Receiver/Signal Processor,     *
				 * Control, Pedestal, Archive, User, RPG,  *
				 * Wideband.				   */
	short sample;		/* Alarm reporting count (Edge Detected).  */
        short spare;            /* Added for alignment			   */
	char alarm_text[MAX_RDA_ALARM_TEXT_SIZE];	
				/* String containing Text message from	   *
				 * RDA/RPG ICD.				   */
} RDA_alarm_entry_t;

/* The following typedef supports ASCII RAT access			   */
typedef struct {

   size_t  size_bytes;     	/* Size of memory array of RAT entries  */
   RDA_alarm_entry_t *ptr;	/* pointer to array of RAT entries      */

} Orpgrat_mem_ary_t;

/*	Macros for parsing RDA Alarms Table configuration file. */
#define ORPGRAT_CS_DFLT_ALARM_FNAME         "rda_alarms_table"
#define ORPGRAT_CS_DFLT_ORDA_ALARM_FNAME    "orda_alarms_table"

#define ORPGRAT_CS_ALARM_COMMENT       '#'
#define ORPGRAT_CS_ALRMTBL_KEY         "RDA_alarms_table"
#define ORPGRAT_CS_TEXT_BUFSIZE        81
#define ORPGRAT_CS_CODE_KEY            "code"
#define ORPGRAT_CS_CODE_TOK            (1 | (CS_SHORT))
#define ORPGRAT_CS_STATE_KEY           "state"
#define ORPGRAT_CS_STATE_TOK            (1 | (CS_SHORT))
#define ORPGRAT_CS_TYPE_KEY            "type"
#define ORPGRAT_CS_TYPE_TOK            (1 | (CS_SHORT))
#define ORPGRAT_CS_DEVICE_KEY          "device"
#define ORPGRAT_CS_DEVICE_TOK          (1 | (CS_SHORT))
#define ORPGRAT_CS_SAMPLE_KEY          "sample"
#define ORPGRAT_CS_SAMPLE_TOK          (1 | (CS_SHORT))
#define ORPGRAT_CS_TEXT_KEY            "text"
#define ORPGRAT_CS_TEXT_TOK            1

/*	Various return status values for RDA status functions	*/

#define ORPGRAT_ERROR                   -9998
#define	ORPGRAT_DATA_NOT_FOUND		-9999
#define ORPGRAT_UNDEFINED_CODE             -1
#define ORPGRAT_NOT_APPLICABLE              0

/*      Prototypes for RDA Alarm Tables (contains alarms definitions)   */
int ORPGRAT_read_rda_alarms_tbl( int table_num );
int ORPGRAT_clear_rda_alarms_tbl( int table_num );
char* ORPGRAT_get_alarm_text( int code );
char* ORPGRAT_get_alarm_data( int code );

#ifdef __cplusplus
}
#endif

#endif

