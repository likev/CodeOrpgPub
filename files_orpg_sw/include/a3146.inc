C
C RCS info
C $Author: ccalvert $
C $Locker:  $
C $Date: 2002/12/31 23:00:00
C $Id: a3146.inc,v 1.22 2010/05/12 16:05:47 ccalvert Exp $
C $Revision: 1.22 $
C $State: Exp $
C
*.********************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A3146.INC
*.
*.  INCLUDE FILE VERSION: 11
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#          PROGRAMMER         NOTES
*.  ---------    --   -----         ----------------   ---------------
*.  10/10/90     00   90899         PAUL JENDROWSKI
*.  01/07/91     01   90888         PAUL JENDROWSKI   **A3146CG
*.  10/20/92     02   NA92-28001    BRADLEY SUTKER    PS,LFM,C1
*.  10/26/92     02   NA90-93082    JOSEPH WHEELER    **A314C1
*.  07/01/94     03   NW94-10102    CHERYL STEPHENSON **A314C2
*.  08/04/94     03   NW94-08402    CHERYL STEPHENSON **A3146C2,A3146PS,A314C3
*.                                                    **A3146PX
*.  08/04/94     03   NW94-06018    MIKE ISTOK        **A3146P1
*.  05/31/95     04   NA94-35301    ROBERT RIERSON    (NA94-08102) **A3146C2
*.  10/11/95     04   NA94-13201    RICHARD FULTON    **A3146PS
*.  01/22/96     04   NA94-33207    TIM O'BANNON      **A3146PS
*.  09/09/97     05   NA96-09201    JAY BREIDENBACH   Delete **A3146PX
*.  01/31/02     06   NA01-27501    Dennis A. Miller  **A3146C2, **A3146PS,
*.                                                    **A3146PU
*.                                         (removed)  **A3146CG, **A3146CG1,
*.                                                    **A3146CG3
*.  07/31/02     07   NA02-15801    Jihong Liu        **A3146P3 
*.  07/31/02     07   NA02-11605    Cham Pham         **A3146PS
*.  06/30/03     08   NA02-06508    C. Pham           A3146PS,A3146PD,A3146PU  
*.  01/13/05     09   NA04-33201    C. Pham, D.Miller A3146PS
*.  10/26/05     10   NA05-09504    Cham Pham         Added B3_3HR_IDX field 
*.                                                    to **A3146C2
*.  10/26/05     10   NA05-20605    Cham Pham         **A3146PS - fixed unit  
*.                                                    label
*.  01/26/07     11   NA07-01702    Jihong Liu        A3146PS 
*.  10/23/09     12   NA07-21402    James Ward        Fixed CZM, CZP units
*.

*.********************************************************************

**A3146P1
*VERSION:1
C ******************************************************************
C
C    NAME             TYPE    DESCRIPTION
C    ---------------  ------  --------------------------------------
C    KGRID_SIZE         I*4   # of boxes per side of Cartesian grid.
C                             The grid encompasses an area equiv. to a
C                             polar grid with diameter of 460 KM.
C    KGRID_BOX_SIZE     I*4   Length of one Cartesian grid box, in KM.
C    KPOLAR_BIN_SIZE    I*4   Length of one polar bin, in KM
C    K2KM_WORDS         I*4   # I*4 words in 232x232 Cartesian grid
C    KDENOM_WORDS       I*4   # I*2 words in 232x232 demoninator array
C    KGRID_CENTER       I*4   Center point of 232x232 Cartesian grid
C    KBIN_CENTER_OFFSET I*4   Offset to the center of a 2 KM polar bin
C    KSQUARED_GRID_     I*4   Square of the Cartesian grid center
C                CENTER
C    KRADS              I*4   # of radians in the 232KM polar grid
C    KBINS              I*4   # of 2KM bins in the 232KM polar grid
C    AZ_IN_KQUAD        I*4   # of degrees in 1/4 of the polar grid
C    KNOSPLIT_RANGE     I*4   Range of 2KM polar bins for which
C                             azimuth need not be split to obtain a
C                             good mapping (few holes) from polar to
C                             Cartesian coordinates
C    KSPLIT_RANGE       I*4   Minimum range from which azimuth is
C                             split to avoid excessive holes in
C                             polar to Cartesian conversion mapping
C    KOLDEST            I*4   Index in GRID_ORDER to oldest hourly
C                             data in the precipitation data base
C    KMIDDLE            I*4   Index in GRID_ORDER to middle of three
C                             hourly precipitation data arrays in the
C                             precipitation data base
C    KNEWEST            I*4   Index in GRID_ORDER to most recent
C                             hourly data in precipitation data base
C    KMISSING           I*4   Flag value: no precipitation data
C                             available for that hour
C    KRADIAN           Real   # of radians in one degree of arc.
C                             1 degree = pi/180 radians.
C    SCALE_78_79       Real   Scaling parameter for quantizing the
C                             OHP and THP products
C    SCALE_80          Real   Scaling parameter for quantizing the
C                             STP product
C
C ******************************************************************
C
      INTEGER KGRID_SIZE
      REAL    KGRID_BOX_SIZE, KPOLAR_BIN_SIZE, KBIN_CENTER_OFFSET
      INTEGER K2KM_WORDS, KDENOM_WORDS,  KPOLAR_WORDS, KQUADS
      INTEGER KRADS, KBINS, AZ_IN_KQUAD, KSQUARED_GRID_CENTER
      INTEGER KGRID_CENTER, KZONE1, KZONE2, N_ZONES
      INTEGER KNOSPLIT_RANGE, KSPLIT_RANGE
      INTEGER  KMISSING
      INTEGER WRITKODE, READKODE
      INTEGER HDB_DISK_LU
      REAL KRADIAN
      INTEGER DIR_REC, HRA_REC, HRB_REC, HRC_REC, STM_REC, RLE_REC
      INTEGER DPA_REC
      INTEGER GOOD, IINIT, IBEG, INCRMENT
      INTEGER*4 CODE_1HR, CODE_3HR, CODE_STT, CODE_DIG, CODE_SUP
