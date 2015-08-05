/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/06/17 20:19:41 $
 * $Id: recclalg_codedW2W.h,v 1.5 2006/06/17 20:19:41 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef RECCLALG_CODEDW2W_H
#define RECCLALG_CODEDW2W_H



extern float DopplerRes;

static float codedW2W (int Wcode) {
/*  Converts the Level II "biased" data to Spectrum Width (in m/s) */

	float	spectrumWidth;

	if (Wcode > 1)
		spectrumWidth = (Wcode - 129) * DOP_RES_FACTOR;
	else if (Wcode == 1)
		spectrumWidth = REC_RF_FLAG;
	else
		spectrumWidth = REC_MISSING_FLAG;    /*  Below threshold  */
/*
        LE_send_msg( GL_INFO, "In codedW2W: Wcode: %d, spectrumWidth: %f6.2\n",
                     Wcode, spectrumWidth );
*/

	return(spectrumWidth);
}

#endif
