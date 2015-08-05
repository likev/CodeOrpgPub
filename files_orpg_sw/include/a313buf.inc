XCOMM RCS info 
XCOMM $Author: steves $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2008/04/08 15:15:02 $ 
XCOMM $Id: a313buf.inc,v 1.4 2008/04/08 15:15:02 steves Exp $ 
XCOMM $Revision: 1.4 $ 
XCOMM $State: Exp $
*.*******************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A313BUF.INC
*.
*.  INCLUDE FILE VERSION: 01
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#      PROGRAMMER         NOTES
*.  ---------    --   -----     ----------------   -----------------
*.  04 25 91     00   92083     D. C. ERICKSON     ADD PROLOGUE
*.  06 27 94     01 93158-003A00 STEVE SMITH
*.
*.*******************************************************************
**A313P0
*VERSION:0
C***********************************************************
C*                VIL/ECHO TOPS COMMON                     *
C*                   PARAMETER FILE                        *
C*                       A313P0                            *
C*                                                         *
C***********************************************************
C
C Dimensional Parameters
C
C MAXBINS = The maximum number of bins to be processed.
C NCOL = The number of analysis box columns, to the
C        nearest even integer value.
C NROW = The number of analysis box rows, to the nearest
C        even integer value.
C BOXHGHT = The height (y-direction) of a VIL/ECHO TOPS
C           analysis BOX, in km.
C BOXWDTH = The width (x-direction) of a VIL/ECHO TOPS
C           analysis BOX, in km.
C MAXRNG = Maximum range of the VIL/ECHO TOPS analysis, in km.
C NUMW = The number of fullwords in the
C        bitmap (OVERLAY) that checks for the echo top
C        found at the maximum elevation.
C
      INTEGER MAXBINS,NCOL,NROW,BOXHGHT,BOXWDTH,MAXRNG
      INTEGER NUMW,SHIFTFAC,RSS
      REAL RSSKM,HALFCOL,HALFROW,GRID_CENTER_I
      REAL GRID_CENTER_J
      PARAMETER ( MAXBINS=460, MAXRNG=230 )
      PARAMETER ( BOXHGHT=4, BOXWDTH=4 )
      PARAMETER ( NCOL=((MAXRNG+(BOXWDTH-1))/BOXWDTH)*2 )
      PARAMETER ( NROW=((MAXRNG+(BOXHGHT-1))/BOXHGHT)*2 )
      PARAMETER ( NUMW=((NCOL*NROW)+31)/32 )
      PARAMETER ( HALFCOL=NCOL*0.5, HALFROW=NROW*0.5 )
      PARAMETER ( SHIFTFAC=65536, RSS=1000,RSSKM=1.0 )
      PARAMETER ( GRID_CENTER_I=(HALFCOL+1)*SHIFTFAC )
      PARAMETER ( GRID_CENTER_J=(HALFROW+1)*SHIFTFAC )
C
C Common supplimental array positional parameters
C
C ONCO = The pointer to the number of columns in both the VIL
C        and Echo Tops supplimental variable arrays.
C ONRO = The pointer to the number of rows in both the VIL
C        and Echo Tops supplimental variable arrays.
C OMSR = The pointer to the maximum analysis slant range
C        in both the VIL and Echo Tops supplimental variable
C        arrays.
C ONZE = The pointer to the minimum reflectivity threshold in
C        both the VIL and Echo Tops supplimental varaible
C        arrays.
      INTEGER ONCO,ONRO,OMSR,ONZE
      PARAMETER ( ONCO=1, ONRO=2, OMSR=3, ONZE=4 )
C***********************************************************
C*                  ECHO TOPS OUTPUT                       *
C*                     PARAMETERS                          *
C*                                                         *
C***********************************************************
C
C Dimensional Parameters
C
C NETCOL = The number of columns in the Echo tops product.
C NETROW = The number of rows in the Echo Tops product.
C NETSIZ = The size of the Echo Tops supplemental variable
C          array.
C
      INTEGER NETCOL,NETROW,NETSIZ
      PARAMETER ( NETCOL=NCOL, NETROW=NROW, NETSIZ=7 )
C
C Offset Parameters
C
C OSET = The offset to the array of parameters describing
C        the size, maximum value and thresholds of the
C        Echo Tops product.
C OVET = The offset to the array of Echo Tops values.
C OBIT = The offset to the maximum Echo Top/last elevation
C        identifier Bit Map.
C
      INTEGER OSET,OVET,OBIT
      PARAMETER ( OSET=0, OBIT=NETSIZ, OVET=NUMW+NETSIZ )
C
C Local supplimental array positional parameters
C
C OMET = The pointer into the Echo Tops supplimental variable
C        array containing the maximum echo top value.
C OMETC = The pointer into the Echo Tops supplimental variable
C         array containing the maximum echo top column position.
C OMETR = The pointer into the Echo Tops supplimental variable
C         array containing the maximum echo top row position.
      INTEGER OMET,OMETC,OMETR
      PARAMETER ( OMET=5, OMETC=6, OMETR=7 )