C
C  PRODUCT CODES
C
      PARAMETER (CODE_1HR=78, CODE_3HR=79, CODE_STT=80)
      PARAMETER (CODE_DIG=81, CODE_SUP=82)
C
C  SCALE PARAMETERS FOR QUANTIZING PRODUCTS 78, 79, AND 80
C
      REAL SCALE_78_79,SCALE_80
      PARAMETER (SCALE_78_79=0.2, SCALE_80=0.1)
C
C DISK FILE RECORD PARAMETERS
C
      PARAMETER(DIR_REC=1, HRA_REC=2, HRB_REC=3, HRC_REC=4, STM_REC=5,
     $           RLE_REC=6, DPA_REC=7)
      PARAMETER(HDB_DISK_LU=8)
      PARAMETER (WRITKODE=1, READKODE=2)
C
C  CARTESIAN CONVERSION PARAMETERS
C
      PARAMETER (KGRID_SIZE = 232, KGRID_CENTER = KGRID_SIZE/2)
      PARAMETER (KGRID_BOX_SIZE = 2.)
      PARAMETER (KSQUARED_GRID_CENTER = KGRID_CENTER*KGRID_CENTER)
      PARAMETER (KSPLIT_RANGE = 58, KNOSPLIT_RANGE = KSPLIT_RANGE-1)
      PARAMETER (N_ZONES=2, KZONE1=1, KZONE2=2)
C
C  POLAR GRID PARAMETERS
C
      PARAMETER (KRADS = 360, KBINS = 115, KQUADS=4)
      PARAMETER (AZ_IN_KQUAD = KRADS/KQUADS)
      PARAMETER (KPOLAR_BIN_SIZE = 2., KBIN_CENTER_OFFSET = 1.)
C
C  SCRATCH BUFFER SIZES IN I*4 WORDS
C
      PARAMETER (K2KM_WORDS = KGRID_SIZE*KGRID_SIZE)
      PARAMETER (KDENOM_WORDS = (K2KM_WORDS + 1)/2)
      PARAMETER (KPOLAR_WORDS = (KBINS*KRADS + 1)/2)
C
C  MISCELLANEOUS PARAMETERS
C
      PARAMETER (KRADIAN = 0.01745329)
      PARAMETER(GOOD=0, IINIT=0, IBEG=1, INCRMENT=1 )
      PARAMETER ( KMISSING = -1)
