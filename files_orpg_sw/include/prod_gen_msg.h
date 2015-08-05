/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/21 23:04:09 $
 * $Id: prod_gen_msg.h,v 1.21 2006/09/21 23:04:09 steves Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */
/**************************************************************************

      Module: prod_gen_msg.h

 Description: ORPG Product Generation Message header file

       Notes: All constants defined in this file begin with the prefix
              PGM_.
 **************************************************************************/

#ifndef PROD_GEN_MSG_H
#define PROD_GEN_MSG_H

#include <time.h>
#include <lb.h>

/* for c++ compatability */
#ifdef __cplusplus
extern "C"
{
#endif


#define PGM_NUM_PARAMS	 6

/* The following Prod_gen_msg.len macros are used for indicating prod
   generation failure */
#define PGM_CPU_LOADSHED		-1
#define PGM_MEM_LOADSHED		-2
#define PGM_SCAN_ABORT			-3
#define PGM_DISABLED_MOMENT		-4
#define PGM_ABORT_REMAIN_SCAN  		-5
#define PGM_TASK_FAILURE        	-6
#define PGM_REPLAY_DATA_UNAVAILABLE     -7
#define PGM_INVALID_REQUEST             -8
#define PGM_SLOT_UNAVAILABLE            -9
#define PGM_INPUT_DATA_ERROR           -10
#define PGM_TASK_SELF_TERMINATED       -11
#define PGM_PROD_NOT_GENERATED         -12

#define PGM_MIN_GEN_FAILURE     PGM_PROD_NOT_GENERATED
#define PGM_MAX_GEN_FAILURE     PGM_CPU_LOADSHED

#define PGM_REALTIME_STREAM     ORPGTAT_REALTIME_STREAM
#define PGM_REPLAY_STREAM       ORPGTAT_REPLAY_STREAM

#define PGM_IS_LIVE      1
#define PGM_IS_ARCHIVED  -1
/*
 * Product Generation Messages are written to the ORPGDAT_PROD_GEN_MSGS
 * data store.
 */
typedef struct {
    short prod_id;		/* Product id (buffer number) */
    short input_stream;		/* Replay or realtime product */
    LB_id_t id;			/* corresponding product LB message ID */
    time_t gen_t;		/* local clock (Unix) generation time */
    int vol_t;			/* volscan start time (secs since 01JAN1970) */
    int len;			/* Total product length (bytes).  If
				   this value is negative, indicates
				   an abort message.  The macro definitions
				   for abort reasons are define above. */
    unsigned short req_num;	/* Product request number */
    short elev_ind;	        /* elevation index */
    unsigned int vol_num;	/* Volume scan sequence number */
    short req_params[PGM_NUM_PARAMS];
                                /* Product-dependent parameters of product
                                   request. Refer to figure 3-5 and table V
                                   of RPG/APUP ICD */
    short resp_params[PGM_NUM_PARAMS];
                                /* Product-dependent parameters of product
                                   response. Refer to figure 3-5 and table V 
                                   of RPG/APUP ICD */
} Prod_gen_msg;


/* header for all products; The original RPG product and intermediate 
   product will follow this header. */
typedef struct {	/* total size - 96 bytes */

    Prod_gen_msg g;	/* product generation info */

    int elev_t;		/* elevation time; This is also set for non-elevation 
			   based product; elev_t = vol_t indicates that this 
			   is the first elevation in a volume */
    short elev_cnt;	/* generation sequence number (start with 1) of this 
			   elevation based product within a volume; 
			   elev_cnt = 1 indicates that this is the first 
			   product generated in the volume which may not 
			   correspond to the first elevation in the volume */
    short archIII_flg;	/* This flag indicates if product being read from the
												   database is a live or archived product. Two possible
												   values for this field are PGM_IS_LIVE and 
												   PGM_IS_ARCHIVED */
    short bd_status;	/* status field in the base data header of the last 
			   radial that generated this product (it contains 
			   useful end of volume info) */
    short spare;
    int spot_blank_bitmap;
			/* the spot blank bitmap from the RPG basedata header */

    short wx_mode;	/* weather mode; 0 = maintenance; 
			   1 = clear air; 2 = precipitation */
    short vcp_num;	/* VCP number */

    int compr_method;	/* Compression method used. */
    int orig_size;	/* Original size of the compressed product (does not include 
			   product header size). */
    int reserved [4];

} Prod_header;


#ifdef __cplusplus
}
#endif

#endif /*DO NOT REMOVE!*/
