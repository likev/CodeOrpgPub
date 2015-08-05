C
C RCS info
C $Author: steves $
C $Locker:  $
C $Date: 2012/10/25 14:49:57 $
C $Id: a309adpt.inc,v 1.67 2012/10/25 14:49:57 steves Exp $
C $Revision: 1.67 $
C $State: Exp $
C
*.********************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A309ADPT.INC
*.
*.  INCLUDE FILE VERSION: 0014
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#          PROGRAMMER         NOTES
*.  ---------    --   -----         ----------------   --------------
*.  06 01 90     00   90698         P. STALEY
*.  07 11 90     01   90819         G. JUBINSKI
*.  08 03 90     01   90869         STEVE ANDERSON
*.  09 07 90     01   90952         PAUL JENDROWSKI
*.  01 12 91     02   91380         STEVE ANDERSON      **A3CD70C1
*.  02 25 91     02   90921         STAN MANOSKI        **A3CD70CA
*.  03 25 91     02   90921         STAN MANOSKI        **A3CD70PA
*.  04 11 91     02   90956         DENNIS MILLER       **A3CD70C2
*.  06 13 91     02   92083         DAVID ETRIS
*.  05 21 92     03   92610         D. WALKER           **A3CD70C9
*.  08 05 92     04   NA92-14002    VALERIE HOLDEN      **A3CD70PA
*.  10 26 92     05   NA92-28001    D. WALKER           **A3CD70C2
*.  06 25 93     06   NA93-17501    BRADLEY SUTKER      **A3CD70CA
*.  04/28/94     07  92119-001A00   STEVE SMITH         **A3CD70C2
*.  04/28/94     07  NW-94-007-10   STEVE SMITH         **A3CD70C2
*.  04/28/94     07  NW-94-048-06   JOSEPH WHEELER      **A3CD70CA
*.  04/29/94     07  NW-94-084-01   CHERYL STEPHENSON   **A3CD70C2
*.  04/29/94     07  NW-94-084-02   CHERYL STEPHENSON   **A3CD70C2
*.  10/25/94     07  NW-94-119-02   STEVE SMITH
*.  03/31/95     08  NA94-11252     STEVE SMITH         **A3CD70C9
*.  04/04/95     08  NA94-35301     C. STEPHENSON       **A3CD70CA
*.  05/05/95     08  NA94-26301     STEVE SMITH         **A3CD70C3
*.  05/05/95     08  NA94-26301     STEVE SMITH         **A3CD70C2
*.  05/05/95     08  NA94-26302     STEVE SMITH         **A3CD70C3
*.  05/05/95     08  NA94-26302     STEVE SMITH         **A3CD70C2
*.  05/25/95     08  NA95-13201     CHERYL STEPHENSON   **A3CD70C2
*.  03/20/95     08  NA94-33204     ROBERT RIERSON      **A3CD70CU
*.  08/25/95     08  NA94-08462     ELLIOTT FRIEND      **A4CD70C4
*.  11/13/95     08  NA94-33201     RICH FULTON         **A4CD70C4
*.  05/02/97     09  NA94-34702     DALE HARRIS         **A3CD70PA
*.  06/13/97     09  NA96-18401     CHERYL STEPHENSON   **A3CD70C2
*.  06/16/97     09  NA96-17903     STEVE SMITH         **A3CD70C2
*.  06/17/97     09  NA95-33502     CHERYL STEPHENSON   **A3CD70CA
*.  08/14/97     09  NA96-31001     STEVE SMITH         **A3CD70CU
*.  08/25/97     09  NA97-25502     CHERYL STEPHENSON   PRODSEL
*.  01/31/02     10  NA01-27501     Dennis A. Miller    **A3CD70C2, **A3CD70C4
*.  07/31/02     11  NA02-11605     C. Pham, D. Miller  **A3CD70C2
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_VIL_ECHO_TOPS
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_PRECIP_DETECT
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_HYDROMET_PREP
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_HYDROMET_RATE
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_HYDROMET_ACC
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_HYDROMET_ADJ
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_LAYER_REF
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_STORM_CELL_SEG
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_STORM_CELL_COMP
*.  12/08/03     12  NA03-30002     CHRIS CALVERT      **A3CD70_STORM_CELL_TRACK
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_HAIL
*.  12/08/03     12  NA03-30002     CHRIS CALVERT      **A3CD70_CELL_PROD
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_VAD
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_MESOCYCLONE
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_TDA
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_COMBINED_SHEAR
*.  12/08/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_MPDA
*.  12/09/03     12  NA03-30002     CHRIS CALVERT       **A3CD70_RADAZVD
*.  01/29/04     13  NA03-30002     CHRIS CALVERT       **A3CD70_DEA_NAMES
*.  01/13/05     14  NA04-33201     D. Miller; C. Pham  **A3CD70_HYDROMET_RATE
*.
*.********************************************************************
**A3CD70PA
*VERSION:0
C******************************************************************
C    SITE ADAPTATION DATA
C
C    PARAMETER DESCRIPTIONS:
C
C    NAME      FORMAT          RANGE         DESCRIPTION
C    ----      ------          -----         -----------
C    NBMAX     INTEGER         47            MAXIMUM # OF NB LINES
C
C    NBTAELEM  INTEGER         8             NUMBER OF NB TABLE
C                                             ELEMENTS
C
C    NBTOMAX   INTEGER         5             NUMBER OF NB TIMEOUTS
C
C    WBTOMAX   INTEGER         5             NUMBER OF WB TIMEOUTS
C
C    WBMAX     INTEGER         3             MAXIMUM # OF WB LINES
C
C    WBTAELEM  INTEGER         3             NUMBER OF WB TABLE
C                                             ELEMENTS
C
C    FAS_STATIONS_MAX INTEGER  4             MAXIMUM # OF FAS STATIONS
C
C    DISKMAX   INTEGER         3             MAXIMUM NUMBER OF DISKS
C
C    TAPEMAX  INTEGER          2             MAXIMUM NUMBER OF TAPES
C
C    MAXPROD   INTEGER         90            MAXIMUM NUMBER OF PRODUCT
C                                             CODES
C
C    MAXMAPR   INTEGER         4             MAXIMUM NUMBER OF MAPS
C                                             PER PRODUCT
C
C    MAXWXMOD  INTEGER         2 TO N        MAXIMUM NUMBER OF WX
C                                             MODES
C
C    FIRSTMAP  INTEGER         128           NUMBER OF FIRST MAP
C
C    LASTMAP   INTEGER         211           NUMBER OF LAST MAP
C
C...SINBTAB Indices
C    SIN_LINE  INTEGER         1             COL OF LINE NUMBER
C
C    SIN_BAUD  INTEGER         2             COL OF BAUD RATE
C
C    SIN_CLASS INTEGER         3             COL OF LINE CLASS
C
C    SIN_TYPE  INTEGER         4             COL OF LINE TYPE
C
C    SIN_MNEM  INTEGER         5             COL OF LINE MNEMONIC
C
C    SIN_DIST                  6             COL OF LINE DIST. METHOD
C
C    SIN_TIME                  7             COL OF LINE TIME LIMIT
C
C    SIN_FIRSTCLASS            1             ROW OF 1ST CLASS 1
C
C    SIN_LASTCLASS1            41            ROW OF LAST CLASS 1
C
C
C...SIWBTAB Indices
C
C    SIW_MNEM  INTEGER         1             COL OF LINE MNEMONIC
C
C    SIW_CLASS INTEGER         2             COL OF LINE CLASS
C
C    SIW_TYPE  INTEGER         3             COL OF LINE TYPE
C      Values are: DIRECT_CONNECT, MICRO_WAVE, FIBER_OPTIC
C
C
C...SIWBTO Indices
C
C    SIW_TOCON INTEGER         1             COL OF CONNECT TIMEOUT
C
C
C*********************************************************************
C
C  Required for OS RPG compilation
C  Adaptation data array dimensions
       INTEGER*4 NBMAX
       PARAMETER (NBMAX    = 47)
       INTEGER*4 NBTAELEM
       PARAMETER (NBTAELEM = 8)
       INTEGER*4 NBTOMAX
       PARAMETER (NBTOMAX   = 5)
       INTEGER*4 WBTOMAX
       PARAMETER (WBTOMAX  = 5)
       INTEGER*4 WBMAX
       PARAMETER (WBMAX    = 3)
       INTEGER*4 WBTAELEM
       PARAMETER (WBTAELEM = 3)
       INTEGER*4 FAS_STATIONS_MAX
       PARAMETER (FAS_STATIONS_MAX = 4)
       INTEGER*4 DISKMAX
       PARAMETER (DISKMAX  = 3)
       INTEGER*4 TAPEMAX
       PARAMETER (TAPEMAX  = 2)
       INTEGER*4 MAXPROD
       PARAMETER (MAXPROD  = 90)
       INTEGER*4 MAXMAPPR
       PARAMETER (MAXMAPPR = 6)
       INTEGER*4 MAX_APUPS
       PARAMETER (MAX_APUPS=40)
       INTEGER*4 MAXWXMOD
       PARAMETER (MAXWXMOD=2)
       INTEGER*4 MAX_CLASS1_USERS
       PARAMETER(MAX_CLASS1_USERS=41)
