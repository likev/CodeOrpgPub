/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2004/03/03 18:04:52 $
 * $Id: cvt_orpg_build_diffs.h,v 1.1 2004/03/03 18:04:52 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */



/* cvt_orpg_build_diffs.h */

#ifndef _CVT_ORPG_BUILD_DIFFS_H_ 
#define _CVT_ORPG_BUILD_DIFFS_H_

#include "prod_gen_msg.h"

/* BUILD 5 AND EARLIER INTERNAL PRODUCT HEADER */

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
    int req_num;		/* Product request number */
    unsigned int vol_num;	/* Volume scan sequence number */
    short req_params[PGM_NUM_PARAMS];
                                /* Product-dependent parameters of product
                                   request. Refer to figure 3-5 and table V
                                   of RPG/APUP ICD */
    short resp_params[PGM_NUM_PARAMS];
                                /* Product-dependent parameters of product
                                   response. Refer to figure 3-5 and table V 
                                   of RPG/APUP ICD */
} Prod_gen_msg_b5;


/* header for all products; The original RPG product and intermediate 
   product will follow this header. */
typedef struct {	/* total size - 96 bytes */

    Prod_gen_msg_b5 g;	/* product generation info */

    int elev_t;		/* elevation time; This is also set for non-elevation 
			   based product; elev_t = vol_t indicates that this 
			   is the first elevation in a volume */
    short elev_cnt;	/* generation sequence number (start with 1) of this 
			   elevation based product within a volume; 
			   elev_cnt = 1 indicates that this is the first 
			   product generated in the volume which may not 
			   correspond to the first elevation in the volume */
    short elev_ind;	/* elevation index */
    short archIII_flg;	/* This flag indicates if product being read from the
			   database is a live or archived product. Two possible
			   values for this field are PGM_IS_LIVE and 
			   PGM_IS_ARCHIVED */
    short bd_status;	/* status field in the base data header of the last 
			   radial that generated this product (it contains 
			   useful end of volume info) */
    int spot_blank_bitmap;
			/* the spot blank bitmap from the RPG basedata header */

    short wx_mode;	/* weather mode; 0 = maintenance; 
			   1 = clear air; 2 = precipitation */
    short vcp_num;	/* VCP number */

    int compr_method;	/* Compression method used. */
    int orig_size;	/* Original size of the compressed product (does not include 
			   product header size). */
    int reserved [4];

} Prod_header_b5;




/* BUILD 6 AND LATER INTERNAL PRODUCT HEADER */

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
} Prod_gen_msg_b6;


/* header for all products; The original RPG product and intermediate 
   product will follow this header. */
typedef struct {	/* total size - 96 bytes */

    Prod_gen_msg_b6 g;	/* product generation info */

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

} Prod_header_b6;







#endif