/*
**A3146P2
*VERSION:0
C
C** PARAMETERS FOR BUFFER SIZE OF THE IMAGE PRODUCTS AND MAX RLE LENGTH
C
      INTEGER PRODTRLR_I2, IPROD_I4SIZE, MX_RLE_I2
      PARAMETER(MX_RLE_I2 = KDENOM_WORDS + RASTA_BEGIN)
      PARAMETER (PRODTRLR_I2 = 2000)
      PARAMETER (IPROD_I4SIZE = (MX_RLE_I2 + PRODTRLR_I2 + 1)/2)

/*
**A3146P3
*VERSION:0
C  A3146P3 is added by Jihong

C** PARAMETERS FOR BUFFER SIZE OF THE DIGITAL STORM TOTAL PRODUCT
C
      INTEGER    NI2, CNTRL_WDS, MAXIND, DIG_STM_SIZ
      INTEGER    PAD_SPACE, START_WORD
      INTEGER    MAX_ADJ_P1
      PARAMETER (CNTRL_WDS=3)
      PARAMETER (START_WORD=76)
      PARAMETER (PAD_SPACE=50)
      PARAMETER (NI2=2)
      PARAMETER (MAX_ADJ_P1 = MAX_ADJBINS + 1)
C
      PARAMETER (MAXIND =((MAX_ADJ_P1+CNTRL_WDS*NI2)*MAX_AZMTHS)/NI2
     $                                                  + START_WORD)
C
      PARAMETER (DIG_STM_SIZ = MAXIND/NI2 + PAD_SPACE +
     $                                     (NI2_ASCII_LYR_WRDS+1)/NI2)
C
/*
**A3146C1
*VERSION:0
C ******************************************************************
C
C    NAME             TYPE    DESCRIPTION
C    ---------------  ------  --------------------------------------
C    POLAR_TO_         Log    Flag indicating whether the polar to
C    CARTESIAN_INIT_DONE      Cartesian mapping arrays have been
C                             initialized
C    ZONE1_CARTGRID    I*4    Array of Cartesian coordinates
C                             corresponding to polar coordinates for
C                             one quadrant (90 degrees) out to a
C                             bin range of 57 (114 KM)
C    ZONE2_CARTGRID    I*4    Array of Cartesian coordinates
C                             corresponding to polar coordinates for
C                             one quadrant (90 degrees) for a bin
C                             range of 58 to 115 (116 to 230 KM).
C                             This is a 3-dimensional array, since
C                             each polar coordinate produces two
C                             Cartesian coordinates.
C    INNER_HOLES       I*4    Array of Cartesian grid boxes into
C                             which no polar coordinates were mapped.
C                             These grid boxes correspond to polar
C                             coordinates within the 230 KM radius.
C    INNER_COUNT       I*4    # of actual hole coordinates in
C                             array INNER_HOLES
C
C ******************************************************************
C
C
      INTEGER ZONE1_CARTGRID (KNOSPLIT_RANGE,AZ_IN_KQUAD)
      INTEGER ZONE2_CARTGRID (KSPLIT_RANGE:KBINS,AZ_IN_KQUAD,2)
      INTEGER INNER_HOLES (500)
      INTEGER INNER_COUNT
C
      COMMON /A3146C1/ ZONE1_CARTGRID, ZONE2_CARTGRID,
     1                 INNER_HOLES, INNER_COUNT
C
/*
**A3146C2
*VERSION: 3
C
C ****** COMMON BLOCK THAT DEFINES THE DIRECTORY OF THE DISK FILE
C ****** THAT IS MAINTAINED BY THE PRODUCT TASK
C ****** ANY ADDITIONS MUST BE MADE BETWEEN THE START AND END VRBLS
C
C   BIAS_ESTM_1HR  : Radar bias estimate at end of clock-hour for each
C                    hour in 3-hour product; uses HYDRSUPL(CUR_BIAS)
C   SUM_BIAS       : Sum of 1-hr biases in period of 3-hour product
C   BIAS_ESTM_3HR  : Bias average estimate for 3-hour product
C
C   BIAS_GRPSIZ_1HR: Effective sample size associated with bias for each
C                    hour in 3-hour product; uses HYDRSUPL(CUR_GRPSIZ)
C   SUM_GRPSIZ     : Sum of sample sizes in period of 3-hour product
C   BIAS_GRPSIZ_3HR: Effective sample size average for 3-hour product 
C
C   BIAS_MSPAN_1HR : Memory span associated with selected bias for each
C                    hour in 3-hour product; uses HYDRSUPL(CUR_MSPAN)
C   SUM_MSPAN      : Sum of memory spans in period of 3-hour product
C   BIAS_MSPAN_3HR : Memory span average for 3-hour product 
C   B3_3HR_IDX     : Index to Block 3 location in 3-hour product
C
C
C
      INTEGER NSCANS
      PARAMETER (NSCANS = 3)
      REAL    BIAS_ESTM_1HR(NSCANS),   SUM_BIAS,   BIAS_ESTM_3HR,
     +        BIAS_GRPSIZ_1HR(NSCANS), SUM_GRPSIZ, BIAS_GRPSIZ_3HR,
     +        BIAS_MSPAN_1HR(NSCANS),  SUM_MSPAN,  BIAS_MSPAN_3HR
      INTEGER START_DIR
      INTEGER KNEWEST, KMIDDLE, KOLDEST
      INTEGER HRLY_EDATE(NSCANS),HRLY_ETIME(NSCANS)
      INTEGER SB_STATUS_HRLY(NSCANS)
      INTEGER FLG_ZERO_HRLY(NSCANS), FLG_NO_HRLY(NSCANS)
      INTEGER HRLY_SCAN_TYPE(NSCANS), FLG_ADJUST                        9408402
      INTEGER SUM_TIMES(NSCANS), HOURLY_BUFFERS, FLG_NO_3HR
      INTEGER SB_STAT_3HR, B3_3HR_IDX
      INTEGER FILE_REC(NSCANS)
      INTEGER RLE_WORDS, MXVAL_3HR
C
C  STMTOT_BDAT  - STORM TOTAL SCAN'S BEGINNING DATE
C  STMTOT_BTIM  - STORM TOTAL SCAN'S BEGINNING TIME
C
C  STMTOT_EDAT  - STORM TOTAL SCAN'S BEGINNING DATE
C  STMTOT_ETIM  - STORM TOTAL SCAN'S BEGINNING TIME
C  STMTOT_FLG_FRST - LOGICAL FLAG INDICATING IF 1ST SCAN OF
C                    STORM TOTAL
C
C   ST_SUM_BIAS   : Sum of 1-hr biases in period of Storm Total product
C   ST_SUM_GRPSIZ : Sum of sample sizes in period of Storm Total product
C   ST_SUM_MSPAN  : Sum of memory spans in period of Storm Total product
C
C
      INTEGER  STMTOT_BDAT,STMTOT_BTIM,STMTOT_EDAT,STMTOT_ETIM
      INTEGER  NUM_ST_SCANS, STMTOT_SB
      REAL     ST_SUM_BIAS, ST_SUM_GRPSIZ, ST_SUM_MSPAN
      LOGICAL  STMTOT_FLG_FRST
C
      INTEGER END_DIR
C
C** SET I*2 ARRAY SO THAT COMMON EQUALS ONE DISK SECTOR EXACTLY
C** (I.E. 256 BYTES)
C
      INTEGER*2 C2SECTOR(128)
      EQUIVALENCE (START_DIR,C2SECTOR(1))
C
      COMMON /A3146C2/ START_DIR
      COMMON /A3146C2/ KNEWEST, KMIDDLE, KOLDEST
      COMMON /A3146C2/ FILE_REC, HRLY_EDATE, HRLY_ETIME
      COMMON /A3146C2/ FLG_ZERO_HRLY, FLG_NO_HRLY, HRLY_SCAN_TYPE
      COMMON /A3146C2/ FLG_ADJUST, SUM_TIMES, SB_STATUS_HRLY
      COMMON /A3146C2/ HOURLY_BUFFERS
      COMMON /A3146C2/ BIAS_ESTM_1HR, BIAS_GRPSIZ_1HR, BIAS_MSPAN_1HR
      COMMON /A3146C2/ BIAS_ESTM_3HR, BIAS_GRPSIZ_3HR, BIAS_MSPAN_3HR
      COMMON /A3146C2/ RLE_WORDS, MXVAL_3HR, FLG_NO_3HR, SB_STAT_3HR
      COMMON /A3146C2/ B3_3HR_IDX
C
C** STORM TOTAL INFORMATION
C
      COMMON /A3146C2/ STMTOT_SB
      COMMON /A3146C2/ STMTOT_BDAT, STMTOT_BTIM
      COMMON /A3146C2/ STMTOT_EDAT,STMTOT_ETIM, STMTOT_FLG_FRST
      COMMON /A3146C2/ NUM_ST_SCANS
      COMMON /A3146C2/ ST_SUM_BIAS, ST_SUM_GRPSIZ, ST_SUM_MSPAN
C
      COMMON /A3146C2/ END_DIR
/*
**A3146C3
*VERSION:0
C
C ***************************************************************
C
C  THIS BLOCK CONTAINS LOCAL COMMON DATA TO THE TASK THAT DEFINES
C  THE GENERAL STATE OF THE TASK AT ANY TIME
C
      INTEGER FIL_STAT,LAST_OPERATION,LAST_REC
      COMMON /A3146C3/ FIL_STAT, LAST_OPERATION, LAST_REC
C
/*
**A3146D1
*VERSION:0
C**********************************************
C
C    NAME             TYPE    DESCRIPTION
C    ---------------  ------  --------------------------------------
C    POLAR_CART_       Log    Flag indicating whether the polar to
C           INIT_OK           Cartesian mapping arrays have been
C                             initialized
C    POLAR_LFM_        Log    Flag indicating whether the polar to
C           INIT_OK           LFM grid mapping arrays have been
C                             initialized
C
C**********************************************
C
      LOGICAL POLAR_CART_INIT_OK
      LOGICAL POLAR_LFM_INIT_OK
C
      COMMON /A3146D1/ POLAR_CART_INIT_OK, POLAR_LFM_INIT_OK
C
/*
**A3146LFP
*VERSION:1
C
C** LFM related parameters...
C
      INTEGER    HYZ_LFM40
      PARAMETER (HYZ_LFM40 = 131 )
C
C** LFM Flag value parameters...
C
      INTEGER    BEYOND_RANGE, WITHIN_RANGE, HOLE_FLAG
      PARAMETER (BEYOND_RANGE=-99, WITHIN_RANGE=0, HOLE_FLAG=-88)
C
C** Rate scan bin sizing parameters...
C
      INTEGER    R_BIN_SIZE,   R_MID_OFS
      PARAMETER (R_BIN_SIZE=2, R_MID_OFS=1)
C
/*
**A3146LFX
*VERSION: 0
C
C POLAR TO LFM CONVERSION CONSTANTS
C
      INTEGER LFM4_IDX,LFM16_IDX,LFM40_IDX,LFMMX_IDX
      PARAMETER (LFM4_IDX=1,LFM16_IDX=2,LFM40_IDX=3,LFMMX_IDX=3)
      INTEGER OFFSET(LFMMX_IDX),IKA(LFMMX_IDX)
      DOUBLE PRECISION B_CON(LFMMX_IDX),ANGLE_THRESH,KA(LFMMX_IDX)
      DOUBLE PRECISION LFM4_RNG,LFM16_RNG,LFM40_RNG
      DOUBLE PRECISION RE_PROJ,RE_PROJ_SQ,CONST,PRIME,R2KO,IP,JP
      PARAMETER (IP=433.0,JP=433.0,ANGLE_THRESH=9.81E-6,CONST=135.0)
      PARAMETER (LFM4_RNG=230.0,LFM16_RNG=460.0,LFM40_RNG=230.0)
      PARAMETER (R2KO=249.6348607,PRIME=105.0)
      PARAMETER (RE_PROJ=6380.0,RE_PROJ_SQ=6380.0*6380.0)
      DATA B_CON/1.0,.25,.10/
      DATA IKA/1,4,10/
      DATA KA/1.0,4.0,10.0/
      DATA OFFSET/7,49,66/
/*
**A3146PS
*VERSION: 3
C
C DATA STATEMENTS FOR BLOCK-3 OF HYDROMET PRODUCTS
C ------------------------------------------------
C
      INTEGER      NBIAS_FLDS, BIAS_VAL, GRP_SIZ, MEM_SPAN
      PARAMETER   (NBIAS_FLDS=3, BIAS_VAL=1, GRP_SIZ=2, MEM_SPAN=3)
C
      INTEGER      PG1,PG2,PG3,PG4,PG5,NLINES,NPAGES,
     &             NCHAR_LN, NI2CHAR_LN, L
C
      INTEGER      C3,C4,C5,C6,C7,
     &             L1S,L2S,L3S,L4S,L5S,L6S,L7S,
     &             L1E,L2E,L3E,L4E,L5E,L7E, L7E_LNX
C
      PARAMETER   (PG1=4,
     &             PG2=14, PG3=ASIZ_RATE,
     &             PG4=ASIZ_ACUM, PG5=ASIZ_ADJU+1,
     &             NLINES=PG1+PG2+PG3+PG4+PG5, NPAGES=5,
     &             NCHAR_LN=80, NI2CHAR_LN=NCHAR_LN/2 )
      PARAMETER   (C3=63, C4=65, C5=70, C6=71, C7=72,
     &             L1S=5,  L2S=11,  L3S=17,  L4S=19, L5S=25,
     &             L1E=10, L2E=16,  L3E=18,  L4E=24, L5E=30,
     &             L6S=31, L7S=32,
     &             L7E=35, L7E_LNX=36)
C
      INTEGER NLINES_PG(NPAGES)
      DATA    NLINES_PG /PG1, PG2, PG3, PG4, PG5 /
C
      CHARACTER*80 BLK3_CHARLN(NLINES),TEMP_CHARLN
      INTEGER*2    INTLINE(NI2CHAR_LN)
      EQUIVALENCE (INTLINE,TEMP_CHARLN)
C
C............................
C IF THERE WAS A PROBLEM WITH STORM-TOTAL (PRODUCT-81), AND THERE
C WAS NO DATA, BUT THE PRODUCT STILL NEEDS TO BE PRODUCED,
C USE THE FOLLOWING, TO BUILD A BLOCK3 MESSAGE:
C
      INTEGER      NPG_ERRS, NLN_ERRS, NCR_ERRS, NREASONS
      PARAMETER  ( NPG_ERRS=1, NLN_ERRS=2, NREASONS=7, NCR_ERRS=80 )
      CHARACTER    CHARLN_ERRS(NLN_ERRS)*(NCR_ERRS),
     +             CHARLN_REASONS(NREASONS)*(NCR_ERRS)
C
      DATA CHARLN_ERRS(1)
     + /' DATA NOT AVAILABLE                                   ' /
      DATA (CHARLN_REASONS(L),L=1,4)
     + /' BECAUSE: NO SCRATCH BUFFER SPACE FOR PRODUCT         ',
     +  ' BECAUSE: STORM DATA NOT AVAILABLE FROM DISK          ',
     +  ' BECAUSE: BAD STATUS FOR PRECIPITATION DATA FILE      ',
     +  ' BECAUSE: NO SCRATCH BUFFER SPACE FOR STORM TOTAL DATA'/
      DATA (CHARLN_REASONS(L),L=5,NREASONS)
     + /' BECAUSE: INSUFFICIENT NUMBER OF HOURLY ACCUMULATIONS ',
     +  ' BECAUSE: INSUFFICIENT DATA FOR HOURLY ACCUMULATION   ',
     +  ' BECAUSE: CURRENT ACCUMULATION SCAN IS FLAGGED AS BAD ' /
C............................
C
C SUPPLEMENTAL DATA
C
#IFDEF LINUX
C
C SUPPLEMENTAL DATA
C
      DATA (BLK3_CHARLN(L),L=1,PG1)
     A/'          GAGE/RADAR BIAS ESTIMATE .........................',
     B '          SAMPLE SIZE (EFFECTIVE NO. GAGE/RADAR PAIRS) .....',
     C '          MEMORY SPAN (HOURS) OVER WHICH BIAS DETERMINED ...',
     D '          PRODUCT ADJUSTED BY BIAS ESTIMATE? ...............'/

C PRECIPITATION ENHANCED PRE-PROCESSING ALGORITHM
C
      DATA (BLK3_CHARLN(L),L=L1S,L1E)
     A/'RADAR HALF POWER BEAM WIDTH.................................
     +       DEG     ',
     B 'MAXIMUM ALLOWABLE PERCENT OF BEAM  BLOCKAGE.................
     +        %      ',
     C 'MAXIMUM ALLOWABLE PERCENT LIKELIHOOD OF CLUTTER.............
     +        %      ',
     D 'PERCENT OF BEAM REQUIRED TO COMPUTE AVERAGE POWER...........
     +        %      ',
     E 'PERCENT OF HYBRID SCAN NEEDED TO BE CONSIDERED FULL.........
     +        %      ',
     F 'LOW REFLECTIVITY THRESHOLD (dBZ) FOR BASE DATA..............
     +       dBZ     '/

      DATA (BLK3_CHARLN(L),L=L2S,L2E)
     G/'REFLECTIVITY (dBZ) REPRESENTING SIGNIFICANT RAIN............
     +       dBZ     ',
     H 'AREA WITH REFLECTIVITY EXCEEDING SIGNIFICANT RAIN THRESHOLD.
     +       KM**2   ',
     I 'THRESHOLD TIME WITHOUT RAIN FOR RESETTING STP ..............  
     +       MINUTES ',
     J 'REFLECT-TO-PRECIP RATE CONVERSION MULTIPLICATIVE COEFFICIENT
     +               ',
     K 'REFLECT-TO-PRECIP RATE CONVERSION POWER COEFFICIENT.........
     +               ',
     L 'MIN DBZ FOR CONVERTING TO PRECIP RATE (VIA TABLE LOOKUP)....
     +       dBZ     '/

      DATA  (BLK3_CHARLN(L),L=L3S,L3E)
     M/'MAX DBZ FOR CONVERTING TO PRECIP RATE (VIA TABLE LOOKUP)....
     +       dBZ     ',
     N 'NUMBER OF EXCLUSION ZONES...................................
     +               '/
C
C PRECIPITATION RATE ALGORITHM
C
      DATA (BLK3_CHARLN(L),L=L4S,L4E)
     A/'RANGE BEYOND WHICH TO APPLY RANGE-EFFECT CORRECTION.........
     +       KM      ',
     B '1ST COEFFICIENT OF RANGE-EFFECT FUNCTION....................
     +       dBR     ',
     C '2ND COEFFICIENT OF RANGE-EFFECT FUNCTION....................
     +       dBR     ',
     D '3RD COEFFICIENT OF RANGE-EFFECT FUNCTION....................
     +       dBR     ',
     E 'MIN RATE SIGNIFYING PRECIPITATION...........................
     +       MM/Hr    ',
     F 'MAX PRECIPITATION RATE......................................
     +       MM/Hr    '/
C
C PRECIPITATION ACCUMULATION ALGORITHM
C
      DATA (BLK3_CHARLN(L),L=L5S,L5E)
     A/'REINITIALIZATION TIME LAPSE THRESHOLD (FOR ACCUM PROCESS)...
     +       MINUTES ',
     B 'MAX TIME DIFFERENCE BETWEEN SCANS FOR INTERPOLATION.........
     +       MINUTES ',
     C 'MIN TIME NEEDED TO ACCUMULATE HOURLY TOTALS.................
     +       MINUTES ',
     D 'THRESHOLD FOR HOURLY OUTLIER ACCUMULATION...................
     +       MM      ',
     E 'HOURLY GAGE ACCUMULATION SCAN ENDING TIME...................
     +       MINUTES ',
     F 'MAX ACCUMULATION PER SCAN-TO-SCAN PERIOD....................
     +       MM      '/
      DATA  BLK3_CHARLN(L6S)
     G/'MAX ACCUMULATION PER HOURLY PERIOD..........................
     +       MM      '/
C
C PRECIPITATION ADJUSTMENT ALGORITHM
C
      DATA (BLK3_CHARLN(L),L=L7S,L7E_LNX)
     A/'MINUTES AFTER CLOCK HOUR WHEN BIAS IS UPDATED...............
     +       MINUTES ',
     B 'THRESHOLD # OF GAGE/RADAR PAIRS NEEDED TO SELECT BIAS.......
     +               ',
     C 'RESET VALUE OF GAGE/RADAR BIAS ESTIMATE.....................
     +               ',
     D 'LONGEST ALLOWABLE LAG FOR USE OF BIAS FROM BIAS TABLE.......
     +       HOURS   ',
     E 'MOST RECENT BIAS SOURCE.....................................'/
#ELSE

      DATA (BLK3_CHARLN(L),L=1,PG1)
     A/'          GAGE/RADAR BIAS ESTIMATE .........................',
     B '          SAMPLE SIZE (EFFECTIVE NO. GAGE/RADAR PAIRS) .....',
     C '          MEMORY SPAN (HOURS) OVER WHICH BIAS DETERMINED ...',
     D '          PRODUCT ADJUSTED BY BIAS ESTIMATE? ...............'/
C
C ADAPTATION DATA
C
C PRECIPITATION ENHANCED PREPROCESSING ALGORITHM
C
      DATA (BLK3_CHARLN(L),L=L1S,L1E)
     A/'RADAR HALF POWER BEAM WIDTH.................................  ',
     B 'MAXIMUM ALLOWABLE PERCENT OF BEAM  BLOCKAGE.................  ',
     C 'MAXIMUM ALLOWABLE PERCENT LIKELIHOOD OF CLUTTER.............  ',
     D 'PERCENT OF BEAM REQUIRED TO COMPUTE AVERAGE POWER...........  ',
     E 'PERCENT OF HYBRID SCAN NEEDED TO BE CONSIDERED FULL.........  ',
     F 'LOW REFLECTIVITY THRESHOLD (dBZ) FOR BASE DATA..............  '/

      DATA (BLK3_CHARLN(L),L=L2S,L2E)
     G/'REFLECTIVITY (dBZ) REPRESENTING SIGNIFICANT RAIN............  ',
     H 'AREA WITH REFLECTIVITY EXCEEDING SIGNIFICANT RAIN THRESHOLD.  ',
     I 'THRESHOLD TIME WITHOUT RAIN FOR RESETTING STP ..............  ',
     J 'REFLECT-TO-PRECIP RATE CONVERSION MULTIPLICATIVE COEFFICIENT  ',
     K 'REFLECT-TO-PRECIP RATE CONVERSION POWER COEFFICIENT.........  ',
     L 'MIN DBZ FOR CONVERTING TO PRECIP RATE (VIA TABLE LOOKUP)....  '/

      DATA (BLK3_CHARLN(L),L=L3S,L3E)
     M/'MAX DBZ FOR CONVERTING TO PRECIP RATE (VIA TABLE LOOKUP)....  ',
     N 'NUMBER OF EXCLUSION ZONES...................................  '/

C
C PRECIPITATION RATE ALGORITHM
C
      DATA (BLK3_CHARLN(L),L=L4S,L4E)
     A/'RANGE BEYOND WHICH TO APPLY RANGE-EFFECT CORRECTION.........  ',
     B '1ST COEFFICIENT OF RANGE-EFFECT FUNCTION....................  ',
     C '2ND COEFFICIENT OF RANGE-EFFECT FUNCTION....................  ',
     D '3RD COEFFICIENT OF RANGE-EFFECT FUNCTION....................  ',
     E 'MIN RATE SIGNIFYING PRECIPITATION...........................  ',
     F 'MAX PRECIPITATION RATE......................................  '/
C
C PRECIPITATION ACCUMULATION ALGORITHM
C
      DATA (BLK3_CHARLN(L),L=L5S,L5E)
     A/'REINITIALIZATION TIME LAPSE THRESHOLD (FOR ACCUM PROCESS)...  ',
     B 'MAX TIME DIFFERENCE BETWEEN SCANS FOR INTERPOLATION.........  ',
     C 'MIN TIME NEEDED TO ACCUMULATE HOURLY TOTALS.................  ',
     D 'THRESHOLD FOR HOURLY OUTLIER ACCUMULATION...................  ',
     E 'HOURLY GAGE ACCUMULATION SCAN ENDING TIME...................  ',
     F 'MAX ACCUMULATION PER SCAN-TO-SCAN PERIOD....................  '/
      DATA  BLK3_CHARLN(L6S)
     G/'MAX ACCUMULATION PER HOURLY PERIOD..........................  '/
C
C PRECIPITATION ADJUSTMENT ALGORITHM
C
      DATA (BLK3_CHARLN(L),L=L7S,L7E)
     A/'MINUTES AFTER CLOCK HOUR WHEN BIAS IS UPDATED...............  ',
     B 'THRESHOLD # OF GAGE/RADAR PAIRS NEEDED TO SELECT BIAS.......  ',
     C 'RESET VALUE OF GAGE/RADAR BIAS ESTIMATE.....................  ',
     D 'LONGEST ALLOWABLE LAG FOR USE OF BIAS FROM BIAS TABLE.......  '/
C
C
C DATA AMOUNTS - COLUMNS 64-70 (C4-C5)
C UNITS OF MEASURE - COLUMNS 72-79 (C7-C8)
C
C
      DATA (BLK3_CHARLN(L)(C7:NCHAR_LN),L=L1S,L3E)
     &/'DEG      ',' %       ',' %       ',' %       ',
     & ' %       ','dBZ      ','dBZ      ','KM**2    ',
     & 'MINUTES  ','         ','         ','dBZ      ',
     & 'dBZ      ','         '                       /
      DATA (BLK3_CHARLN(L)(C7:NCHAR_LN),L=L4S,L4E)
     &/'KM       ','dBR      ',
     & 'dBR      ','dBR      ','MM/Hr    ','MM/Hr    '/
      DATA (BLK3_CHARLN(L)(C7:NCHAR_LN),L=L5S,L6S)
     &/'MINUTES  ','MINUTES  ','MINUTES  ','MM       ',
     & 'MINUTES  ','MM       ','MM       '            /
      DATA (BLK3_CHARLN(L)(C7:NCHAR_LN),L=L7S,L7E)
     &/'MINUTES  ','         ','         ','HOURS    '/
#ENDIF
C
C
C CLEAR AND BLANK-FILL UNUSED PLACES OF CHARACTER LINE:
C
#IFDEF LINUX
C     NOTHING
#ELSE
      DATA (BLK3_CHARLN(L)(C3:C6),L=1,NLINES) /NLINES*'         '/
#ENDIF
C
/*
**A3146PD
*VERSION:1
C ******************************************************************
C
      INTEGER MAX_RATESCANS, SUP_PARAMS, MAX_RS_RLE
      INTEGER RS_DATE, RS_TIME, HS_NBLK, HS_NCLT
      INTEGER HS_NHRE, AS_BMPD, AS_BMPT, AS_EMPD
      INTEGER AS_EMPT, RS_NRLE, HS_HELA, HS_HYSF
C
      PARAMETER ( MAX_RATESCANS=22, SUP_PARAMS=12 )
      PARAMETER ( MAX_RS_RLE = ((HYZ_LFM4+1)/2 + 1)*HYZ_LFM4 )
      PARAMETER ( RS_DATE= 1, RS_TIME= 2, HS_NBLK = 3 )
      PARAMETER ( HS_NCLT= 4, HS_HYSF= 5 )
      PARAMETER ( HS_HELA= 6, HS_NHRE= 7, AS_BMPD= 8, AS_BMPT= 9 )
      PARAMETER ( AS_EMPD= 10, AS_EMPT=11, RS_NRLE=12 )
C
      INTEGER BAD_SCAN, NO_HOURLY, ZERO_HOURLY, BAD_FILE
      PARAMETER (BAD_SCAN=1,NO_HOURLY=2,ZERO_HOURLY=3,BAD_FILE=4)
C
/*
**A3146CD
*VERSION:0
C******************************************************************
C
      INTEGER NEWEST_RS_INDX, OLDEST_RS_INDX, BASE_DATE, BHT, EHT
      INTEGER SUP_DB(SUP_PARAMS,MAX_RATESCANS)
      LOGICAL USE_RS(MAX_RATESCANS)
      INTEGER*2 RS_DB(MAX_RS_RLE,MAX_RATESCANS)
C
      COMMON /A3146CD/ NEWEST_RS_INDX, OLDEST_RS_INDX, SUP_DB, RS_DB
      COMMON /A3146CD/ USE_RS, BASE_DATE, BHT, EHT
C
/*
**A3146PU
*VERSION: 1
C
      INTEGER    NLINES, NCHAR_LN, NI2_LN,
     +           TAB1, TAB2, TAB3, TAB4, I, L
      PARAMETER (NLINES=14, NCHAR_LN=80, NI2_LN=(NCHAR_LN+1)/2,
     +           TAB1=1,TAB2=36,TAB3=37,TAB4=44 )
C
      INTEGER*2    PRODLINE(NI2_LN*NLINES)
      CHARACTER    CHARLINE(NLINES)*(NCHAR_LN)
      EQUIVALENCE (CHARLINE,PRODLINE)
C
C**   DEFINE INDEX IDENTIFIER FOR EACH LINE OF TEXT :
C
      INTEGER    END_HDATE, END_HTIME, TB_NBLK
      INTEGER    TB_NCLT, TB_BINF, TB_HYSF, TB_HELA,
     +           TB_NHRE, NUM_BADSCNS,
     +           BIAS, NUM_GR_PAIR,
     +           MEM_SPAN, VCP_OFF, WXMODE_OFF
C
      PARAMETER (END_HDATE = 1, END_HTIME = 2)
      PARAMETER (TB_NBLK = 3,   TB_NCLT=    4,
     +           TB_BINF = 5,   TB_HYSF=  6 ,
     +           TB_HELA=  7,   TB_NHRE=    8,
     +           NUM_BADSCNS=9 ,BIAS=    10,NUM_GR_PAIR=11,
     +           MEM_SPAN=    12 ,VCP_OFF=13 ,WXMODE_OFF =14)
C
C**  ESTABLISH THE VALUES IN THE CHARACTER LINES (POSITIONS 1-29) :
C
C--FILL IN COLUMNS 1-36 WITH LINE HEADINGS, ON ALL LINES:
C
      DATA CHARLINE(END_HDATE) /
     +'HOURLY ACCUMULATION END DATE.......:        '/
      DATA CHARLINE(END_HTIME) /
     +'HOURLY ACCUMULATION END TIME.......:        '/
      DATA CHARLINE(TB_NBLK) /
     +'TOTAL NO. OF BLOCKAGE BINS REJECTED:        '/
      DATA CHARLINE(TB_NCLT) /
     +'TOTAL NO. OF CLUTTER BINS REJECTED.:        '/
      DATA CHARLINE(TB_BINF) /
     +'NUMBER OF BINS SMOOTHED............:        '/
      DATA CHARLINE(TB_HYSF) /
     +'PERCENT OF HYBRID SCAN BINS FILLED.:        '/
      DATA CHARLINE(TB_HELA) /
     +'HIGHEST ELEV. ANGLE USED IN HYBSCAN:        '/
      DATA CHARLINE(TB_NHRE) /
     +'TOTAL HYBRID SCAN RAIN AREA........:        '/
      DATA CHARLINE(NUM_BADSCNS) /
     +'NUMBER OF BAD SCANS IN HOUR........:        '/
      DATA CHARLINE(BIAS) /
     +'BIAS ESTIMATE......................:        '/
      DATA CHARLINE(NUM_GR_PAIR) /
     +'EFFECTIVE # G/R PAIR...............:        '/
      DATA CHARLINE(MEM_SPAN) /
     +'MEMORY SPAN (HOURS)................:        '/
      DATA CHARLINE(VCP_OFF) /
     +'CURRENT VOLUME COVERAGE PATTERN....:        '/
      DATA CHARLINE(WXMODE_OFF) /
     +'CURRENT OPERATIONAL (WEATHER) MODE.:        '/
C
      INTEGER NERRS
      PARAMETER (NERRS = BAD_FILE)
      CHARACTER ERRMSG(NERRS)*(NCHAR_LN)
      INTEGER*2 ERR_LINE(NI2_LN,NERRS)
      EQUIVALENCE (ERR_LINE,ERRMSG)
      DATA ERRMSG(BAD_SCAN)/
     $'NO HOURLY ACCUMULATION BECAUSE RATE SCAN FLAGGED BAD  '/
      DATA ERRMSG(NO_HOURLY)/
     $'NO HOURLY ACCUMULATION BECAUSE NOT ENOUGH DATA IN HOUR'/
      DATA ERRMSG(ZERO_HOURLY)/
     $'NO PRECIPITATION DETECTED IN LAST HOUR                '/
      DATA ERRMSG(BAD_FILE)/
     $'NO SUPPLEMENTAL DATA AVAILABLE DUE TO DISK ERROR      '/
C
/*
**A3146P8
*VERSION:0
C
C** PRECIPITATION PRODUCTS DATABASE DISK FILE PARAMETER SPECIFICATIONS
C
      INTEGER NHDR_SCTR, NPLR_SCTR, NRLE_SCTR, NDPA_SCTR
      INTEGER  HDR_SCTR, HRA_SCTR, HRB_SCTR, HRC_SCTR, STM_SCTR
      INTEGER  RLE_SCTR, DPA_SCTR
      INTEGER NHDR_BYTES, NPLR_BYTES, NRLE_BYTES, NDPA_BYTES
      INTEGER DB_SIZE
C
      PARAMETER (NHDR_BYTES=256, NPLR_BYTES=KBINS*KRADS*2)
      PARAMETER (NRLE_BYTES=KDENOM_WORDS*2)
      PARAMETER (NDPA_BYTES = (SUP_PARAMS*MAX_RATESCANS + 2)*4 +
     $            MAX_RS_RLE*MAX_RATESCANS*2 )
      PARAMETER (NHDR_SCTR = (NHDR_BYTES-1)/256 + 1)
      PARAMETER (NPLR_SCTR = (NPLR_BYTES-1)/256 + 1)
      PARAMETER (NRLE_SCTR = (NRLE_BYTES-1)/256 + 1)
      PARAMETER (NDPA_SCTR = (NDPA_BYTES-1)/256 + 1)
      PARAMETER (HDR_SCTR = 0)
      PARAMETER (HRA_SCTR = HDR_SCTR + NHDR_SCTR)
      PARAMETER (HRB_SCTR = HRA_SCTR + NPLR_SCTR)
      PARAMETER (HRC_SCTR = HRB_SCTR + NPLR_SCTR)
      PARAMETER (STM_SCTR = HRC_SCTR + NPLR_SCTR)
      PARAMETER (RLE_SCTR = STM_SCTR + NPLR_SCTR)
      PARAMETER (DPA_SCTR = RLE_SCTR + NRLE_SCTR)
      PARAMETER (DB_SIZE  = DPA_SCTR + NDPA_SCTR + 1)
C
C
C     ORPG Issue 13
C     ORPG is using NPLR_BYTES-sized records for this direct-access
C     file ...
      INTEGER  ORPG_HDR_REC, ORPG_HRA_REC
      INTEGER  ORPG_HRB_REC, ORPG_HRC_REC
      INTEGER  ORPG_STM_REC, ORPG_RLE_REC
      INTEGER  ORPG_DPA_REC
      PARAMETER (ORPG_HDR_REC = 1)
      PARAMETER (ORPG_HRA_REC = 2)
      PARAMETER (ORPG_HRB_REC = 3)
      PARAMETER (ORPG_HRC_REC = 4)
      PARAMETER (ORPG_STM_REC = 5)
      PARAMETER (ORPG_RLE_REC = 6)
      PARAMETER (ORPG_DPA_REC = 7)
C
/*
**A3146CG
*VERSION:0
C
      LOGICAL BAD_GAGE_FILE
      COMMON /A3146CG/ BAD_GAGE_FILE
/*
**A314C1
*VERSION:0
C
C POLAR TO LFM GRID MAPPING COMMON AREA AND
C MAX RATE VALUE FOR PROD #82
C
      INTEGER*4 FLAG_AZ,FLAG_RNG
      PARAMETER (FLAG_AZ=1, FLAG_RNG=2)
C
      INTEGER*4 BEYOND_GRID
      PARAMETER (BEYOND_GRID=-77)
C
      INTEGER*4 HYZ_LFM16,RNG_LFM16
      PARAMETER (HYZ_LFM16=100, RNG_LFM16=460)
      INTEGER*4 NUM_LFM4,NUM_LFM16,NUM_LFM40
      PARAMETER (NUM_LFM4=HYZ_LFM4*HYZ_LFM4)
      PARAMETER (NUM_LFM16=HYZ_LFM16*HYZ_LFM16)
      PARAMETER (NUM_LFM40=HYZ_LFM40*HYZ_LFM40)
C
      INTEGER*4 GRID_LAT,GRID_LON,END_LAT,END_LON
      INTEGER*2 LFM4GRID(MAX_RABINS,MAX_AZMTHS)
      INTEGER*2 LFM4FLAG(NUM_LFM4,FLAG_RNG)
      INTEGER*2 LFM16GRID(RNG_LFM16,MAX_AZMTHS)
      INTEGER*2 LFM16FLAG(NUM_LFM16,FLAG_RNG)
      INTEGER*2 LFM40GRID(MAX_RABINS,MAX_AZMTHS)
      INTEGER*2 LFM40FLAG(NUM_LFM40,FLAG_RNG)
C
      INTEGER*4 MAX82VAL
C
      COMMON /A314C1/ GRID_LAT,GRID_LON
      COMMON /A314C1/ LFM4GRID,LFM4FLAG,LFM16GRID,LFM16FLAG
      COMMON /A314C1/ LFM40GRID,LFM40FLAG
      COMMON /A314C1/ MAX82VAL
      COMMON /A314C1/ END_LAT,END_LON
/*
**A314C2                                                                9410102
*VERSION:0                                                              9410102
C                                                                       9410102
C  MM TO dBA conversion and scaling lookup table                        9410102
C                                                                       9410102
      INTEGER MMTLOG(1:3651)                                            9410102
      COMMON /A314C2/ MMTLOG                                            9410102
/*                                                                      9410102
**A314C3                                                                9408402
*VERSION:0                                                              9408402
C  BIAS_TOGGLE - FLAG INDICATING WHETHER BIAS WAS APPLIED               9408402
C  OR NOT                                                               9408402
C                                                                       9408402
      LOGICAL BIAS_TOGGLE                                               9408402
C                                                                       9408402
      COMMON/A314C3/BIAS_TOGGLE                                         9408402
/*
