/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/06/17 20:18:49 $
 * $Id: recclalg_codedV2V.h,v 1.5 2006/06/17 20:18:49 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef RECCLALG_CODEDV2V_H
#define RECCLALG_CODEDV2V_H



extern float DopplerRes;

static float codedV2V (int Vcode) {
/*  Converts the Level II "biased" data to Velocity  */

	float	Velocity;

	if (Vcode > 1)
		Velocity = (Vcode - 129) * DopplerRes;
	else if (Vcode == 1)
		Velocity = REC_RF_FLAG;
	else
		Velocity = REC_MISSING_FLAG;    /*  Below threshold  */
/*
        LE_send_msg( GL_INFO, "In codedV2V: Vcode = %d, Velocity = %5.2f DopRes=%5.3f\n",
                     Vcode, Velocity,DopplerRes);
*/

	return(Velocity);
}

#endif
