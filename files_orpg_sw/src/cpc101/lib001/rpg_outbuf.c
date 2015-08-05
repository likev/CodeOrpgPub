/********************************************************************

	This module contains the functions that support 
	A31215__GET_OUTBUF and A31216__REL_OUTBUF.

********************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/10/24 14:24:44 $
 * $Id: rpg_outbuf.c,v 1.62 2013/10/24 14:24:44 steves Exp $
 * $Revision: 1.62 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/times.h>
#include <limits.h>

#include <a309.h>
#include <orpg.h>

#include <lb.h>
#include <misc.h>
#include <product.h>
#include <en.h>
#include <rpg_port.h>
#include <itc.h>
#include <rpg_vcp.h>

#include <prod_gen_msg.h>	/* Prod_gen_msg			*/
#include <prod_request.h>
#include <gen_stat_msg.h>
#include <alg_cpu_stats.h>

#include <rpg.h>

static char *Task_name = NULL;
                        /* Task Name (not necessarily executable name). */

static Buf_regist Buffers [MAXN_BUFS]; 		/* array of allocated buffers; */
static int N_bufs = 0;				/* number of allocated buffers; */

static Out_data_type Out_list [MAXN_OUTS]; 	/* list of output products */
static int N_outs = 0;				/* number of output products */

static In_data_type *Inp_list; 			/* list of input products */
static int N_inps = 0;				/* number of input products */

static Prod_header Hd_info;			/* for storing the product header info */
static int Hd_info_set = 0;			/* flag indicating that Hd_info is set */

static int Input_stream = PGM_REALTIME_STREAM; 	/* Input data stream. */

static int Use_suppl_scans = 0;                 /* Use supplemental scans ... force use if
                                                   task produces a RADIAL_DATA output. */

/*
 * Change the params from APUP Table 10 to APUP table 2-A.
 * 
 * This table is indexed by Product ID and Product-Dependent Parameter Number.
 * The legacy Product Code is provided for reference only.  The numbers
 * listed at the top and bottom of the table identify the Graphic Product
 * Message word numbers that correspond to the various parameters.
 * 
 * Positive values correspond to (zero-ordinal) indices into the
 * product-dependent parameters array.
 * 
 * If the HalfWord is defined in Table V of RPG/APUP ICD Doc. # 2620001 and
 * not used then its index value is -2 and if it is not defined in that table
 * then its index value is -1. 
 * 
 * Note: This table only necessary to support legacy products.  With new
 *       products and product code, this table becomes obsolete because
 *       of rule-based product generation.
 */

#define NUM_DEPEND_PARAMS      10

