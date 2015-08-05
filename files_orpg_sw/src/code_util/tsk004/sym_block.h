/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:56 $
 * $Id: sym_block.h,v 1.6 2009/05/15 17:37:56 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/* sym_block.h */

#ifndef _SYM_BLOCK_H_
#define _SYM_BLOCK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ORPG includes */
#include <misc.h>


#include "misc_functions.h"

#include "cvt.h"



#define FALSE 0
#define TRUE 1

/* cvt 4.4 moved to cvt.h */
/*typedef struct{    */
/*  short divider;   */
/*  short blockID;   */
/*  int block_length;*/
/*  short n_layers;  */
/*  }Sym_hdr;        */


int print_symbology_block(char *buffer,int *flag);
int print_symbology_header(char *buffer);

extern void packet_0E03(char *buffer,int *offset);
extern void packet_3501(char *buffer,int *offset);
extern void packet_0802(char *buffer,int *offset);
extern void packet_AF1F(char *buffer,int *offset,int *flag);
extern void packet_BA07(char *buffer,int *offset,int *flag);

extern void packet_1(char *buffer,int *offset);
extern void packet_2(char *buffer,int *offset,int *flag);
extern void packet_3(char *buffer,int *offset);
extern void packet_4(char *buffer,int *offset);
extern void packet_5(char *buffer,int *offset);
extern void packet_6(char *buffer,int *offset);
extern void packet_7(char *buffer,int *offset);
extern void packet_8(char *buffer,int *offset);
extern void packet_9(char *buffer,int *offset);
extern void packet_10(char *buffer,int *offset);
extern void packet_11(char *buffer,int *offset);
extern void packet_12(char *buffer,int *offset);
extern void packet_13(char *buffer,int *offset);
extern void packet_14(char *buffer,int *offset);
extern void packet_15(char *buffer,int *offset,int *flag);
extern void packet_16(char *buffer,int *offset,int *flag);
extern void packet_17(char *buffer,int *offset,int *flag);
extern void packet_18(char *buffer,int *offset,int *flag);
extern void packet_19(char *buffer,int *offset,int *flag);
extern void packet_20(char *buffer,int *offset);
extern void packet_23(char *buffer,int *offset);
extern void packet_24(char *buffer,int *offset);
extern void packet_25(char *buffer,int *offset);
extern void packet_26(char *buffer,int *offset);
extern void packet_27(char *buffer,int *offset);
/* CVT 4.4 - added the flag parameter */
extern void packet_28(char *buffer,int *offset,int *flag);

#endif



