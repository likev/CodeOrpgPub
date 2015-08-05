/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 22:07:20 $
 * $Id: padback.h,v 1.2 2002/11/26 22:07:20 nolitam Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/************************************************************************
Module:         padback.h  

Description:    include file for padback.c that pads missing bytes to 
                0 at the end of a run length encoded radial         
                
Authors:        Andy Stern, Software Engineer, Mitretek Systems
                    astern@mitretek.org
                Tom Ganger, Systems Engineer,  Mitretek Systems
                    tganger@mitretek.org
                Version 1.0, October 2000
$Id: padback.h,v 1.2 2002/11/26 22:07:20 nolitam Exp $
*************************************************************************/
#ifndef _PADBACK_H_
#define _PADBACK_H_

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE  1

/* Local Prototypes ----------------------------------------------------*/
int pad_back(int byte_flag, short* outbuf, int buff_step, 
   int begin_index, int end_index, int num_bins);
   
/* External Prototypes -------------------------------------------------*/
extern int short_isbyte(short input_word, short* value, 
   int store_position);

#endif
