XCOMM RCS info 
XCOMM $Author: steves $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2008/04/08 15:15:44 $ 
XCOMM $Id: a314.inc,v 1.7 2008/04/08 15:15:44 steves Exp $ 
XCOMM $Revision: 1.7 $ 
XCOMM $State: Exp $
*.*******************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A314.INC
*.
*.  INCLUDE FILE VERSION: 02
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#      PROGRAMMER         NOTES
*.  ---------    --   -----     ----------------   -----------------
*.  04 25 91     00   92083     D. C. ERICKSON     ADD PROLOGUE
*.  06 19 92     01   93013     DENNIS MILLER      **A314P1
*.  08 25 97     02  NA97-25502 CHERYL STEPHENSON  ADD A314C6,MOD C5
*.
*.*******************************************************************
**A314C5
*VERSION:2
C****************************************************
C*      LAYER COMPOSITE REFLECTIVITY                *
C*          LOCAL ADAPTATION DATA                   *
C*                  A314C5                          *
C*                                                  *
C****************************************************
C
C LAYBND1 = The top of layer #1, in feet.
C LAYBND2 = The top of layer #2, in feet.
C LAYBND3 = THE TOP OF LAYER #3, IN FEET.
C RADHGT = The height of the radar site, in feet MSL.
C RANGE = The maximum analysis slant range, in km.
*
      INTEGER LAYBND1,LAYBND2,LAYBND3,RADHGT,RANGE
      INTEGER LAYBND0
C
      COMMON /A314C5/ LAYBND1,LAYBND2,LAYBND3,RADHGT,RANGE
      COMMON /A314C5/LAYBND0
/*
**A314P5
*VERSION:2
C***********************************************************
C*               LAYER COMPOSITE REFLECTIVITY              *
C*                  COMMON PARAMETER FILE                  *
C*                       A314P5                            *
C*                                                         *
C***********************************************************
C
C Dimensional Parameters
C
C BOXHGHT = The height (y-direction) of a LAYER COMPOSITE
C           REFLECTIVITY analysis BOX, in km.
C BOXWDTH = The width (x-direction) of a LAYER COMPOSITE
C           REFLECTIVITY analysis BOX, in km.
C MAXPRODS = The maximum number of products per volume.
C MAXBUFS = The number of scratch buffers required
C           when the max number of products is requested.
C LAYMAX = Flag signifying a max type product.
C LAYAVG = Flag signifying an avg type product.
C LAYER1 = Flag signifying the lowest layer.
C LAYER2 = Flag signifying the middle layer.
C LAYER3 = Flag signifying the highest layer.
C
      INTEGER BOXHGHT,BOXWDTH,MAXPRODS,MAXBUFS
      PARAMETER ( BOXHGHT=4, BOXWDTH=4, MAXPRODS=6 )
      PARAMETER ( MAXBUFS = MAXPRODS+3 )
      INTEGER LAYAVG,LAYMAX,LAYER1,LAYER2,LAYER3
      PARAMETER (LAYMAX=10, LAYAVG=20, LAYER1=1, LAYER2=2)
      PARAMETER (LAYER3=3)
/*
**A314C6
*VERSION: 0
*
*  PARAMETERS AND VARIABLES USED IN LAYER COMPOSITE
*  REFLECTIVITY LAYER CALCULATIONS
*
      REAL CONMTOKM, DEGTORAD
      PARAMETER (CONMTOKM = .001)
      PARAMETER (DEGTORAD = 0.017453292)
      REAL AZRAD,RSSKM,INVRSSKM
      INTEGER RSS
      INTEGER L1BIN_ST,L1BIN_END,L2BIN_ST,L2BIN_END
      INTEGER L3BIN_ST,L3BIN_END
      COMMON/A314C6/L1BIN_ST,L1BIN_END,L2BIN_ST,L2BIN_END
      COMMON/A314C6/L3BIN_ST,L3BIN_END
      COMMON/A314C6/RSS,AZRAD,RSSKM,INVRSSKM
/*
