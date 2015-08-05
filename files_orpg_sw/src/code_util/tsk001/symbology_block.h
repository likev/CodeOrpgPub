/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:47:48 $
 * $Id: symbology_block.h,v 1.7 2008/03/13 22:47:48 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* symbology_block.h */

#ifndef _SYMBOLOGY_BLOCK_H_
#define _SYMBOLOGY_BLOCK_H_

#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include <Xm/ToggleB.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <prod_gen_msg.h>
#include <product.h>


#include "byteswap.h"

#include "global.h"
#include "grow_array.h"
#include "packet_definitions.h"

#define FALSE 0
#define TRUE 1

typedef struct{
  short divider;
  short blockID;
  int block_length;
  short n_layers;
}Sym_hdr;

/*  This would include orpg_product.h (don't want to do this in CVG ) */
/*  A source of RPGP_print function prototypes which we also do not use. */
/* #include <rpgp.h> */
#include "cvg_orpg_product.h"

/*  from orpg file packet_28.h */
typedef struct
{
   short  code;         /* Packet code                                   */
   short  align;
   int    num_bytes;    /* Byte length not including self or packet code */
} packet_28_t;






extern assoc_array_i *msg_type_list;
extern Widget shell;

extern screen_data *sd;
/* extern screen_data *sd1, *sd2, *sd3; */
extern int verbose_flag;


extern int orpg_build_i;
extern short get_elev_ind(char *bufptr, int orpg_build);


/* prototypes */
int read_word(char *buffer,int *offset);
int read_word_swap_result(char *buffer,int *offset);
int read_word_flip(char *buffer,int *offset);

short read_half(char *buffer,int *offset);
short read_half_swap_result(char *buffer,int *offset);
short read_half_flip(char *buffer,int *offset);

unsigned char read_byte(char *buffer,int *offset);

int parse_packet_numbers(char *buffer);


/* cvg 8.2 */
void accept_block_error_cb(Widget w,XtPointer client_data, XtPointer call_data);
int check_offset_length(char *bufptr, int verbose);
/* CVG 8.2 */
/* void skip_over_packet(int pcode, int *offset, char *icd_product); */
int skip_over_packet(int pcode, int *offset, char *icd_product);


extern void delete_layer_info(layer_info *linfo, int size);
extern void packet_1_skip(char *,int *);
extern void packet_2_skip(char *,int *);
extern void packet_3_skip(char *,int *);
extern void packet_4_skip(char *,int *);
extern void packet_5_skip(char *,int *);
extern void packet_6_skip(char *,int *);
extern void packet_7_skip(char *,int *);
extern void packet_8_skip(char *,int *);
extern void packet_9_skip(char *,int *);
extern void packet_10_skip(char *,int *);
extern void packet_11_skip(char *,int *);
extern void packet_12_skip(char *,int *);
extern void packet_13_skip(char *,int *);
extern void packet_14_skip(char *,int *);
extern void packet_15_skip(char *,int *);
extern void packet_16_skip(char *,int *);
extern void packet_17_skip(char *,int *);
extern void packet_18_skip(char *,int *);
extern void packet_19_skip(char *,int *);
/*** NEW PACKET ***/
extern void packet_20_skip(char *,int *);
extern void packet_23_skip(char *,int *);
extern void packet_24_skip(char *,int *);
extern void packet_25_skip(char *,int *);
extern void packet_26_skip(char *,int *);
extern void packet_28_skip(char *,int *);
extern void packet_0802_skip(char *,int *);
extern void packet_3501_skip(char *,int *);
extern void packet_0E03_skip(char *,int *);
extern void packet_AF1F_skip(char *,int *);
extern void packet_BA07_skip(char *,int *);
extern void digital_raster_skip(char *buffer,int *offset);

extern int cvg_RPGP_product_deserialize (char *serial_data,
                        int size, void **prod);

#endif