C***********************************************************
C*                     VIL OUTPUT                          *
C*                     PARAMETERS                          *
C*                                                         *
C***********************************************************
C
C Dimensional Parameters
C
C NVILCOL = The number of columns in the VIL product.
C NVILROW = The number of rows in the VIL product.
C NVILSIZ = The size of the VIL supplemental variable array.
C
      INTEGER NVILCOL,NVILROW,NVILSIZ
      PARAMETER ( NVILCOL=NCOL, NVILROW=NROW, NVILSIZ=8 )
C
C Offset Parameters
C
C OSVI = The offset to an array describing the size,
C        the maximum and thresholds of the VIL product.
C OVVI = The offset to the array of VIL values.
C
      INTEGER OSVI,OVVI
      PARAMETER ( OSVI=0,OVVI=NVILSIZ )
C
C Local supplimental array positional parameters
C
C OMVT = The pointer into the VIL supplimental variable
C        array containing the maximum VIL threshold value.
C OMVI = The pointer into the VIL supplimental variable
C        array containing the maximum VIL value.
C OMVIC = The pointer into the VIL supplimental variable
C         array containing the maximum VIL column position.
C OMVIR = The pointer into the VIL supplimental variable
C         array containing the maximum VIL row position.
C
      INTEGER OMVT,OMVI,OMVIC,OMVIR
      PARAMETER ( OMVT=5, OMVI=6, OMVIC=7, OMVIR=8 )
/*
**A313Z0
*VERSION:0
C***********************************************************
C*                  ECHO TOPS OUTPUT                       *
C*                  DEFINITION FILE                        *
C*                       A313Z0                            *
C*                                                         *
C***********************************************************
C
C                   DEFINITION FILE
C
C Array and Variable Definitions
C
C ETPAR(ONCO) = The number of columns in the Echo Tops
C               product.
C ETPAR(ONRO) = The number of rows in the Echo Tops
C               product.
C ETPAR(OMSR) = The maximum analysis slant range of the
C               Echo Tops product, in km.
C ETPAR(ONZE) = The minimum reflectivity threshold of the
C               Echo Tops product (multiplied by 10), in dbz.
C ETPAR(OMET) = The maximum Echo Top value of the Echo
C               Tops product, in thousands of feet.
C ETPAR(OMETC) = The maximum Echo Top column position.
C ETPAR(OMETR) = The maximum Echo Top row position.
C ETVAL( NETCOL*NETROW ) = The array of Echo Tops values,
C                          in thousands of feet.
C OVERLAY( NUMW ) = The Bit Map identifying which
C                     Echo Tops were found at the
C                     maximum elevation angle.
C
      INTEGER ETPAR (NETSIZ), OVERLAY( NUMW )
      INTEGER*2 ETVAL( NETCOL*NETROW )
/*
**A313ZA
*VERSION:0
C
C SAME AS A313Z1 WITH THE EXCEPTION THAT ETVAL IS DEFINED
C AS A TWO-DIMENSIONAL ARRAY.
C
      INTEGER ETPAR (NETSIZ), OVERLAY( NUMW )
      INTEGER*2 ETVAL( NETCOL,NETROW )
/*
**A313Z1
*VERSION:0
C***********************************************************
C*                     VIL OUTPUT                          *
C*                  DEFINITION FILE                        *
C*                       A313Z1                            *
C*                                                         *
C***********************************************************
C
C                   DEFINITION FILE
C
C Array and Variable Definitions
C
C VILPAR(ONCO) = The number of columns in the VIL
C                product.
C VILPAR(ONRO) = The number of rows in the VIL
C                product.
C VILPAR(ONSR) = The maximum analysis slant range of the
C                VIL product, in km.
C VILPAR(ONZE) = The minimum reflectivity threshold of the
C                VIL product (multiplied by 10), in dbz.
C VILPAR(OMVT) = The maximum allowable VIL value threshold, of
C                the VIL product.
C VILPAR(OMVI) = The maximum VIL value of the VIL
C                product (multiplied by 10).
C VILPAR(OMVIC) = The maximum VIL column position.
C VILPAR(OMVIR) = The maximum VIL row position.
C VILVAL( NVILCOL*NVILROW ) = The array of VIL values,
C                             in kg/m**2.
C
      INTEGER VILPAR(NVILSIZ)
      INTEGER*2 VILVAL( NVILCOL*NVILROW )
/*
**A313ZB
*VERSION:0
C
C SAME AS A313Z1 WITH THE EXCEPTION THAT VILVAL IS DEFINED AS
C A TWO-DIMENSIONAL ARRAY.

      INTEGER VILPAR(NVILSIZ)
      INTEGER*2 VILVAL( NVILCOL,NVILROW )
/*
