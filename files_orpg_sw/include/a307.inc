XCOMM RCS info 
XCOMM $Author: steves $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2006/01/03 20:07:47 $ 
XCOMM $Id: a307.inc,v 1.9 2006/01/03 20:07:47 steves Exp $ 
XCOMM $Revision: 1.9 $ 
XCOMM $State: Exp $
*.*******************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A307.INC
*.
*.  INCLUDE FILE VERSION: 04
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#          PROGRAMMER         NOTES
*.  ---------    --   -----         ----------------   --------------
*.  09/01/90     00   90884         RICH MIHOCI
*.  09/11/91     01   92356         ED WENDOWSKI       **A307B7
*.  06/19/92     02   93013         DENNIS MILLER      **A307P2
*.  06/19/92     02   93013         DENNIS MILLER      **A307P3
*.  07/22/94     03   93211-001A00  STEVE SMITH        **A304C4
*.  08/15/01     04   NA00-19201    C. Stephenson      **A307CD
*.
*.*******************************************************************
**A307C1
*VERSION: 0000
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                              C
C         COMMON DATA A307C1                                   C
C              WRITTEN: JAN. 18, 1985                          C
C              USE: LOCAL COMMON DATA DECLARATION FILE FOR     C
C                   PRIMARY MODULE NO. 1 OF CPC NO. 7, FORM    C
C                   BASE REFLECTIVITY MAP.                     C
C                                                              C
C**************************************************************C
C**************************************************************C
C                                                              C
C              DESCRIPTION OF COMMON DATA ELEMENTS:            C
C                                                              C
C                   PFLAG - LOGICAL ARRAY WHICH KEEPS TRACK OF C
C                              WHAT RANGES/RESOLUTIONS OF      C
C                              PRODUCTS NEED TO BE GENERATED   C
C                   ENDELCUT - END OF ELEVATION CUT FLAG       C
C                   RADCOUNT - RADIAL COUNT                    C
C                   MAXDL230 - KEEPS TRACK OF MAXIMUM DATA     C
C                                 LEVEL OUT TO 230 KM. RANGE   C
C                   MAXDL460 - KEEPS TRACK OF MAXIMUM DATA     C
C                                 LEVEL OUT TO 460 KM. RANGE   C
C                   VC1 - SCALE FACTOR WITH THE VERTICAL COR-  C
C                            RELATION CORRECTION FIGURED IN    C
C                            FOR PRODUCTS CONTAINING 230 PIX-  C
C                            ELS PER RADIAL TO PAINT AT THE    C
C                            RAMTEK                            C
C                   VC2 - SCALE FACTOR WITH THE VERTICAL COR-  C
C                            RELATION CORRECTION FIGURED IN    C
C                            FOR PRODUCTS CONTAINING 115 PIX-  C
C                            ELS PER RADIAL TO PAINT AT THE    C
C                            RAMTEK                            C
C                   ELMEAS - ELEVATION ANGLE MEASUREMENT       C
C                               (AT LEVEL OFF POINT FOR THE    C
C                                RADAR ANTENNA)                C
C                   TIME - GENERATION TIME OF PRODUCT          C
C                   RFSPT - BEGINNING OF REFLECTIVITY DATA IN  C
C                              THE PREPROCESSED RADIAL BUFFER  C
C                   RF230END - END OF REFLECTIVITY DATA OUT TO C
C                                 230 KM. RANGE IN THE PREPRO- C
C                                 CESSED RADIAL BUFFER         C
C                   RF460END - END OF REFLECTIVITY DATA OUT TO C
C                                 460 KM. RANGE IN THE PREPRO- C
C                                 CESSED RADIAL BUFFER         C
C                                                              C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
      INTEGER TPRODS,RFSPT,RF230END,RF460END,BYTESPWD
      PARAMETER (TPRODS=6,BYTESPWD=2)
      INTEGER PBUFFIND(TPRODS),CLTHTIND(TPRODS),NRLEB(TPRODS)
C
C
      INTEGER*2 TIME(BYTESPWD),RADCOUNT,ELMEAS,VC1,VC2,MAXDL230,
     1          MAXDL460
C
      LOGICAL PFLAG(TPRODS),ENDELCUT
C
      COMMON /A307C1/ PFLAG,ENDELCUT,RADCOUNT,MAXDL230,MAXDL460,
     1                VC1,VC2,ELMEAS,TIME,RFSPT,RF230END,RF460END,
     2                PBUFFIND,CLTHTIND,NRLEB
