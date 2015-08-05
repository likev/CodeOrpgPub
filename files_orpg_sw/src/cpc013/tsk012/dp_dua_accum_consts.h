/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/03/25 17:21:43 $
 * $Id: dp_dua_accum_consts.h,v 1.4 2011/03/25 17:21:43 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/****************************************************************************

	File:		dp_dua_accum_consts.h
	Author:		Zhan Zhang
	Date:		Jan 7, 2008
	
	Description:
	------------

	This is the header file to be included by dp_dua_accum_Main.c.

****************************************************************************/

#ifndef DP_DUA_ACCUM_CONSTS_H
#define DP_DUA_ACCUM_CONSTS_H

#define MAX_NUM_REQS        10
#define MSECS_PER_MINUTE    60000 /* milli-seconds per minutes */
#define MINS_PER_DAY	    1440
#define PROD_ID             173
#define OUTBUF_SIZE         350000  /* 360 * 920 + header in bytes */
#define SECS_PER_MINUTE	    60
#define MINS_PER_HOUR	    60
#define HOURS_PER_DAY	    24
#define SECONDS_PER_DAY     86400;
#define MIN_TIME_SPAN	    15      /* minimum time span is 15 minutes */
#define MSG_HDR_SIZE        18      /* msg header size in bytes */
#define PDB_SIZE            102     /* prod description block size in bytes */
#define SYMB_HDR_SIZE       16      /* symb block header size in bytes  */
#define PACKET_HDR_SIZE     14      /* packet 16 header size in bytes */
#define RADIAL_HDR_SIZE     6       /* packet 16 radial data header size
                                                                in bytes */

#define SYMB_BLK_ENTRY      120     /* offset to symb block in bytes */
#define PCKT_HDR_ENTRY      136     /* offset to packet header in bytes */
#define PCKT_DATA_ENTRY     150     /* offset to 1st radial data in bytes */

#define HYDRO_DEA_FILE  "alg.hydromet_adj."


#define DUA_PRODUCT_ID 173

#define ICENTER          0
#define JCENTER          0
#define RANGE_SCALE_FACT 250
#define SCALE_FACTOR     10
#define DELTA_ANGLE      10

#define SYMB_OFFSET  120

#define PROMPT1 "No precipitation detected during the specified time span"
#define PROMPT2 "No accumulation records available for the specified time span"

#define DP_DUA_ACCUM_DEBUG FALSE

#define IN_PROD_NAME        "DP_S2S_ACCUM"
#define OUTPUT_PRODUCT_NAME "DUAPROD"
#define SIZE_P173           335000 /* DSA size CCR NA10-00355 */
#define DUAPROD_ID          173

#define DEBUG FALSE

#endif /* DP_DUA_ACCUM_CONSTS_H */
