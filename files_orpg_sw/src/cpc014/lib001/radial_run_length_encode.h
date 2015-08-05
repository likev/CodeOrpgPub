/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:25:07 $
 * $Id: radial_run_length_encode.h,v 1.1 2004/01/21 20:25:07 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/************************************************************************
Module:         radial_run_length_encode.h 

Description:    include file for the radial run length encode routine
                
Authors:        Andy Stern, Software Engineer, Mitretek Systems
                    astern@mitretek.org
                Tom Ganger, Systems Engineer,  Mitretek Systems
                    tganger@mitretek.org
                Version 1.0, October 2000

*************************************************************************/

#ifndef _RADIAL_RUN_LENGTH_ENCODE_H_
#define _RADIAL_RUN_LENGTH_ENCODE_H_

#include <stdio.h> 

#define FALSE 0
#define TRUE  1

/* Local Prototypes ----------------------------------------------------*/
int radial_run_length_encode(int start_angle, int angle_delta, 
   short *inbuf, int start_index, int end_index, int num_data_bins, 
   int buff_step, int ctable_index, int buf_index, short *outbuf,
   int prod_id);

/* External Prototypes -------------------------------------------------*/
extern int pad_front(int start_index, int buff_step, short* outbuf,
   int outbuf_index, int* sub_from_start);

extern int pad_back(int byte_flag, short* outbuf, int buff_step, 
   int begin_index, int end_index, int num_bins);

extern int short_isbyte(short input_word, short* value, 
   int store_position);

#endif


