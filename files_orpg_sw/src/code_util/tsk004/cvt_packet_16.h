/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:52 $
 * $Id: cvt_packet_16.h,v 1.4 2009/05/15 17:37:52 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/* cvt_packet_16.h */

#ifndef _PACKET_16_H_
#define _PACKET_16_H_

#include <stdio.h>
#include <stdlib.h>


#include "misc_functions.h" 
#include "bscan_format.h"

#include "cvt.h"

#define FALSE 0
#define TRUE 1

/* CVT 4.4 added the rad parameter and the dec_places parameter */
void print_radial(char *msg,int rad,short start_angle,short delta,char *buffer,
                  int *offset,short num_bytes,int flag, int dec_places);
   
void packet_16(char *buffer,int *offset,int *flag);

/* CVG 4.4 */
extern float scale_parameter(short val,int flag);
extern int get_decode_params(int decode_flag, short *product, 
                                             decode_params_t *params);
extern void decode_level(unsigned int d_level, char *decode_val, 
                         decode_params_t *params, int num_dec_spaces);




#endif

