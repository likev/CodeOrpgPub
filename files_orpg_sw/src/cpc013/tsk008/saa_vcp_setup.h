/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:12:48 $
 * $Id: saa_vcp_setup.h,v 1.1 2004/01/21 20:12:48 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
 

#ifndef SAA_VCP_SETUP_H
#define SAA_VCP_SETUP_H

#ifndef VCP_H
	#include <vcp.h>
#endif

#ifndef RDACNT_H
	#include <rdacnt.h>
#endif

/*** Structure to hold VCP elevation angles ***/
typedef struct {
	short el_angle[VCPMAX];
	int vcp_when_last_built; /* holds the vcp number when last built
				  * to prevent the table to be built again
				  * if not necessary */
}saa_vcp_t;

saa_vcp_t vcp;

/*** Function that fills in the saa_vcp_t vcp struct with
     valid elevation angles for the currentvcp ***/
void saa_vcp_setup (int currentvcpnumber,int* vcp_status);




#endif
