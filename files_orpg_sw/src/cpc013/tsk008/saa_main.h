/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/03/18 15:33:00 $
 * $Id: saa_main.h,v 1.5 2005/03/18 15:33:00 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef SAA_MAIN_H_
#define SAA_MAIN_H_

/* System and ORPG Includes------------------------------------------- */
#include <itc.h>
#include <vcp.h>
#include <rpgc.h>
#include <rpgcs.h>
#include <orpg.h> 
#include <orpgctype.h>
#include <product.h>
#include <orpgpat.h>
#include <orpgsite.h>
#include <rpg.h>
#include <basedata.h>
#include <rdacnt.h>
#include <saa_params.h>
#include <saa_arrays.h>
#include <alg_adapt.h>


#ifndef SAA_BUFSIZE
	#define SAA_BUFSIZE  (sizeof(saa_inp))
#endif

int currentvcpnumber;
int data_read_from_files;   /* flag to indicate that data is read from files */
int data_read_from_total_files;
typedef struct{
	unsigned short halfword[SAA_BUFSIZE];
}Output_buf_t;

saa_adapt_params_t saa_params;

void Event_handler( int queued_parameter );

#endif  /* _SAA_MAIN_H */
