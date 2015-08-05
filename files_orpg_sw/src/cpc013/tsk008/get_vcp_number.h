/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:12:18 $
 * $Id: get_vcp_number.h,v 1.1 2004/01/21 20:12:18 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/*****************************************************************
File	: get_vcp_number.h
Details : returns the VCP number
******************************************************************/

#ifndef SAA_VCP_NUMBER_H
#define SAA_VCP_NUMBER_H

#include <orpgrda.h>
#include <rda_status.h>
#include <orpg.h>

/*****************************************************************
Method	: get_vcp_number
Details	: returns the vcp number
******************************************************************/
int get_vcp_number (){

	return ORPGRDA_get_status(RS_VCP_NUMBER);

}/*end get_vcp_number */

/***************************************************************/
#endif
