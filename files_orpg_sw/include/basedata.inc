C
C RCS info
C $Author: steves $
C $Locker:  $
C $Date: 2014/07/08 21:02:55 $
C $Id: basedata.inc,v 1.15 2014/07/08 21:02:55 steves Exp $
C $Revision: 1.15 $
C $State: Exp $
C

C/*********************************************************************

	Header file defining the data structures and constants used 
	for radar basedata.

C*********************************************************************/


**basedata
C	/* the following is used by FORTRAN programs */

C	/* ---- radial status  ---- */
	INTEGER BAD_RADIAL

C	/* This can be "OR"ed in the status field to indicate a bad radial */
	PARAMETER (BAD_RADIAL	      = X'80')	

C       /* These flag bits are used to determine if moments enabled or
C          disabled. */
        INTEGER*2 REF_ENABLED_BIT, VEL_ENABLED_BIT, WID_ENABLED_BIT
        PARAMETER (REF_ENABLED_BIT = 13)
        PARAMETER (VEL_ENABLED_BIT = 12)
        PARAMETER (WID_ENABLED_BIT = 11)

	INTEGER PBD_ABORT_NO, PBD_ABORT_FOR_NEW_EE
	INTEGER PBD_ABORT_FOR_NEW_EV, PBD_ABORT_FOR_NEW_VV

	PARAMETER (PBD_ABORT_NO		=0)
	PARAMETER (PBD_ABORT_FOR_NEW_EE	=1)
	PARAMETER (PBD_ABORT_FOR_NEW_EV	=2)
	PARAMETER (PBD_ABORT_FOR_NEW_VV	=3)

C       /* control flag abort reason */
        INTEGER PBD_ABORT_UNKNOWN 
        INTEGER PBD_ABORT_RADIAL_MESSAGE_EXPIRED
        INTEGER PBD_ABORT_VCP_RESTART_COMMANDED
        INTEGER PBD_ABORT_INPUT_DATA_ERROR
        INTEGER PBD_ABORT_UNEXPECTED_VCP_RESTART

        PARAMETER (PBD_ABORT_UNKNOWN = 7)
        PARAMETER (PBD_ABORT_RADIAL_MESSAGE_EXPIRED = 8)
        PARAMETER (PBD_ABORT_VCP_RESTART_COMMANDED = 16)
        PARAMETER (PBD_ABORT_INPUT_DATA_ERROR = 32)
        PARAMETER (PBD_ABORT_UNEXPECTED_VCP_RESTART = 48)  

C	/* base data message size in shorts */
	INTEGER BASEDATA_SIZE, MAX_BASEDATA_SIZE
	PARAMETER (BASEDATA_SIZE = 2960, MAX_BASEDATA_SIZE = 4340)

C       /* base data message size constructed from RDA/RPG Message 31, */
C       /* in shorts. */
	INTEGER MAX_GENERIC_BASEDATA_SIZE
	PARAMETER (MAX_GENERIC_BASEDATA_SIZE = 8460)

/* */




