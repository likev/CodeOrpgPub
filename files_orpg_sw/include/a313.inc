XCOMM RCS info 
XCOMM $Author: steves $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2008/04/08 15:15:00 $ 
XCOMM $Id: a313.inc,v 1.5 2008/04/08 15:15:00 steves Exp $ 
XCOMM $Revision: 1.5 $ 
XCOMM $State: Exp $
*.*******************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A313.INC
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
*.  11 05 91     01   92648     BRAD SUTKER        REVISE LW TABLE
*.  06 08 94     02   93158-003A0 STEVE SMITH      ADD IREFMIN
*.
*.*******************************************************************
**A313B1
*VERSION:1
C********************************************
C*               A313B1                     *
C*                                          *
C* THE LOOK-UP TABLE CONVERTING THE I*2     *
C* BASE REFLECTIVITY TO LIQUID WATER        *
C* VALUES (multiplied by 100)               *
C********************************************
C
C N_MAXLW = THE MAXIMUM NUMBER OF LIQUID WATER VALUES
C           IN THE LOOK-UP TABLE.
C
C LWC(0:N_MAXLW) = THE TABLE VALUE AT POSITION 0 TO N_MAXLW FOR
C LIQUID WATER, IN G/M**3 .
C
      INTEGER*2 N_MAXLW
      PARAMETER(N_MAXLW=256)
C
      INTEGER LWC(0:N_MAXLW)
#IFDEF LINUX
      INTEGER LWX
#ELSE
      INTEGER*2 LWX
#ENDIF
C
      COMMON /A313C1/ LWC
C
C Conversion table for liquid water ( multiplied by 100 )
C
C NOTE: -999,-888, and -777 indicate: below threshold,
C       range folded, and missing data, respectively.
C These bad data flags will not be processed since they have
C values below 0.
C
C
      DATA (LWC(LWX),LWX=0,41) /-999,-888,
     $   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     $   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     $   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     $   0,  0,  0,  0,  0,  0,  0,  0,  0,  0/

      DATA (LWC(LWX),LWX=42,81) /
     $  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     $  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     $  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     $  0,  0,  0,  0,  0,  0,  0,  0,  0,  0/

      DATA (LWC(LWX),LWX=82,121) /
     $  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,
     $  1,  2,  2,  2,  2,  2,  2,  3,  3,  3,
     $  3,  3,  4,  4,  4,  5,  5,  5,  6,  6,
     $  7,  7,  8,  8,  9,  9, 10, 11, 12, 12/

      DATA (LWC(LWX),LWX=122,161) /
     $ 13, 14, 15, 16, 17, 19, 20, 21, 23, 24,
     $ 26, 28, 30, 32, 34, 36, 39, 41, 44, 47,
     $ 51, 54, 58, 62, 66, 70, 75, 80, 86, 92,
     $ 98,105,112,120,128,136,146,156,166,178/

      DATA (LWC(LWX),LWX=162,201) /
     $190,203,217,231,247,264,282,301,322,344,
     $367,392,419,447,477,510,540,540,540,540,
     $540,540,540,540,540,540,540,540,540,540,
     $540,540,540,540,540,540,540,540,540,540/

      DATA (LWC(LWX),LWX=202,241) /
     $540,540,540,540,540,540,540,540,540,540,
     $540,540,540,540,540,540,540,540,540,540,
     $540,540,540,540,540,540,540,540,540,540,
     $540,540,540,540,540,540,540,540,540,540/

      DATA (LWC(LWX),LWX=242,256) /
     $540,540,540,540,540,540,540,540,540,540,
     $540,540,540,540,-777/
/*
**A313C0
*VERSION:0
C  *************************************************
C  *                   A313C0                      *
C  *          COMMON ADAPTATATION DATA             *
C  *              FOR VIL/ECHO TOPS                *
C  *                                               *
C  *************************************************
C  VIL/ECHO TOPS COMMON ADAPTABLE DATA
C
      INTEGER MAXVILTH,RADHGT,IREFMIN
      REAL REFMIN,BMWDTH
C
      COMMON /A313C0/  MAXVILTH,RADHGT,REFMIN,BMWDTH,IREFMIN
/*
