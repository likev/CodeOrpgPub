/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:53 $
 * $Id: saa_vcp_setup.c,v 1.2 2008/01/04 20:54:53 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/************************************************************************* 
File		: SAA_vcp_setup.c
Description	: Program to use ORPG adaptation data to get information about MPDA VCPs
Modification
History		:

R. Zachariah	8/03    Modified further for SAA
T. O'Bannon 	6/03	Modified/simplified for Snow Accumulation Algorithm.
			Note, all we want is elevation angle data.
R. May		3/03	Modified to use more ORPG structs
R. May 		6/02	Changed to use adaptation data from ORPG
W. Zittel	1/02	Original Development

***************************************************************************/

#include "saa_vcp_setup.h"
#include <memory.h>
#include <orpg.h>
#include <rpgcs.h>
#include "saaConstants.h"

#define SAA_ERROR 	-1
#define SAA_ALL_OK 	0

#ifdef LINUX
	#define vcp_setup_ vcp_setup__
#endif

/*************************************************************************
Method : saa_vcp_setup
Details: fills the saa_vcp_t vcp struct with the elevation angles
	 of the vcp as specified by the currentvcpnumber
**************************************************************************/
void saa_vcp_setup(
int  currentvcpnumber,
int* vcp_status)
{

	int i,n_ele;
	int elev_angle;
	
	/*Check for NULL pointers */
	if (vcp_status == NULL) {
		LE_send_msg(GL_ERROR,"SAA:vcp_setup - NULL pointer passed in as argument.\n");
	  	return;
	}/*end if */
	
	if( (vcp.vcp_when_last_built == currentvcpnumber) &&
	    (vcp.el_angle[0] 	     != 0) ){
		/*No need to rebuild - just return */
		*vcp_status = (int)SAA_ALL_OK;
		return;
	}
	
	/*initialize vcp.el_angle array to zero */
	for(i=0;i<VCPMAX;i++){
		vcp.el_angle[i] = 0;
	}/*end for */
	
	/*get the number of unique elevation angles for the */
	/*currentvcpnumber */
	n_ele = RPGCS_get_last_elev_index(currentvcpnumber);
	
	/*error checking */
	if(n_ele == RPGCS_ERROR){
		LE_send_msg(GL_ERROR,"SAA:vcp_setup: RPGCS_get_last_elev call failed.\n");
		*vcp_status = SAA_ERROR;
		return;
	}/*end if */
	if(SAA_DEBUG){fprintf(stderr,"Number of Elevations for VCP (%d) : %d.\n",currentvcpnumber,n_ele);}
	
	/*the indexing starts at 1 */
	for(i=1;i<=n_ele;i++){
		/*get the elevation angles */
		elev_angle = RPGCS_get_target_elev_ang(currentvcpnumber,i);
		/*error handling */
		if(elev_angle == RPGCS_ERROR){
			LE_send_msg(GL_ERROR,"SAA:vcp_setup: RPGCS_get_target_elev_angle call failed for elev index %d.\n",i);
			*vcp_status = SAA_ERROR;
			return;
		}/*end if */
		
		vcp.el_angle[i-1] = (short)elev_angle;
	}/*end for */
	
	if(SAA_DEBUG){
		/* printing for debug purposes */
		fprintf(stderr,"Elevation angles for VCP: %d\n", currentvcpnumber);
		fprintf(stderr, "{");
		for(i=0;i<VCPMAX;i++){
			fprintf(stderr,"%d",vcp.el_angle[i]);
			if(i != VCPMAX-1){
				fprintf(stderr,",");
			}
		}/*end for */
		fprintf(stderr,"}\n");
		/*end debug printing */
	}/*end if */
	
	vcp.vcp_when_last_built = currentvcpnumber;
	*vcp_status = (int)SAA_ALL_OK;
	return;
	
}/*end function saa_vcp_setup  */

/************************************************************************/