C
C
C   Indices for SINBTAB, SIWBTAB, and SIWBTO
       INTEGER LASTMAP
       INTEGER FIRSTMAP
       INTEGER SIN_LINE
       INTEGER SIN_BAUD
       INTEGER SIN_CLASS
       INTEGER SIN_TYPE
       INTEGER SIN_FIRSTCLASS1
       INTEGER SIN_LASTCLASS1
       INTEGER SIN_MNEM
       INTEGER SIN_DIST
       INTEGER SIN_TIME
       INTEGER SIN_COMMSOPT
       INTEGER SIW_MNEM
       INTEGER SIW_CLASS
       INTEGER SIW_TYPE
C
       INTEGER MICRO_WAVE, FIBER_OPTIC, DIRECT_CONNECT
C
       INTEGER SIW_TOCON

       PARAMETER (FIRSTMAP   = 128)
       PARAMETER (LASTMAP    = 211)
       PARAMETER (SIN_LINE   = 1)
       PARAMETER (SIN_BAUD   = 2)
       PARAMETER (SIN_CLASS  = 3)
       PARAMETER (SIN_TYPE   = 4)
       PARAMETER (SIN_MNEM   = 5)
       PARAMETER (SIN_FIRSTCLASS1 = 1)
       PARAMETER (SIN_LASTCLASS1  = 41)
       PARAMETER (SIN_DIST   = 6)
       PARAMETER (SIN_TIME   = 7)
       PARAMETER (SIN_COMMSOPT=8)
       PARAMETER (SIW_MNEM   = 1)
       PARAMETER (SIW_CLASS  = 2)
       PARAMETER (SIW_TYPE   = 3)
       PARAMETER (FIBER_OPTIC = 1)
       PARAMETER (MICRO_WAVE = 2)
       PARAMETER (DIRECT_CONNECT = 3)
       PARAMETER (SIW_TOCON  = 4)
      INTEGER     MAX_AUTH_USERS
      PARAMETER   (MAX_AUTH_USERS=425)
