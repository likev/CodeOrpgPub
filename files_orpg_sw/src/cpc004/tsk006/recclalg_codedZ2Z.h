/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/06/17 20:19:51 $
 * $Id: recclalg_codedZ2Z.h,v 1.4 2006/06/17 20:19:51 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#ifndef RECCLALG_CODEDZ2Z_H
#define RECCLALG_CODEDZ2Z_H



static float codedZ2Z (int Zcode) {
/*  Converts the Level II "biased" reflectivity data to power (in dBZ)  */

	float	dBZ;

	if (Zcode > 1) 
		dBZ = (Zcode - 66) * 0.5;  
	else if (Zcode == 1) 
		dBZ = RF;
	else
		dBZ = MISSING;    /*  Below threshold  */

	return(dBZ);
}

#endif

