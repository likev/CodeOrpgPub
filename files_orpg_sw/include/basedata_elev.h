/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/22 13:36:21 $
 * $Id: basedata_elev.h,v 1.6 2006/09/22 13:36:21 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifndef BASEDATA_ELEV_H
#define BASEDATA_ELEV_H

#include <basedata.h>
#include <a309.h>

/* Macro definitions. */
#define MAX_RADIALS_ELEV    	400
#define SR_MAX_RADIALS_ELEV    	800

/* Useful macro definitions. */
#define BASEDATA_HDR_SIZE    	sizeof(Base_data_header)
#define RADAR_DATA_SIZE      	(MAX_BASEDATA_REF_SIZE+(2*BASEDATA_DOP_SIZE))
#define MAX_RADIAL_SIZE      	(RADAR_DATA_SIZE+BASEDATA_HDR_SIZE)

/* Define the structure of the radial message. */
/* NOTE:  The msg_len field in the Base_data_header is redefined for the 
          elevation product.  The length is in terms of bytes, not shorts. */
typedef struct { 

   Base_data_header bdh;       
   unsigned char    radar_data[RADAR_DATA_SIZE];

} Compact_radial;

/* Define the structure of the elevation product. */
typedef struct {

   int num_radials;
   short type;
   short elev_ind;
   Compact_radial radial[1];

} Compact_basedata_elev;

#endif