/*
**A3CD70P6
*VERSION:0
C******************************************************************
C    PRODUCT GENERATION CONTROL ADAPTATION DATA
C
C    PARAMETER DESCRIPTIONS:
C
C    NAME      FORMAT          RANGE         DESCRIPTION
C    ----      ------          -----         -----------
C    PGCTAMAX  INTEGER         100           NUMBER OF PRODUCT CODES
C
C    PGCTAELEM INTEGER         20            NUMBER OF PROD GENERATION
C                                             CONTROL TABLE ELEMENTS
C
C...ADPGCTABLE Indices
C
C    UCPRIO     I*4             1            ROW # FOR PRODUCT CODE
C
C    UCPELAZ    I*4             2            ROW # FOR EL/AZ PROD ID
C
C    UCPAZOFW   I*4             3            ROW # FOR P1 OF PROD ID
C
C    UCPRANOFW  I*4             4            ROW # FOR P2 OF PROD ID
C
C    UCPGENFRQ  I*4             5            ROW # FOR GENERATION FREQ
C
C    UCPARCHFRQ I*4             6            ROW # FOR ARCHIVE FREQ
C
C    UCPSTRFRQ  I*4             7            ROW # FOR STORAGE FREQ
C
C    UCPSTRRTIM I*4             8            ROW # FOR STORAGE TIME
C
C    UCPNAPD    I*4             9            ROW # FOR NA PUP DIST
C                                             FREQ
C
C    UCPPUED    I*4            10            ROW # FOR PUE DIST FREQ
C
C    UCPRFCD    I*4            11            ROW # FOR RFC DIST FREQ
C
C    UCPOTHRD   I*4            12            ROW # FOR OTHER USER DIST
C                                             FREQ
C    UCPPARM3   I*4            13            ROW # FOR P3 OF PROD ID
C    UCPPARM4   I*4            14            ROW # FOR P4 OF PROD ID
C*********************************************************************
C
C  Adaptation data array dimensions
       INTEGER*4 PGCTAMAX
       PARAMETER (PGCTAMAX = 100)
       INTEGER*4 PGCTAELM
       PARAMETER (PGCTAELM = 20)
C
C   Indices for ADPGCTABLE
       INTEGER UCPRIO
       INTEGER UCPELAZ
       INTEGER UCPAZOFW
       INTEGER UCPRANOFW
       INTEGER UCPGENFRQ
       INTEGER UCPARCHFRQ
       INTEGER UCPSTRFRQ
       INTEGER UCPSTRRTIM
       INTEGER UCPNAPD
       INTEGER UCPPUED
       INTEGER UCPRFCD
       INTEGER UCPOTHRD
       INTEGER UCPPARM3
       INTEGER UCPPARM4

       PARAMETER (UCPRIO     =  1)
       PARAMETER (UCPELAZ    =  2)
       PARAMETER (UCPAZOFW   =  3)
       PARAMETER (UCPRANOFW  =  4)
       PARAMETER (UCPGENFRQ  =  5)
       PARAMETER (UCPARCHFRQ =  6)
       PARAMETER (UCPSTRFRQ  =  7)
       PARAMETER (UCPSTRRTIM =  8)
       PARAMETER (UCPNAPD    =  9)
       PARAMETER (UCPPUED    = 10)
       PARAMETER (UCPRFCD    = 11)
       PARAMETER (UCPOTHRD   = 12)
       PARAMETER (UCPPARM3   = 13)
       PARAMETER (UCPPARM4   = 14)
C
/*
**A3CD70P8
*VERSION: 0
C******************************************************************
C    RDA CONTROL ADAPTATION DATA
C
C    PARAMETER DESCRIPTIONS:
C
C    NAME      FORMAT          RANGE         DESCRIPTION
C    ----      ------          -----         -----------
C    WXVCPMAX  INTEGER         21            MAXIMUM NUMBER OF VCP'S
C                                             PER WX MODE
C
C    COMPMAX   INTEGER         2             MAXIMUM NUMBER OF
C                                             COMPONENTS
C
C    WXMAX     INTEGER         2             MAXIMUM NUMBER OF WX
C                                             MODES
C
C    VCPMAX    INTEGER         20            MAXIMUM NUMBER OF VCP'S
C
C    VOL_ATTR  INTEGER         11            NUM VCP ATTRIBUTES
C
C    EL_ATTR   INTEGER         23            NUM ELEVATION ANG ATTRIBS
C
C    VCPSIZ    INTEGER                       MAXIMUM SIZE OF A VCP
C
C..VCP Indices
C....FOR EACH VCP
C    PFNHW     INTEGER         1             NUMBER OF HALF WORDS
C    PFPATTYP  INTEGER         2             PATTERN TYPE
C    PFPATNUM  INTEGER         3             PATTERN NUMBER
C    PFNELEV   INTEGER         4             NUMBER OF ELEVATION CUTS
C    PFCLMGRP  INTEGER         5             CLUTTER MAP GROUP NUMBER
C    PFDVRES   INTEGER         6             VELOCITY RESOLUTION
C    PFPULWID  INTEGER         7             PULSE WIDTH
C    SPARE#    INTEGER         8-12          SPARES
C....FOR EACH ELEVATION CUT
C    PFELANG   INTEGER          1            ELEVATION ANGLE
C    PFWAVTYP  INTEGER          2            WAVE TYPE
C    PFP2NUM   INTEGER          3            SURVEILLANCE PRF #
C    PFP2PULC  INTEGER          4            SURVEILLANCE PULSE COUNT
C    PFAZRATE  INTEGER          5            AZIMUTH RATE
C    PFS1AZAN  INTEGER         12            SEG 1 AZIMUTH ANGLE
C    PFS1P1NU  INTEGER         13            SEG 1 DOPPLER PRF #
C    PFS1P1PC  INTEGER         14            SEG 1 PULSE COUNT
C    PFS1THRP  INTEGER          6            SURV THR PARAMETER
C    PFS2AZAN  INTEGER         16            SEG 2 AZIMUTH ANGLE
C    PFS2P1NU  INTEGER         17            SEG 2 DOPPLER PRF #
C    PFS2P1PC  INTEGER         18            SEG 2 PULSE COUNT
C    PFS2THRP  INTEGER          7            DOP VEL THRSH PARAMETER
C    PFS3AZAN  INTEGER         20            SEG 3 AZIMUTH ANGLE
C    PFS3P1NU  INTEGER         21            SEG 3 DOPPLER PRF #
C    PFS3P1PC  INTEGER         22            SEG 3 PULSE COUNT
C    PFS3THRP  INTEGER          8            SPEC WDTH THRSH PARAMETER
C    SPARE#    INTEGER    9-11,15,19,23      SPARE
C
C** Dimensional Parameters
C   MAX_CLUT    INT*4   Maximum number of clutter zones
C   MAX_CZAT    INT*4   Maximum number of clutter zone attributes
C** Positional Parameters
C   CZ_BAZ      INT*4   Pointer to the beginning azimuth angle
C   CZ_EAZ      INT*4   Pointer to the ending azimuth angle
C   CZ_BRG      INT*4   Pointer to the beginning slant range
C   CZ_ERG      INT*4   Pointer to the ending slant range
C** Data Description
C    ALLOWABLE PRF'S AND PULSE COUNTS FOR VCP'S
C    VCPMAX    INT*2              20         Maximum number of VCP's
C    ECUTMAX   INT*2              25         Maximum num. of elevation
C                                            cuts per VCP
C    PRFMAX    INT*2               8         Maximum number of PRF's
C                                            per VCP
C    NAPRFVCP  INT*2              10         Number of parameters for
C                                            each VCP
C    NAPRFELV  INT*2               9         Number of parameters for
C                                            each elevation cut
C    MXALWPRF  INT*2             190         Number of I*2 words for
C                                            each VCP
C..VCP INDICIES
C....FOR EACH VCP
C    VCPNUMBR  INT*2               1         VCP number
C    NUMALPRF  INT*2               2         Number of allowable PRF's
C    ALOW1PRF  INT*2               3         1st allowable PRF number
C    ALOW2PRF  INT*2               4         2nd allowable PRF number
C    ALOW3PRF  INT*2               5         3rd allowable PRF number
C    ALOW4PRF  INT*2               6         4th allowable PRF number
C    ALOW5PRF  INT*2               7         5th allowable PRF number
C    ALOW6PRF  INT*2               8         6th allowable PRF number
C    ALOW7PRF  INT*2               9         7th allowable PRF number
C    ALOW8PRF  INT*2              10         8th allowable PRF number
C....FOR EACH ELEVATION CUT
C    NUM1PULS  INT*2               1         Pulse count for PRF # = 1
C    NUM2PULS  INT*2               2         Pulse count for PRF # = 2
C    NUM3PULS  INT*2               3         Pulse count for PRF # = 3
C    NUM4PULS  INT*2               4         Pulse count for PRF # = 4
C    NUM5PULS  INT*2               5         Pulse count for PRF # = 5
C    NUM6PULS  INT*2               6         Pulse count for PRF # = 6
C    NUM7PULS  INT*2               7         Pulse count for PRF # = 7
C    NUM8PULS  INT*2               8         Pulse count for PRF # = 8
C    DEFLTPRF  INT*2               9         Default PRF number
C
C....DATA ARRAYS
C    ALWBLPRF(MXALWPRF,VCPMAX)    INT*2   Data array containing the
C                                         allowable PRF, pulse counts,
C                                         and default PRF for all VCPs
C    PRFVALUE(PRFMAX)             R*4     Data array containing the
C                                         actual PRF values for all
C                                         PRF numbers
C*********************************************************************
C
C  Adaptation data array dimensions
       INTEGER*4 WXVCPMAX
       PARAMETER (WXVCPMAX = 21)
       INTEGER*4 COMPMAX
       PARAMETER (COMPMAX  = 2)
       INTEGER*4 WXMAX
       PARAMETER (WXMAX    = 2)
       INTEGER*4 VCPMAX
       PARAMETER (VCPMAX   = 20)
       INTEGER*4 ECUTMAX
       PARAMETER (ECUTMAX  = 25)
       INTEGER*4 VOL_ATTR
       PARAMETER (VOL_ATTR = 11)
       INTEGER*4 EL_ATTR
       PARAMETER (EL_ATTR  = 23)
       INTEGER*4 VCPSIZ
       PARAMETER (VCPSIZ=(ECUTMAX*EL_ATTR)+VOL_ATTR)
C
C  Limits for MPDA VCP numbers 
       INTEGER VCP_MIN_MPDA
       PARAMETER (VCP_MIN_MPDA = 100)
       INTEGER VCP_MAX_MPDA
       PARAMETER (VCP_MAX_MPDA = 199)
C
C  Indices for RDC_WHERE_DEFINED
       INTEGER VCP_RDA_DEFINED
       PARAMETER (VCP_RDA_DEFINED = 1)
       INTEGER VCP_RPG_DEFINED
       PARAMETER (VCP_RPG_DEFINED = 2)
C
C  Indices of fields in RDCVCPTA
       INTEGER*4 PFNVCP
       INTEGER*4 PFWXCLA
       INTEGER*4 PFWXCONV
C
       INTEGER*4 PFNHW
       INTEGER*4 PFPATTYP
       INTEGER*4 PFPATNUM
       INTEGER*4 PFNELEV
       INTEGER*4 PFCLMGRP
       INTEGER*4 PFDVRES
       INTEGER*4 PFPULWID
       INTEGER*4 PFELANG
       INTEGER*4 PFWAVTYP
       INTEGER*4 PFP2NUM
       INTEGER*4 PFP2PULC
       INTEGER*4 PFAZRATE
       INTEGER*4 PFS1AZAN
       INTEGER*4 PFS1P1NU
       INTEGER*4 PFS1P1PC
       INTEGER*4 PFS1THRP
       INTEGER*4 PFS2AZAN
       INTEGER*4 PFS2P1NU
       INTEGER*4 PFS2P1PC
       INTEGER*4 PFS2THRP
       INTEGER*4 PFS3AZAN
       INTEGER*4 PFS3P1NU
       INTEGER*4 PFS3P1PC
       INTEGER*4 PFS3THRP
       INTEGER*4 SPARE1
       INTEGER*4 SPARE2
       INTEGER*4 SPARE3
       INTEGER*4 SPARE4
       INTEGER*4 SPARE5
       INTEGER*4 SPARE6
       INTEGER*4 SPARE7
       INTEGER*4 SPARE8
       INTEGER*4 SPARE9
       INTEGER*4 SPAREA
       INTEGER*4 SPAREB
C
       PARAMETER (PFNVCP   = 1)
       PARAMETER (PFWXCLA  = 1)
       PARAMETER (PFWXCONV = 2)
C
       PARAMETER (PFNHW    = 1)
       PARAMETER (PFPATTYP = 2)
       PARAMETER (PFPATNUM = 3)
       PARAMETER (PFNELEV  = 4)
       PARAMETER (PFCLMGRP = 5)
       PARAMETER (PFDVRES  = 6)
       PARAMETER (PFPULWID = 6)
       PARAMETER (SPARE1   = 7)
       PARAMETER (SPARE2   = 8)
       PARAMETER (SPARE3   = 9)
       PARAMETER (SPARE4   =10)
       PARAMETER (SPARE5   =11)
       PARAMETER (PFELANG  = 1)
       PARAMETER (PFWAVTYP = 2)
       PARAMETER (PFP2NUM  = 3)
       PARAMETER (PFP2PULC = 4)
       PARAMETER (PFAZRATE = 5)
       PARAMETER (PFS1THRP = 6)
       PARAMETER (PFS2THRP = 7)
       PARAMETER (PFS3THRP = 8)
       PARAMETER (SPARE6   = 9)
       PARAMETER (SPARE7   =10)
       PARAMETER (SPARE8   =11)
       PARAMETER (PFS1AZAN =12)
       PARAMETER (PFS1P1NU =13)
       PARAMETER (PFS1P1PC =14)
       PARAMETER (SPARE9   =15)
       PARAMETER (PFS2AZAN =16)
       PARAMETER (PFS2P1NU =17)
       PARAMETER (SPAREA   =19)
       PARAMETER (PFS2P1PC =18)
       PARAMETER (PFS3AZAN =20)
       PARAMETER (PFS3P1NU =21)
       PARAMETER (PFS3P1PC =22)
       PARAMETER (SPAREB   =23)
C
C    ALLOWABLE PRF'S AND PULSE COUNTS FOR VCP'S
      INTEGER*4 NAPRFVCP,NAPRFELV,MXALWPRF,PRFMAX
      INTEGER*4 VCPNUMBR,NUMALPRF,ALOW1PRF,ALOW2PRF,ALOW3PRF
      INTEGER*4 ALOW4PRF,ALOW5PRF,ALOW6PRF,ALOW7PRF,ALOW8PRF
      INTEGER*4 NUM1PULS,NUM2PULS,NUM3PULS,NUM4PULS,NUM5PULS
      INTEGER*4 NUM6PULS,NUM7PULS,NUM8PULS,DEFLTPRF
      INTEGER*4 DELPRI_MAX
      PARAMETER (PRFMAX=8,NAPRFVCP=PRFMAX+2,NAPRFELV=PRFMAX+1)
      PARAMETER (MXALWPRF=NAPRFVCP+ECUTMAX*NAPRFELV)
      PARAMETER (VCPNUMBR=1,NUMALPRF=2,ALOW1PRF=3,ALOW2PRF=4)
      PARAMETER (ALOW3PRF=5,ALOW4PRF=6,ALOW5PRF=7,ALOW6PRF=8)
      PARAMETER (ALOW7PRF=9,ALOW8PRF=10)
      PARAMETER (NUM1PULS=1,NUM2PULS=2,NUM3PULS=3,NUM4PULS=4)
      PARAMETER (NUM5PULS=5,NUM6PULS=6,NUM7PULS=7,NUM8PULS=8)
      PARAMETER (DEFLTPRF=PRFMAX+1)
      PARAMETER (DELPRI_MAX=5)
C*
/*
C*************************************************************
C
C  This element contains the definitions of all RPG adaptation
C  data.  The adaptation data is grouped into a number of common
C  blocks containing related data values.
C  All common blocks are bound into a single
C  shared image (A3CD70).  The separate elements defined herein
C  are included at compile time by any module requiring access
C  to adaptation data.  Each RPG task "RESOLVES" against
C  A3CD70 at link time.
C
C  NOTE: Prior to version 12, all algorithm adaptation data was 
C        included under three large common blocks and one label
C        (A3CD70C2). Version 12 separated the algorithm adaptation
C        data into smaller common blocks with each block having
C        its own label.
C
C* ALGORITHM ADAPTATION DATA
C         A3CD70_VIL_ECHO_TOPS
C         A3CD70_HYDROMET_PREP
C         A3CD70_HYDROMET_RATE
C         A3CD70_HYDROMET_ACC
C         A3CD70_HYDROMET_ADJ
C         A3CD70_LAYER_REF
C         A3CD70_STORM_CELL_SEG
C         A3CD70_STORM_CELL_COMP
C         A3CD70_STORM_CELL_TRACK
C         A3CD70_HAIL
C         A3CD70_CELL_PROD
C         A3CD70_VAD
C         A3CD70_MESOCYCLONE
C         A3CD70_TDA
C         A3CD70_COMBINED_SHEAR
C         A3CD70_MPDA
C         A3CD70_RADAZVD
C* SITE ADAPTATION DATA
C         A3CD70CA-      THE SITE ADAPTATION DEFINITIONS
C        /SITEADP/ - the common block that contains the data
C* ALERT ADAPTATION DATA
C         A3CD70C3-      THE ALERTING ADAPTATION DEFINITIONS
C        /ALRTAD/ - the common block that contains the data
C* RDA CONTROL ADAPTATION DATA
C         A3CD70C8-      THE RDA CONTROL ADAPTATION DEFINITIONS
C        /RDACNT/ - the common block that contains the data
C* PRODUCT SELECTION CONTROL ADAPTATION DATA
C         A3CD70C7-      THE PROD SELECT CONTROL ADAPTATION DEFINITION
C        /PRODSEL/ - the common block that contains the data
C* PRODUCT COLOR TABLE ADAPTATION DATA
C         A3CD70C5-      THE PROD COLOR TABLES ADAPTATION DEFINITIONS
C        /COLRTBL/ - the common block that contains the data
C
C*************************************************************
C
C
C*************************************************************
C  START ALGORITHM ADAPTATION DATA
C*************************************************************
C
**A3CD70_VIL_ECHO_TOPS
*VERSION:6
C
C  NOTE: The VIL_ECHO_TOPS common block maps to the ORPG header file
C        vil_echo_tops.h
C
      REAL    EBMWT
      REAL    ENREF
      INTEGER MEVIL
C
      COMMON /VIL_ECHO_TOPS/ EBMWT, ENREF, MEVIL
/*
C
**A3CD70_VCP_SELECT
*VERSION:6
C
C  NOTE: The VCP_SELECT common block maps to the ORPG header file
C        mode_select.h
C
      INTEGER  NUMCATS
      PARAMETER (NUMCATS=8)
      INTEGER VCP_SELECT( NUMCATS )
C
      COMMON /VCP_SELECT/ VCP_SELECT
/*
C
**A3CD70_HYDROMET_PREP
*VERSION:6
C
C  NOTE: The HYDROMET_PREP common block maps to the ORPG header file
C        hydromet_prep.h
C
      REAL    BEAM_WIDTH
      REAL    BLOCKAGE_THRESHOLD
      INTEGER CLUTTER_THRESHOLD
      REAL    WEIGHT_THRESHOLD
      REAL    FULL_HYS_THRESHOLD
      REAL    LOW_DBZ_THRESHOLD
      REAL    RAIN_DBZ_THRESHOLD
      INTEGER RAIN_AREA_THRESHOLD
      INTEGER RAIN_TIME_THRESHOLD
      REAL    MINDBZRFL
      REAL    MAXDBZRFL
      INTEGER NUMZONE
      REAL    BEG_AZM1, BEG_AZM2, BEG_AZM3, BEG_AZM4, BEG_AZM5
      REAL    BEG_AZM6, BEG_AZM7, BEG_AZM8, BEG_AZM9, BEG_AZM10
      REAL    BEG_AZM11, BEG_AZM12, BEG_AZM13, BEG_AZM14
      REAL    BEG_AZM15, BEG_AZM16, BEG_AZM17, BEG_AZM18
      REAL    BEG_AZM19, BEG_AZM20
      REAL    END_AZM1, END_AZM2, END_AZM3, END_AZM4, END_AZM5
      REAL    END_AZM6, END_AZM7, END_AZM8, END_AZM9, END_AZM10
      REAL    END_AZM11, END_AZM12, END_AZM13, END_AZM14
      REAL    END_AZM15, END_AZM16, END_AZM17, END_AZM18
      REAL    END_AZM19, END_AZM20
      INTEGER BEG_RNG1, BEG_RNG2, BEG_RNG3, BEG_RNG4, BEG_RNG5
      INTEGER BEG_RNG6, BEG_RNG7, BEG_RNG8, BEG_RNG9, BEG_RNG10
      INTEGER BEG_RNG11, BEG_RNG12, BEG_RNG13, BEG_RNG14
      INTEGER BEG_RNG15, BEG_RNG16, BEG_RNG17, BEG_RNG18
      INTEGER BEG_RNG19, BEG_RNG20
      INTEGER END_RNG1, END_RNG2, END_RNG3, END_RNG4, END_RNG5
      INTEGER END_RNG6, END_RNG7, END_RNG8, END_RNG9, END_RNG10
      INTEGER END_RNG11, END_RNG12, END_RNG13, END_RNG14
      INTEGER END_RNG15, END_RNG16, END_RNG17, END_RNG18
      INTEGER END_RNG19, END_RNG20
      REAL    ELEV_AGL1, ELEV_AGL2, ELEV_AGL3, ELEV_AGL4
      REAL    ELEV_AGL5, ELEV_AGL6, ELEV_AGL7, ELEV_AGL8
      REAL    ELEV_AGL9, ELEV_AGL10, ELEV_AGL11, ELEV_AGL12
      REAL    ELEV_AGL13, ELEV_AGL14, ELEV_AGL15, ELEV_AGL16
      REAL    ELEV_AGL17, ELEV_AGL18, ELEV_AGL19, ELEV_AGL20
C
      COMMON /HYDROMET_PREP/ BEAM_WIDTH, BLOCKAGE_THRESHOLD,
     *                 CLUTTER_THRESHOLD,
     *                 WEIGHT_THRESHOLD, FULL_HYS_THRESHOLD,
     *                 LOW_DBZ_THRESHOLD, RAIN_DBZ_THRESHOLD,
     *                 RAIN_AREA_THRESHOLD, RAIN_TIME_THRESHOLD,
     *                 MINDBZRFL, MAXDBZRFL, NUMZONE,
     *    BEG_AZM1, END_AZM1, BEG_RNG1, END_RNG1,ELEV_AGL1,
     *    BEG_AZM2, END_AZM2, BEG_RNG2, END_RNG2,ELEV_AGL2,
     *    BEG_AZM3, END_AZM3, BEG_RNG3, END_RNG3,ELEV_AGL3,
     *    BEG_AZM4, END_AZM4, BEG_RNG4, END_RNG4,ELEV_AGL4,
     *    BEG_AZM5, END_AZM5, BEG_RNG5, END_RNG5,ELEV_AGL5,
     *    BEG_AZM6, END_AZM6, BEG_RNG6, END_RNG6,ELEV_AGL6,
     *    BEG_AZM7, END_AZM7, BEG_RNG7, END_RNG7,ELEV_AGL7,
     *    BEG_AZM8, END_AZM8, BEG_RNG8, END_RNG8,ELEV_AGL8,
     *    BEG_AZM9, END_AZM9, BEG_RNG9, END_RNG9,ELEV_AGL9,
     *    BEG_AZM10, END_AZM10, BEG_RNG10, END_RNG10,ELEV_AGL10,
     *    BEG_AZM11, END_AZM11, BEG_RNG11, END_RNG11,ELEV_AGL11,
     *    BEG_AZM12, END_AZM12, BEG_RNG12, END_RNG12,ELEV_AGL12,
     *    BEG_AZM13, END_AZM13, BEG_RNG13, END_RNG13,ELEV_AGL13,
     *    BEG_AZM14, END_AZM14, BEG_RNG14, END_RNG14,ELEV_AGL14,
     *    BEG_AZM15, END_AZM15, BEG_RNG15, END_RNG15,ELEV_AGL15,
     *    BEG_AZM16, END_AZM16, BEG_RNG16, END_RNG16,ELEV_AGL16,
     *    BEG_AZM17, END_AZM17, BEG_RNG17, END_RNG17,ELEV_AGL17,
     *    BEG_AZM18, END_AZM18, BEG_RNG18, END_RNG18,ELEV_AGL18,
     *    BEG_AZM19, END_AZM19, BEG_RNG19, END_RNG19,ELEV_AGL19,
     *    BEG_AZM20, END_AZM20, BEG_RNG20, END_RNG20,ELEV_AGL20
/*
C
**A3CD70_HYDROMET_RATE
*VERSION:7
C
C  NOTE: The HYDROMET_RATE common block maps to the ORPG header file
C        hydromet_rate.h
C
      INTEGER RNGCUTOFF
      REAL    RNGCOEF1
      REAL    RNGCOEF2
      REAL    RNGCOEF3
      REAL    MINPRATE
      REAL    MAXPRATE
      REAL    CZM
      REAL    CZP
C
      COMMON /HYDROMET_RATE/ RNGCUTOFF, RNGCOEF1, RNGCOEF2,
     *                       RNGCOEF3,  MINPRATE,  MAXPRATE,
     *                       CZM, CZP
/*
C
**A3CD70_HYDROMET_ACC
*VERSION:6
C
C  NOTE: The HYDROMET_ACC common block maps to the ORPG header file
C        hydromet_acc.h
C
      INTEGER TIMRESTRT
      INTEGER MAXTIMINT
      INTEGER MINTIMPD
      INTEGER THOURLI
      INTEGER ENTIMGAG
      INTEGER MAXPRDVAL
      INTEGER MAXHLYVAL
C
      COMMON /HYDROMET_ACC/ TIMRESTRT, MAXTIMINT, MINTIMPD,  THOURLI,
     *                 ENTIMGAG,  MAXPRDVAL, MAXHLYVAL
/*
C
**A3CD70_HYDROMET_ADJ
*VERSION:6
C
C  NOTE: The HYDROMET_ADJ common block maps to the ORPG header file
C        hydromet_adj.h
C
      INTEGER TIMBIEST
      REAL    RESETBI
      INTEGER LONGSTLAG
      INTEGER MINNPAIRS
      LOGICAL BIAS_FLAG
C
      COMMON /HYDROMET_ADJ/ TIMBIEST, MINNPAIRS, RESETBI, LONGSTLAG, 
     *                 BIAS_FLAG
/*
C
**A3CD70_LAYER_REF
*VERSION:6
C
C  NOTE: The LAYER_REF common block maps to the ORPG header file
C        layer_reflectivity.h
C
      REAL      ADMINDBZ
      INTEGER*4 ADALTLIM1
      INTEGER*4 ADALTLIM2
      INTEGER*4 ADDISLIM1
      INTEGER*4 ADDISLIM2
      INTEGER*4 ADDISLIM3
      REAL      ADELVLIM1
      REAL      ADELVLIM2
      REAL      ADREJVEL
      REAL      ADREJWID
      REAL      ADACPTVEL
      REAL      ADACPTWID
      INTEGER*4 ADRNGGATE
      REAL      ADDBZDIFF
      LOGICAL   ADIFEXTND
      LOGICAL   ADIFMEDIAN
      INTEGER*4 ADRNGMEDIAN
      INTEGER*4 ADCRMEDIAN
      REAL      ADPGDMEDIAN
C
      COMMON /LAYER_REF/ ADMINDBZ,    ADALTLIM1,  ADALTLIM2, ADDISLIM1,
     *                 ADDISLIM2,   ADDISLIM3,  ADELVLIM1, ADELVLIM2,
     *                 ADREJVEL,    ADREJWID,   ADACPTVEL, ADACPTWID,
     *                 ADIFEXTND,   ADRNGGATE,  ADDBZDIFF, ADIFMEDIAN,
     *                 ADRNGMEDIAN, ADCRMEDIAN, ADPGDMEDIAN
/*
C
**A3CD70_STORM_CELL_SEG
*VERSION:6
C
C  NOTE: The STORM_CELL_SEG common block maps to the ORPG header file
C        storm_cell_seg.h
C
      INTEGER*4 MAX_REF_THRESH
      PARAMETER (MAX_REF_THRESH = 7)

      INTEGER*4 REFLECTH(MAX_REF_THRESH)
      REAL      SEGLENTH(MAX_REF_THRESH)
      INTEGER*4 DRREFDFF
      INTEGER*4 NDROPBIN
      INTEGER*4 NREFLEVL
      INTEGER*4 SEGRNGMX
      INTEGER*4 RADSEGMX
      INTEGER*4 NUMSEGMX
      INTEGER*4 NUMAVGBN
      REAL      MWGTFCTR
      REAL      MULTFCTR
      REAL      MCOEFCTR
      REAL      FILKERSZ
      REAL      FRACTREQ
      INTEGER*4 FILTERON
C
      COMMON /STORM_CELL_SEG/ REFLECTH, SEGLENTH, DRREFDFF, NDROPBIN,
     *                 NREFLEVL, SEGRNGMX, RADSEGMX, NUMSEGMX,
     *                 NUMAVGBN, MWGTFCTR, MULTFCTR, MCOEFCTR,
     *                 FILKERSZ, FRACTREQ, FILTERON
/*
C
**A3CD70_STORM_CELL_COMP
*VERSION:6
C
C  NOTE: The STORM_CELL_COMP common block maps to the ORPG header file
C        storm_cell_comp.h
C
      INTEGER*4 MAX_RADIUS_THRESH
      PARAMETER (MAX_RADIUS_THRESH = 3)
C
      INTEGER*4 OVLAPADJ
      INTEGER*4 MXPOTCMP
      INTEGER*4 NUMCMPMX
      INTEGER*4 MXDETSTM
      INTEGER*4 NBRSEGMN
      INTEGER*4 NUMSTMMX
      INTEGER*4 STMVILMX
      REAL      CMPARETH(MAX_REF_THRESH)
      REAL      RADIUSTH(MAX_RADIUS_THRESH)
      REAL      DEPTHDEL
      REAL      HORIZDEL
      REAL      ELVMERGE
      REAL      HGTMERGE
      REAL      HRZMERGE
      REAL      AZMDLTHR
C
      COMMON /STORM_CELL_COMP/ OVLAPADJ, MXPOTCMP, NUMCMPMX, MXDETSTM,
     *                 NBRSEGMN, NUMSTMMX, STMVILMX, CMPARETH,
     *                 RADIUSTH, DEPTHDEL, HORIZDEL, ELVMERGE,
     *                 HGTMERGE, HRZMERGE, AZMDLTHR
/*
C
**A3CD70_STORM_CELL_TRACK
*VERSION:6
C
C  NOTE: The STORM_CELL_TRACK common block maps to the ORPG header file
C        storm_cell_track.h
C
         INTEGER*4 NPASTVOL
         INTEGER*4 NUMFRCST
         INTEGER*4 FRCINTVL
         INTEGER*4 ALLOWERR
         INTEGER*4 ERRINTVL
         INTEGER*4 DEFDIREC
         INTEGER*4 MAXVTIME
         REAL      DEFSPEED
         REAL      CORSPEED
         REAL      SPEEDMIN
C
      COMMON /STORM_CELL_TRACK/ NPASTVOL, NUMFRCST, FRCINTVL, ALLOWERR,
     *                 ERRINTVL, DEFDIREC, MAXVTIME, DEFSPEED,
     *                 CORSPEED, SPEEDMIN
/*
C
**A3CD70_HAIL
*VERSION:6
C
C  NOTE: The HAIL common block maps to the ORPG header file
C        hail_algorithm.h
C
      INTEGER*4 REF_W_LL
      INTEGER*4 REF_W_UL
      INTEGER*4 MNPOHREF
      REAL     HKECOEF1
      REAL     HKECOEF2
      REAL     HKECOEF3
      REAL     POSHCOEF
      INTEGER*4 POSHOFST
      INTEGER*4 MXHALRNG
      REAL     HAILSZCF
      REAL     HAILSZEX
      REAL     WT_SM_CF
      REAL     WT_SM_OF
      REAL     POHTDIF1
      REAL     POHTDIF2
      REAL     POHTDIF3
      REAL     POHTDIF4
      REAL     POHTDIF5
      REAL     POHTDIF6
      REAL     POHTDIF7
      REAL     POHTDIF8
      REAL     POHTDIF9
      REAL     POHTDIF0
      INTEGER*4 RCMPRBHL
      INTEGER*4 RCMPOSHL
      REAL     HT0
      REAL     HT20
      INTEGER*4 HADATE(3)
      INTEGER*4 HATIME(3)
C
      COMMON /HAIL/ REF_W_LL, REF_W_UL, MNPOHREF, HKECOEF1,
     *                 HKECOEF2, HKECOEF3, POSHCOEF, POSHOFST,
     *                 MXHALRNG, HAILSZCF, HAILSZEX, WT_SM_CF,
     *                 WT_SM_OF, POHTDIF1, POHTDIF2, POHTDIF3,
     *                 POHTDIF4, POHTDIF5, POHTDIF6, POHTDIF7,
     *                 POHTDIF8, POHTDIF9, POHTDIF0, RCMPRBHL,
     *                 RCMPOSHL,
     *                 HT0, HT20, HADATE, HATIME
/*
C
**A3CD70_CELL_PROD
*VERSION:6
C
C  NOTE: The CELL_PROD common block maps to the ORPG header file
C        cell_prod_params.h
C
      INTEGER*4 MAX_STI_ALPHA_CELLS
      INTEGER*4 MAX_SS_ALPHA_CELLS
      INTEGER*4 MAX_HI_ALPHA_CELLS
      INTEGER*4 MAX_STI_ATTR_CELLS
      INTEGER*4 MAX_CAT_ATTR_CELLS
      INTEGER*4 MAX_HI_ATTR_CELLS
C
      COMMON /CELL_PROD/ MAX_STI_ALPHA_CELLS, MAX_SS_ALPHA_CELLS,
     *                 MAX_HI_ALPHA_CELLS,  MAX_STI_ATTR_CELLS,
     *                 MAX_CAT_ATTR_CELLS,  MAX_HI_ATTR_CELLS
/*
C
**A3CD70_VAD
*VERSION:6
C
C  NOTE: The VEL_AZ_DISP common block maps to the ORPG header file vad.h
C
      REAL    EVEL
      INTEGER ENFT
      INTEGER NPTS_IN_SAMPLE
      REAL    EVR
      REAL    EBA
      REAL    EEA
      REAL    ESYM
C
      COMMON /VEL_AZ_DISP/ EVEL, ENFT, NPTS_IN_SAMPLE, EVR, EBA,
     *                 EEA, ESYM
/*
C
**A3CD70_MESOCYCLONE
*VERSION:6
C
C  NOTE: The MESOCYCLONE common block maps to the ORPG header file
C        mesocyclone.h
C
      INTEGER EMNF
      INTEGER EMNM
      REAL    EFH
      REAL    ERD
      REAL    ELM
      REAL    EHM
      REAL    ELS
      REAL    EHS
      REAL    EPVA
      REAL    EPVN
      REAL    EMXR
      REAL    EMNR
      REAL    EFMXR
      REAL    EFMNR
      REAL    ERNG
C
      COMMON /MESOCYCLONE/ EMNF,  EMNM,  EFH,  ERD,  ELM,  EHM,
     *                 ELS,   EHS,   EPVA, EPVN, EMXR, EMNR,
     *                 EFMXR, EFMNR, ERNG
/*
C
**A3CD70_TDA
*VERSION:6
C
C  NOTE: The TDA common block maps to the ORPG header file tda.h
C
      INTEGER  MINREFL
      INTEGER  MINPVDV
      INTEGER  MAXPVRNG
      REAL  MAXPVHT
      INTEGER  MAXNUMPV
      INTEGER  TH2DDV1
      INTEGER  TH2DDV2
      INTEGER  TH2DDV3
      INTEGER  TH2DDV4
      INTEGER  TH2DDV5
      INTEGER  TH2DDV6
      INTEGER  MIN1DP2D
      REAL  MAXPVRD
      REAL  MAXPVAD
      REAL  MAX2DAR
      REAL  THCR1
      REAL  THCR2
      INTEGER  THCRR
      INTEGER  MAXNUM2D
      INTEGER  MIN2DP3D
      REAL  MINTVSD
      INTEGER  MINLLDV
      INTEGER  MINMTDV
      INTEGER  MAXNUM3D
      INTEGER  MAXNUMTV
      INTEGER  MAXNUMET
      REAL  MINTVSBH
      REAL  MINTVSBE
      REAL  MINADVHT
      REAL  MAXTSTMD
C
      COMMON /TDA/ MINREFL, MINPVDV,  MAXPVRNG, MAXPVHT,  MAXNUMPV,
     *                TH2DDV1, TH2DDV2,  TH2DDV3,  TH2DDV4,  TH2DDV5,
     *                TH2DDV6, MIN1DP2D, MAXPVRD,  MAXPVAD,  MAX2DAR,
     *                THCR1,   THCR2,    THCRR,    MAXNUM2D, MIN2DP3D,
     *                MINTVSD, MINLLDV,  MINMTDV,  MAXNUM3D, MAXNUMTV,
     *                MAXNUMET,MINTVSBH, MINTVSBE, MINADVHT, MAXTSTMD
/*
C
**A3CD70_MPDA
*VERSION:0
C
C  NOTE: The MPDA common block maps to the ORPG header file mpda_parameters.h
C
      REAL GUI_MPDA_TOVER
      INTEGER*2 GUI_MIN_TRIP_FIX
      INTEGER*2 GUI_MAX_TRIP_FIX
C
      COMMON /MPDA/ GUI_MPDA_TOVER, GUI_MIN_TRIP_FIX, GUI_MAX_TRIP_FIX
/*
C
**A3CD70_RADAZVD
*VERSION:0
C
C  NOTE: The RADAZVD common block maps to the ORPG header file radazvd.h
C
      INTEGER  NUM9PT_LKBK,NUM9PT_LKAHD
      PARAMETER(NUM9PT_LKBK=4,NUM9PT_LKAHD=5)
C
      INTEGER*2 SHORT_PULSE, LONG_PULSE
      PARAMETER(SHORT_PULSE=2, LONG_PULSE=4)
C
      INTEGER  AD_NUM_BIN_FSTCHK_S, AD_NUM_BIN_FSTCHK_L
      INTEGER  AD_NUM_REP_LKAHD_S, AD_NUM_REP_LKAHD_L
      INTEGER  AD_NUM_REP_LKBK_S, AD_NUM_REP_LKBK_L
      INTEGER  AD_NUM_LKBK_S, AD_NUM_LKBK_L
      INTEGER  AD_NUM_LKFOR_S, AD_NUM_LKFOR_L
      INTEGER  AD_TH_CONBIN_REJ_S, AD_TH_CONBIN_REJ_L
      INTEGER  AD_TH_MAX_CONAZJMP_S, AD_TH_MAX_CONAZJMP_L
      INTEGER  AD_TH_MXMISS_S, AD_TH_MXMISS_L
      INTEGER  AD_NUM_REUNF_PRAZS_S, AD_NUM_REUNF_PRAZS_L
      INTEGER  AD_TH_MXBINS_JMP_S, AD_TH_MXBINS_JMP_L
      REAL     AD_TH_DIFF_UNFOLD_S, AD_TH_DIFF_UNFOLD_L
      INTEGER  AD_TH_BINS_LRG_AZJMP_S, AD_TH_BINS_LRG_AZJMP_L
      INTEGER  AD_NUM_REUNF_CAZS_S, AD_NUM_REUNF_CAZS_L
      REAL     AD_TH_VEL_JMP_FRAD_S, AD_TH_VEL_JMP_FRAD_L
      REAL     AD_TH_VEL_JMP_FAZ_S, AD_TH_VEL_JMP_FAZ_L
      REAL     AD_TH_SCL_STDEV_S, AD_TH_SCL_STDEV_L
      REAL     AD_TH_SCL_DIFF_UNFOLD_S, AD_TH_SCL_DIFF_UNFOLD_L
      INTEGER  AD_ENV_SOUND_TO_S, AD_ENV_SOUND_TO_L
      INTEGER  AD_NUM_INTRVL_CHK_S, AD_NUM_INTRVL_CHK_L
      LOGICAL  AD_USE_SOUNDINGS_FLAG_S, AD_USE_SOUNDINGS_FLAG_L
      LOGICAL  AD_REP_REJECTED_VEL_S, AD_REP_REJECTED_VEL_L
      LOGICAL  AD_DISABLE_RADAZ_DEALIASING
C
      COMMON /RADAZVD/ AD_NUM_REP_LKAHD_S
      COMMON /RADAZVD/ AD_NUM_REP_LKBK_S,AD_NUM_LKBK_S,AD_NUM_LKFOR_S
      COMMON /RADAZVD/ AD_TH_CONBIN_REJ_S,AD_TH_MXMISS_S
      COMMON /RADAZVD/ AD_NUM_REUNF_PRAZS_S, AD_NUM_REUNF_CAZS_S
      COMMON /RADAZVD/ AD_TH_DIFF_UNFOLD_S,AD_TH_MXBINS_JMP_S
      COMMON /RADAZVD/ AD_TH_VEL_JMP_FRAD_S,AD_TH_VEL_JMP_FAZ_S
      COMMON /RADAZVD/ AD_TH_SCL_STDEV_S,AD_TH_MAX_CONAZJMP_S
      COMMON /RADAZVD/ AD_TH_SCL_DIFF_UNFOLD_S,AD_TH_BINS_LRG_AZJMP_S
      COMMON /RADAZVD/ AD_NUM_BIN_FSTCHK_S,AD_ENV_SOUND_TO_S
      COMMON /RADAZVD/ AD_USE_SOUNDINGS_FLAG_S,AD_REP_REJECTED_VEL_S
      COMMON /RADAZVD/ AD_NUM_INTRVL_CHK_S
      COMMON /RADAZVD/ AD_NUM_REP_LKAHD_L,AD_NUM_REP_LKBK_L
      COMMON /RADAZVD/ AD_NUM_LKBK_L,AD_NUM_LKFOR_L,AD_TH_CONBIN_REJ_L
      COMMON /RADAZVD/ AD_TH_MXMISS_L,AD_NUM_REUNF_PRAZS_L
      COMMON /RADAZVD/ AD_NUM_REUNF_CAZS_L
      COMMON /RADAZVD/ AD_TH_DIFF_UNFOLD_L,AD_TH_MXBINS_JMP_L
      COMMON /RADAZVD/ AD_TH_VEL_JMP_FRAD_L,AD_TH_VEL_JMP_FAZ_L
      COMMON /RADAZVD/ AD_TH_SCL_STDEV_L,AD_TH_MAX_CONAZJMP_L
      COMMON /RADAZVD/ AD_TH_SCL_DIFF_UNFOLD_L,AD_TH_BINS_LRG_AZJMP_L
      COMMON /RADAZVD/ AD_NUM_BIN_FSTCHK_L,AD_ENV_SOUND_TO_L
      COMMON /RADAZVD/ AD_USE_SOUNDINGS_FLAG_L,AD_REP_REJECTED_VEL_L
      COMMON /RADAZVD/ AD_NUM_INTRVL_CHK_L,AD_DISABLE_RADAZ_DEALIASING
/*
C
C*************************************************************
C  END ALGORITHM ADAPTATION DATA
C*************************************************************
C*************************************************************
C  START SITE ADAPTATION DATA
C*************************************************************
C
/*
**A3CD70CA
*VERSION:4
$INCLUDE A309ADPT.INC/G, **A3CD70PA (NLIST)
C
C   SITE ADAPTATION DATA DEFINITIONS
C
C   NOTE: This SITEADP maps to ORPG header file siteadp.h
C
       INTEGER*4 SIRDALAT
       INTEGER*4 SIRDALON
       INTEGER*4 SIRDAHGT
       INTEGER*4 SIRPGID
       INTEGER*4 SIWXSTRT
       INTEGER*4 SIWXAVCP
       INTEGER*4 SIWXBVCP
       INTEGER*4 SIHMLOS
       INTEGER*4 SIHRMS
       INTEGER*4 SIHBDDS
       INTEGER*4 SIHARCH
       INTEGER*4 SIHORDA
       INTEGER*4 SIPCODE

       CHARACTER*8 SIRDANM
C
      COMMON /SITEADP/ SIRDALAT, SIRDALON, SIRDAHGT,
     *                 SIRPGID, SIWXSTRT, SIWXAVCP,
     *                 SIWXBVCP, SIHMLOS, SIHRMS,
     *                 SIHBDDS, SIHARCH, SIHORDA,
     *                 SIPCODE, SIRDANM
C
C*************************************************************
C  END SITE ADAPTATION DATA
C*************************************************************
C*************************************************************
C  START ALERT ADAPTATION DATA
C*************************************************************
C
/*
**A3CD70C3
*VERSION:3
C
C*************************************************************
C
C    PARAMETER DESCRIPTIONS:
C
C    NAME      FORMAT          RANGE         DESCRIPTION
C    ----      ------          -----         -----------
C    ALCATMAX  INTEGER         41            MAXIMUM NUMBER OF ALERT
C                                             CATEGORIES
C
C    ALTAELEM  INTEGER         10            NUMBER OF ALERT TABLE
C                                             ELEMENTS
C*********************************************************************
C
C  Adaptation data array dimensions
       INTEGER*4 ALCATMAX
       PARAMETER (ALCATMAX = 41)
       INTEGER*4 ALTAELEM
       PARAMETER (ALTAELEM = 10)
C
C  Alert Adaptation Data
       INTEGER*2 ALTTABLE(ALTAELEM,ALCATMAX)
C
      COMMON /ALRTAD/ ALTTABLE
C
C*************************************************************
C  END ALERT ADAPTATION DATA
C*************************************************************
C*************************************************************
C  START RDA CONTROL ADAPTATION DATA
C*************************************************************
C
/*
**A3CD70C8
*VERSION:2
$INCLUDE A309ADPT.INC/G, **A3CD70P8 (NLIST)
C  RDA Control Adaptation Data
       INTEGER*2 RDCWXVCP (WXVCPMAX,WXMAX)
       INTEGER*2 RDC_WHERE_DEFINED (VCPMAX, COMPMAX)
       INTEGER*2 RDCVCPTA (VCPSIZ,VCPMAX)
       INTEGER*2 RDCCON (ECUTMAX,VCPMAX)
       INTEGER*2 VCP_FLAGS (VCPMAX)
C
C  Run times (in seconds) of volume scan patterns...
      INTEGER*2 VCP_TIMES(VCPMAX)
C
      INTEGER*2 ALWBLPRF(MXALWPRF,VCPMAX)
      REAL PRFVALUE(PRFMAX)
      INTEGER UNAMBIGR(PRFMAX,DELPRI_MAX)
      INTEGER UNAMBIGR_SPRT(PRFMAX,DELPRI_MAX)
C
      INTEGER DELTA_PRI
C
      INTEGER RDACNT_FIRST,RDACNT_LAST
      COMMON /RDACNT/ RDACNT_FIRST
      COMMON /RDACNT/ RDCWXVCP
      COMMON /RDACNT/ RDC_WHERE_DEFINED
      COMMON /RDACNT/ RDCVCPTA
      COMMON /RDACNT/ RDCCON
      COMMON /RDACNT/ ALWBLPRF
      COMMON /RDACNT/ PRFVALUE
      COMMON /RDACNT/ VCP_TIMES
      COMMON /RDACNT/ VCP_FLAGS
      COMMON /RDACNT/ UNAMBIGR, UNAMBIGR_SPRT
      COMMON /RDACNT/ DELTA_PRI
      COMMON /RDACNT/ RDACNT_LAST
C
C*************************************************************
C  END RDA CONTROL ADAPTATION DATA
C*************************************************************
C*************************************************************
C  START PRODUCT SELECTION CONTROL ADAPTATION DATA
C*************************************************************
C
/*
**A3CD70C7
*VERSION:2
C
C  NOTE: The common block PRODSEL maps to the ORPG header file
C        prodsel.h
C
C  Product Selection Control Adaptation Data
       INTEGER MAXVHGTS, MAX_VAD_HTS, MAX_NUM_AMBRNG, MAX_VAD_ELVS
       PARAMETER  (MAXVHGTS=19, MAX_VAD_HTS = 30, MAX_NUM_AMBRNG=3)
       PARAMETER  (MAX_VAD_ELVS = 20)
       INTEGER*4 PSCPNL0D
       INTEGER*4 PSCPNL1D
       INTEGER*4 PSCPNL2D
       INTEGER*4 PSCPNL3D
       INTEGER*4 PSCPNRNG
       REAL*4    XOFF,YOFF,DELTATHE,BOXSIZ
       REAL      THRESH_GT230
       INTEGER   NCENTROIDS,VAD_RCM_HGTS(MAXVHGTS),VAD_AHTS(MAX_VAD_HTS)
C
      INTEGER PRODSEL_FIRST,PRODSEL_LAST
C
      COMMON /PRODSEL/ PRODSEL_FIRST
C
C   --Layer Product Parameters (product_parameters.h)
C
      COMMON /PRODSEL/ PSCPNL0D, PSCPNL1D, PSCPNL2D, PSCPNL3D,
     *                 PSCPNRNG
C
C   --RCM Product Parameters (product_parameters.h)
C
      COMMON /PRODSEL/ THRESH_GT230, NCENTROIDS, BOXSIZ, 
     *                 DELTATHE, XOFF, YOFF
C
C   --VAD/RCM Height Selection (product_parameters.h)
C
      COMMON/PRODSEL/ VAD_AHTS, VAD_RCM_HGTS
C
      COMMON /PRODSEL/ PRODSEL_LAST
C
C*************************************************************
C  END PRODUCT SELECTION CONTROL ADAPTATION DATA
C*************************************************************
C*************************************************************
C  START PRODUCT COLOR TABLE ADAPTATION DATA
C*************************************************************
/*
**A3CD70C5
*VERSION:0
C******************************************************************
C
C    PARAMETER DESCRIPTIONS:
C
C    NAME      FORMAT          RANGE         DESCRIPTION
C    ----      ------          -----         -----------
C    PCTTHSIZ  I*2             16            MAX # OF COLOR LEVELS
C
C    PCTTHMAX  I*2             34            MAX # OF THRESHOLD TABLES
C
C    PCTCDMIN  I*2             0             MIN RANGE FOR A GIVEN
C                                             COLOR DATA TABLE
C
C    PCTCDMAX  I*2             256           MAX VALUE FOR A GIVEN
C                                             COLOR DATA TABLE
C*********************************************************************
C
C  Adaptation data array dimensions
       INTEGER*4 PCTTHSIZ
       PARAMETER (PCTTHSIZ = 16)
       INTEGER*4 PCTTHMAX
       PARAMETER (PCTTHMAX = 34)
       INTEGER*4 PCTCDMIN
       PARAMETER (PCTCDMIN = 0)
       INTEGER*4 PCTCDMAX
       PARAMETER (PCTCDMAX = 256)
C
C  Product Data Levels Adaptation Data
       INTEGER*2 THRESH(PCTTHSIZ,PCTTHMAX)
       INTEGER*2 COLDAT(PCTCDMIN:PCTCDMAX,PCTTHMAX)
C
      INTEGER COLRTBL_FIRST,COLRTBL_LAST
      COMMON /COLRTBL/ COLRTBL_FIRST
      COMMON /COLRTBL/ THRESH,COLDAT
      COMMON /COLRTBL/ COLRTBL_LAST
C
C*************************************************************
C  END PRODUCT COLOR TABLE ADAPTATION DATA
C*************************************************************
C*************************************************************
C  START ENVIRON ADAPTATION DATA
C*************************************************************
/*
**A3CD70C4
*VERSION: 4
C
      INTEGER ENVIRON_FIRST, ENVIRON_LAST
C
C** HYDROMET FILE NAMES:
C**         OCC_FN = OCCULTATION ADAPTATION DATA
C**        SCTR_FN = HYBRID SCAN SECTORS ADAPTATION DATA
C**         GAS_FN = GAGE ACCUMULATION SCAN DATA FILE
C**         GDB_FN = GAGE DATA BASE FILE
C**      HYPROD_FN = PRECIP. PRODUCTS INTERNAL DATA BASE FILE
C**     USERSEL_FN = USER SEL. PRODUCTS INTERNAL DATA BASE FILE
C**       BTABL_FN = BIAS TABLE DATA FILE
C
      CHARACTER*12 OCC_FN, SCTR_FN, GAS_FN, GDB_FN
      CHARACTER*12 HYPROD_FN, USERSEL_FN, BTABL_FN
C
C** VARIABLE TO PREVENT ALIGNMENT COMPILER WARNING
C
      CHARACTER*2 CPAD
C
      COMMON /ENVIRON/ ENVIRON_FIRST
      COMMON /ENVIRON/ OCC_FN, SCTR_FN, GAS_FN, GDB_FN
      COMMON /ENVIRON/ HYPROD_FN, USERSEL_FN , BTABL_FN
      COMMON /ENVIRON/ ENVIRON_LAST
C
C*************************************************************
C  END ENVIRON ADAPTATION DATA
C*************************************************************
C*************************************************************
C  START DEA NAME PARAMETERS
C*************************************************************
/*
**A3CD70_DEA_NAMES
*VERSION: 1
C
       CHARACTER*18 CELL_PROD_DEA_NAME
       PARAMETER (CELL_PROD_DEA_NAME="alg.cell_product.$")
       CHARACTER*10 HAIL_DEA_NAME
       PARAMETER (HAIL_DEA_NAME="alg.hail.$")
       CHARACTER*18 HYDROMET_ACC_DEA_NAME
       PARAMETER (HYDROMET_ACC_DEA_NAME="alg.hydromet_acc.$")
       CHARACTER*18 HYDROMET_ADJ_DEA_NAME
       PARAMETER (HYDROMET_ADJ_DEA_NAME="alg.hydromet_adj.$")
       CHARACTER*19 HYDROMET_PREP_DEA_NAME
       PARAMETER (HYDROMET_PREP_DEA_NAME="alg.hydromet_prep.$")
       CHARACTER*19 HYDROMET_RATE_DEA_NAME
       PARAMETER (HYDROMET_RATE_DEA_NAME="alg.hydromet_rate.$")
       CHARACTER*24 LAYER_REF_DEA_NAME
       PARAMETER (LAYER_REF_DEA_NAME="alg.layer_reflectivity.$")
       CHARACTER*9 MDA_DEA_NAME
       PARAMETER (MDA_DEA_NAME="alg.mda.$")
       CHARACTER*17 MESOCYCLONE_DEA_NAME
       PARAMETER (MESOCYCLONE_DEA_NAME="alg.mesocyclone.$")
       CHARACTER*10 MPDA_DEA_NAME
       PARAMETER (MPDA_DEA_NAME="alg.mpda.$")
       CHARACTER*13 MODE_SELECT_DEA_NAME
       PARAMETER (MODE_SELECT_DEA_NAME="mode_select.$")
       CHARACTER*13 RADAZVD_DEA_NAME
       PARAMETER (RADAZVD_DEA_NAME="alg.radazvd.$")
       CHARACTER*14 RECCLALG_DEA_NAME
       PARAMETER (RECCLALG_DEA_NAME="alg.recclalg.$")
       CHARACTER*9 SAA_DEA_NAME
       PARAMETER (SAA_DEA_NAME="alg.saa.$")
       CHARACTER*26 STORM_CELL_COMP_DEA_NAME
       PARAMETER (STORM_CELL_COMP_DEA_NAME="alg.storm_cell_component.$")
       CHARACTER*24 STORM_CELL_SEG_DEA_NAME
       PARAMETER (STORM_CELL_SEG_DEA_NAME="alg.storm_cell_segment.$")
       CHARACTER*22 STORM_CELL_TRACK_DEA_NAME
       PARAMETER (STORM_CELL_TRACK_DEA_NAME="alg.storm_cell_track.$")
       CHARACTER*13 SUPEROB_DEA_NAME
       PARAMETER (SUPEROB_DEA_NAME="alg.superob.$")
       CHARACTER*9 TDA_DEA_NAME
       PARAMETER (TDA_DEA_NAME="alg.tda.$")
       CHARACTER*9 VAD_DEA_NAME
       PARAMETER (VAD_DEA_NAME="alg.vad.$")
       CHARACTER*19 VIL_ECHO_TOPS_DEA_NAME
       PARAMETER (VIL_ECHO_TOPS_DEA_NAME="alg.vil_echo_tops.$")
       CHARACTER*11 SITE_INFO_DEA_NAME
       PARAMETER (SITE_INFO_DEA_NAME="site_info.$")
       CHARACTER*7 ALERTING_DEA_NAME
       PARAMETER (ALERTING_DEA_NAME="Alert.$")
C
C*************************************************************
C  END DEA NAME PARAMETERS
C*************************************************************
/*