/*
**A307P1
*VERSION: 0000
C***************************************************************
C
C                    PARAMETERS FOR REFLECTIVITY TASK
C****************************************************************
C
      INTEGER PROD16,PROD17,PROD18,PROD19,PROD20,PROD21
      PARAMETER(PROD16=1,PROD19=2,PROD17=3,PROD20=4,
     1          PROD18=5,PROD21=6)

      CHARACTER*32 R16, R17, R18, R19, R20, R21
      PARAMETER( R16 = 'BREF16$' )
      PARAMETER( R17 = 'BREF17$' )
      PARAMETER( R18 = 'BREF18$' )
      PARAMETER( R19 = 'BREF19$' )
      PARAMETER( R20 = 'BREF20$' )
      PARAMETER( R21 = 'BREF21$' )
/*
**A307C2
*VERSION: 0000
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                              C
C         COMMON DATA A307C2                                   C
C              WRITTEN: FEB. 19, 1985                          C
C              USE: LOCAL COMMON DATA DECLARATION FILE FOR     C
C                   PRIMARY MODULE NO. 2 OF CPC NO. 7, FORM    C
C                   BASE VELOCITY MAP.                         C
C                                                              C
C**************************************************************C
C**************************************************************C
C                                                              C
C              DESCRIPTION OF COMMON DATA ELEMENTS:            C
C                                                              C
C                   PFLAG - LOGICAL ARRAY WHICH KEEPS TRACK OF C
C                              WHAT RANGES/RESOLUTIONS OF      C
C                              PRODUCTS NEED TO BE GENERATED   C
C                   ENDELCUT - END OF ELEVATION CUT FLAG       C
C                   RADCOUNT - RADIAL COUNT                    C
C                   MAXDL60 - KEEPS TRACK OF MAXIMUM DATA      C
C                                LEVEL OUT TO 60 KM. RANGE     C
C                   MAXDL115 - KEEPS TRACK OF MAXIMUM DATA     C
C                                LEVEL OUT TO 115 KM. RANGE    C
C                   MAXDL230 - KEEPS TRACK OF MAXIMUM DATA     C
C                                 LEVEL OUT TO 230 KM. RANGE   C
C                   AMAX60 - ONE-BYTE SCALED, ABSOLUTE MAXIMUM C
C                               DATA LEVEL OUT TO 60 KM. RANGE C
C                   AMAX115 - ONE-BYTE SCALED, ABSOLUTE MAXI-  C
C                               MUM DATA LEVEL OUT TO 115 KM.  C
C                               RANGE                          C
C                   AMAX230 - ONE-BYTE SCALED, ABSOLUTE MAXI-  C
C                               MUM DATA LEVEL OUT TO 230 KM.  C
C                               RANGE                          C
C                   VC1 - SCALE FACTOR WITH THE VERTICAL COR-  C
C                            RELATION CORRECTION FIGURED IN    C
C                            FOR PRODUCTS CONTAINING 240 PIX-  C
C                            ELS PER RADIAL TO PAINT AT THE    C
C                            RAMTEK                            C
C                   VC2 - SCALE FACTOR WITH THE VERTICAL COR-  C
C                            RELATION CORRECTION FIGURED IN    C
C                            FOR PRODUCTS CONTAINING 230 PIX-  C
C                            ELS PER RADIAL TO PAINT AT THE    C
C                            RAMTEK                            C
C                   ELMEAS - ELEVATION ANGLE MEASUREMENT       C
C                               (AT LEVEL OFF POINT FOR THE    C
C                                RADAR ANTENNA)                C
C                   TIME - GENERATION TIME OF PRODUCT          C
C                                                              C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
      INTEGER TPRODS,BYTESPWD
C
      PARAMETER (TPRODS=6,BYTESPWD=2)
C
      INTEGER*2 TIME(BYTESPWD),RADCOUNT,ELMEAS,VC1,VC2,
     *          MXNEG230,MXNEG115,MXNEG60,MXPOS230,MXPOS115,
     *          MXPOS60
C
      LOGICAL PFLAG(TPRODS),ENDELCUT
C
      COMMON /A307C2/ PFLAG,ENDELCUT,RADCOUNT,MXNEG230,MXNEG115,
     1                MXNEG60,VC1,VC2,ELMEAS,TIME,MXPOS230,MXPOS115,
     2                MXPOS60
/*
**A307C3
*VERSION: 0000
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                              C
C         COMMON DATA A307C3                                   C
C              WRITTEN: FEB. 20, 1985                          C
C              USE: LOCAL COMMON DATA DECLARATION FILE FOR     C
C                   PRIMARY MODULE NO. 3 OF CPC NO. 7, FORM    C
C                   BASE SPECTRUM WIDTH MAP.                   C
C                                                              C
C**************************************************************C
C**************************************************************C
C                                                              C
C              DESCRIPTION OF COMMON DATA ELEMENTS:            C
C                                                              C
C                   PFLAG - LOGICAL ARRAY WHICH KEEPS TRACK OF C
C                              WHAT RANGES/RESOLUTIONS OF      C
C                              PRODUCTS NEED TO BE GENERATED   C
C                   ENDELCUT - END OF ELEVATION CUT FLAG       C
C                   RADCOUNT - RADIAL COUNT                    C
C                   MAXDL60 - KEEPS TRACK OF MAXIMUM DATA      C
C                                LEVEL OUT TO 60 KM. RANGE     C
C                   MAXDL115 - KEEPS TRACK OF MAXIMUM DATA     C
C                                LEVEL OUT TO 115 KM. RANGE    C
C                   MAXDL230 - KEEPS TRACK OF MAXIMUM DATA     C
C                                 LEVEL OUT TO 230 KM. RANGE   C
C                   VC1 - SCALE FACTOR WITH THE VERTICAL COR-  C
C                            RELATION CORRECTION FIGURED IN    C
C                            FOR PRODUCTS CONTAINING 240 PIX-  C
C                            ELS PER RADIAL TO PAINT AT THE    C
C                            RAMTEK                            C
C                   VC2 - SCALE FACTOR WITH THE VERTICAL COR-  C
C                            RELATION CORRECTION FIGURED IN    C
C                            FOR PRODUCTS CONTAINING 230 PIX-  C
C                            ELS PER RADIAL TO PAINT AT THE    C
C                            RAMTEK                            C
C                   ELMEAS - ELEVATION ANGLE MEASUREMENT       C
C                               (AT LEVEL OFF POINT FOR THE    C
C                                RADAR ANTENNA)                C
C                   TIME - GENERATION TIME OF PRODUCT          C
C                                                              C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
      INTEGER TPRODS,BYTESPWD
C
      PARAMETER (TPRODS=3,BYTESPWD=2)
C
      INTEGER*2 TIME(BYTESPWD),RADCOUNT,ELMEAS,VC1,VC2,MAXDL60,
     1          MAXDL115,MAXDL230
C
      LOGICAL PFLAG(TPRODS),ENDELCUT
C
      COMMON /A307C3/ PFLAG,ENDELCUT,RADCOUNT,MAXDL60,MAXDL115,
     1                MAXDL230,VC1,VC2,ELMEAS,TIME
/*
**A307C4
*VERSION: 0000
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                              C
C         COMMON DATA A307C4                                   C
C              WRITTEN: MAY 20, 1985                           C
C              USE: LOCAL COMMON DATA DECLARATION FILE FOR     C
C                   PRIMARY MODULE NO. 4 OF CPC NO. 7, FORM    C
C                   COMPOSITE REFLECTIVITY POLAR GRID          C
C                                                              C
C**************************************************************C
C                                                              C
C              DECRIPTION OF CONSTANTS:                        C
C                                                              C
C                   RSS - SURVEILLANCE DATA SAMPLE SIZE (KM)   C
C                   POLRES - RANGE RESOLUTION OF GRID (KM)     C
C                   SCFACT - SCALE FACTOR FOR MAPPING RADIAL   C
C                            BINS TO POLAR COORDINATES         C
C                                                              C
C              DESCRIPTION OF COMMON DATA ELEMENTS:            C
C                                                              C
C                   BEGVSCAN - BEGINNING OF VOLUME SCAN FLAG   C
C                   ENDVSCAN - END OF VOLUME SCAN FLAG         C
C                   RFSPT - BEGINNING OF REFLECTIVITY DATA IN  C
C                              THE PREPROCESSED RADIAL BUFFER  C
C                   RF460END - END OF REFLECTIVITY DATA OUT TO C
C                                 460 KM. RANGE IN THE PREPRO- C
C                                 CESSED RADIAL BUFFER         C
C                                                              C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
      INTEGER RFSPT,RF460END
C
      REAL  SCFACT, RSS, POLRES
      PARAMETER (RSS = 1.0, POLRES = 1.0)
      PARAMETER (SCFACT = 2.**16 * RSS/POLRES)
C
      LOGICAL BEGVSCAN,ENDVSCAN
C
      COMMON /A307C4/ RFSPT,RF460END,BEGVSCAN,ENDVSCAN
/*
**A307C6
*VERSION: 0000
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                              C
C         COMMON DATA A307C6                                   C
C              WRITTEN: JAN. 07, 1999                          C
C              USE: LOCAL COMMON DATA DECLARATION FILE FOR     C
C                   PRIMARY MODULE NO. 6 OF CPC NO. 7,         C
C                   FAA/ITWS DIGITAL BASE VELOCITY MAP.        C
C                                                              C
C**************************************************************C
C**************************************************************C
C                                                              C
C              DESCRIPTION OF COMMON DATA ELEMENTS:            C
C                                                              C
C                   ENDELCUT - END OF ELEVATION CUT FLAG       C
C                   RADCOUNT - RADIAL COUNT                    C
C                   MAXNEG   - KEEPS TRACK OF MAXIMUM NEGATIVE C
C                                DATA LEVEL OUT TO CHOSEN KM   C
C                                RANGE                         C
C                   MAXPOS   - KEEPS TRACK OF MAXIMUM POSITIVE C
C                                DATA LEVEL OUT TO CHOSEN KM   C
C                                RANGE                         C
C                   VC  - SCALE FACTOR WITH THE VERTICAL COR-  C
C                            RELATION CORRECTION FIGURED IN    C
C                            FOR PRODUCT  CONTAINING 115 PIX-  C
C                            ELS PER RADIAL                    C
C                   ELMEAS - ELEVATION ANGLE MEASUREMENT       C
C                               (AT LEVEL OFF POINT FOR THE    C
C                                RADAR ANTENNA)                C
C                   TIME - GENERATION TIME OF PRODUCT          C
C                   PROC_RAD - PROCESS RADIAL FLAG             C
C                                                              C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
      INTEGER BYTESPWD
C
      PARAMETER (BYTESPWD=2)
C
      INTEGER*2 TIME(BYTESPWD),RADCOUNT,ELMEAS,VC,
     *          MXNEG,MXPOS
C
      LOGICAL ENDELCUT,PROC_RAD
C
      COMMON /A307C6/ ENDELCUT,PROC_RAD,RADCOUNT,MXNEG,MXPOS,VC,
     1                ELMEAS,TIME
/*
**A307C9
*VERSION: 0000
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                              C
C         COMMON DATA A307C9                                   C
C              WRITTEN: MAY 20, 1985                           C
C              USE: LOCAL COMMON DATA DECLARATION FILE FOR     C
C                   PRIMARY MODULE NO. 5 OF CPC NO. 7, FORM    C
C                   AND RUN-LENGTH-ENCODED PRODUCTS            C
C                                                              C
C**************************************************************C
C                                                              C
C              DESCRIPTION OF COMMON DATA ELEMENTS:            C
C                                                              C
C                   PFLAG - LOGICAL ARRAY WHICH KEEPS TRACK OF C
C                              WHAT RANGES/RESOLUTIONS OF      C
C                              PRODUCTS NEED TO BE GENERATED   C
C                                                              C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
      INTEGER TPRODS
C
      PARAMETER( TPRODS=6 )
C
      LOGICAL PFLAG( TPRODS )
C
      COMMON /A307C9/ PFLAG
/*
**A30788
*VERSION: 0001
C*
C*
C*****************************************************************
C*     A 3 0 7 8 8 .PRM   PARAMETERS FOR VERTICAL CROSS SECTION
C*
C*****************************************************************
C*
C*
C*   NUMBER OF COLUMNS AND ROWS FOR THE VERTICAL CROSS
C*   SECTION CARTESIAN MAP. 70,000 KFT BY 230 KMS.
C*   RESOLUTION = .5KM VERTICALLY, 1KM HORIZONTALLY
C*
      INTEGER*4   CEILING
      PARAMETER   ( CEILING = 70000 )
      REAL*4      FEETTOKM
      PARAMETER   ( FEETTOKM = 0.0003048 )
      INTEGER*4   HALFKM
      PARAMETER   ( HALFKM = 2 )
C
C
      REAL*4      BOXHGHT, BOXWDTH
      INTEGER     GRID_ROWS
      PARAMETER   ( BOXHGHT = 0.5 )
      PARAMETER   ( BOXWDTH = 1.0 )
      PARAMETER   ( GRID_ROWS = CEILING*FEETTOKM/BOXHGHT + .5 )
C
C** DISPLAY GRID PIXEL DEFINITIONS
C
      INTEGER    YTITLE_X,      YTITLE_Y
      INTEGER    X_TOP_LINE,    Y_TOP_LINE
      INTEGER    X_LFT_LINE,    X_RGT_LINE
      INTEGER    RAS_DATA_X,    RAS_DATA_Y
      INTEGER    Y_PIX_PER_BOX
      INTEGER    X_BOT_LINE,    Y_BOT_LINE
      INTEGER    XTITLE_X,      XTITLE_Y
      INTEGER    YT_LFT_LINE,   YB_LFT_LINE
C
      PARAMETER (Y_PIX_PER_BOX = 10)
      PARAMETER (YTITLE_X = 1,            YTITLE_Y = 31)
      PARAMETER (X_TOP_LINE = YTITLE_X,   Y_TOP_LINE = YTITLE_Y + 10)
      PARAMETER (X_LFT_LINE = 50,         X_RGT_LINE = 511)
      PARAMETER (RAS_DATA_X = X_LFT_LINE + 1)
      PARAMETER (RAS_DATA_Y = Y_TOP_LINE + 1)
C
      PARAMETER( X_BOT_LINE = YTITLE_X )
      PARAMETER( Y_BOT_LINE = Y_TOP_LINE +Y_PIX_PER_BOX*GRID_ROWS +1 )
      PARAMETER( XTITLE_X = 34 )
      PARAMETER( XTITLE_Y = Y_BOT_LINE + 2 )
      PARAMETER( YT_LFT_LINE = YTITLE_Y )
      PARAMETER( YB_LFT_LINE = XTITLE_Y + 18 )
C
C** HORIZONTAL SPECIFICATIONS FOR THE DIFFERENT GRIDS
C
      INTEGER      NGRIDS
      PARAMETER  ( NGRIDS = 3 )
      INTEGER      X_PIX_PER_KM(NGRIDS)
      DATA X_PIX_PER_KM / 5,  3,  2/
C
C*
C*********************************************************************
C*
C*  THE VALID PRODUCT CODES FOR CROSS SECTION PRODUCT ARE:
C*
      INTEGER*4   REFLPROD16, VELOPROD16, SPECTPROD8
      INTEGER*4   REFLPROD8, VELOPROD8
      PARAMETER   ( REFLPROD16 = 50 )
      PARAMETER   ( VELOPROD16 = 51 )
      PARAMETER   ( SPECTPROD8 = 52 )
      PARAMETER   ( REFLPROD8  = 85 )
      PARAMETER   ( VELOPROD8  = 86 )
C
C*
C*  THE VALID MOMENT CODES FOR CROSS SECTION PRODUCT ARE:
C*
      INTEGER*4   REFLECTIVITY, VELOCITY, SPECTRUM_WIDTH
      PARAMETER   ( REFLECTIVITY   = 1 )
      PARAMETER   ( VELOCITY       = 2 )
      PARAMETER   ( SPECTRUM_WIDTH = 3 )
C*
C**  DEFINE THE NTR NUMBER OF VERTICAL CROSS SECTION PRODUCTS.
C*
      INTEGER     NTR
      PARAMETER   ( NTR=14 )
C*
C**  DEFINE THE LAYOUT OF REQTAB
C*
C* ROWS, COLUMNS
C*
      INTEGER     REQROWS
      INTEGER     REQCOLS
      PARAMETER   ( REQROWS = 10 )
      PARAMETER   ( REQCOLS = 50 )
C*
C*  INDEXES FOR TABLE REQTAB(10,50)
C*
      INTEGER     PCODE,P1_AZ,P1_RANGE,P2_AZ,P2_RANGE,ELINDEX
      PARAMETER   ( PCODE    = 1 )
      PARAMETER   ( P1_AZ    = 2 )
      PARAMETER   ( P1_RANGE = 3 )
      PARAMETER   ( P2_AZ    = 4 )
      PARAMETER   ( P2_RANGE = 5 )
      PARAMETER   ( ELINDEX   =8 )
C*
C*  OFFSETS INTO THIGHRES USED TO CALL A3CM54 ON FAILED GET OUTBUFS
C*
      INTEGER*4    CUSTIDX, CCOPYNO
      PARAMETER    ( CUSTIDX = 9, CCOPYNO = 10 )
C*
C*  OFFSETS INTO ORIG_PTS
C*
      INTEGER   P1_AZZ, P2_AZZ, P1_RAN, P2_RAN
      PARAMETER(P1_AZZ = 1, P1_RAN = 2, P2_AZZ = 3, P2_RAN = 4)
C
C*  PARAMETERS FOR HEMI ARRAY
C
      INTEGER SOUTH_HEMI, NORTH_HEMI, ORIG_CROSS
      PARAMETER (SOUTH_HEMI = 0)
      PARAMETER (NORTH_HEMI = 1)
      PARAMETER (ORIG_CROSS = 2)
C*
C*********************************************************************
C*
      INTEGER*4    ZERO, ONE, INC1, DEC1, TWO, NOT_MAPPED
      REAL*4       HALF
      REAL         DEGTORAD
C
      PARAMETER    ( ZERO = 0 )
      PARAMETER    ( HALF = 0.5 )
      PARAMETER    ( ONE  = 1 )
      PARAMETER    ( INC1 = 1, DEC1 = 1 )
      PARAMETER    ( TWO = 2 )
      PARAMETER    ( NOT_MAPPED = -1 )
      PARAMETER    ( DEGTORAD = .0174533 )
C*
C*********************************************************************
/*
**A3078CA
*VERSION: 0000
C*********************************************************************
C*    C O M M O N      D A T A
C*
      REAL*4       ORIG_PTS(4,REQCOLS)
      REAL*4       THETA3(REQCOLS)
      REAL*4       BEG_AZ(REQCOLS)
      REAL*4       END_AZ(REQCOLS)
      REAL*4       P1P2_LENG(REQCOLS)
C
      INTEGER*4    HEMI(REQCOLS)
      INTEGER*4    INTERVAL(REQCOLS)
      INTEGER*4    GRID_XCOLS(REQCOLS)
      INTEGER*4    GRID_YROWS(REQCOLS)
C
      LOGICAL      ZEROCROSS(REQCOLS)
C
      COMMON / A3078CA / THETA3, HEMI, INTERVAL, ORIG_PTS
      COMMON / A3078CA / GRID_YROWS,GRID_XCOLS,P1P2_LENG
      COMMON  / A3078CA /  ZEROCROSS, BEG_AZ, END_AZ
C
C*
C*********************************************************************
/*
**A3078CM
*VERSION: 0001
C*
C*  COMMON FOR FORMATTED GRID VECTORS AND LABELS
C
      INTEGER MAX_SIZE
      PARAMETER (MAX_SIZE = 250)
C
      INTEGER GRID_I2S(NGRIDS)
      INTEGER*2 GRID_VECTORS(MAX_SIZE, NGRIDS)
C
      COMMON /A3078CM/ GRID_I2S, GRID_VECTORS
C
/*
**A30789
*VERSION: 0000
C*
C*
C*****************************************************************
C*
C*       VERTICAL CROSS SECTION DEFINITIONS FOR
C*       GRID PATTERN VECTOR ANNOTATIONS, AND GRID
C*       PATTERN TEXT ANNOTATIONS.
C*
C*       GRID_PATT( ) = GRID VECTOR ANNOTATIONS
C*       TEXT ( )     = GRID TEXT   ANNOTATIONS
C*
C*
C*
C*    VERTICAL SCALE FACTOR EQUALS  11 PIXELS TO 1 ROW OF RASTER DATA.
C*
C********************************************************************
C*
C*
      INTEGER*2 GRID_PATT (-2:GRIDSIZE)
C*
C*
      INTEGER BOTTOM,TOP
      PARAMETER( BOTTOM=501 )
      PARAMETER( TOP = BOTTOM-473 )
C*
C*
C******************************
C* LAYER DIVIDER, LENGTH OF DATA IN BYTES
      DATA GRID_PATT(-2),GRID_PATT(-1),GRID_PATT(0)/-1,0,PACKETSIZE/
C*  UNLINKED VECTOR FORMAT CODE:
      DATA GRID_PATT(1) /7/
C*  LENGTH OF BLOCK HEREAFTER:
      DATA GRID_PATT(2) /216/
C*  LEFT GRID BORDER:
      DATA GRID_PATT(3),GRID_PATT(4) /52,0/
      DATA GRID_PATT(5),GRID_PATT(6) /52,511/
C*  BOTTOM GRID BORDER:
      DATA GRID_PATT(7),GRID_PATT(8) /0,BOTTOM/
      DATA GRID_PATT(9),GRID_PATT(10) /511,BOTTOM/
C*  TOP GRID BORDER:
      DATA GRID_PATT(11),GRID_PATT(12) /0,TOP/
      DATA GRID_PATT(13),GRID_PATT(14) /511,TOP/
C*
C* VERTICAL HASH MARKS
C*
C*   FIVE KFT HASH
      DATA GRID_PATT(15),GRID_PATT(16) /52,467/
      DATA GRID_PATT(17),GRID_PATT(18) /57,467/
C*   TEN KFT  HASH
      DATA GRID_PATT(19),GRID_PATT(20) /52,433/
      DATA GRID_PATT(21),GRID_PATT(22) /511,433/
C*   FIFTEEN KFT HASH
      DATA GRID_PATT(23),GRID_PATT(24) /52,400/
      DATA GRID_PATT(25),GRID_PATT(26) /57,400/
C*   TWENTY KFT  HASH
      DATA GRID_PATT(27),GRID_PATT(28) /52,366/
      DATA GRID_PATT(29),GRID_PATT(30) /511,366/
C*   TWENTY FIVE KFT HASH
      DATA GRID_PATT(31),GRID_PATT(32) /52,334/
      DATA GRID_PATT(33),GRID_PATT(34) /57,334/
C*   THIRTY KFT HASH
      DATA GRID_PATT(35),GRID_PATT(36) /52,298/
      DATA GRID_PATT(37),GRID_PATT(38) /511,298/
C*   THIRTY FIVE KFT HASH
      DATA GRID_PATT(39),GRID_PATT(40) /52,265/
      DATA GRID_PATT(41),GRID_PATT(42) /57,265/
C*   FORTY KFT HASH
      DATA GRID_PATT(43),GRID_PATT(44) /52,231/
      DATA GRID_PATT(45),GRID_PATT(46) /511,231/
C*   FORTY FIVE KFT HASH
      DATA GRID_PATT(47),GRID_PATT(48) /52,197/
      DATA GRID_PATT(49),GRID_PATT(50) /57,197/
C*   FIFTY KFT HASH
      DATA GRID_PATT(51),GRID_PATT(52) /52,163/
      DATA GRID_PATT(53),GRID_PATT(54) /511,163/
C*   FIFTY FIVE KFT HASH
      DATA GRID_PATT(55),GRID_PATT(56) /52,129/
      DATA GRID_PATT(57),GRID_PATT(58) /57,129/
C*   SIXTY KFT HASH
      DATA GRID_PATT(59),GRID_PATT(60) /52,96/
      DATA GRID_PATT(61),GRID_PATT(62) /511,96/
C*   SIXTY FIVE KFT HASH
      DATA GRID_PATT(63),GRID_PATT(64) /52,62/
      DATA GRID_PATT(65),GRID_PATT(66) /57,62/
C*
C*
C*   HASH MARKS ON THE HORIZONTAL DEFINED.
C*
C*
C*   TWENTY KM
      DATA GRID_PATT(67),GRID_PATT(68) /92,496/
      DATA GRID_PATT(69),GRID_PATT(70) /92,501/
C*   FORTY KM HASH
      DATA GRID_PATT(71),GRID_PATT(72) /132,496/
      DATA GRID_PATT(73),GRID_PATT(74) /132,501/
C*   SIXTY KM HASH
      DATA GRID_PATT(75),GRID_PATT(76) /172,496/
      DATA GRID_PATT(77),GRID_PATT(78) /172,501/
C*   EIGHTY KM HASH
      DATA GRID_PATT(79),GRID_PATT(80) /212,496/
      DATA GRID_PATT(81),GRID_PATT(82) /212,501/
C*   ONE HUNDRED KM HASH
      DATA GRID_PATT(83),GRID_PATT(84) /252,496/
      DATA GRID_PATT(85),GRID_PATT(86) /252,501/
C*   ONE HUNDRED TWENTY KM
      DATA GRID_PATT(87),GRID_PATT(88) /292,496/
      DATA GRID_PATT(89),GRID_PATT(90) /292,501/
C*   ONE HUNDRED FORTY KM
      DATA GRID_PATT(91),GRID_PATT(92) /332,496/
      DATA GRID_PATT(93),GRID_PATT(94) /332,501/
C*   ONE HUNDRED SIXTY KM
      DATA GRID_PATT(95),GRID_PATT(96) /372,496/
      DATA GRID_PATT(97),GRID_PATT(98) /372,501/
C*   ONE HUNDRED EIGHTY KM
      DATA GRID_PATT(99),GRID_PATT(100) /412,496/
      DATA GRID_PATT(101),GRID_PATT(102) /412,501/
C*   TWO HUNDRED KM HASH
      DATA GRID_PATT(103),GRID_PATT(104) /452,496/
      DATA GRID_PATT(105),GRID_PATT(106) /452,501/
C*   TWO HUNDRED TWENTY KM HASH
      DATA GRID_PATT(107),GRID_PATT(108) /492,496/
      DATA GRID_PATT(109),GRID_PATT(110) /492,501/
C*
C*********************************************************************
C*
C*
C*      GRID TEXT ANNOTATION DEFINED.
C*
C*
C*        TEXT ( )
C*
C*********************************************************************
C*
C*
      INTEGER*2  TEXT( TEXTSIZE )
C********************************************************
C*    DEFINE THE ANNOTATION FOR THE VERTICAL TICK MARKS.
C********************************************************
C*     TEXT CODE:
      DATA TEXT(1) /1/
C*     LENGTH OF BLOCK HEREAFTER:
      DATA TEXT(2) /12/
C*    "ALT KFT"
      DATA TEXT(3),TEXT(4) /1,19/
      DATA TEXT(5),TEXT(6),TEXT(7),TEXT(8) /Z414C,Z5420,Z4B46,Z5420/
C******************************
C*  TEXT CODE,LENGTH OF BLOCK:
      DATA TEXT(9),TEXT(10) /1,10/
C*  "K M S"
      DATA TEXT(11),TEXT(12) /8,502/
      DATA TEXT(13),TEXT(14),TEXT(15) /Z4B20,Z4D20,Z5320/
C******************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(16),TEXT(17) /1,6/
C*  "60"
      DATA TEXT(18),TEXT(19) /31,92/
      DATA TEXT(20) /Z3630/
C******************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(21),TEXT(22) /1,6/
C*  "50"
      DATA TEXT(23),TEXT(24) /31,159/
      DATA TEXT(25)  /Z3530/
C*****************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(26),TEXT(27) /1,6/
C*  "40"
      DATA TEXT(28),TEXT(29) /31,227/
      DATA TEXT(30)  /Z3430/
C*****************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(31),TEXT(32) /1,6/
C*  "30"
      DATA TEXT(33),TEXT(34) /31,294/
      DATA TEXT(35) /Z3330/
C*****************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(36),TEXT(37) /1,6/
C*  "20"
      DATA TEXT(38),TEXT(39) /31,362/
      DATA TEXT(40)  /Z3230/
C*****************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(41),TEXT(42) /1,6/
C*  "10"
      DATA TEXT(43),TEXT(44) /31,429/
      DATA TEXT(45) /Z3130/
C********************************************************
C*  DEFINE THE ANNOTATION FOR THE HORIZONTAL TICK MARKS.
C********************************************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(46),TEXT(47) /1,6/
C*  "20"
      DATA TEXT(48),TEXT(49) /85,502/
      DATA TEXT(50)  /Z3230/
C*****************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(51),TEXT(52) /1,6/
C*  "40"
      DATA TEXT(53),TEXT(54) /125,502/
      DATA TEXT(55)  /Z3430/
C******************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(56),TEXT(57) /1,6/
C*  "60"
      DATA TEXT(58),TEXT(59) /165,502/
      DATA TEXT(60)  /Z3630/
C******************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(61),TEXT(62)  /1,6/
C*  "80"
      DATA TEXT(63),TEXT(64) /205,502/
      DATA TEXT(65)  /Z3830/
C******************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(66),TEXT(67) /1,8/
C*  "100"
      DATA TEXT(68),TEXT(69) /242,502/
      DATA TEXT(70),TEXT(71)  /Z3130,Z3020/
C******************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(72),TEXT(73) /1,8/
C*  "140"
      DATA TEXT(74),TEXT(75) /322,502/
      DATA TEXT(76),TEXT(77)  /Z3134,Z3020/
C******************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(78),TEXT(79) /1,8/
C*  "180"
      DATA TEXT(80),TEXT(81)  /402,502/
      DATA TEXT(82),TEXT(83) /Z3138,Z3020/
C******************************
C*  TEXT CODE, LENGTH OF BLOCK
      DATA TEXT(84),TEXT(85) /1,8/
C*  "220"
      DATA TEXT(86),TEXT(87) /482,502/
      DATA TEXT(88),TEXT(89) /Z3232,Z3020/
C******************************
C*
C*
C*
/*
**A307AI
*VERSION:1
C*********************************************************************
C*          ***** DIMENSIONAL PARAMETERS *****                       *
C* MAXSVWXA       Maximum number of products possible                *
C* NUMCTL_PARM    Number of parameters in the product control table  *
C*          ******* OFFSET PARAMETERS ********                       *
C* MSGID          Offset to message code                             *
C* BUFID          Offset to buffer ID.                               *
C* CAZ            Offset to window center azimuth                    *
C* CRNG           Offset to product center range                     *
C* OPOINT         Offset to product buffer pointer                   *
C* GENDATE        Offset to product generation date                  *
C* GENTIME        Offset to product generation time                  *
C* MAX_AZ         Offset to the product max. azimuth                 *
C* MIN_AZ         Offset to product min. azimuth ang.                *
C* MAX_RG         Offset to the product max. range                   *
C* MIN_RG         Offset to product minimum range                    *
C* ELFLAG         Offset to flag indication first radial falls within*
C*                a window                                           *
C* INCZDEG        Offset to flag indicating that the product includes*
C*                zero degrees azm.                                  *
C* COLIX          Offset - product color lookup table                *
C* MINBIN         Offset to min. bin to process in the input radial  *
C* MAXBIN         Offset to max. bin # to process on the input radial*
C* MAXI2_IN_OBUF  Offset to # of I*2 words allocated for the product *
C* NEED           Offset to flag denoting the current radial is      *
C*                needed by the product                              *
C* STATUS         Offset to product status flag                      *
C* NUM_RADLS      Offset to the number of radials in the product     *
C* BUFCNT         Offset to # of I*2 words in product                *
C* NRLEB          Number of RLE bytes for this radial                *
C* MAXNEG         Offset to maximum neg. vel. found in the product   *
C* MAXPOS         Offset to maximum pos. vel. found in the product   *
C* ALT_CAT        Offset to alert category                           *
C*                                                                   *
C*          ******* ARRAY DEFINATIONS ********                       *
C* TBUFCTL(NUMCTL_PARM,MAXSVWXA)   Product control table             *
C*          ***** SUB-ARRAY DEFINATIONS ******                       *
C* TBUFCTL(MSGID,MAXSVWXA)         Product codes (43, 44, OR 45)     *
C* TBUFCTL(BUFID,MAXSVWXA)         Buffer data types (60, 61, OR 62) *
C* TBUFCTL(CAZ,MAXSVWXA)           Window center azimuths (deg*10)   *
C* TBUFCTL(CRNG,MAXSVWXA)          Product center ranges (km*8)      *
C* TBUFCTL(OPOINT,MAXSVWXA)        Pointers to product buffers       *
C* TBUFCTL(GENDATE,MAXSVWXA)       Product generation dates          *
C* TBUFCTL(GENTIME,MAXSVWXA)       Product generation times          *
C* TBUFCTL(MAX_AZ,MAXSVWXA)        Window max. azimuth angles (deg)  *
C* TBUFCTL(MIN_AZ,MAXSVWXA)        Window min. azmiuth angles (deg)  *
C* TBUFCTL(MAX_RG,MAXSVWXA)        Window maximum ranges (km)        *
C* TBUFCTL(MIN_RG,MAXSVWXA)        Window minimum ranges (km)        *
C* TBUFCTL(ELFLAG,MAXSVWXA)        Flag indicating first radial of   *
C*                                 elevation scan lies within the    *
C*                                 window, (YES or NO).              *
C* TBUFCTL(INCZDEG,MAXSVWXA)       Flag indicating that the window   *
C*                                 includes zero deg. azimuth angle  *
C*                                 (YES or NO).                      *
C* TBUFCTL(COLIX,MAXSVWXA)         Product color lookup table index  *
C* TBUFCTL(MINBIN,MAXSVWXA)        Bin numbers of minimum slant range*
C* TBUFCTL(MAXBIN,MAXSVWXA)        Bin numbers or maximum slant range*
C* TBUFCTL(MAXI2_IN_OBUF,MAXSVWXA) Number of I*2 words allocated for *
C*                                 each of the products              *
C* TBUFCTL(NEED,MAXSVWXA)          Flags indicating the product needs*
C*                                 the current radial (YES or NO)    *
C* TBUFCTL(STATUS,MAXSVWXA)        Product status flags,  (NOBUF,    *
C*                                 WINDOW_OPEN,RELEASED)             *
C* TBUFCTL(NUM_RADLS,MAXSVWXA)     Number of radials in the product  *
C* TBUFCTL(BUFCNT,MAXSVWXA)        Number of 2-byte words in the     *
C*                                 product buffers (running total)   *
C* TBUFCTL(NRLEB,MAXSVWXA)        Number of bytes run-length encoded *
C*                                 in the product (running total)    *
C* TBUFCTL(MAXNEG,MAXSVWXA)        Maximum neg. velocity found in the*
C*                                 in the window (running maximum)   *
C* TBUFCTL(MAXPOS,MAXSVWXA)        Maximum pos. velocity found in the*
C*                                 in the window (running maximum)   *
C* TBUFCTL(ALT_CAT,MAXSVWXA)       Alert category                    *
C*                                                                   *
C*          ******* OTHER PARAMETERS *********                       *
C* INITVAL        Initial product control table values               *
C* TBUFCTL        Product control table                              *
C* INITCNT        I*2 position in product buffer to begin appending  *
C*                run-length-encoded radials                         *
C* YES            Flag indicating "true" conclusion                  *
C* NO             Flag indicating "false" condition                  *
C* RELEASED       Flag meaning, product buffer released              *
C* WINDOW_OPEN    Flag meaning, product window open                  *
C* NOBUF          Flag meaning, there is no buffer                   *
C* ZERO_VEL       Zero radial velocity value (scaled) ,129           *
C*          *******  OTHER VARIABLES *********                       *
C* TNUMPROD       Total number of products set up in TBUFCTL         *
C* ONUMPROD       Outstanding number of products (unfinished)        *
C* VOLNUM         Volume scan number                                 *
C* ELEV_INDX      Elevation scan index of current scan               *
C* ELEV_ANG       Requested elev. angle, deg*10                      *
C* DOP_RESO       Doppler velocity resolution flag (1 or 2)          *
C*********************************************************************
      INTEGER*4 TNUMPROD,ONUMPROD,MAXSVWXA,NUMCTL_PARM
      INTEGER*4 RELEASED,WINDOW_OPEN,NOBUF,YES,NO,ZERO_VEL
      INTEGER*4 BUFID,BUFCNT,OPOINT,CAZ,CRNG,MSGID,MAX_AZ
      INTEGER*4 MAX_RG,MIN_AZ,MIN_RG,INCZDEG,ELFLAG,NEED,MAXI2_IN_OBUF
      INTEGER*4 NRLEB,STATUS,NUM_RADLS,GENDATE,GENTIME,MINBIN,MAXBIN
      INTEGER*4 MAXNEG,MAXPOS,INITVAL,COLIX,INITCNT,ALT_CAT
      INTEGER*4 VOLNUM,ELEV_INDX,ELEV_ANG,DOP_RESO
      INTEGER*4 REFLEC,VELCTY,SPWDTH
      PARAMETER (NUMCTL_PARM=25,MAXSVWXA=30,YES=1,NO=0,RELEASED=-1)
      PARAMETER (WINDOW_OPEN=1,NOBUF=0,ZERO_VEL=129)
      PARAMETER (MSGID=1,BUFID=2,CAZ=3,CRNG=4,OPOINT=5)
      PARAMETER (GENDATE=6,GENTIME=7,MAX_AZ=8,MIN_AZ=9,MAX_RG=10)
      PARAMETER (MIN_RG=11,ELFLAG=12,INCZDEG=13,COLIX=14,MINBIN=15)
      PARAMETER (MAXBIN=16,MAXI2_IN_OBUF=17)
      PARAMETER (NEED=18,STATUS=19,NUM_RADLS=20,BUFCNT=21,NRLEB=22)
      PARAMETER (MAXNEG=23,MAXPOS=24,ALT_CAT=25)
      PARAMETER (INITVAL=0)
      PARAMETER (REFLEC=43,VELCTY=44,SPWDTH=45)
      INTEGER*4 TBUFCTL(NUMCTL_PARM,MAXSVWXA)
      COMMON /A307AI/VOLNUM,ELEV_INDX,ELEV_ANG,DOP_RESO,TNUMPROD,
     $               ONUMPROD,TBUFCTL,INITCNT
/*
**A30799
*VERSION:1
C**************************************************
C*                A30799                          *
C*    BEAM SPLITTING DATA DECLARATIONS            *
C**************************************************
C
C  ANG_SFT = THE RADIAL DIFFERANCE (+/-) OF EACH SUBBEAM
C            THAT THE BIN IS SPLIT INTO.
C  ZMNBIN  = THE FIRST RADIAL BIN IN EACH BEAM SPLITTING
C            RANGE ZONE
C  ZMXBIN  = THE LAST RADIAL BIN IN EACH BEAM SPLITTING
C            RANGE ZONE
C
      DATA  RSS/1.0/
      DATA HALFGRID/0.5/, HALFBIN/0.5/, ROUNDUP/1.0/
      DATA ZMNBIN/Z1MNBIN,  Z2MNBIN,  Z3MNBIN,  Z4MNBIN,
     $            Z5MNBIN,  Z6MNBIN,  Z7MNBIN,  Z8MNBIN,
     $            Z9MNBIN,  Z10MNBIN, Z11MNBIN, Z12MNBIN,
     $            Z13MNBIN, Z14MNBIN, Z15MNBIN, Z16MNBIN/
      DATA ZMXBIN/Z1MXBIN,  Z2MXBIN,  Z3MXBIN,  Z4MXBIN,
     $            Z5MXBIN,  Z6MXBIN,  Z7MXBIN,  Z8MXBIN,
     $            Z9MXBIN,  Z10MXBIN, Z11MXBIN, Z12MXBIN,
     $            Z13MXBIN, Z14MXBIN, Z15MXBIN, Z16MXBIN/
      DATA NBEAMS/1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16/
      DATA (ANG_SFT(1,I),I=1,16)/
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(2,I),I=1,16)/
     $  -0.436332E-02,+0.436332E-02,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(3,I),I=1,16)/
     $  -0.581776E-02,+0.000000E+00,+0.581776E-02,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(4,I),I=1,16)/
     $  -0.654499E-02,-0.218166E-02,+0.218166E-02,+0.654499E-02,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(5,I),I=1,16)/
     $  -0.698132E-02,-0.349066E-02,+0.000000E+00,+0.349066E-02,
     $  +0.698132E-02,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(6,I),I=1,16)/
     $  -0.727221E-02,-0.436332E-02,-0.145444E-02,+0.145444E-02,
     $  +0.436332E-02,+0.727221E-02,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(7,I),I=1,16)/
     $  -0.747998E-02,-0.498665E-02,-0.249333E-02,+0.000000E+00,
     $  +0.249333E-02,+0.498665E-02,+0.747998E-02,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(8,I),I=1,16)/
     $  -0.763582E-02,-0.545415E-02,-0.327249E-02,-0.109083E-02,
     $  +0.109083E-02,+0.327249E-02,+0.545415E-02,+0.763582E-02,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(9,I),I=1,16)/
     $  -0.775702E-02,-0.581776E-02,-0.387851E-02,-0.193925E-02,
     $  +0.000000E+00,+0.193925E-02,+0.387851E-02,+0.581776E-02,
     $  +0.775702E-02,+0.000000E+00,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(10,I),I=1,16)/
     $  -0.785398E-02,-0.610866E-02,-0.436332E-02,-0.261799E-02,
     $  -0.872663E-03,+0.872667E-03,+0.261800E-02,+0.436333E-02,
     $  +0.610866E-02,+0.785398E-02,+0.000000E+00,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(11,I),I=1,16)/
     $  -0.793331E-02,-0.634665E-02,-0.475999E-02,-0.317333E-02,
     $  -0.158666E-02,-0.208059E-08,+0.158666E-02,+0.317332E-02,
     $  +0.475999E-02,+0.634665E-02,+0.793331E-02,+0.000000E+00,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(12,I),I=1,16)/
     $  -0.799942E-02,-0.654499E-02,-0.509055E-02,-0.363610E-02,
     $  -0.218166E-02,-0.727222E-03,+0.727219E-03,+0.218166E-02,
     $  +0.363610E-02,+0.509054E-02,+0.654498E-02,+0.799942E-02,
     $  +0.000000E+00,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(13,I),I=1,16)/
     $  -0.805536E-02,-0.671281E-02,-0.537024E-02,-0.402768E-02,
     $  -0.268512E-02,-0.134256E-02,+0.000000E+00,+0.134256E-02,
     $  +0.268512E-02,+0.402768E-02,+0.537024E-02,+0.671281E-02,
     $  +0.805536E-02,+0.000000E+00,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(14,I),I=1,16)/
     $  -0.810332E-02,-0.685665E-02,-0.560999E-02,-0.436332E-02,
     $  -0.311666E-02,-0.186999E-02,-0.623330E-03,+0.623335E-03,
     $  +0.187000E-02,+0.311666E-02,+0.436333E-02,+0.560999E-02,
     $  +0.685666E-02,+0.810332E-02,+0.000000E+00,+0.000000E+00/
      DATA (ANG_SFT(15,I),I=1,16)/
     $  -0.814487E-02,-0.698132E-02,-0.581776E-02,-0.465421E-02,
     $  -0.349066E-02,-0.232711E-02,-0.116355E-02,+0.000000E+00,
     $  +0.116355E-02,+0.232711E-02,+0.349066E-02,+0.465421E-02,
     $  +0.581776E-02,+0.698132E-02,+0.814487E-02,+0.000000E+00/
      DATA (ANG_SFT(16,I),I=1,16)/
     $  -0.818123E-02,-0.709040E-02,-0.599957E-02,-0.490874E-02,
     $  -0.381791E-02,-0.272708E-02,-0.163625E-02,-0.545415E-03,
     $  +0.545415E-03,+0.163625E-02,+0.272708E-02,+0.381791E-02,
     $  +0.490874E-02,+0.599957E-02,+0.709040E-02,+0.818123E-02/
/*
**A3074C
*VERSION: 0000
C
C   TABLE OF HOLES' COORDINATES FOR COMPOSITE REFLECTIVITY
C       CARTESIAN GRID, 1X1 RESOLUTION, 230KM RANGE
C   THERE ARE 1547 HOLES IN THE GRID
C   TABLES ARE SET UP ONE EACH FOR I & J COORDINATES,
C
      INTEGER NHOLES, HNDEX
      PARAMETER (NHOLES=1547)
      INTEGER*2 HI(NHOLES),HJ(NHOLES)
C
      COMMON /HOLES/ HI,HJ
C
/*
**A3074H
*VERSION: 0000
C
C   I-COORDINATES OF HOLES
C
      DATA(HI(HNDEX),HNDEX=    1,  75)
     +/ 133,332,119,346,186,279,110,182,283,355,128,337,117,348,126,
     +  339,104,361,117,348,103,110,355,362,130,335, 93,115,350,372,
     +   87,378, 92, 97,114,351,368,373, 96,111,354,369,130,335, 80,
     +   86,379,385, 79,121,344,386,116,127,338,349,113,352, 80, 86,
     +   92,100,137,328,365,373,379,385, 79,124,341,386,104,128,337 /
C
      DATA(HI(HNDEX),HNDEX=   76, 150)
     +/ 361,101,364, 73,392, 72, 82, 88, 97,115,129,336,350,368,377,
     +  383,393, 71, 81,384,394,101,119,124,141,324,341,346,364, 76,
     +   89,376,389, 75, 94,123,342,371,390, 77, 84, 90,102,110,355,
     +  363,375,381,388,129,336, 64, 68, 79,109,356,386,397,401, 63,
     +   67,100,135,330,365,398,402, 73, 87,128,145,320,337,378,392 /
C
      DATA(HI(HNDEX),HNDEX=  151, 223)
     +/  72,107,358,393,127,331,338, 77, 84, 94,114,351,371,381,388,
     +  108,357, 63, 67,113,118,347,352,398,402, 57, 81,166,299,384,
     +  408, 72,106,122,343,359,393, 75, 89,105,129,336,360,376,390,
     +   77, 95,113,352,370,388, 94,118,142,158,162,303,307,323,347,
     +  371, 58, 67, 93,106,167,298,359,372,398,407, 56, 92 /
C
      DATA(HI(HNDEX),HNDEX=  224, 298)
     +/ 105,137,156,309,328,360,373,409, 76, 84, 91,134,165,300,331,
     +  374,381,389, 90, 97,368,375, 60, 96,369,405, 58, 72, 95, 98,
     +  108,114,125,340,351,357,367,370,393,407, 97,107,146,319,358,
     +  368,139,160,305,326, 67, 80,126,156,309,339,385,398, 70, 74,
     +  101,114,143,322,351,364,391,395, 77,110,119,147,318,346,355 /
C
      DATA(HI(HNDEX),HNDEX=  299, 375)
     +/ 388, 51,414, 49, 69,105,153,312,360,396,416, 89, 93,104,139,
     +  148,326,361,372,376, 88, 92,138,143,322,327,373,377, 82, 98,
     +  160,305,367,383, 85, 97,108,137,142,147,318,323,328,357,368,
     +  380, 79,386, 39, 51, 77,102,119,141,153,312,324,346,363,388,
     +  414,426, 60,405,135,140,325,330, 66, 86, 90,113,352,375,379,
     +  399, 58 /
C
      DATA(HI(HNDEX),HNDEX=  376, 450)
     +/  84, 97,101,122,125,134,139,144,321,326,331,340,343,364,368,
     +  381,407, 56, 72,133,151,314,332,393,409, 65,120,345,400, 44,
     +   50,415,421, 86, 91,374,379, 36, 74,102,110,121,124,133,139,
     +  326,332,341,344,355,363,391,429,116,120,123,158,307,342,345,
     +  349, 64,119,122,143,322,343,346,401, 88,114,121,134,331,344 /
C
      DATA(HI(HNDEX),HNDEX=  451, 525)
     +/ 351,377, 76,120,133,139,186,279,326,332,345,389, 68, 74,119,
     +  126,339,346,391,397, 97,114,125,340,351,368, 48,100,124,341,
     +  365,417, 65, 83,160,305,382,400, 40, 69, 81,144,180,285,321,
     +  384,396,425, 72, 78, 89,376,387,393, 55, 62,130,335,403,410,
     +  145,320,138,327, 33,115,119,123,175,290,342,346,350,432, 83 /
C
      DATA(HI(HNDEX),HNDEX=  526, 600)
     +/  94,114,122,343,351,371,382, 80,112,166,299,353,385,141,153,
     +  312,324, 67, 93,108,137,328,357,372,398,106,132,333,359, 99,
     +  105,114,119,123,171,294,342,346,351,360,366,112,144,147,318,
     +  321,353, 74,110,136,329,355,391, 91,108,142,160,305,323,357,
     +  374,101,106,121,344,359,364,114,128,140,150,315,325,337,351 /
C
      DATA(HI(HNDEX),HNDEX=  601, 673)
     +/  81,131,182,283,334,384, 98,367,102,108,140,325,357,363,150,
     +  159,306,315,149,152,164,174,291,301,313,316,144,148,151,314,
     +  317,321,115,150,160,168,297,305,315,350,149,165,300,316,104,
     +  110,136,329,355,361,154,311, 93,100,162,303,365,372, 91,120,
     +  345,374,148,159,306,317, 99,107,127,142,151,168,201 /
C
      DATA(HI(HNDEX),HNDEX=  674, 748)
     +/ 264,297,314,323,338,358,366, 91,156,309,374,149,193,272,316,
     +   94,152,313,371, 87,135,166,178,287,299,330,378, 92,190,275,
     +  373,151,160,305,314,176,289,139,171,197,268,294,326,149,178,
     +  287,316,133,177,288,332,169,176,289,296,175,290,166,174,178,
     +  287,291,299,180,285,128,179,286,337, 39,145,187,278,320,426 /
C
      DATA(HI(HNDEX),HNDEX=  749, 823)
     +/ 183,282, 38,123,342,427,182,283,167,298,164,301,171,294,160,
     +  305,214,251,205,260,207,258,212,253,204,261,204,261,212,253,
     +  207,258,205,260,214,251,160,305,171,294,164,301,167,298,182,
     +  283, 38,123,342,427,183,282, 39,145,187,278,320,426,128,179,
     +  286,337,180,285,166,174,178,287,291,299,175,290,169,176,289 /
C
      DATA(HI(HNDEX),HNDEX=  824, 898)
     +/ 296,133,177,288,332,149,178,287,316,139,171,197,268,294,326,
     +  176,289,151,160,305,314, 92,190,275,373, 87,135,166,178,287,
     +  299,330,378, 94,152,313,371,149,193,272,316, 91,156,309,374,
     +   99,107,127,142,151,168,201,264,297,314,323,338,358,366,148,
     +  159,306,317, 91,120,345,374, 93,100,162,303,365,372,154,311 /
C
      DATA(HI(HNDEX),HNDEX=  899, 971)
     +/ 104,110,136,329,355,361,149,165,300,316,115,150,160,168,297,
     +  305,315,350,144,148,151,314,317,321,149,152,164,174,291,301,
     +  313,316,150,159,306,315,102,108,140,325,357,363, 98,367, 81,
     +  131,182,283,334,384,114,128,140,150,315,325,337,351,101,106,
     +  121,344,359,364, 91,108,142,160,305,323,357,374, 74 /
C
      DATA(HI(HNDEX),HNDEX=  972,1045)
     +/ 110,136,329,355,391,112,144,147,318,321,353, 99,105,114,119,
     +  123,171,294,342,346,351,360,366,106,132,333,359, 67, 93,108,
     +  137,328,357,372,398,141,153,312,324, 80,112,166,299,353,385,
     +   94,114,122,343,351,371,382, 33,115,119,123,175,290,342,346,
     +  350,432,138,327,145,320, 55, 62,130,335,403,410, 72, 78 /
C
      DATA(HI(HNDEX),HNDEX= 1046,1120)
     +/  89,376,387,393, 40, 69, 81,144,180,285,321,384,396,425, 65,
     +   83,160,305,382,400, 48,100,124,341,365,417, 97,114,125,340,
     +  351,368, 68, 74,119,126,339,346,391,397, 76,120,133,139,186,
     +  279,326,332,345,389, 88,114,121,134,331,344,351,377, 64,119,
     +  122,143,322,343,346,401,116,120,123,158,307,342,345,349, 36 /
C
      DATA(HI(HNDEX),HNDEX= 1121,1195)
     +/  74,102,110,121,124,133,139,326,332,341,344,355,363,391,429,
     +   86, 91,374,379, 44, 50,415,421, 65,120,345,400, 56, 72,133,
     +  151,314,332,393,409, 58, 84, 97,101,122,125,134,139,144,321,
     +  326,331,340,343,364,368,381,407, 66, 86, 90,113,352,375,379,
     +  399,135,140,325,330, 60,405, 39, 51, 77,102,119,141,153,312 /
C
      DATA(HI(HNDEX),HNDEX= 1196,1270)
     +/ 324,346,363,388,414,426, 79,386, 85, 97,108,137,142,147,318,
     +  323,328,357,368,380, 82, 98,160,305,367,383, 88, 92,138,143,
     +  322,327,373,377, 89, 93,104,139,326,361,372,376, 49, 69,105,
     +  153,312,360,396,416, 51,414, 77,110,119,147,318,346,355,388,
     +   70, 74,101,114,143,322,351,364,391,395, 67, 80,126,156,309 /
C
      DATA(HI(HNDEX),HNDEX= 1271,1345)
     +/ 339,385,398,139,160,305,326, 97,107,146,319,358,368, 58, 72,
     +   95, 98,108,114,125,340,351,357,367,370,393,407, 60, 96,369,
     +  405, 90, 97,368,375, 76, 84, 91,134,165,300,331,374,381,389,
     +   56, 92,105,137,156,309,328,360,373,409, 58, 67, 93,106,167,
     +  298,359,372,398,407, 94,118,142,158,162,303,307,323,347,371 /
C
      DATA(HI(HNDEX),HNDEX= 1346,1419)
     +/  77, 95,113,352,370,388, 75, 89,105,129,336,360,376,390, 72,
     +  106,122,343,359,393, 57, 81,166,299,384,408, 63, 67,113,118,
     +  347,352,398,402,108,357, 77, 84, 94,114,351,371,381,388,127,
     +  134,331,338, 72,107,358,393, 73, 87,128,145,320,337,378,392,
     +   63, 67,100,135,330,365,398,402, 64, 68, 79,109,356,386 /
C
      DATA(HI(HNDEX),HNDEX= 1420,1494)
     +/ 397,401,129,336, 77, 84, 90,102,110,355,363,375,381,388, 75,
     +   94,123,342,371,390, 76, 89,376,389,101,119,124,141,324,341,
     +  346,364, 71, 81,384,394, 72, 82, 88, 97,115,129,336,350,368,
     +  377,383,393, 73,392,101,364,104,128,337,361, 79,124,341,386,
     +   80, 86, 92,100,137,328,365,373,379,385,113,352,116,127,338 /
C
      DATA(HI(HNDEX),HNDEX= 1495,1547)
     +/ 349, 79,121,344,386, 80, 86,379,385,130,335, 96,111,354,369,
     +   92, 97,114,351,368,373, 87,378, 93,115,350,372,130,335,103,
     +  110,355,362,117,348,104,361,126,339,117,348,128,337,182,283,
     +  110,355,186,279,119,346,133,332  /
C
C   J-COORDINATES OF HOLES
C
      DATA(HJ(HNDEX),HNDEX=    1,  75)
     +/  33, 33, 36, 36, 38, 38, 39, 39, 39, 39, 40, 40, 44, 44, 48,
     +   48, 49, 49, 50, 50, 51, 51, 51, 51, 55, 55, 56, 56, 56, 56,
     +   57, 57, 58, 58, 58, 58, 58, 58, 60, 60, 60, 60, 62, 62, 63,
     +   63, 63, 63, 64, 64, 64, 64, 65, 65, 65, 65, 66, 66, 67, 67,
     +   67, 67, 67, 67, 67, 67, 67, 67, 68, 68, 68, 68, 69, 69, 69 /
C
      DATA(HJ(HNDEX),HNDEX=   76, 150)
     +/  69, 70, 70, 71, 71, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
     +   72, 72, 73, 73, 73, 73, 74, 74, 74, 74, 74, 74, 74, 74, 75,
     +   75, 75, 75, 76, 76, 76, 76, 76, 76, 77, 77, 77, 77, 77, 77,
     +   77, 77, 77, 77, 78, 78, 79, 79, 79, 79, 79, 79, 79, 79, 80,
     +   80, 80, 80, 80, 80, 80, 80, 81, 81, 81, 81, 81, 81, 81, 81 /
C
      DATA(HJ(HNDEX),HNDEX=  151, 223)
     +/  82, 82, 82, 82, 83, 83, 83, 84, 84, 84, 84, 84, 84, 84, 84,
     +   85, 85, 86, 86, 86, 86, 86, 86, 86, 86, 87, 87, 87, 87, 87,
     +   87, 88, 88, 88, 88, 88, 88, 89, 89, 89, 89, 89, 89, 89, 89,
     +   90, 90, 90, 90, 90, 90, 91, 91, 91, 91, 91, 91, 91, 91, 91,
     +   91, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 93, 93 /
C
      DATA(HJ(HNDEX),HNDEX=  224, 298)
     +/  93, 93, 93, 93, 93, 93, 93, 93, 94, 94, 94, 94, 94, 94, 94,
     +   94, 94, 94, 95, 95, 95, 95, 96, 96, 96, 96, 97, 97, 97, 97,
     +   97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98,
     +   98, 99, 99, 99, 99,100,100,100,100,100,100,100,100,101,101,
     +  101,101,101,101,101,101,101,101,102,102,102,102,102,102,102 /
C
      DATA(HJ(HNDEX),HNDEX=  299, 375)
     +/ 102,103,103,104,104,104,104,104,104,104,104,105,105,105,105,
     +  105,105,105,105,105,106,106,106,106,106,106,106,106,107,107,
     +  107,107,107,107,108,108,108,108,108,108,108,108,108,108,108,
     +  108,109,109,110,110,110,110,110,110,110,110,110,110,110,110,
     +  110,110,111,111,112,112,112,112,113,113,113,113,113,113,113,
     +  113,114 /
C
      DATA(HJ(HNDEX),HNDEX=  376, 450)
     +/ 114,114,114,114,114,114,114,114,114,114,114,114,114,114,114,
     +  114,114,115,115,115,115,115,115,115,115,116,116,116,116,117,
     +  117,117,117,118,118,118,118,119,119,119,119,119,119,119,119,
     +  119,119,119,119,119,119,119,119,120,120,120,120,120,120,120,
     +  120,121,121,121,121,121,121,121,121,122,122,122,122,122,122 /
C
      DATA(HJ(HNDEX),HNDEX=  451, 525)
     +/ 122,122,123,123,123,123,123,123,123,123,123,123,124,124,124,
     +  124,124,124,124,124,125,125,125,125,125,125,126,126,126,126,
     +  126,126,127,127,127,127,127,127,128,128,128,128,128,128,128,
     +  128,128,128,129,129,129,129,129,129,130,130,130,130,130,130,
     +  131,131,132,132,133,133,133,133,133,133,133,133,133,133,134 /
C
      DATA(HJ(HNDEX),HNDEX=  526, 600)
     +/ 134,134,134,134,134,134,134,135,135,135,135,135,135,136,136,
     +  136,136,137,137,137,137,137,137,137,137,138,138,138,138,139,
     +  139,139,139,139,139,139,139,139,139,139,139,140,140,140,140,
     +  140,140,141,141,141,141,141,141,142,142,142,142,142,142,142,
     +  142,143,143,143,143,143,143,144,144,144,144,144,144,144,144 /
C
      DATA(HJ(HNDEX),HNDEX=  601, 673)
     +/ 145,145,145,145,145,145,146,146,147,147,147,147,147,147,148,
     +  148,148,148,149,149,149,149,149,149,149,149,150,150,150,150,
     +  150,150,151,151,151,151,151,151,151,151,152,152,152,152,153,
     +  153,153,153,153,153,154,154,156,156,156,156,156,156,158,158,
     +  158,158,159,159,159,159,160,160,160,160,160,160,160 /
C
      DATA(HJ(HNDEX),HNDEX=  674, 748)
     +/ 160,160,160,160,160,160,160,162,162,162,162,164,164,164,164,
     +  165,165,165,165,166,166,166,166,166,166,166,166,167,167,167,
     +  167,168,168,168,168,169,169,171,171,171,171,171,171,174,174,
     +  174,174,175,175,175,175,176,176,176,176,177,177,178,178,178,
     +  178,178,178,179,179,180,180,180,180,182,182,182,182,182,182 /
C
      DATA(HJ(HNDEX),HNDEX=  749, 823)
     +/ 183,183,186,186,186,186,187,187,190,190,193,193,197,197,201,
     +  201,204,204,205,205,207,207,212,212,214,214,251,251,253,253,
     +  258,258,260,260,261,261,264,264,268,268,272,272,275,275,278,
     +  278,279,279,279,279,282,282,283,283,283,283,283,283,285,285,
     +  285,285,286,286,287,287,287,287,287,287,288,288,289,289,289 /
C
      DATA(HJ(HNDEX),HNDEX=  824, 898)
     +/ 289,290,290,290,290,291,291,291,291,294,294,294,294,294,294,
     +  296,296,297,297,297,297,298,298,298,298,299,299,299,299,299,
     +  299,299,299,300,300,300,300,301,301,301,301,303,303,303,303,
     +  305,305,305,305,305,305,305,305,305,305,305,305,305,305,306,
     +  306,306,306,307,307,307,307,309,309,309,309,309,309,311,311 /
C
      DATA(HJ(HNDEX),HNDEX=  899, 971)
     +/ 312,312,312,312,312,312,313,313,313,313,314,314,314,314,314,
     +  314,314,314,315,315,315,315,315,315,316,316,316,316,316,316,
     +  316,316,317,317,317,317,318,318,318,318,318,318,319,319,320,
     +  320,320,320,320,320,321,321,321,321,321,321,321,321,322,322,
     +  322,322,322,322,323,323,323,323,323,323,323,323,324 /
C
      DATA(HJ(HNDEX),HNDEX=  972,1045)
     +/ 324,324,324,324,324,325,325,325,325,325,325,326,326,326,326,
     +  326,326,326,326,326,326,326,326,327,327,327,327,328,328,328,
     +  328,328,328,328,328,329,329,329,329,330,330,330,330,330,330,
     +  331,331,331,331,331,331,331,332,332,332,332,332,332,332,332,
     +  332,332,333,333,334,334,335,335,335,335,335,335,336,336 /
C
      DATA(HJ(HNDEX),HNDEX= 1046,1120)
     +/ 336,336,336,336,337,337,337,337,337,337,337,337,337,337,338,
     +  338,338,338,338,338,339,339,339,339,339,339,340,340,340,340,
     +  340,340,341,341,341,341,341,341,341,341,342,342,342,342,342,
     +  342,342,342,342,342,343,343,343,343,343,343,343,343,344,344,
     +  344,344,344,344,344,344,345,345,345,345,345,345,345,345,346 /
C
      DATA(HJ(HNDEX),HNDEX= 1121,1195)
     +/ 346,346,346,346,346,346,346,346,346,346,346,346,346,346,346,
     +  347,347,347,347,348,348,348,348,349,349,349,349,350,350,350,
     +  350,350,350,350,350,351,351,351,351,351,351,351,351,351,351,
     +  351,351,351,351,351,351,351,351,352,352,352,352,352,352,352,
     +  352,353,353,353,353,354,354,355,355,355,355,355,355,355,355 /
C
      DATA(HJ(HNDEX),HNDEX= 1196,1270)
     +/ 355,355,355,355,355,355,356,356,357,357,357,357,357,357,357,
     +  357,357,357,357,357,358,358,358,358,358,358,359,359,359,359,
     +  359,359,359,359,360,360,360,360,360,360,360,360,361,361,361,
     +  361,361,361,361,361,362,362,363,363,363,363,363,363,363,363,
     +  364,364,364,364,364,364,364,364,364,364,365,365,365,365,365 /
C
      DATA(HJ(HNDEX),HNDEX= 1271,1345)
     +/ 365,365,365,366,366,366,366,367,367,367,367,367,367,368,368,
     +  368,368,368,368,368,368,368,368,368,368,368,368,369,369,369,
     +  369,370,370,370,370,371,371,371,371,371,371,371,371,371,371,
     +  372,372,372,372,372,372,372,372,372,372,373,373,373,373,373,
     +  373,373,373,373,373,374,374,374,374,374,374,374,374,374,374 /
C
      DATA(HJ(HNDEX),HNDEX= 1346,1419)
     +/ 375,375,375,375,375,375,376,376,376,376,376,376,376,376,377,
     +  377,377,377,377,377,378,378,378,378,378,378,379,379,379,379,
     +  379,379,379,379,380,380,381,381,381,381,381,381,381,381,382,
     +  382,382,382,383,383,383,383,384,384,384,384,384,384,384,384,
     +  385,385,385,385,385,385,385,385,386,386,386,386,386,386 /
C
      DATA(HJ(HNDEX),HNDEX= 1420,1494)
     +/ 386,386,387,387,388,388,388,388,388,388,388,388,388,388,389,
     +  389,389,389,389,389,390,390,390,390,391,391,391,391,391,391,
     +  391,391,392,392,392,392,393,393,393,393,393,393,393,393,393,
     +  393,393,393,394,394,395,395,396,396,396,396,397,397,397,397,
     +  398,398,398,398,398,398,398,398,398,398,399,399,400,400,400 /
C
      DATA(HJ(HNDEX),HNDEX= 1495,1547)
     +/ 400,401,401,401,401,402,402,402,402,403,403,405,405,405,405,
     +  407,407,407,407,407,407,408,408,409,409,409,409,410,410,414,
     +  414,414,414,415,415,416,416,417,417,421,421,425,425,426,426,
     +  426,426,427,427,429,429,432,432  /
/*
**A307CD
*VERSION: 0000
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                              C
C         COMMON DATA A307CD                                   C
C              WRITTEN: FEB. 19, 2000                          C
C              USE: LOCAL COMMON DATA DECLARATION FILE FOR     C
C                   PRIMARY MODULE NO.13 OF CPC NO. 7,         C
C                   256-LEVEL BASE REFLECTIVITY MAP.           C
C                                                              C
C**************************************************************C
C**************************************************************C
C                                                              C
C              DESCRIPTION OF COMMON DATA ELEMENTS:            C
C                                                              C
C                   ENDELCUT - END OF ELEVATION CUT FLAG       C
C                   RADCOUNT - RADIAL COUNT                    C
C                   MXPOS - KEEPS TRACK OF MAXIMUM POSITIVE    C
C                          DATA LEVEL IN INTERNAL BIASED UNITS C
C                   VC  - SCALE FACTOR WITH THE VERTICAL COR-  C
C                            RELATION CORRECTION FIGURED IN    C
C                            FOR PRODUCT CONTAINING 460 PIXELS C
C                            PER RADIAL                        C
C                   ELMEAS - ELEVATION ANGLE MEASUREMENT       C
C                               (AT LEVEL OFF POINT FOR THE    C
C                                RADAR ANTENNA)                C
C                   TIME - GENERATION TIME OF PRODUCT          C
C                   PROC_RAD - PROCESS RADIAL FLAG             C
C                                                              C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
      INTEGER BYTESPWD
C
      PARAMETER (BYTESPWD=2)
C
      INTEGER*2 TIME(BYTESPWD),RADCOUNT,ELMEAS,VC,MXPOS
C
      LOGICAL ENDELCUT,PROC_RAD
C
      COMMON /A307CD/ ENDELCUT,PROC_RAD,RADCOUNT,MXPOS,VC,
     1                ELMEAS,TIME
/*
**A307CE
*VERSION: 0000
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                              C
C         COMMON DATA A307CE                                   C
C              WRITTEN: DEC. 29, 1999                          C
C              USE: LOCAL COMMON DATA DECLARATION FILE FOR     C
C                   PRIMARY MODULE NO.14 OF CPC NO. 7,         C
C                   256-LEVEL BASE VELOCITY MAP.               C
C                                                              C
C**************************************************************C
C**************************************************************C
C                                                              C
C              DESCRIPTION OF COMMON DATA ELEMENTS:            C
C                                                              C
C                   ENDELCUT - END OF ELEVATION CUT FLAG       C
C                   RADCOUNT - RADIAL COUNT                    C
C                   MAXNEG   - KEEPS TRACK OF MAXIMUM NEGATIVE C
C                                DATA LEVEL OUT TO CHOSEN KM   C
C                                RANGE                         C
C                   MAXPOS   - KEEPS TRACK OF MAXIMUM POSITIVE C
C                                DATA LEVEL OUT TO CHOSEN KM   C
C                                RANGE                         C
C                   VC  - SCALE FACTOR WITH THE VERTICAL COR-  C
C                            RELATION CORRECTION FIGURED IN    C
C                            FOR PRODUCT  CONTAINING 115 PIX-  C
C                            ELS PER RADIAL                    C
C                   ELMEAS - ELEVATION ANGLE MEASUREMENT       C
C                               (AT LEVEL OFF POINT FOR THE    C
C                                RADAR ANTENNA)                C
C                   TIME - GENERATION TIME OF PRODUCT          C
C                   PROC_RAD - PROCESS RADIAL FLAG             C
C                                                              C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
      INTEGER BYTESPWD
C
      PARAMETER (BYTESPWD=2)
C
      INTEGER*2 TIME(BYTESPWD),RADCOUNT,ELMEAS,VC,
     *          MXNEG,MXPOS
C
      LOGICAL ENDELCUT,PROC_RAD
C
      COMMON /A307CE/ ENDELCUT,PROC_RAD,RADCOUNT,MXNEG,MXPOS,VC,
     1                ELMEAS,TIME
/*
