/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/03/25 17:21:43 $
 * $Id: dp_dua_accum_types.h,v 1.4 2011/03/25 17:21:43 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/****************************************************************************

	File:		dp_dua_accum_types.h
	Author:		Zhan Zhang
	Date:		Jan 7, 2008
	
	Description:
	------------
	This is the header file to be included by dp_dua_accum_Main.c.
****************************************************************************/

#ifndef DP_DUA_ACCUM_TYPES_H
#define DP_DUA_ACCUM_TYPES_H

#include "dp_dua_accum_consts.h"
#include "dp_dua_accum_common.h"

/* output buffer metadata structure */

/* 20101007 Ward - At Steve Smith's suggestion removed:
 *
 * int max_accum;
 *
 * from dua_accum_buf_metadata_t as it is not used. CCR NA10-00355 */

typedef struct
{
    time_t start_time;	
    time_t end_time;
    int    missing_period_flag;
    int    null_product_flag;
    float  bias;	
} dua_accum_buf_metadata_t;

/* output buffer structure */

typedef struct
{
    dua_accum_buf_metadata_t metadata;      /* metadata part of the message */
    int dua_accum_grid[MAX_AZM][MAX_BINS];	/* grid data for accumulation */
} dua_accum_buf_t;



/* product dependent parameters in PDB */
typedef struct
{
  short param_1;	/* halfword 27 - End Time */
  short param_2;	/* halfword 28 - Time Span  */
  short param_3;	/* halfword 30 - Missing flag (high byte) + Null flag
                                                              (low byte)*/
  short param_4;	/* halfword 47 - Max Accum Value */
  short param_5;	/* halfword 48 - End Date */
  short param_6;	/* halfword 49 - Start Time */
  short param_7;	/* halfword 50 - Mean-field Bias */
  short param_8;	/* halfword 51 - Not used */
  short param_9;	/* halfword 52 - Not used */
  short param_10;	/* halfword 53 - Not used */
} prod_dep_para_t;

/* packet 1 header structure   */
typedef struct {
  short packet_code;
  short data_block_length; /* length of data block in bytes */
}Packet1_hdr_t;

/* packet 1 data block structure  */
typedef struct {
  short I_start; /* screen coordinates */
  short J_start;
  char data[80];
}Packet1_data_t;

#endif
