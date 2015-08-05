/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2011/05/22 17:12:19 $
 * $Id: test_base_prods_8bit.h,v 1.1 2011/05/22 17:12:19 cmn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef TEST_DP_PRODS_H
#define TEST_DP_PRODS_H

#include <orpg_product.h>
#include <packet_16.h>
#include <rpgc.h>
#include <rpgcs.h>
#include <rpgp.h>
#include <siteadp.h>

/* Enumerations, constants, etc. */

enum { REF_PROD_INDEX, VEL_PROD_INDEX, SPW_PROD_INDEX, ZDR_PROD_INDEX, PHI_PROD_INDEX, RHO_PROD_INDEX, NUM_TEST_BASE_PRODS };
enum { REF_MOMENT_INDEX, VEL_MOMENT_INDEX, SPW_MOMENT_INDEX, ZDR_MOMENT_INDEX, PHI_MOMENT_INDEX, RHO_MOMENT_INDEX, DRF2_MOMENT_INDEX, MAX_NUM_MOMENTS_AVAILABLE };

#define	FIRST_PROD_INDEX	REF_PROD_INDEX
#define MAX_REQUESTS		1
#define	MAX_NUM_RADIALS		720
#define	MAX_460_NUM_BINS	1840 /* 460km x 0.25km */
#define	MAX_300_NUM_BINS	1200 /* 300km x 0.25km */
#define	MAX_INFO_MSG_LEN	256
#define	GP_SIZE			sizeof(Graphic_product)
#define	SYMB_SIZE		sizeof(Symbology_block)
#define	PKT16_HDR_SIZE		sizeof(Packet_16_hdr_t)
#define	PKT16_300_DATA_SIZE	(sizeof(Packet_16_data_t) + MAX_300_NUM_BINS)
#define	PKT16_460_DATA_SIZE	(sizeof(Packet_16_data_t) + MAX_460_NUM_BINS)
#define	OFFSET_TO_DATA		(GP_SIZE + SYMB_SIZE + PKT16_HDR_SIZE)
#define	MAX_300_DATA_SIZE	(MAX_NUM_RADIALS * PKT16_300_DATA_SIZE)
#define	MAX_460_DATA_SIZE	(MAX_NUM_RADIALS * PKT16_460_DATA_SIZE)
#define	MAX_300_PROD_SIZE	(OFFSET_TO_DATA + MAX_300_DATA_SIZE)
#define	MAX_460_PROD_SIZE	(OFFSET_TO_DATA + MAX_460_DATA_SIZE)

/* Product Information structure. */

typedef struct Product_info {

  char *name;		/* Product name string ... matches TAT name. */
  int code;		/* Product code. */
  int id;		/* Product ID. */
  int max_prod_size;	/* Maximum size of product, in bytes. */
  int num_bins;         /* Number of bins per radial. */
  int num_radials;      /* Number of radials. */
  int generate;		/* Flag to indicate if product is generated. */
  char *outbuf;		/* Pointer to output buffer. */
  int data_index;	/* Current index to end of data in outbuf. */
  User_array_t request[MAX_REQUESTS]; /* Pointer to request data. */
  int max_data_value;	/* Maximum data value. */
  int min_data_value;	/* Minimum data value. */
  float data_offset;    /* Offset where Float = (N - Offset)/Scale. */
  float data_scale;     /* Scale where Float = (N - Offset)/Scale. */
  float data_precision; /* Precision of data. */
  char moment[8];	/* Name of moment. */

} Product_info_t;

Product_info_t Prod_info[NUM_TEST_BASE_PRODS];
char Msg_buf[MAX_INFO_MSG_LEN];

/* Function Prototypes. */

int Test_base_prods_8bit_buffer_control();

# endif