static char p10_p6[LEGACY_MAX_BUFFERNUM+1][NUM_DEPEND_PARAMS] = {
/* Bfr Code      27,   28,   30,   47,   48,   49,   50,   51,   52,   53 */
/*   0  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*   1  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*   2  19 */  { -1,   -1,    2,   -2,   -1,   -1,   -1,   -2,   -2,   -1} ,
/*   3  16 */  { -1,   -1,    2,   -2,   -1,   -1,   -1,   -2,   -2,   -1} ,
/*   4  20 */  { -1,   -1,    2,   -2,   -1,   -1,   -1,   -2,   -2,   -1} ,
/*   5  17 */  { -1,   -1,    2,   -2,   -1,   -1,   -1,   -2,   -2,   -1} ,
/*   6  21 */  { -1,   -1,    2,   -2,   -1,   -1,   -1,   -2,   -2,   -1} ,
/*   7  18 */  { -1,   -1,    2,   -2,   -1,   -1,   -1,   -2,   -2,   -1} ,
/*   8  28 */  { -1,   -1,    2,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*   9  29 */  { -1,   -1,    2,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  10  30 */  { -1,   -1,    2,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  11  25 */  { -1,   -1,    2,   -2,   -2,   -1,   -1,   -1,   -1,   -1} ,
/*  12  22 */  { -1,   -1,    2,   -2,   -2,   -1,   -1,   -1,   -1,   -1} ,
/*  13  26 */  { -1,   -1,    2,   -2,   -2,   -1,   -1,   -1,   -1,   -1} ,
/*  14  23 */  { -1,   -1,    2,   -2,   -2,   -1,   -1,   -1,   -1,   -1} ,
/*  15  27 */  { -1,   -1,    2,   -2,   -2,   -1,   -1,   -1,   -1,   -1} ,
/*  16  24 */  { -1,   -1,    2,   -2,   -2,   -1,   -1,   -1,   -1,   -1} ,
/*  17  49 */  {  0,    1,    2,   -2,   -2,   -2,   -2,   -1,   -1,   -1} ,
/*  18  39 */  { -1,   -1,   -1,   -2,   -1,   -1,   -1,   -2,   -2,   -2} ,
/*  19  40 */  { -1,   -1,   -1,   -2,   -1,   -1,   -1,   -2,   -2,   -2} ,
/*  20  na */  { -1,   -1,    2,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  21  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  22  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  23  37 */  { -1,   -1,   -1,   -2,   -1,   -1,   -1,   -2,   -2,   -1} ,
/*  24  35 */  { -1,   -1,   -1,   -2,   -1,   -1,   -1,   -2,   -2,   -1} ,
/*  25  38 */  { -1,   -1,   -1,   -2,   -1,   -1,   -1,   -2,   -2,   -1} ,
/*  26  36 */  { -1,   -1,   -1,   -2,   -1,   -1,   -1,   -2,   -2,   -1} ,
/*  27  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  28  42 */  { -1,   -1,   -1,   -2,   -1,   -1,   -1,   -1,   -1,    5} ,
/*  29  41 */  { -1,   -1,   -1,   -2,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  30  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  31  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  32  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  33  59 */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  34  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  35  48 */  { -1,   -1,   -1,   -2,   -2,   -2,   -1,   -1,   -1,   -1} ,
/*  36  63 */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  37  64 */  { -1,   -1,   -1,   -2,   -2,   -2,   -1,   -2,   -2,   -1} ,
/*  38  65 */  { -1,   -1,   -1,   -2,   -2,   -2,   -1,   -2,   -2,   -1} ,
/*  39  66 */  { -1,   -1,   -1,   -2,   -2,   -2,   -1,   -2,   -2,   -1} ,
/*  40  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  41  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  42  60 */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  43  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  44  67 */  { -1,   -1,   -1,   -2,   -2,   -2,   -1,   -1,   -1,   -1} ,
/*  45  68 */  { -1,   -1,   -1,   -2,   -2,   -2,   -1,   -1,   -1,   -1} ,
/*  46  69 */  { -1,   -1,   -1,   -2,   -2,   -2,   -1,   -1,   -1,   -1} ,
/*  47  an */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  48  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  49  62 */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  50  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  51  58 */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  52  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  53  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  54  83 */  { -1,   -1,   -1,   -1,   -1,   -2,   -2,   -1,   -1,   -1} ,
/*  55  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  56  31 */  {  0,    1,   -2,   -2,   -2,   -2,   -2,   -2,   -2,   -2} ,
/*  57  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  58  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  59  46 */  {  0,    1,    2,   -2,   -2,   -2,   -1,   -2,   -1,   -1} ,
/*  60  43 */  {  0,    1,    2,   -2,   -1,   -2,   -1,   -2,   -1,   -1} ,
/*  61  45 */  {  0,    1,    2,   -2,   -1,   -2,   -1,   -2,   -1,   -1} ,
/*  62  44 */  {  0,    1,    2,   -2,   -2,   -2,   -1,   -2,   -1,   -1} ,
/*  63  47 */  { -1,   -1,   -1,   -2,   -2,   -1,   -1,   -1,   -1,   -1} ,
/*  64  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  65  70 */  { -1,   -1,   -1,   -2,   -2,   -2,   -1,   -1,   -1,   -1} ,
/*  66  71 */  { -1,   -1,   -1,   -2,   -2,   -2,   -1,   -1,   -1,   -1} ,
/*  67  72 */  { -1,   -1,   -1,   -1,   -2,   -2,   -1,   -1,   -1,   -1} ,
/*  68  56 */  { -1,   -1,    2,   -2,   -2,   -2,   -1,    3,    4,   -1} ,
/*  69  55 */  {  0,    1,    2,   -2,   -2,   -2,   -2,    3,    4,   -2} ,
/*  70  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  71  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  72  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  73  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  74  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  75  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  76  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  77  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  78  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  79  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  80  61 */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  81  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  82  52 */  { -1,   -1,   -1,    0,    1,    2,    3,   -1,   -1,   -1} ,
/*  83  51 */  { -1,   -1,   -1,    0,    1,    2,    3,   -1,   -1,   -1} ,
/*  84  50 */  { -1,   -1,   -1,    0,    1,    2,    3,   -2,   -2,   -1} ,
/*  85  57 */  { -1,   -1,   -1,   -2,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  86  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  87  93 */  { -1,   -1,    2,   -2,   -2,   -1,   -1,   -1,   -1,   -1} ,
/*  88  53 */  {  0,    1,   -2,   -2,   -2,    3,    4,   -1,   -1,   -1} ,
/*  89  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  90  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  91  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  92  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  93  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  94  94 */  { -1,   -1,    2,   -2,   -1,   -1,   -1,   -2,   -2,   -1} ,
/*  95  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  96  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  97  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  98  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/*  99  99 */  { -1,   -1,    2,   -2,   -2,   -1,   -1,   -1,   -1,   -1} ,
/* 100  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 101  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 102  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 103  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 104  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 105  78 */  { -1,   -1,   -1,   -2,   -2,   -2,   -2,   -2,   -1,   -1} ,
/* 106  79 */  { -1,   -1,   -1,   -2,   -2,   -2,   -2,   -2,   -1,   -1} ,
/* 107  80 */  { -1,   -1,   -1,   -2,   -2,   -2,   -2,   -2,   -2,   -2} ,
/* 108  81 */  { -1,   -1,   -1,   -2,   -2,   -2,   -2,   -2,   -1,   -1} ,
/* 109  82 */  { -1,   -1,   -1,   -2,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 110  84 */  { -1,   -1,    2,   -2,   -2,   -2,   -2,   -2,   -1,   -1} ,
/* 111  85 */  { -1,   -1,   -1,    0,    1,    2,    3,   -2,   -2,   -1} ,
/* 112  86 */  { -1,   -1,   -1,    0,    1,    2,    3,   -1,   -1,   -1} ,
/* 113  na */  { -1,   -1,   -2,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 114  87 */  { -1,   -1,   -2,   -2,   -2,   -2,   -2,   -1,   -1,   -1} ,
/* 115  88 */  { -1,   -1,   -2,   -2,   -2,   -2,   -2,   -1,   -1,   -1} ,
/* 116  89 */  { -1,   -1,   -1,   -2,   -2,   -2,   -1,   -2,   -2,   -1} ,
/* 117  90 */  { -1,   -1,   -1,   -2,   -2,   -2,   -1,   -2,   -2,   -1} ,
/* 118  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 119  34 */  {  0,   -1,   -1,   -1,   -2,   -2,   -2,   -2,   -1,   -1} ,
/* 120  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 121  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 122  na */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 123  97 */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 124  95 */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 125  98 */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 126  96 */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 127  ?? */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 128  ?? */  { -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1} ,
/* 129  74 */  { -1,   -1,   -1,   -1,   -1,   -2,   -2,   -2,   -1,   -1} ,
/* 130  83 */  { -1,   -1,   -1,   -1,   -1,   -2,   -2,   -2,   -1,   -1} 
/* Buf Code      27,   28,   30,   47,   48,   49,   50,   51,   52,   53 */
};

/* Product dependent parameter (short) offsets used for HYUSPACC 
   special processing.  Some are shared for USWACCUM and ASDACCUM
   special processing. */
#define USP_END_TIME          0
#define USP_DURATION          1
#define HRDB_INIT_VALUE      -2

/* Product dependent parameter (short) offsets used for SRMRVMAP and
   SRMRVREG special processing. */
#define STORM_SPEED_INDEX     3
#define STORM_DIRECTION_INDEX 4

/* The number of bits in an unsigned long */
#define BITS_PER_BYTE         8
#define NUM_BITS              (BITS_PER_BYTE*sizeof( unsigned long ))

/* Local functions */
static void Set_outbuf_prod_hdr (Buf_regist *buffer, int olind);
static void More_phd_fields (Graphic_product *phd);
static int  Output_product (int ind, int olind);
static int  Output_alert_message_product (char *buf, int ind, int olind);
static void Product_specific_processing( Prod_header *phd, 
                                         Graphic_product *descrip_block );
static void User_selectable_precip_processing( Prod_header *phd );
static int  Set_prod_depend_params( int ind, int olind );
static int  Write_product( char *buf, int ind, int olind, int stream );
static int  Write_informational_message( char *buf, int olind, 
                                         int stream, int len );
static int  Register_output( int data_type );
static void Write_prod_gen_msg( Prod_gen_msg *hd );
static int  Find_buffer_in_list( int *bufptr, int *buf_ind, int *out_ind,
                                 int datdis );

#ifdef PROD_STATISTICS_DEFINED
static void Product_statistics( int olind, int len );
#endif

/* Public Functions. */

/********************************************************************

   Description: 
      This function registers the output in the output data list.

   Input:	
      data_type - The output data type (RPG buffer type)
      timing - OBSOLETE ... value set using PAT.
      product_code - OBSOLETE ... value set using PAT.

   Output:

   Return:	
      Return value is 0 on success or -1 on failure.

   Notes:
      For compatibility, product_code remains in the argument list.
      It is ignored.  The product_code is derived internally from
      the data_type via the PAT.

********************************************************************/
int RPG_out_data( fint *data_type, fint *timing, fint *product_code ){

    /* Register the data type in Out_list. */
    if( Register_output( *data_type ) >= 0 )
       return (0);

    return (-1);

/* End of RPG_out_data() */
}

/********************************************************************

   Description: 
      This function registers the output in the output data list.
      An event ID associated with this output data is also provided.

   Input:
      data_name - The output data name
      event_id - Event ID associated with this output data.  Upon 
                 release of this output data, an event is posted with 
                 this event code identifier.

   Output:
      product_id - product ID associated with the data name.

   Return:	
      Return value is 0 on success or -1 on failure.

   Notes:

********************************************************************/
int RPG_out_data_by_name_wevent( char *data_name,  EN_id_t *event_id, 
                                 fint *product_id ){

   int timing = 0;
   int product_code = 0;

   OB_get_id_from_name( data_name, product_id );
   if( *product_id <= 0 )
      return(-1);

   return( RPG_out_data_wevent( product_id, &timing, event_id, 
                                &product_code ) );

/* End of RPG_out_data_wevent() */
}

/********************************************************************

   Description: 
      This function registers the output in the output data list.
      An event ID associated with this output data is also provided.

   Input:
      data_type - The output data type (RPG buffer type)
      timing - OBSOLETE .... value set using PAT
      event_id - Event ID associated with this output data.  Upon 
                 release of this output data, an event is posted with 
                 this event code identifier.
      product_code - OBSOLETE ... value set using PAT

   Output:

   Return:	
      Return value is 0 on success or -1 on failure.

   Notes:

********************************************************************/
int RPG_out_data_wevent( fint *data_type, fint *timing, EN_id_t *event_id,
                         fint *product_code){

    int ind;

    /* Register the data type in Out_list. */
    ind = Register_output( *data_type );

    /* If registration was successful. */
    if( ind >= 0 ){

       /* Register the event id. */
       Out_list [ind].event_id = *event_id;

       return (0);

    }
    
    return (-1);

/* End of RPG_out_data_wevent() */
}

/********************************************************************

   Description:
      Returns the product code given the data name.

   Inputs:
      data_name - the product's data name.

   Outputs:
      pcode - Either value pcode if data_name refers to the final
              product, 0 if data_name refers to an intermediate 
              product, or negative number on error.

********************************************************************/
void RPG_get_code_from_name( char *data_name, int *pcode ){

   int prod_id;
  
   OB_get_id_from_name( data_name, &prod_id );

   *pcode = ORPGPAT_get_code( prod_id );

   return;

/* End of RPG_get_code_from_name() */
}

/********************************************************************

   Description:
      Returns the product code given the prod id.

   Inputs:
      prod_id - the product ID.

   Outputs:
      pcode - Either value pcode if prod_id refers to the final
              product, 0 if prod_id refers to an intermediate 
              product, or negative number on error.

********************************************************************/
void RPG_get_code_from_id( int *prod_id, int *pcode ){

   *pcode = ORPGPAT_get_code( *prod_id );

   return;

/* End of RPG_get_code_from_id() */
}

/********************************************************************

   Description: 
      This function is used to get product customizing data for all 
      products generated by the calling task.

   Inputs:
       elev_index - elevation index of product

   Outputs:	
      user_array - array containing customizing data.
      num_requests - number of product requests at elevation index
      status - status of operation (0 - normal, 1 - failure )

   Notes:	
      The user_array elements are defined as follows:

	 	user_array[][0] = product_code
		user_array[][1] = product dependent parm 1
		user_array[][2] = product dependent parm 2
		user_array[][3] = product dependent parm 3
		user_array[][4] = product dependent parm 4
		user_array[][5] = product dependent parm 5
		user_array[][6] = product dependent parm 6
		user_array[][7] = elevation index
		user_array[][8] = request number
		user_array[][9] = not used

    	 The product dependent parameters are defined in
	 the RPG/Associated PUP ICD.  See RPG_get_request
	 for more information.

   Returns: 	 
      There is no return value defined for this module.

********************************************************************/
void RPG_get_customizing_info( fint *elev_index, 
                               fint2 *user_array[][UA_NUM_FIELDS],
                               fint *num_requests, 
                               fint *status ){

   int buf_type;
   int prod_code;
   int index;
   int el_ind;
   int product_found;
   int i;

   /* Initialize status to normal. */
   *status = 0;

   /* Examine all outputs registered by this task.  If the
      output type is associated with a product code, get the
      customizing data for that product. */
   index = 1;
   product_found = 0;
   for( i = 0; i < N_outs; i++ ){

      if( Out_list [i].int_or_final != INT_PROD ){

         product_found = 1;

         buf_type = Out_list [i].type;  	/* Buffer or product id */
         prod_code = Out_list [i].int_or_final;	/* Product code */
         el_ind = *elev_index;			/* Elevation index */
         RPG_get_request( el_ind, buf_type, prod_code,
                          &index, (short *) user_array );
      }                       

   }

   /* If no product codes registered for this task, return status of 1. */
   if( !product_found ) *status = 1;

   /* Set number of requests. */
   *num_requests = index - 1;

   return;

/*End of RPG_get_customizing_data() */
}

/* Private Functions. */
/***********************************************************************

    Description: 

        This function is the emulated A31215__GET_OUTBUF 
	function. The following is the original description of 
	the function:

	Allocate a buffer for the calling task to use for product generation.

	Further info was found in "nexrad software maintenance training",
	page RPG-SG-04-02-3.

    Inputs:	

        mem - the MEM array
	dattyp - data type (new prod id; i.e. buf id) to be put in the buffer;
	bufsiz - requested buffer size in number of (4-byte) integers.

    Outputs:	

        bufptr - index in MEM for the first element of the buf;
	opstat - status as defined in iobuf.h: NORMAL, NOT_REQD, which 
                 means that "dattyp" is not requested by the task, or
                 NO_MEM which means an output buffer could not be 
                 allocated.

    Return:	

        Return value is never used.

    Notes:	


	For radial timing buffer, only a single buffer is allocated. The buffer 
        will not be freed for higher efficiency. The ORPG header is allocated but 
        never used in this case.

***********************************************************************/
int OB_get_outbuf( int *mem, int *dattyp, int *bufsiz, int *bufptr, 
                   int *opstat){

    static char *bd_buf = NULL;	/* base data buffer which is never freed */
    static int bd_buf_size = 0;	/* size of bd_buf */
    char *cpt;
    Prod_header *phd;
    int olind;			/* index in Out_list of this buffer */
    int size;
    int n_tests;
    int i;

    /* Check whether the product is registered and requested.  If product 
       timing is DEMAND_DATA, product is always required. Ignore SCRATCH 
       buffers. */
    if (*dattyp != SCRATCH) {

	for (olind = 0; olind < N_outs; olind++) {

            /* If match found with registered output, then ... */
	    if (*dattyp == Out_list[olind].type) {

                /* If not requested and not DEMAND_DATA, data is not
                   required. */
		if (!(Out_list[olind].requested)
                              &&
                     (Out_list[olind].timing != DEMAND_DATA)) {

		    *opstat = NOT_REQD;
		    return (0);

		}
		break;

	    }

        /* End of "for" loop. */
	}
	if (olind >= N_outs)
	    PS_task_abort ("Output type (%d) is not registered\n", *dattyp);

    }
    else
	olind = -1;

    /* Determine total buffer size, in bytes.  Includes orpg product
       header. */
    size = *bufsiz * 4 + sizeof (Prod_header);
    cpt = NULL;

    /* If not a SCRATCH buffer, then ... */
    if (olind >= 0) {

        /* Save buffer size (in bytes). */
	Out_list[olind].len = size;
	if (Out_list[olind].timing == RADIAL_DATA) {

            /* For radial buffers, we only allocate buffer once. We reuse
               this buffer for all subsequent outputs since the size is
               fixed.  This is done for efficiency. */
	    cpt = bd_buf;
	    if( (cpt != NULL) && (bd_buf_size < size) ){

                /* If the size changes and is larger, use this newer,
                   larger size. */
		LE_send_msg( GL_INFO, "Basedata Buf of Diff Size Requested\n");
                free( cpt );
                cpt = NULL;
                bd_buf_size = 0;

            }

	}

    }

    /* Allocate the space for the output buffer.  Allocation will be attempted
       twice before we give up and return NO_MEM. */
    n_tests = 0;
    while (cpt == NULL && n_tests < 2) {

	cpt = (char *) malloc (size);
	if (cpt == NULL){

           /* try again after a short sleep */
           n_tests++;
	   msleep (1000);

        }
	else if (olind >= 0 && Out_list[olind].timing == RADIAL_DATA){

           /* Save the buffer pointer and size if this is a radial buffer. */
	   bd_buf = cpt;
	   bd_buf_size = size;

	}

    }

    /* If failure to allocate an output buffer, return NO_MEM. */
    if( cpt == NULL ){

       LE_send_msg( GL_MEMORY, "Output Buffer Memory Allocation Failed.\n" );
       *opstat = NO_MEM;
       return (0);

    }

    /* Output buffer acquired successfully.  Initialize orpg product header. */
    phd = (Prod_header *) cpt;
    memset( phd, -1, sizeof(Prod_header) );

    /* Initialize the compression method and size fields.  All the other fields
       will be set upon release of the output buffer. */ 
    phd->compr_method = COMPRESSION_NONE;
    phd->orig_size = phd->g.len = size;

    /* Calculate index into "mem" buffer.  This is necessary to support legacy
       algorithms. */
    *bufptr = (int *)(cpt + sizeof (Prod_header)) - mem + 1;

    /* Find empty (unused) slot in buffer pointer list. */
    for (i = 0; i < N_bufs; i++){

	if (Buffers [i].bpt == NULL)
	    break;

    }

    /* Register the buffer. */
    if (i < MAXN_BUFS) {

	Buffers [i].type = *dattyp;
	Buffers [i].size = *bufsiz;
	Buffers [i].ptr = *bufptr;
	Buffers [i].bpt = cpt;
	Buffers [i].req = NULL;

	if (i >= N_bufs)
	    N_bufs = i + 1;

    }
    else 
	PS_task_abort ("Too Many Output Buffers Registered\n");

    /* If dattyp associated with a product code, then initialize the product
       header and description block before returning. */
    if( ORPGPAT_get_code( *dattyp ) > 0 )
       memset( (void *) &mem[*bufptr-1], 0, sizeof( Graphic_product ) );

    /* Return status of operation as NORMAL. */
    *opstat = NORMAL;
    return (0);

/* End of OB_get_outbuf() */
}


/***********************************************************************

    Description: 

        This function is the emulated A31216__REL_OUTBUF function. The 
        following is the original description of the function:

	Release a completed output buffer for further processing.  The
	data may be either forwarded to predetermined tasks or may be
	destroyed.

    Inputs:	

        bufptr - the index into the array MEM of the starting position of 
                 the buffer to be rleased

	datdis - defines the disposition of the data contained in the buffer: 
                 FORWARD or DESTROY;

        ... - extended argument.   If defined, contains the size of the 
              output. 

    Return:	

        Returns 0 is everything normal, otherwise:

           RPG_BUF_NOT_FOUND - buffer pointer (bufptr) not registered and
                               dispositions is DESTROY

    Notes:  
 
        If disposition is FORWARD and an error occurs, the process is 
        terminated

***********************************************************************/
int OB_rel_outbuf (int *bufptr, int *datdis, ... ){

    int ind, olind, error = 0;
    int disposition = (*datdis) & DISPOSITION_MASK;
    int extended_args = (*datdis) & EXTENDED_ARGS_MASK;

    va_list ap;
    int *size = NULL;

    /* Search for the buffer in the list of allocated buffers. */
    if( (error = Find_buffer_in_list( bufptr, &ind, &olind, *datdis )) < 0 )
       return( error );

    /* If there is an extended argument, process it here. */
    if( extended_args ){

       va_start( ap, datdis );
       size = va_arg( ap, int * );
       va_end( ap );

       /* Cannot change the size of radial data messages and
          final products.  Furthermore if the disposition is
          DESTROY, no need to set the size. */ 
       if( (Out_list[olind].timing != RADIAL_DATA) 
                           &&
           (Out_list[olind].int_or_final == INT_PROD) 
                           &&
           (disposition != DESTROY) ){

          Prod_header *phd = (Prod_header *) Buffers[ind].bpt;

          /* If the size specified is an improbable length, don't change
             the size.   Release the full product. */
          if( (*size < 0) || ((*size + sizeof(Prod_header)) > phd->g.len) )
             LE_send_msg( GL_ERROR, "Improbable Size (%d) in RPG_rel_product\n",
                          *size );
       
          else{
   
             /* A value of 0 for size indicates the size can be ignored. */
             if( *size != 0 )
                phd->g.len = phd->orig_size = *size + sizeof(Prod_header);

          }

       }

    }

    /* Change the generation time of product. */
    if( (Out_list[olind].int_or_final != INT_PROD)
                         &&
              (disposition != DESTROY) ){

       /* Does this product have a product description block?  Products
          codes >= ORPGDBM_MIN_PCODE. */
       unsigned int gen_date = 0, gen_time = 0;
       time_t cur_time = 0;
       Graphic_product *descrip_block = (Graphic_product *) bufptr;

       /* Get the produce code from the Product ID.  If code is 
          >= ORPGDBM_MIN_PCODE, then set the product generation time 
          in the Product Description Block. */
       if( (Out_list[olind].int_or_final >= ORPGDBM_MIN_PCODE) 
                                &&
           (Out_list[olind].int_or_final == descrip_block->prod_code) ){

          int yr, mo, day, hr, min, sec;
 
          /* Set the product generation date and time. */
          cur_time = time( NULL );
          gen_time = RPG_TIME_IN_SECONDS( cur_time );
          gen_date = RPG_JULIAN_DATE( cur_time );
          RPG_set_product_int( (void *) &descrip_block->gen_time, (void *) &gen_time );
          descrip_block->gen_date = gen_date;
 
          unix_time( &cur_time, &yr, &mo, &day, &hr, &min, &sec );
          if( yr >= 2000 )
             yr -= 2000;
          else
             yr -= 1900;
          LE_send_msg( GL_INFO, "Product Generation Date/Time: %02d/%02d/%02d %02d:%02d:%02d\n",
                       yr, mo, day, hr, min, sec );

       }

    }

    /* Write product if disposition is FORWARD. Release storage associated 
       with output buffer. */
    OB_release_output_buffer( ind, olind, disposition );

    return( 0 );

/* End of OB_rel_outbuf() */
}

/***********************************************************************

    Description: 
       Release all acquired outputs with disposition "datdis".

    Inputs:	

       datdis - defines the disposition of the data contained in the buffer: 
                FORWARD or DESTROY;

    Outputs:

    Return:	
       Currently always returns 0.

    Notes:  
 
***********************************************************************/
int OB_rel_all_outbufs (int *datdis){

    int ind;
    int olind;
    int disposition = *datdis;

    /* Write out debug information if DEBUG flag set. */
#ifdef DEBUG 
    LE_send_msg( GL_INFO, "--->Releasing all output buffers\n" );
#endif

    /* Search for the buffer in the list of allocated buffers. */
    for (ind = 0; ind < N_bufs; ind++) {

       /* Find the index of the datatype in the list of registered output
          data types (Out_list). */
       for (olind = 0; olind < N_outs; olind++) {

          if (Out_list[olind].type == Buffers[ind].type)
	     break;

       }

       /* Write out debug information if DEBUG flag set. */
#ifdef DEBUG 
       LE_send_msg( GL_INFO, "------>Destroying output buffer @ %p\n",
                    Buffers[ind].bpt );
#endif

       /* Release storage associated with output buffer. */
       OB_release_output_buffer( ind, olind, disposition );

    }

    return( 0 );

/* End of OB_rel_all_outbufs() */
}

/*******************************************************************

   Description:
      Frees space allocated to output buffer.  

      If disposition is FORWARD, writes all registered ITCs, sets 
      the product header, and writes the product buffer.  

   Inputs:
      ind - index of buffer in the allocated buffers list.
      olind - index of the buffer in the outputs list.
      disposition - disposition of the buffer.  Can be either
                    FORWARD or DESTROY.

   Returns:
      Always returns 0.  If an attempt is made to forward an
      unregisted output, the process with terminate.

*******************************************************************/
int OB_release_output_buffer( int ind, int olind, int disposition ){

   /* Output the product if data disposition is FORWARD. */
   if (disposition == FORWARD) {

      if (olind >= N_outs)
         PS_task_abort ("Attempted to Forward Unregistered Output.\n");

      /* Write all ITCs which are associated with the release of 
         datatype; This must be before Output_product. */
      ITC_write_all( Buffers[ind].type );

      /* Set product orpg header. */
      Set_outbuf_prod_hdr (Buffers + ind, olind);

      /* Output product buffer. */
      Output_product (ind, olind);

      if( Out_list[olind].timing == ELEVATION_DATA )
         WA_set_output_info( Buffers[ind].bpt, olind );

   }

   /* Free the buffer (registered radial output is not freed). */
   if (Buffers[ind].bpt != NULL) {

      /* NOTE: olind >= N_outs indicates a SCRATCH buffer. */
      if (olind >= N_outs || Out_list[olind].timing != RADIAL_DATA){

         free (Buffers[ind].bpt);

         if( olind >= N_outs && Buffers[ind].type != SCRATCH )
            LE_send_msg( GL_ERROR, "Attempted Release of Invalid Buffer\n" );

       }

       Buffers[ind].bpt = NULL;

   }

   /* Free the product request data if defined. */
   if (Buffers[ind].req != NULL ){

      free(Buffers[ind].req);
      Buffers[ind].req = NULL;

   }

   return (0);

/* End of OB_release_output_buffer */
}

/**************************************************************
   Description:
      RPG Interface for all outputs to be registered using
      a single function call.
    
   Inputs:
      task_name - the ASCII task name (see product/task table).

   Returns:
      Returns -1 on error, or 0 otherwise.

**************************************************************/
void OB_reg_outputs( int *status ){

   Orpgtat_entry_t *tat = NULL;
   int num_outputs, timing = 0, product_code = 0, i;
   int *output_ids = NULL;
  
   *status = 0;

   /* Register for LE services if not already registered. */
   RPG_init_log_services_c();

   /* Get my task_name ... may be different than executable name.
      Output registration are based on task_name, not executable 
      name. */
   Task_name = INIT_task_name();

   /* Get the TAT entry for this task name. */
   if( (Task_name == NULL) 
                || 
       ((tat = ORPGTAT_get_entry( Task_name )) == NULL) )
      PS_task_abort( "Unable to Get Task Entry in RPGC_reg_outputs\n" );
  
   /* Get the number of outputs and the output data IDs (i.e.,
      the product IDs). */
   num_outputs = tat->num_output_dataids;
   output_ids = (int *) (((char *) tat) + tat->output_data);

   /* Register each output. */
   for( i = 0; i < num_outputs; i++ ){

      LE_send_msg( GL_INFO, "Registering Output (product ID) %d\n",
                   output_ids[i]);

      RPG_out_data( &output_ids[i], &timing, &product_code );

   }

   if( tat != NULL )
      free( tat );

   return;

/* End of OB_reg_outputs() */
}

/***********************************************************************

   Description: 
      This function returns Out_list, the output data type table, and 
      N_outs, the number of outputs.

   Inputs:
      olist - pointer to pointer to Out_data_type type to receive 
              address of Out_list

   Outputs:	
      olist - pointer to pointer to Out_data_type type which holds 
              address of Out_list

   Returns:
      N_outs - The number of registered outputs.

   Notes:
 
***********************************************************************/
int OB_out_list (Out_data_type **olist){

    *olist = Out_list;
    return (N_outs);

/* End of OB_out_list */
}

/***********************************************************************
  
   Description:
      Returns the value of Use_suppl_scans. 

***********************************************************************/
int OB_use_supplemental_scans(){

   return( Use_suppl_scans );

/* End of OB_use_supplemental_scans(). */
}

/***********************************************************************

   Description:
      This function returns Buffers, the allocated buffers list, and
      N_bufs, the number of allocated buffers.

   Inputs:
      buffers - pointer to pointer to Buf_regist type to receive 
                address of Buffers
      n_bufs - pointer to pointer to int type to receive 
                address of number of "Buffers".


   Outputs:	
      olist - pointer to pointer to Buf_regist type which holds 
              address of Buffers

   Returns:
      N_bufs - The number of registered buffers.

   Notes:
 
***********************************************************************/
int OB_buffers_list( Buf_regist **buffers, int **n_bufs ){

   *buffers = Buffers;
   *n_bufs = &N_bufs;
   return(N_bufs);

/* End of OB_buffers_list */
}

/***********************************************************************

   Description: 
      This function returns pointer to Hd_info.

   Inputs:

   Outputs:

   Returns:	 
       Pointer to Hd_info.

   Notes:
 
***********************************************************************/
Prod_header *OB_hd_info(){

    return (&Hd_info);

/* End of OB_hd_info */
}

/***********************************************************************

   Description: 
      This function returns TRUE if Hd_info_set is set, false otherwise.

   Inputs:

   Outputs:

   Returns:
      See description.

   Notes:

***********************************************************************/
int OB_hd_info_set(){

    if( Hd_info_set )
       return( TRUE );

    return ( FALSE );

/* End of OB_hd_info_set */
}

/***********************************************************************

   Description: 
      This function initializes this module.  Initialization includes:

         - Validate all output data timing values.
         - Register for system configuration change events.
         - Open all output LBs and the product generation message LB.
         - Set the ORPG product header Hd_info to zeros (0).

   Inputs:

   Outputs:

   Returns:

   Notes:

***********************************************************************/
void OB_initialize ( Orpgtat_entry_t *task_table ){

    int j, i, num_outputs = 0, *output_ids = NULL;
    int warehoused_product = 0, open_write_permission = 0;
    char *output_names = NULL;

    /* Check that the task_table is valid. */
    if( task_table == NULL )
       PS_task_abort( "OB_initialize Failed .... task_table is NULL" );

    /* For all registered output datatype. */
    for (i = 0; i < N_outs; i++) {

        /* Validate the output timing. */
	if (Out_list [i].timing != TIME_DATA &&
	    Out_list [i].timing != ELEVATION_DATA &&
	    Out_list [i].timing != VOLUME_DATA &&
	    Out_list [i].timing != DEMAND_DATA &&
	    Out_list [i].timing != EXTERNAL_DATA &&
	    Out_list [i].timing != REQUEST_DATA &&
	    Out_list [i].timing != RADIAL_DATA)
	    PS_task_abort (
		"Unknown Output Data Timing (Type: %d, Timing: %d)\n",
		Out_list [i].type, Out_list [i].timing);

        /* If output timing is RADIAL_DATA, force use of Supplemental Scans. */
        if( Out_list [i].timing == RADIAL_DATA )
           Use_suppl_scans = 1;

        if( (Out_list [i].int_or_final != INT_PROD)
                         ||
            (((warehoused_product = ORPGPAT_get_warehoused( Out_list [i].type )) > 0)
                         &&
              (Out_list[i].timing != RADIAL_DATA) ))
           open_write_permission = 1;

#ifdef RPGC_LIBRARY
        /* Register for scan summary updates automatically if product is a final
           product. */
        if( Out_list [i].int_or_final != INT_PROD )
           SS_register();
#endif

        /* Do for all outputs in the task table entry ..... */
        num_outputs = task_table->num_output_dataids;
        output_ids = (int *) (((char *) task_table) + task_table->output_data);
        if( task_table->output_names > 0 )
           output_names = (char *) (((char *) task_table) + task_table->output_names);

        if( output_names != NULL ){

           for( j = 0; j < num_outputs; j++ ){

              if( output_ids[j] == Out_list[i].type ){

                 Out_list[i].data_name = calloc( 1, strlen(output_names)+1 );
                 if( Out_list[i].data_name == NULL )
                    PS_task_abort( "calloc Failed for %d Bytes\n", strlen(output_names)+1 );
                 if( output_names != NULL )
                    strcpy( Out_list[i].data_name, output_names ); 

                 else
                    strcpy( Out_list[i].data_name, ""); 

                 break;

              }

              output_names += (strlen(output_names) + 1);

           } 
 
        }

    }

    if( open_write_permission )
       ORPGDA_open( ORPGDAT_PRODUCTS, LB_WRITE );

    /* Determine this tasks input data stream. */
    Input_stream = INIT_get_task_input_stream();

    /* Get address of Inp_list. */
    N_inps = IB_inp_list( &Inp_list );

    /* Initialize the ORPG product header info */
    memset ((char *)&Hd_info, 0, sizeof (Prod_header));

    for( j = 0; j < N_outs; j++ ){

      LE_send_msg( GL_INFO, "Attributes for Registered Output %d\n", j );
      LE_send_msg( GL_INFO, "--->Type: %8d, Timing: %4d\n",
                   Out_list[j].type, Out_list[j].timing );
      if( Out_list[j].data_name != NULL )
         LE_send_msg( GL_INFO, "--->Name: %s\n", Out_list[j].data_name );

   }


    return;

/* End of OB_initialize */
}

/***********************************************************************

   Description:
      Returns the number of elevations this product has been generated.

   Inputs:
      olind - index into Out_list for this product.

   Outputs:

   Returns:
      Returns the number of elevations this volume scan the product
      has been generated.  Returns -1 on failure.

   Notes:
      It is assumed the number of elevation cuts in a VCP is less than
      or equal to the number of bits-1 in an unsigned long.

***********************************************************************/
int OB_get_elev_cnt( int olind ){

   unsigned long bit_field = Out_list [olind].elev_cnt;
   int i, elev_cnt = 0;

   /* For ELEVATION_DATA, count the number of bits set in elevation count
      field. */
   if( Out_list [olind].timing == ELEVATION_DATA ){

      /* Count the number of elevation bits set. */
      for( i = 1; i <= NUM_BITS; i++ ){

         if( bit_field & (1 << i) )
            elev_cnt++;

      }

      return(elev_cnt);

   }

   /* For RADIAL_DATA, VOLUME_DATA or DEMAND data, assume processing
      of all elevation cuts. */
   if( Out_list [olind].timing == RADIAL_DATA ||
       Out_list [olind].timing == VOLUME_DATA ||
       Out_list [olind].timing == DEMAND_DATA ){

      Scan_Summary *summary;
      int vol_num, num_elevs;

      /* Check if a product of this type was generated. If not, return
         0 elevations. For radial outputs, the "elev_cnt" field
         is not set.  Assume for now, radials are generated every elevation. */
      if( !bit_field && Out_list [olind].timing != RADIAL_DATA )
         return (0);

      /* Get the current volume scan number. */
      PS_get_current_vol_num( &vol_num );
      if( vol_num > MAX_SCAN_SUM_VOLS )
         vol_num = MAX_SCAN_SUM_VOLS;
      else if( vol_num < 0 )
         vol_num = 0;

      /* Read the number of elevations in the volume scan. */
      if( (summary = ORPGSUM_get_scan_summary( vol_num )) == NULL )
         return( -1 );

      /* The scan summary data holds the number of elevation cuts in
         the current volume scan. */
      num_elevs = summary->rpg_elev_cuts;

      return( num_elevs );

   }

   /* If data not ELEVATION_DATA, VOLUME_DATA, RADIAL_DATA, or DEMAND_DATA,
      then return 0 elevations processed. */
   return( 0 );
   
/* End of OB_get_elev_cnt() */
}

/***************************************************************************

   Description:
      This module reports the CPU utilization for the current volume scan.

   Inputs:
      task_name - task name
      vol_scan_num - volume scan sequence number which to report the
                     CPU.
      vol_aborted - flag, if set, indicates the volume scan associated
                    with vol_scan_num did not complete.
      expected_vol_dur - expected volume scan duration, in seconds.

   Outputs:

   Returns:

   Notes:
      CPU is not reported for aborted volume scans.

**************************************************************************/
void OB_report_cpu_stats( char *task_name, unsigned int vol_scan_num, 
                          int vol_aborted, unsigned int expected_vol_dur ){


   int olind, cnt, max = 0;
   struct tms proc_util;
   static struct tms old_proc_util;
   static int first_time = 1;
   clock_t elapsed_time;
   alg_cpu_stats_t  cpu_stats;

   if( first_time ){

      first_time = 0;
      memset( (void *) &old_proc_util, 0, sizeof( struct tms ) );

   }

   /* If the volume scan was aborted, do not update statistics. */
   if( vol_aborted ){

      LE_send_msg( GL_INFO, 
                   "Volume Scan Reported Aborted.  No CPU Stats Available.\n" );
      elapsed_time = times( &proc_util );
      old_proc_util = proc_util;
      return;

   }

   /* For all registered output datatype. */
   for (olind = 0; olind < N_outs; olind++) {

      /* Find maximum of all elevation counts. */
      if( (cnt = OB_get_elev_cnt( olind )) > max )
         max = cnt;
       
   }
   
   /* Get cpu time for this process. */
   if( max > 0 ){

      elapsed_time = times( &proc_util );
      if( elapsed_time == -1 )
         LE_send_msg( GL_INFO, "CPU Time Stats Currently Unavailable For Task %s\n",
                      task_name );
      else{

         unsigned long delta_time;

         delta_time = (proc_util.tms_utime - old_proc_util.tms_utime) +
                      (proc_util.tms_stime - old_proc_util.tms_stime);

         /* Save the cpu statistics. */
         cpu_stats.vol_scan_num = vol_scan_num;
         cpu_stats.elev_cnt = max;
#ifdef LINUX
         cpu_stats.total_cpu = delta_time / (float) sysconf(_SC_CLK_TCK);
#else
         cpu_stats.total_cpu = delta_time / (float) CLK_TCK;
#endif
         cpu_stats.avg_cpu = cpu_stats.total_cpu / (float) max;

         LE_send_msg( GL_INFO, "Task CPU Time (Secs/Volume) %6.2f\n", cpu_stats.total_cpu );
         LE_send_msg( GL_INFO, "Task CPU Time (Secs/Elevation (%d)) %6.2f\n", 
                      max, cpu_stats.avg_cpu );

         if( expected_vol_dur != 0 ){

            cpu_stats.avg_cpu_percent = 
                (cpu_stats.total_cpu / (float) expected_vol_dur)*100.0;
            LE_send_msg( GL_INFO, "Task CPU Time (Percent/Volume) %6.2f\n", 
                         cpu_stats.avg_cpu_percent );

         }  

         /* Save statistics for next pass. */
         old_proc_util = proc_util;

#ifdef ENABLE_ALG_CPU_STATS  
{
         int ret;

         /* Write the cpu statistics. */
         ret = ORPGDA_write( ORPGDAT_ALG_CPU_STATS, (void *) &cpu_stats, 
                             sizeof( alg_cpu_stats_t ), getpid() );
         if( ret < 0 )
            LE_send_msg( GL_INFO, "Unable To Publish CPU Stats For Task ID %d\n",
                         getpid() );
}
#endif
      }

   }

/* End of OB_report_cpu_stats() */
}

/***********************************************************************

   Description: 
      This function sets the product header info based on the base data 
      header and saves the product header info for product output. This 
      function is called when a basedata driving input is read and a new 
      elevation/volume starts.

   Inputs:	
      bd_hd - the base data header
      new_vol - new volume if flag set

   Outputs:

   Returns:

   Notes:

***********************************************************************/
void OB_set_prod_hdr_info (Base_data_header *bd_hd, char *phd, int new_vol){

    /* If the basedata header is set, fill product header fields from
       basedata header information. */
    if( bd_hd != NULL ){

       Hd_info.wx_mode = bd_hd->weather_mode;
       Hd_info.vcp_num = bd_hd->vcp_num;

       /* Set volume time and volume sequence number. */
       Hd_info.g.vol_t = PS_get_volume_time (bd_hd);
       Hd_info.g.vol_num = ORPGMISC_vol_seq_num( (int) bd_hd->vol_num_quotient,
                                              (int) bd_hd->volume_scan_num );

       /* Set elevation time one beginning of elevation or volume. */
       if (bd_hd->status == GOODBEL || bd_hd->status == GOODBVOL)
          Hd_info.elev_t = (bd_hd->date - 1) * 86400 + bd_hd->time / 1000;

       Hd_info.g.elev_ind = bd_hd->rpg_elev_ind;
       Hd_info.spot_blank_bitmap = bd_hd->spot_blank_flag;

    /* Write out debug information if DEBUG flag set. */
#ifdef DEBUG 

       LE_send_msg( GL_INFO, "--->Base Data Header Populates Product Header\n" );
       LE_send_msg( GL_INFO, "------>Hd_info.wx_mode:    %d\n", Hd_info.wx_mode );
       LE_send_msg( GL_INFO, "------>Hd_info.vcp_num:   %d\n", Hd_info.vcp_num );
       LE_send_msg( GL_INFO, "------>Hd_info.g.vol_t:   %d\n", Hd_info.g.vol_t );
       LE_send_msg( GL_INFO, "------>Hd_info.g.vol_num: %d\n", Hd_info.g.vol_num );
       LE_send_msg( GL_INFO, "------>Hd_info.elev_t:  %d\n", Hd_info.elev_t );
       LE_send_msg( GL_INFO, "------>Hd_info.g.elev_ind:  %d\n", Hd_info.g.elev_ind );

       LE_send_msg( GL_INFO, "--->bd_hd->status: %d, bd_hd->elev_num: %d\n",
                    bd_hd->status, bd_hd->elev_num );

#endif

    }
    else if( phd != NULL ){

       memcpy ((char *)&Hd_info, (char *)phd, sizeof (Prod_header));

#ifdef DEBUG
    {
       Prod_header *hd = (Prod_header *) phd;

       LE_send_msg( GL_INFO, "--->Product Header\n" );
       LE_send_msg( GL_INFO, "------>Product ID:        %d\n", hd->g.prod_id );
       LE_send_msg( GL_INFO, "------>Input Stream:      %d\n", hd->g.input_stream );
       LE_send_msg( GL_INFO, "------>LB ID:             %d\n", hd->g.id );
       LE_send_msg( GL_INFO, "------>Generation Time:   %d\n", hd->g.gen_t );
       LE_send_msg( GL_INFO, "------>Volume Time:       %d\n", hd->g.vol_t );
       LE_send_msg( GL_INFO, "------>Product Length:    %d\n", hd->g.len );
       LE_send_msg( GL_INFO, "------>Vol Seq Number:    %d\n", hd->g.vol_num   );
       LE_send_msg( GL_INFO, "------>Elev Time:         %d\n", hd->elev_t );
       LE_send_msg( GL_INFO, "------>Elev Count:        %d\n", hd->elev_cnt );
       LE_send_msg( GL_INFO, "------>Elev Index:        %d\n", hd->g.elev_ind );
       LE_send_msg( GL_INFO, "------>Base Data Status:  %d\n", hd->bd_status );
       LE_send_msg( GL_INFO, "------>Wx Mode:           %d\n", hd->wx_mode );
       LE_send_msg( GL_INFO, "------>VCP Number:        %d\n", hd->vcp_num );

       LE_send_msg( GL_INFO, "--->new_vol flag has value %d\n", new_vol );

    } 
#endif

       /* Save the product header for the product support function. */
       /* NOTE: We may eventually have PS_initialize get the address of
                HD_info. */
       
       PS_register_prod_hdr( (Prod_header *) phd );

    }

    /* Reset product generation and elevation count on beginning of volume. */
    if (new_vol) {

       int i;

       for (i = 0; i < N_outs; i++){

          Out_list [i].gen_cnt = 0;
          Out_list [i].elev_cnt = 0;

       }

    }    

    /* Write out debug information if DEBUG flag set. */
#ifdef DEBUG
{
       int i;


       for (i = 0; i < N_outs; i++)
          LE_send_msg( GL_INFO, "--->Out_list[%d].gen_cnt: %d, Out_list[%d].elev_cnt: %d\n",
                       i, Out_list[i].gen_cnt, i, Out_list[i].elev_cnt );

}
#endif

    /* Set flag to indicate the header information was set. */
    Hd_info_set = 1;
    return;

/* End of OB_set_prod_hdr_info() */
}

/***********************************************************************

   Description: 
      This function returns the vol_number from the input product header.

   Inputs:

   Outputs:

   Returns:
      See description above.

   Notes:

***********************************************************************/
int OB_vol_number (){

   if (Hd_info_set)
      return (Hd_info.g.vol_num);

   return (-1);

/* End of OB_vol_number */
}

/***********************************************************************

   Description: 
      This function sets up the product header before product output. 
      Much of the info is stored in Hd_info. This function finds out the 
      total message length and generation time. This function also adds 
      addtional fields for the RPG product header by calling 
      More_phd_fields.

   Inputs:
      buffer - pointer to the buffer registration struct
      olind - index in Out_list of this product.

   Outputs:
      buffer - pointer to the buffer registration struct

   Returns:

   Notes:
      Upon failure, PS_task_abort is called to terminate the task.

***********************************************************************/
static void Set_outbuf_prod_hdr (Buf_regist *buffer, int olind){

   Prod_header *phd;
   int rad_status, temp_len, temp_orig_size;

   if (Out_list [olind].timing == RADIAL_DATA)
      return;				/* header unused for radial data */

   phd = (Prod_header *)buffer->bpt;	/* The product header */

   /* The size fields may have been changed on release of the output 
      buffer.  Save them in temp storage so they can be restored
      after the copy. */
   temp_len = phd->g.len;
   temp_orig_size = phd->orig_size;

   /* Copy Hd_info over the product header. */
   memcpy ((char *)phd, (char *)&Hd_info, sizeof (Prod_header));

   /* Restore the size fields. */
   phd->g.len = temp_len;
   phd->orig_size = temp_orig_size;
   phd->compr_method = COMPRESSION_NONE;

   /* Set the product ID. */
   phd->g.prod_id = buffer->type;

   /* For the CFC product, we need to set the weather mode explicitly.  Currently,
      this is the only product not based in base data. */  
   if( phd->g.prod_id == CFCPROD ){

      Graphic_product *hd = (Graphic_product *)(buffer->bpt + sizeof (Prod_header)); 
      phd->wx_mode = hd->op_mode;

   }

   /* Set product generation time. */
   phd->g.gen_t = time(NULL);

   /* Set other fields. */
   if ( Out_list [olind].int_or_final != INT_PROD ){

      /* RPG final product */
      Graphic_product *hd;

      hd = (Graphic_product *)(buffer->bpt + sizeof (Prod_header)); 
	
      /* fill out additional fields */
      More_phd_fields (hd);

      phd->g.len = UMC_product_length ((void *)hd) + sizeof (Prod_header);
      phd->orig_size = phd->g.len;

      /* Verify product code matches int_or_final value.  Abort
         if mismatch. */
      if( hd->msg_code != Out_list[olind].int_or_final )
         PS_task_abort("Product Code Does Not Match Registered Value (%d != %d)\n",
                        hd->msg_code, Out_list[olind].int_or_final );


   }

   /* Increment product generation count and set elevation count in product
      header if data timing is ELEVATION_DATA.  If data timing is VOLUME_DATA,
      increment the generation count in Out_list. */
   if( Out_list [olind].timing == ELEVATION_DATA ){

      if( ORPGPAT_get_class_id( Out_list [olind].type ) != BASEDATA_ELEV )
         Out_list [olind].gen_cnt++;

      else{

         Compact_basedata_elev *elev_prod = 
                               (Compact_basedata_elev *) (buffer->bpt + sizeof(Prod_header));
         if( (elev_prod->type & REFLDATA_TYPE) || (elev_prod->type & BASEDATA_TYPE) )
            Out_list [olind].gen_cnt++;

      }

   }

   phd->elev_cnt = Out_list [olind].gen_cnt;

   if (Out_list [olind].timing == VOLUME_DATA)
      Out_list [olind].gen_cnt++;

   /* Set elevation bit in Out_list elevation count field. */
   if (Out_list [olind].timing == ELEVATION_DATA)
      Out_list [olind].elev_cnt |= (1 << (phd->g.elev_ind-1));
   else
      Out_list [olind].elev_cnt = (unsigned long) -1;

   if (Hd_info_set) {

      rad_status = WA_radial_status ();
      if (rad_status >= 0){

         /* Write out debug information if DEBUG flag is set. */
#ifdef DEBUG
         LE_send_msg( GL_INFO, "--->Radial status:  Before - %d, After - %d\n",
                      phd->bd_status, rad_status );
#endif
         phd->bd_status = rad_status;

      }

   }
   else{

      int vol_num;
      unsigned int vol_seq_num;

      /* Header is not set so there there is probably no input. */
      vol_seq_num = PS_get_current_vol_num( &vol_num );
      phd->g.vol_num = vol_seq_num;
      phd->g.vol_t = SS_get_volume_time( vol_num );

      /* If the volume time is 0, set the volume time to current time. */
      if( phd->g.vol_t == 0 ){

         phd->g.vol_t = time(NULL);
         LE_send_msg( GL_INFO, "Volume Time == 0 For Volume # %d\n", vol_num );
         LE_send_msg( GL_INFO, "--->Setting Volume Time to Current Time: %d\n", 
                      phd->g.vol_t );

      }

   }

   /* Tell whether this is a real-time product or replay product. */
   phd->g.input_stream = Input_stream;

   /* Write out debug information if DEBUG flag set. */
#ifdef DEBUG

   LE_send_msg( GL_INFO, "--->Product Header\n" );
   LE_send_msg( GL_INFO, "------>Product ID:        %d\n", phd->g.prod_id );
   LE_send_msg( GL_INFO, "------>Input Stream:      %d\n", phd->g.input_stream );
   LE_send_msg( GL_INFO, "------>LB ID:             %d\n", phd->g.id );
   LE_send_msg( GL_INFO, "------>Generation Time:   %d\n", phd->g.gen_t );
   LE_send_msg( GL_INFO, "------>Volume Time:       %d\n", phd->g.vol_t );
   LE_send_msg( GL_INFO, "------>Product Length:    %d\n", phd->g.len );
   LE_send_msg( GL_INFO, "------>Vol Seq Number:    %d\n", phd->g.vol_num   );
   LE_send_msg( GL_INFO, "------>Elev Time:         %d\n", phd->elev_t );
   LE_send_msg( GL_INFO, "------>Elev Count:        %d\n", phd->elev_cnt );
   LE_send_msg( GL_INFO, "------>Elev Index:        %d\n", phd->g.elev_ind );
   LE_send_msg( GL_INFO, "------>Base Data Status:  %d\n", phd->bd_status );
   LE_send_msg( GL_INFO, "------>Wx Mode:           %d\n", phd->wx_mode );
   LE_send_msg( GL_INFO, "------>VCP Number:        %d\n", phd->vcp_num );
   LE_send_msg( GL_INFO, "------>Compr Method:      %d\n", phd->compr_method );
   LE_send_msg( GL_INFO, "------>Orig Size:         %d\n", phd->orig_size );

#endif

   return;

/* End of Set_outbuf_prod_hdr() */
}

/*********************************************************************
	
   Description: 
      This function sets up the fields msg_date and msg_time in the 
      product header. These fields were filled by the control module 
      in the old RPG.  The generation time and date are used for the 
      message time and date. 

   Input:
      phd - the RPG product header;

   Output:

   Returns:

   Notes: 

*********************************************************************/
static void More_phd_fields( Graphic_product *phd ){

    phd->msg_time = phd->gen_time;
    phd->msg_date = phd->gen_date;

    return;

/* End of More_phd_fields() */
}

/********************************************************************

   Description: 
      This function controls writing the product at index "olind" in 
      the output list to the LB. The product is stored in "buf".

   Inputs:	
      ind - index in Buffers of buffer containing the product
      olind - index in Out_list of this product.

   Outputs:

   Returns:     
      Return value is undefined.

   Notes:
      This functions terminates the task if the product generation
      message LB is not open or if the input stream is undefined.

********************************************************************/
static int Output_product ( int ind, int olind ){

   Prod_header *hd = (Prod_header *)Buffers[ind].bpt;

   /* If this is an alert message product, process separately. */
   if( hd->g.prod_id == ALRTMSG )
      return (Output_alert_message_product( Buffers[ind].bpt, ind, olind ));

   /* Fill in product dependent parameters in product header. */
   if (Out_list [olind].timing != RADIAL_DATA)
      Set_prod_depend_params( ind, olind );

   /* Write the product to appropriate LB(s) and generate product 
      generation message. */
   if( Out_list[olind].timing == RADIAL_DATA || 
       hd->g.input_stream == PGM_REALTIME_STREAM )
      Write_product( Buffers[ind].bpt, ind, olind, PGM_REALTIME_STREAM );

   else if( hd->g.input_stream == PGM_REPLAY_STREAM )
      Write_product( Buffers[ind].bpt, ind, olind, PGM_REPLAY_STREAM );

   else
      PS_task_abort( "Unknown Input Data Stream Type (%d)\n",
                     hd->g.input_stream );

   return (0);

/* End of Output_product() */
}

/***********************************************************************

   Description:
      Given data name, returns the Product ID associated with the name.
      The association is defined in the Task Table entry for the
      calling task.

   Inputs:
      data_name = data name string.

   Outputs:
      prod_id - the product ID.

***********************************************************************/
void OB_get_id_from_name( char *data_name, int *prod_id ){

    int ind, i;
    char str[ORPG_TASKNAME_SIZ];

    /* Initialize the product ID to invalid value. */
    *prod_id = -1;

    /* Check for valid string pointer. */
    if( data_name == NULL )
       return;

   /* Check for any $ in the name ... this is a reserved character. */
    i = 0;
    while( i < ORPG_TASKNAME_SIZ ){

       /* We need to make a copy of the string while we validate.
          If this string is a parameter (constant), modifying it
          will cause segmentation violation. */
       str[i] = data_name[i];

       /* If any $ found, then set to NULL terminator. */
       if( str[i] == '$' )
         str[i] = '\0';

       /* We are at the end of the string. */
       if( str[i] == '\0' )
          break;

       i++;

    }
    
    /* Check to insure string length is valid */
    if( i >= ORPG_TASKNAME_SIZ )
       return;

    /* Find a match in the list of registered outputs. */
    for (ind = 0; ind < N_outs; ind++) {

       if( (Out_list[ind].data_name != NULL)
                    &&
           (strcmp( (char *) &str[0], Out_list[ind].data_name ) == 0) ){

          *prod_id = Out_list[ind].type;
          return;

       }

    }

    /* If the data_id is still undefined, check the task_attr_table entry.
       If Task_name hasn't been defined, then the argument list hasn't been
       processed yet. */
    if( Task_name != NULL ){

       int num_outputs, *output_ids = NULL;
       Orpgtat_entry_t *task_entry = NULL;
       char *output_names = NULL;

       task_entry = ORPGTAT_get_entry( (char *) &(Task_name[0]) );
       if( task_entry == NULL )
          PS_task_abort( "Task_name %s Not in TAT\n", (char *) &Task_name[0] );

       num_outputs = task_entry->num_output_dataids;
       output_ids = (int *) (((char *) task_entry) + task_entry->output_data);
       if( task_entry->output_names > 0 )
          output_names = (char *) (((char *) task_entry) + task_entry->output_names);

       if( output_names != NULL ){

          /* Do For All TAT input entries. */
          for( i = 0; i < num_outputs; i++ ){

             /* Check for match on input data name. */
             if( strcmp( output_names, (char *) &str[0] ) == 0){

                *prod_id = output_ids[i];
                break;

             }

             if( output_names != NULL )
                output_names += (strlen(output_names) + 1);

          }

       }

       if( task_entry != NULL )
          free( task_entry );

    }

    return;

/* End of OB_get_id_from_name() */
}

/********************************************************************

   Description: 
      This function is returns the buffer number associated with a 
      given product code.

   Inputs: 
      prod_code - product code

   Outputs:

   Returns: 	 
      Buffer number if match found, otherwise -1.

   Notes:

********************************************************************/
int OB_get_buffer_number( int prod_code ){

   int i;

   /* Find match of product code with buffers registered for output. */
   for( i = 0; i < N_outs; i++ ){

      /* If match found, return buffer number. */
      if( Out_list [i].int_or_final == prod_code )
         return( Out_list[i].type );

   }
   
   /* Product code not found.  Return error. */
   return(-1);
   
/* OB_get_buffer_number() */
}

/********************************************************************

   Description: 
      This function writes out the alert message product of index 
      "olind" in the output list to the LB. The product is stored in 
      "buf".

   Inputs:	 
      buf - buffer containing the alert message product
      olind - index in Buffers of this product.
      olind - index in Out_list of this product.

   Outputs:

   Returns:      
      Return value is undefined.

   Notes: 
      This function terminates the task on fatal error.

********************************************************************/
static int Output_alert_message_product ( char *buf, int ind, int olind ){

    int len, ret, compress, status;
    int type = Buffers[ind].type;
    char *bpt;
    Prod_header *hd;

    hd = (Prod_header *) buf;
    len = hd->g.len;
    bpt = buf;

    /* convert product to icd format */
    UMC_product_to_icd (buf + sizeof (Prod_header), 
			len - sizeof (Prod_header));

    /* If the product is to be compressed, compress it here.   Currently
       we do not support compression of BASEDATA.  NOTE:  This must be
       done after the UMC call. */
    compress = ORPGPAT_get_compression_type( type );
    if( (compress > 0) && (type != BASEDATA) ){
                                                                                                           
       LE_send_msg( GL_INFO, "Product ID %d To Be Compressed\n", type );
                                                                                                           
       RPG_compress_product( (void *) Buffers[ind].bpt, &compress, &status );
       if( status < 0 )
          LE_send_msg( GL_ERROR, "Product Not Compressed Due To Error\n" );
                                                                                                           
    }

    /* write the product */
    ret = ORPGDA_write (Out_list[olind].type, bpt, len, LB_ANY);
    if (ret != len)
	PS_task_abort ("ORPGDA_write Product %s Failed (%d)\n", 
		       Out_list[olind].name, ret);

    /* advertise the product generation */
    LE_send_msg( GL_INFO, "prod %d (vol# %d, len %d) -> %s (buf size = %d)\n", 
		Out_list[olind].type, hd->g.vol_num, len, Out_list[olind].name, 
		Out_list[olind].len);

    /* If an event is associated with this output data, post it now. */
    if( Out_list[olind].event_id != ORPGEVT_NULL_EVENT ){
    	ret = EN_post( Out_list[olind].event_id, (void *) NULL, (size_t) 0,
                      (int) 0 );
    	if( ret != 0 )
       	   LE_send_msg( GL_EN(ret), "Output_product: EN_post failed. Ret = %d\n", ret );
        else
           LE_send_msg( GL_INFO, "Output_product: Event ID %d posted.\n",
                        Out_list[olind].event_id );
    }

    return (0);

/* End of Output-alert_message_product */
}

/**********************************************************************

   Description:
       This modules performs product specific processing required
       to support legacy product generation and distribution.

   Inputs:
       phd - pointer to product header.
       descrip_block - pointer to product description block.

   Outputs:

   Returns:
       There are no return values defined for this function.

   Note:
       This function may invoke task termination processing.

**********************************************************************/
static void Product_specific_processing( Prod_header *phd, 
                                         Graphic_product *descrip_block ){

   int prod_id;
   
   prod_id = phd->g.prod_id;

   switch( prod_id ){

      case SRMRVMAP:
      case SRMRVREG:
      {

         /* Special processing required for SRM (prod id SRMRVMAP) and SRR 
            (prod id SRMRVREG).  Check if the motion source flag in the 
            product dependent parameters is -1.  If yes, then set the storm 
            speed and storm direction fields in the prod gen msg request
            parameters to -1. */
         if( descrip_block->param_6 == - 1){

            phd->g.req_params[STORM_SPEED_INDEX] = PARAM_ALG_SET;
            phd->g.req_params[STORM_DIRECTION_INDEX] = PARAM_ALG_SET;

         }

         break;

      }
      default:
         break;

   }

   return;

/* End of Product_specific_processing() */
}

/**********************************************************************

   Description:
       This modules performs product specific processing required
       to support legacy product generation and distribution.

   Inputs:
       phd - pointer to product header.

   Outputs:

   Returns:
       There are no return values defined for this function.

   Note:
       This function may invoke task termination processing.

**********************************************************************/
static void User_selectable_precip_processing( Prod_header *phd ){

   int prod_id, end_time, last_update_time = HRDB_INIT_VALUE;
   int vol_scan_num = 0;
   Summary_Data *summary = NULL;
   Scan_Summary *current_summary = NULL;
   
   prod_id = phd->g.prod_id;

   /* Special processing required for USP (prod id HYUSPACC).  If product
      requested with end time -1 (lasted hourly update), then if the current
      USP product end time matches the last time the hourly database updated,
      send a product generation message. */
   if( prod_id == HYUSPACC ){

      Hrdb_date_time *hrdb_data = NULL;

      /* Get the "end time" from the product dependent parameters.  Get the 
         last hourly database update time from ITC or Scan Summary. */
      end_time = phd->g.resp_params[ USP_END_TIME ];
      hrdb_data = (Hrdb_date_time *) ITC_get_data( ITC_CD07_USP );
      if( hrdb_data == NULL ){

         LE_send_msg( GL_INFO, "ITC ITC_CD07_USP Not Found.\n" );
         return;

      }

      last_update_time = hrdb_data->last_time_hrdb;

      /* Compare last update time against initialization value.  The
         initialization value indicates the hourly database has not
         be updated yet.  If equal, get volume scan time from Scan
         Summary data. */
      if( last_update_time == HRDB_INIT_VALUE ){

         if( (summary = (Summary_Data *) SS_get_summary_data()) == NULL ){

            summary = (Summary_Data *) malloc( sizeof( Summary_Data ) );
            if( summary == NULL ){

               LE_send_msg( GL_MEMORY, "malloc Failed For %d Bytes\n",
                            sizeof( Summary_Data ) );
               return;

            }

            SS_send_summary_array( (int *) summary );
            RPG_read_scan_summary();

         }
         else{

            RPG_read_scan_summary();
            summary = (Summary_Data *) SS_get_summary_data();

         }

         vol_scan_num = ORPGMISC_vol_scan_num( phd->g.vol_num );
         current_summary = (Scan_Summary *) &summary->scan_summary[ vol_scan_num ];
         last_update_time = current_summary->volume_start_time/3600;

      }
          
      /* If match on end_time and last_update_time, then .... */
      if( end_time == last_update_time ){

         int index = 1, prod_code, i;
         short uarray[10][10];

         /* Get all requests for this product. */
         prod_code = ORPGPAT_get_code( prod_id );
         if( prod_code <= 0 )
            PS_task_abort( "No Product Code For Product ID %d\n", prod_id );

         RPG_get_request( phd->g.elev_ind, prod_id, prod_code, &index,
                          uarray[0] );

         /* Do for all product requests ... */
         for( i = 0; i < index-1; i++ ){

            /* If "end time" equals flag value (-1) and match on "duration"
               product parameter, then ... */
            if( uarray[i][PREQ_END_TIME] == -1 
                                 &&
                phd->g.req_params[ USP_DURATION ] == uarray[i][PREQ_DURATION] ){

               Prod_header temp;

               /* Copy product header into temp. */
               memcpy( (void *) &temp, (void *) phd, sizeof( Prod_header ) );

               /* Set "end_time" parameter to flag value (-1). */
               temp.g.req_params[ USP_END_TIME ] = -1;

               /* Write product generation message. */
	       Write_prod_gen_msg( &temp.g );

               /* Break out of for-loop. */
               break;

            }
 
         } 

      }

   }

   /* Special processing required for USW/USD (prod id USWACCUM/USDACCUM).  If product
      requested with end time -1, then if the current USW/USD product end time matches
      the current hour, send a product generation message with the End Time set to the
      current hour.  */
   else if( (prod_id == USWACCUM) || (prod_id == USDACCUM) ){

      /* Get the "end time" from the product dependent parameters.  Get the 
         last hourly database update time from Scan Summary. */
      end_time = phd->g.resp_params[ USP_END_TIME ];
      if( (summary = (Summary_Data *) SS_get_summary_data()) == NULL ){

         summary = (Summary_Data *) malloc( sizeof( Summary_Data ) );
         if( summary == NULL ){

            LE_send_msg( GL_MEMORY, "malloc Failed For %d Bytes\n",
                         sizeof( Summary_Data ) );
            return;

         }

         SS_send_summary_array( (int *) summary );
         RPG_read_scan_summary();

      }
      else{

         RPG_read_scan_summary();
         summary = (Summary_Data *) SS_get_summary_data();

      }

      vol_scan_num = ORPGMISC_vol_scan_num( phd->g.vol_num );
      current_summary = (Scan_Summary *) &summary->scan_summary[ vol_scan_num ];
      last_update_time = current_summary->volume_start_time/3600;

      /* If match on end_time and last_update_time, then .... */
      if( end_time == last_update_time ){

         int index = 1, prod_code, i;
         short uarray[10][10];

         /* Get all requests for this product. */
         prod_code = ORPGPAT_get_code( prod_id );
         if( prod_code <= 0 )
            PS_task_abort( "No Product Code For Product ID %d\n", prod_id );

         RPG_get_request( phd->g.elev_ind, prod_id, prod_code, &index,
                          uarray[0] );

         /* Do for all product requests ... */
         for( i = 0; i < index-1; i++ ){

            /* If "end time" equals flag value (-1) and match on "duration"
               product parameter, then ... */
            if( uarray[i][PREQ_END_TIME] == -1 
                                 &&
                phd->g.req_params[ USP_DURATION ] == uarray[i][PREQ_DURATION] ){

               Prod_header temp;

               /* Copy product header into temp. */
               memcpy( (void *) &temp, (void *) phd, sizeof( Prod_header ) );

               /* Set "end_time" parameter to flag value (-1). */
               temp.g.req_params[ USP_END_TIME ] = temp.g.resp_params[ USP_END_TIME ];

               /* Write product generation message. */
	       Write_prod_gen_msg( &temp.g );

               /* Break out of for-loop. */
               break;

            }
 
         } 

      }

   }

/* End of User_selectable_precip_processing() */
}

/******************************************************************

   Description:
      Sets the product dependent parameters in the orpg product 
      header.

   Inputs:
      buf - pointer to product buffer.
      olind - Out_list index corresponding to product.

   Outputs:

   Returns:
      Always returns 0.

   Notes:

******************************************************************/
static int Set_prod_depend_params( int ind, int olind ){

   Prod_header *hd = (Prod_header *) Buffers[ind].bpt;
   Prod_request *prod_request;
   int n_reqs, i;

   /* Initialize the request and response product dependent parameters. */
   for( i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++ )
      hd->g.req_params[i] = hd->g.resp_params[i] = PARAM_UNUSED;

   /* For "intermediate" products */
   if( Out_list[olind].int_or_final == INT_PROD ){

      /* This is to support legacy buffer numbers. */
      if( hd->g.prod_id <= LEGACY_MAX_BUFFERNUM ){

         /* Get the product request data. */
         prod_request = PRQ_get_prod_request( Out_list[olind].type, &n_reqs );

         /* If there is product request data, transfer to orpg product
            header. */
         if( prod_request != NULL ){

            hd->g.req_params[0] = hd->g.resp_params[0] = prod_request->param_1;
            hd->g.req_params[1] = hd->g.resp_params[1] = prod_request->param_2;
            hd->g.req_params[2] = hd->g.resp_params[2] = prod_request->param_3;
            hd->g.req_params[3] = hd->g.resp_params[3] = prod_request->param_4;
            hd->g.req_params[4] = hd->g.resp_params[4] = prod_request->param_5;
            hd->g.req_params[5] = hd->g.resp_params[5] = prod_request->param_6;

         }

      }
      else{

         /* This is to support non-legacy buffer numbers.  If there is product 
            request data, transfer to orpg product header. */
         if( Buffers[ind].req != NULL ){

            User_array_t *user_array = Buffers[ind].req;

            hd->g.req_params[0] = hd->g.resp_params[0] = user_array->ua_dep_parm_0;
            hd->g.req_params[1] = hd->g.resp_params[1] = user_array->ua_dep_parm_1;
            hd->g.req_params[2] = hd->g.resp_params[2] = user_array->ua_dep_parm_2;
            hd->g.req_params[3] = hd->g.resp_params[3] = user_array->ua_dep_parm_3;
            hd->g.req_params[4] = hd->g.resp_params[4] = user_array->ua_dep_parm_4;
            hd->g.req_params[5] = hd->g.resp_params[5] = user_array->ua_dep_parm_5;

         }

      }

   }
   else{

      /* For "final" products. */

      short parameters[NUM_DEPEND_PARAMS], index, i;
      Graphic_product *descrip_block;

      /* Form product description block.  Copy all product dependent 
         parameters.  Unfortunately, there is no elegant way to do this. 
         Refer to RPG/APUP ICD for more detail on product dependent 
         parameters. */
      descrip_block = (Graphic_product *) (Buffers[ind].bpt + sizeof( Prod_header ));
      parameters[0] = (short) descrip_block->param_1;
      parameters[1] = (short) descrip_block->param_2;
      parameters[2] = (short) descrip_block->param_3;
      parameters[3] = (short) descrip_block->param_4;
      parameters[4] = (short) descrip_block->param_5;
      parameters[5] = (short) descrip_block->param_6;
      parameters[6] = (short) descrip_block->param_7;
      parameters[7] = (short) descrip_block->param_8;
      parameters[8] = (short) descrip_block->param_9;
      parameters[9] = (short) descrip_block->param_10;

      /* Convert the NUM_DEPEND_PARAMS parameters to NUM_PROD_DEPENDENT_PARAMS 
         parameters. */

      /* For legacy products, use lookup table. */
      if( hd->g.prod_id <= LEGACY_MAX_BUFFERNUM ){

         for( i = 0; i < NUM_DEPEND_PARAMS; i++ ){

            index = p10_p6[hd->g.prod_id][i];
            if( (index >= 0) && (index < NUM_PROD_DEPENDENT_PARAMS))
               hd->g.req_params[index] = hd->g.resp_params[index] = parameters[i];

         }

      }
      else{

         /* For new products, we assume the first NUM_PROD_DEPENDENT_PARAMS
            in the description block are the same as the request parameters.
            However, if request data has special flag values, then the request
            and response parameters may be different. */
         int num_params = 0;

         num_params = ORPGPAT_get_num_parameters( hd->g.prod_id );
         for( i = 0; i < num_params; i++ ){

            index = ORPGPAT_get_parameter_index( hd->g.prod_id, i );
            if( index >= 0 )
               hd->g.req_params[index] = hd->g.resp_params[index] = parameters[index];

         }

         /* Copy the request parameters if they exist. */
         if( Buffers[ind].req != NULL ){

            User_array_t *user_array = Buffers[ind].req;

            hd->g.req_params[0] = user_array->ua_dep_parm_0;
            hd->g.req_params[1] = user_array->ua_dep_parm_1;
            hd->g.req_params[2] = user_array->ua_dep_parm_2;
            hd->g.req_params[3] = user_array->ua_dep_parm_3;
            hd->g.req_params[4] = user_array->ua_dep_parm_4;
            hd->g.req_params[5] = user_array->ua_dep_parm_5;

         }

      }

   }

   /* Is there some product specific processing for this product. */
   if( Out_list[olind].int_or_final != INT_PROD ){

      Graphic_product *descrip_block;

      descrip_block = (Graphic_product *) (Buffers[ind].bpt + sizeof( Prod_header ));
      Product_specific_processing( hd, descrip_block );

   }

   return 0;

/* End of Set_prod_depend_params() */
}


/**************************************************************************

   Description:
      This module tags the "user_array" data to the output buffer.

   Inputs:
      bufptr - pointer to buffer containing product data.
      user_array - array containing product request information.

   Outputs:

   Returns:
      Always returns 0.

   Notes:
      PS_task_abort is called to terminate task processing if bufptr not
      valid.

**************************************************************************/
int OB_tag_outbuf_wreq( int *bufptr, short *user_array ){

   int ind;
   int olind;

   /* Search for the buffer in the list of allocated buffers. */
   for (ind = 0; ind < N_bufs; ind++) {

      if (Buffers[ind].ptr == *bufptr)
         break;

   }	

   /* If buffer not found in buffer list, abort. */
   if (ind >= N_bufs)	
      PS_task_abort ("Buffer not found - RPG code error\n");

   /* Find the index of the datatype in the list of registered output
      data types (Out_list). */
   for (olind = 0; olind < N_outs; olind++) {

      if (Out_list[olind].type == Buffers[ind].type)
         break;

   }

   if (Out_list [olind].timing == RADIAL_DATA)
      return (0);				/* header unused for radial data */

   /* Tag the request data to the output buffer. */
   Buffers[ind].req = (User_array_t *) calloc( 1, sizeof(User_array_t) );
   if( Buffers[ind].req == NULL )
      PS_task_abort ("calloc Failed in OB_tag_outbuf_wreq() For Size %d\n",
                     sizeof(User_array_t));

   memcpy( Buffers[ind].req, user_array, sizeof(User_array_t) );

   /* Always return normal (0) */
   return( 0 );

/* End of OB_tag_outbuf_wreq() */
}

/**************************************************************************

   Description:
      This module writes the product to appropriate LBs and generates the
      product generation message.  

   Inputs:
      buf - pointer to buffer containing product data.
      olind - index into Buffers for this product.
      olind - index into Out_list for this product.
      stream - input stream of product.

   Outputs:

   Returns:
      Always returns 0.

   Notes:
      PS_task_abort is called to terminate task processing on write 
      failures.

**************************************************************************/
static int Write_product( char *buf, int ind, int olind, int stream ){

   int warehoused_product, len, ret, compress, status;
   int type = Buffers[ind].type;
   char *bpt;
   Prod_header *hd = (Prod_header *) buf;
   LB_id_t prodlb_msg_id = 0, database_msg_id = 0;

   /* If product is of timing RADIAL_DATA, do not include orpg product 
      header. */
   if (Out_list [olind].timing == RADIAL_DATA) {

      len = Out_list [olind].len - sizeof (Prod_header);
      bpt = buf + sizeof (Prod_header);

   }
   
   /* All other products */
   else{	

      len = hd->g.len;
      bpt = buf;
      if (Out_list[olind].int_or_final != INT_PROD) {

         /* 
           Converts to ICD format. 
         */
	 UMC_product_to_icd( buf + sizeof (Prod_header), len-sizeof (Prod_header));

      }

   }
   /* If the product is to be compressed, compress it here.   Currently
      we do not support compression of BASEDATA.  NOTE:  This must be
      done after UMC call. */
   compress = ORPGPAT_get_compression_type( type );
   if( (compress > 0) && (type != BASEDATA) ){


      LE_send_msg( GL_INFO, "Product ID %d To Be Compressed\n", type );

      RPG_compress_product( (void *) Buffers[ind].bpt, &compress, &status );
      if( status < 0 )
         LE_send_msg( GL_ERROR, "Product Not Compressed Due To Error\n" );

      else{

         if( Out_list[olind].timing != RADIAL_DATA ){

            /* Get the new product length (i.e., the length after the product
               has been compressed.) */
            len = hd->g.len;

         }

      }

   }

   /* Set the message ID in orpg product header to 0.  If product is written to
      product LB, this is indicator that product message contains product data.
      Otherwise, a message ID > 0 indicates product message is link to product
      data base. */
   hd->g.id = 0;

   /* Write product to product database if product is not of INT_PROD type or
      if the product is to be warehoused. */
   warehoused_product = ORPGPAT_get_warehoused( Out_list [olind].type );
   if( (Out_list[olind].int_or_final != INT_PROD)
                        ||
       ((warehoused_product > 0) && (Out_list [olind].timing != RADIAL_DATA)) ){ 

      if( (ret = ORPGDA_write( ORPGDAT_PRODUCTS, bpt, len, LB_ANY )) == len ){

         database_msg_id = ORPGDA_get_msg_id();

         if( warehoused_product > 0 )
            LE_send_msg( GL_INFO, 
                         "Product (%d) Is To Be Warehoused In Product Database (msg_id: %d)\n", 
                         hd->g.prod_id, database_msg_id );

      }
      else
         PS_task_abort( "ORPGDA_write Of Product %d To Product Database Failed (%d)\n",
                         Out_list[olind].type, ret );

   }

   /* If product is of INT_PROD type and not warehoused, write product to 
      product LB. */
   else{

      /* We assume any return value >= 0 is OK since we need to account for the 
         case where the data might be compressed .... the return value (len) 
         will not match the "original" length in this case. */
      if( (ret = ORPGDA_write (Out_list[olind].type, bpt, len, LB_ANY)) >= 0 ){

         prodlb_msg_id = ORPGDA_get_msg_id( );

         /* If timing is RADIAL_DATA and data is used to support radial replay, 
            write the data to replay data store. */
         if( (Out_list[olind].timing == RADIAL_DATA) && (warehoused_product > 0) ){

            Base_data_header *bdh = (Base_data_header *) bpt;
            unsigned int sub_type = bdh->msg_type & BASEDATA_TYPE_MASK; 

            ORPGBDR_write_radial( Out_list[olind].type, sub_type, bpt, len );

         }

      }
      else
         PS_task_abort( "ORPGDA_write Of Product %d To Product LB %s Failed (%d)\n",
                        Out_list[olind].type, Out_list[olind].name, ret );

   }

   /* Output informational message concerning product just written. */
   Write_informational_message( buf, olind, stream, len );

   /* Add product to list of products generated in case of abort condition. */
   if( (Out_list [olind].timing != RADIAL_DATA) 
                        &&
       (Inp_list[DRIVING].timing != DEMAND_DATA) )
      AP_add_output_to_prod_list( (Prod_header *) bpt );

   /* For all products which are not RADIAL_DATA or DEMAND_DATA or 
      DEMAND_DATA not associated with a product code. */
   if( Out_list [olind].timing != RADIAL_DATA &&
       ((Out_list [olind].timing != DEMAND_DATA) ||
        (Out_list [olind].timing == DEMAND_DATA && 
         Out_list [olind].int_or_final != INT_PROD)) ) {

      /* If product is a final product or a warehoused product, write
         "link" to product LB. */
      if( (Out_list[olind].int_or_final != INT_PROD)
                                || 
                        (warehoused_product) ){

         Prod_header *prod_hdr = (Prod_header *) bpt;

         /* This is a link to the product in the product database.  The
            message ID in the orpg product header must be changed to
            match the message ID of the product written to product
            database. */
         prod_hdr->g.id = database_msg_id;
         ret = ORPGDA_write (Out_list[olind].type, bpt, sizeof(Prod_header), LB_ANY);

         if (ret != sizeof(Prod_header))
            PS_task_abort( "ORPGDA_write Product %s Failed (%d)\n", 
	   		   Out_list[olind].name, ret );

         /* Get message ID of product just written.  This is the message ID
            of the product just written to the product LB. */
         prodlb_msg_id = database_msg_id;

      }

      /* Set the msg ID in the product header.  The product generation message
         must hold the ID of the message as written to the product LB if 
         intermediate product, or data base if warehoused or final product. */
      hd->g.id = prodlb_msg_id;

      /* Write the product generation message to Product Generation 
         Message LB. */
      Write_prod_gen_msg( &hd->g ); 

      /* Special processing required for USP (prod id HYUSPACC), USW (prod id USWACCUM) and
         USD (prod id USDACCUM). */
      if( (hd->g.prod_id == HYUSPACC) || (hd->g.prod_id == USWACCUM) || (hd->g.prod_id == USDACCUM) ) 
         User_selectable_precip_processing( hd );

   }

   /* If an event is associated with this output data, post event now. */
   if( Out_list[olind].event_id != ORPGEVT_NULL_EVENT ){

      ret = EN_post( Out_list[olind].event_id, (void *) NULL, (size_t) 0,
                     (int) 0 );
      if( ret != 0 )
         LE_send_msg( GL_EN(ret), "Output_product: EN_post Failed. Ret = %d\n", ret );

      else
         LE_send_msg( GL_INFO, "Output_product: Event ID %d Posted.\n",
                      Out_list[olind].event_id );

   }

   return 0;

/* End of Write_product() */
}

/**************************************************************************

   Description:
      This module writes all informational message abort the product
      just generated.

   Inputs:
      buf - pointer to buffer containing product data.
      olind - index into Out_list for this product.
      stream - input stream of product.
      len - length of product, in bytes.  Includes ORPG product header.

   Outputs:

   Returns:
      Always returns 0.

   Notes:

**************************************************************************/
static int Write_informational_message( char *buf, int olind, int stream,
                                        int len ){

   static int cnt = 0;
   Prod_header *hd = (Prod_header *) buf;

   /*
     Output informational message concerning product just written.
   */
   if( stream == PGM_REALTIME_STREAM ){

      if (Out_list [olind].timing != RADIAL_DATA){

         if( Out_list[olind].timing == ELEVATION_DATA )
            LE_send_msg( GL_INFO, 
		"Prod %d (vol# %d, rpg_elev_ind %d) -> %s (len %d, buf size = %d)\n", 
		Out_list[olind].type, hd->g.vol_num, hd->g.elev_ind,  
                Out_list[olind].name, len, Out_list[olind].len);

         else 
	    LE_send_msg( GL_INFO, 
		"Prod %d (vol# %d) -> %s (len %d, buf size = %d)\n", 
		Out_list[olind].type, hd->g.vol_num, Out_list[olind].name, len,
		Out_list[olind].len);

         /* Write this out for all products. */
         LE_send_msg( GL_INFO, 
             "---> Request Params: %d, %d, %d, %d, %d, %d\n",
             hd->g.req_params[0], hd->g.req_params[1], hd->g.req_params[2],
             hd->g.req_params[3], hd->g.req_params[4], hd->g.req_params[5] );

         LE_send_msg( GL_INFO, 
             "---> Response Params: %d, %d, %d, %d, %d, %d\n",
             hd->g.resp_params[0], hd->g.resp_params[1], hd->g.resp_params[2],
             hd->g.resp_params[3], hd->g.resp_params[4], hd->g.resp_params[5] );

      }
      else{

         /*
           Increment count of messages written.  Used in the case of radial
           messages .... don't want to inform operator of every radial written.
         */
         cnt++;

         if ((cnt % 100) == 0)
            LE_send_msg( GL_INFO,
		"%d rads %d (len %d) -> %s (buf size = %d)\n", 
		cnt, Out_list[olind].type, len, Out_list[olind].name, 
                Out_list[olind].len);

      }

   }
   else if( stream == PGM_REPLAY_STREAM ){

      if( Out_list[olind].timing == ELEVATION_DATA )
         LE_send_msg( GL_INFO, 
            "Replay Prod %d (vol# %d, rpg_elev_ind %d) -> %s (len %d, buf size = %d)\n", 
	    Out_list[olind].type, hd->g.vol_num, hd->g.elev_ind,  
            Out_list[olind].name, len, Out_list[olind].len);
      else 
         LE_send_msg( GL_INFO, 
	    "Replay Prod %d (vol# %d) -> %s (len %d, buf size = %d)\n", 
	    Out_list[olind].type, hd->g.vol_num, Out_list[olind].name, len,
	    Out_list[olind].len);

      /* Write this out for all products. */
      LE_send_msg( GL_INFO, 
          "---> Request Params: %d, %d, %d, %d, %d, %d\n",
          hd->g.req_params[0], hd->g.req_params[1], hd->g.req_params[2],
          hd->g.req_params[3], hd->g.req_params[4], hd->g.req_params[5] );

      LE_send_msg( GL_INFO, 
          "---> Response Params: %d, %d, %d, %d, %d, %d\n",
          hd->g.resp_params[0], hd->g.resp_params[1], hd->g.resp_params[2],
          hd->g.resp_params[3], hd->g.resp_params[4], hd->g.resp_params[5] );

   }

#ifdef PROD_STATISTICS_DEFINED

   /* Update product statistics */
   if( Out_list[olind].timing == RADIAL_DATA )
      Product_statistics( olind, len );
 
   else
      Product_statistics( olind, len - sizeof(Prod_header) );

#endif

   return 0;

/* End of Write_informational_message() */
}

/********************************************************************

   Description: 
      This function registers the output in the output data list.

   Input:	
      data_type - The output data type (RPG buffer type)

   Output:
      Out_list and N_outs are updated by registered output.

   Return:	
      Return value is index into Out_list or negative value on 
      error.

   Notes:
      Duplicated data type specifications are ignored.

********************************************************************/
static int Register_output( int data_type ){

    int i, code;
    char *out_name;
    static char *Unknown_name = "??????";

    /* Make sure the output is defined in the PAT. */
    if( ORPGPAT_prod_in_tbl( data_type ) < 0 )
       PS_task_abort( "Product ID %d Not In PAT", data_type );

    /* Check if output already registered. */
    for (i = 0; i < N_outs; i++) {

	if (Out_list [i].type == data_type){

            LE_send_msg( GL_INFO, "Data Type (%d) Already Registered\n",
                         data_type ); 
	    return (i);

        }

    }

    /* Check if too many outputs registered. */
    if( N_outs >= MAXN_OUTS ){	

	LE_send_msg( GL_INFO, "Too Many Outputs Specified\n" );
	return (-1);

    }

    /* Register the output. */
    Out_list [N_outs].type = data_type;
    out_name = ORPGDA_lbname( data_type );
    if( out_name != NULL ){

       int len;

       len = strlen( out_name );
       Out_list [N_outs].name = calloc( 1, len+1 );
       if( Out_list [N_outs].name != NULL )
          memcpy( Out_list [N_outs].name, out_name, len );

    }
    else{

       LE_send_msg( GL_ERROR, "ORPGDA_lbname Returned NULL For Product ID %d\n",
                    data_type );

       Out_list [N_outs].name = Unknown_name;

    }

    Out_list [N_outs].data_name = NULL;
    Out_list [N_outs].timing = ORPGPAT_get_type( data_type );
    Out_list [N_outs].len = 0;
    Out_list [N_outs].requested = FALSE;
    Out_list [N_outs].gen_cnt = 0;
    Out_list [N_outs].event_id = ORPGEVT_NULL_EVENT;
    code = ORPGPAT_get_code( data_type );
    if( code <= 0 )
       Out_list [N_outs].int_or_final = INT_PROD;
    else
       Out_list [N_outs].int_or_final = code;
		
#ifdef PROD_STATISTICS_DEFINED

    /* Initialize product statistics data. */
    memset( &Out_list[N_outs].p_stats, 0, sizeof(Prod_stats_t) );
    Out_list[N_outs].p_stats.prod_id = data_type;

#endif

    N_outs++;

    /* If data is of timing RADIAL_DATA, register it for replay. */
    if( Out_list[N_outs-1].timing == RADIAL_DATA ){ 

       if( ORPGPAT_get_warehoused( Out_list [i].type ) > 0 ){

          int warehouse_id = ORPGPAT_get_warehouse_id( Out_list [i].type );
          int warehouse_acct_id = ORPGPAT_get_warehouse_acct_id( Out_list [i].type );
          int ret = -1;

          if( (warehouse_id > 0) && (warehouse_acct_id > 0) )
             ret = ORPGBDR_reg_radial_replay_type( data_type, warehouse_id,
                                                   warehouse_acct_id );  

          if( ret > 0 ){

             LE_send_msg( GL_INFO, "ORPGBDR_reg_radial_replay_type Successful\n" );
             LE_send_msg( GL_INFO, "--->warehouse ID: %d, warehouse acct ID: %d\n",
                          warehouse_id, warehouse_acct_id );

          }

       }

    }

    /* Return the index into Out_list of this registered output. */
    return( N_outs-1 );

}

/****************************************************************************

   Description:
      If the task's input stream is replay, then write the product generation
      message to the replay response LB.  For all input streams, write 
      product generation message to product generation message LB.

   Inputs:
      hd - pointer to product generation message.

   Outputs:

   Returns:

   Notes:
      Task may terminate if write fails.

****************************************************************************/ 
static void Write_prod_gen_msg( Prod_gen_msg *hd ){

   int ret;

   /* If input stream is replay, write product generation message to replay
      response LB. */
   if( (Input_stream == PGM_REPLAY_STREAM)
                     ||
       (hd->prod_id == CFCPROD) ){

      ret = ORPGDA_write( ORPGDAT_REPLAY_RESPONSES, (char *) hd, 
                          sizeof(Prod_gen_msg), LB_ANY ); 

      if( ret != sizeof(Prod_gen_msg) )
         PS_task_abort( "Replay Response Write Failed (%d)\n", ret );

      EN_post( ORPGEVT_REPLAY_RESPONSES, 0, 0, 0 );

   }

   /* Write the product generation message to Product Generation 
      Message LB. */
   ret = ORPGDA_write( ORPGDAT_PROD_GEN_MSGS, (char *) hd, 
                       sizeof(Prod_gen_msg), LB_ANY ); 
   if( ret != sizeof (Prod_gen_msg) ){

       if( Input_stream == PGM_REALTIME_STREAM) 
          PS_task_abort( "Product Generation Message Write Failed (%d)\n", 
                         ret );

       else
          LE_send_msg( GL_INFO, "Product Generation Message Write Failed (%d)\n", 
                       ret );

   }
    
/* End of Write_prod_gen_msg() */
}


/**************************************************************************** 
   Description:
      Given an output buffer pointer, determine the index of this buffer
      in the "Buffers" and "Out_list" lists.

   Inputs:
      bufptr - pointer to output buffer.
      datdis - disposition (either FORWARD or DESTROY)

   Outputs:
      buf_ind - receives the "Buffers" index.
      out_ind - receives the "Out_list" index.

   Returns:
      Negative value on error or 0 on success.


****************************************************************************/ 
static int Find_buffer_in_list( int *bufptr, int *buf_ind, int *out_ind, 
                                int datdis ){

    int ind;
    int olind;

    /* Search for the buffer in the list of allocated buffers. */
    for (ind = 0; ind < N_bufs; ind++) {

	if (Buffers[ind].ptr == *bufptr)
	    break;

    }	

    /* If buffer not found in buffer list (i.e., attempting to release
       a buffer that was never acquired), abort. */
    if( ind >= N_bufs ){

        if( datdis == DESTROY ){

           LE_send_msg( GL_ERROR, "Buffer not found - RPG code error");
           return( RPG_BUF_NOT_FOUND );

        }
        else
           PS_task_abort( "Buffer not found - RPG code error\n");

    }

    /* Find the index of the datatype in the list of registered output
       data types (Out_list). */
    for (olind = 0; olind < N_outs; olind++) {

	if (Out_list[olind].type == Buffers[ind].type)
	    break;

    }

    /* Set the return values. */
    *buf_ind = ind;
    *out_ind = olind;

    return( 0 );

/* End of Find_buffer_in_list() */
}

#ifdef PROD_STATISTICS_DEFINED

/**************************************************************************

   Description:
      Calculates product min, max, and avg product size and stores in LB.

   Inputs:
      olind - index into Out_list array for this product
      len - len of product, in bytes.

   Outputs:

   Returns:

   Notes:

**************************************************************************/
static void Product_statistics( int olind, int len ){

   Prod_stats_t *p_stat = &Out_list[olind].p_stats;
   int ret;

   if( p_stat->num_prods == 0 ){

      p_stat->min_size = len;
      p_stat->max_size = len;
      p_stat->avg_size = len;

      /* First product in list. */
      p_stat->num_prods = 1;

   }
   else{

      /* Find min, max, and avg product size. */
      if( len < p_stat->min_size )
         p_stat->min_size = len;

      if( len > p_stat->max_size )
         p_stat->max_size = len;

      p_stat->avg_size = ((p_stat->avg_size*p_stat->num_prods) + len)/(p_stat->num_prods + 1);

      /* Increment the number of product for which we are keeping stats. */
      p_stat->num_prods++;

   }

   /* Write this information to LB. */
   if( (ret = ORPGDA_write( ORPGDAT_PROD_STATISTICS, (char *) p_stat,
                            sizeof(Prod_stats_t), p_stat->prod_id )) < 0 )
      LE_send_msg( GL_ERROR, "ORPGDA_write to ORPGDAT_PROD_STATISTICS Failed (%d)",
                   ret ); 

}

#endif
