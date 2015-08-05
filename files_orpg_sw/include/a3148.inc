XCOMM RCS info 
XCOMM $Author: ryans $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2006/02/09 16:56:25 $ 
XCOMM $Id: a3148.inc,v 1.7 2006/02/09 16:56:25 ryans Exp $ 
XCOMM $Revision: 1.7 $ 
XCOMM $State: Exp $ 
*.********************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A3148.INC
*.
*.  INCLUDE FILE VERSION: 05
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   CCR#          PROGRAMMER         NOTES
*.  ---------    --   -----         ----------------   ---------------
*.  08/22/95     00   CCR94-33207   TIM O'BANNON       **A3183HR
*.  11/25/95     00   CCR94-33201   JAY BREIDENBACH    **A3148PX
*.  08/05/97     01   CCR96-18402   ELLIOTT P. FRIEND  **A3148PV
*.  01/31/02     02   NA01-27501    Dennis A. Miller  **A3148PT, **A3148PU,
*.                                                    **A3146PX
*.  06/30/03     03   NA02-06508    C. Pham            **A3148PX  
*.  01/13/05     04   NA04-27811    C. Pham, D. Miller **A3148PX
*.  01/13/05     04   NA04-28102    C. Pham, D. Miller **A3148PX
*.  01/13/05     04   NA04-33201    C. Pham, D. Miller **A3148PX, **A3148PV
*.  10/26/05     05   NA05-09504    Cham Pham          removed A31483HR
*.
*.********************************************************************
**A3148PT
*VERSION: 0
C
C** Template for construction of Bias Table in SPD product
C
C** Information pertaining to individual lines
C
      INTEGER    NCHAR_LN, NI2_LN, I, J, L
      PARAMETER (NCHAR_LN=80, NI2_LN=(NCHAR_LN+1)/2)
C
      INTEGER    NLINS_BTHDR, NLINS_BTABL
      PARAMETER (NLINS_BTHDR=6, NLINS_BTABL=NLINS_BTHDR+N_BIAS_LINES)
C
      INTEGER*2    PRODLINE(NI2_LN*NLINS_BTABL)
      CHARACTER    CHARLINE(NLINS_BTABL)*(NCHAR_LN)
      EQUIVALENCE  (CHARLINE,PRODLINE)
C
      CHARACTER*(NCHAR_LN)  TITL_LINE, DATE_LINE, HDR_LIN1, HDR_LIN2,
     $                      BLNK_LINE
C
C**   Build template for page two (i.e. Bias Table) of the SPD product
C
      DATA TITL_LINE
     +/'                        GAGE-RADAR MEAN FIELD BIAS TABLE'/       

      DATA DATE_LINE
     +/'LAST BIAS UPDATE TIME:  mo/da/yr hr:mn                      BIAS
     + APPLIED ?   xx '/
C
      DATA HDR_LIN1
     +/'  MEMORY SPAN  | EFFECTIVE NO. |   AVG. GAGE   |   AVG. RADAR  |
     +   MEAN FIELD  |'/
C
      DATA HDR_LIN2
     +/'    (HOURS)    |   G-R PAIRS   |   VALUE (MM)  |   VALUE (MM)  |
     +      BIAS     |'/
C
      DATA BLNK_LINE /'   '/
C
C** Define offsets for line numbers
C
      INTEGER  LI_TITLE, LI_BLNK1, LI_DATIM, LI_BLNK2, LI_HDR1, LI_HDR2,
     +         LI_BIAS_1ST, LI_BIAS_LST
C
      PARAMETER( LI_TITLE = 1, LI_BLNK1 = 2, LI_DATIM = 3,
     +           LI_BLNK2 = 4, LI_HDR1  = 5, LI_HDR2  = 6,
     +           LI_BIAS_1ST = LI_HDR2 + 1,
     +           LI_BIAS_LST = LI_HDR2 + N_BIAS_LINES )
C
C** Tabs for beginning & end of formatted information within character lines
C
      INTEGER  TB_UPT, TE_UPT, TB_APL, TE_APL
C
      PARAMETER (TB_UPT=25, TE_UPT=38, TB_APL=77, TE_APL=79)
C
/*
**A3148PU
*VERSION: 0
C
C** Template for construction of Bias Table sub-layer in layer-type 3
C** (ascii layer) of DPA product
C
C** Information pertaining to sub-header (8 char) field
C
      INTEGER    NCHAR_HDR, NI2_HDR
      PARAMETER (NCHAR_HDR=8, NI2_HDR=(NCHAR_HDR+1)/2)
C
      INTEGER*2   PRODHDR(NI2_HDR)
      CHARACTER   CHARHDR*(NCHAR_HDR)
      EQUIVALENCE (CHARHDR,PRODHDR)
C
C** Information pertaining to full (80 char) lines
C
      INTEGER    NCHAR_LN, NI2_LN, I, J, L
      PARAMETER (NCHAR_LN=80, NI2_LN=(NCHAR_LN+1)/2)
C
      INTEGER    NLINS_BIHDR, NLINS_BTABL
      PARAMETER (NLINS_BIHDR=3, NLINS_BTABL=NLINS_BIHDR+N_BIAS_LINES)
C
      INTEGER*2   PRODLINE(NI2_LN*NLINS_BTABL)
      CHARACTER   CHARLINE(NLINS_BTABL)*(NCHAR_LN)
      EQUIVALENCE (CHARLINE,PRODLINE)
C
      CHARACTER*(NCHAR_LN)  TITL_LINE, DATE_LINE, HDR_LINE
C
C**   Build template for the lines (i.e., rows) of the Bias Table
C
      DATA TITL_LINE
     +/'                        GAGE-RADAR MEAN FIELD BIAS TABLE'/       

      DATA DATE_LINE
     +/'LAST BIAS UPDATE TIME:  mo/da/yr hr:mn                      BIAS
     + APPLIED ?   xx '/
C
      DATA HDR_LINE
     +/'  MSPAN (HRS)    NO. G_R PAIRS   AVG. GAGE(MM)   AVG. RADAR(MM) 
     +  MEAN FLD BIAS '/
C
C** Define offsets for line numbers
C
      INTEGER  LN_TITLE, LN_DATIM, LN_HEADR,
     +         LN_BIAS_1ST, LN_BIAS_LST
C
      PARAMETER( LN_TITLE = 1, LN_DATIM = 2, LN_HEADR = 3,
     +           LN_BIAS_1ST = LN_HEADR + 1,
     +           LN_BIAS_LST = LN_HEADR + N_BIAS_LINES )
C
C** Tabs for beginning & end of formatted information within character lines
C
      INTEGER  TB_UPT, TE_UPT, TB_APL, TE_APL
C
      PARAMETER (TB_UPT=25, TE_UPT=38, TB_APL=77, TE_APL=79)
C
/*
**A3148PX
*VERSION: 2
C
      INTEGER    NLINES, NCHAR_LN, NI2_LN, I, J, L
      PARAMETER (NLINES=17, NCHAR_LN=80, NI2_LN=(NCHAR_LN+1)/2)
C
      INTEGER*2    PRODLINE(NI2_LN*NLINES)
      CHARACTER    CHARLINE(NLINES)*(NCHAR_LN)
      EQUIVALENCE  (CHARLINE,PRODLINE)
C
C**   BUILD TEMPLATE FOR THE STAND ALONE TABULAR ALPHA NUMERIC
C**   BLOCK 3 FOR SPD
C
      DATA (CHARLINE(L),L=1, 9) /
     +'SUPPLEMENTAL PRECIPITATION DATA - RDA ID XXXXX  MM/DD/YY HH:MM ',
     +'                                                               ',
     +'VOLUME COVERAGE PATTERN = XXX   MODE = XX                      ',
     +'                                                               ',
     +'          GAGE BIAS APPLIED               -      XXX           ',
     +'               BIAS ESTIMATE              -   XXX.XX           ',
     +'               EFFECTIVE # G/R PAIRS      -   XXX.XX           ',
     +'               MEMORY SPAN (HOURS)        -   XXX.XX           ',
     +'               DATE/TIME LAST BIAS UPDATE - MM/DD/YY HH:MM     '/
      DATA (CHARLINE(L),L=10,17) /
     +'  TOTAL NO. OF BLOCKAGE BINS REJECTED     -   XXXXXX           ',
     +'               CLUTTER BINS REJECTED      -   XXXXXX           ',
     +'               FINAL BINS SMOOTHED        -   XXXXXX           ',
     +'   HYBRID SCAN PERCENT BINS FILLED        -   XXX.XX           ',
     +'               HIGHEST ELEV. USED (DEG)   -   XXX.XX           ',
     +'               TOTAL RAIN AREA (KM**2)    - XXXXX.XX           ',
     +'                                                               ',
     +'        MISSING PERIOD:  NONE                                  '/
C
C**   DEFINE INDEX IDENTIFIER FOR EACH LINE OF TEXT :
C
      INTEGER    TB_NCLT,       TB_HYSF,   TB_HELA,
     +           TB_NHRE,       TB_NBLK,   TB_BFIL,
     +           BIAS,          NUM_GR_PAIR, DQ_FLAG,
     +           MPD_BTIME,     MPD_ETIME,   VCP_OFF,
     +           WXMODE_OFF,    BIAS_FLAG_OFF, TITLE_OFF,
     +           SIRPGID_OFF,   MEM_SPAN,      DA_TIM_BIAS
C
      PARAMETER (TB_NBLK = 10, TB_NCLT= 11, TB_BFIL = 12,
     +           TB_HYSF=  13 ,TB_HELA= 14,
     +           TB_NHRE=     15 ,DQ_FLAG=   3,
     +           BIAS=         6 ,NUM_GR_PAIR= 7,
     +           MPD_BTIME=   17 ,MPD_ETIME=  17 ,VCP_OFF=     3,
     +           WXMODE_OFF=   3 ,BIAS_FLAG_OFF = 5, TITLE_OFF= 1,
     +           SIRPGID_OFF=  1, MEM_SPAN = 8,  DA_TIM_BIAS = 9)
C
/*
/*
**A3148PV
*VERSION:1
C
C** Parameter Specifications for the Adaptation Data sub-layer (in Layer3) of
C**                                the DIGITAL PRECIPITATION ARRAY (DPA) Product
C
C** Layer3 (the ASCII data layer) of the DPA product is composed of 3 sub-layers
C** corresponding to Adaptation Data, the Bias Table, and Supplemental Data,
C** respectively.  Each sub-layer is preceded by a sub-layer header line (Char*8)
C** in the format of 'ADAP(nn)', BIAS(nn), and 'SUPL(nn)' respectively, where
C** nn = number of items in the sub-layer.  For the Adaptation Data sub-layer,
C** nn = number of Char*8 items, and for the Bias Table and Supplemental Data
C** sub-layers, nn = number of 80-character lines.
C
C---------------------
C
C** NCHARLINE    = Total number of Char*8 CHARLINE items = 1 (for sub-layer
C**                  header element) + size of ADAPTATION Data Array (i.e.
C**                  HYZ_ADAP) + 1 (for Bias Applied Flag) + 6 (blank spaces)
C
C** NI2_PRODLINE = Size in halfwords of PRODLINE = NCHARLINE*4 (since 4 I*2
C**                  PRODLINE elements are needed for each Char*8 CHARLINE item)
C
      INTEGER NCHARLINE, NI2_PRODLINE
      PARAMETER(NCHARLINE=1+HYZ_ADAP+1+6, NI2_PRODLINE=NCHARLINE*4)
C
      INTEGER*2   PRODLINE(NI2_PRODLINE)
      CHARACTER*8 CHARLINE(NCHARLINE)
      EQUIVALENCE (CHARLINE,PRODLINE)
C
C---------------------
C
C**   NI2_ADAP_SUBLYR_WRDS =
C**     No. of I*2 Words in the ADAPTATION DATA sub-layer of the DPA product
C
      INTEGER     NI2_ADAP_SUBLYR_WRDS
      PARAMETER   (NI2_ADAP_SUBLYR_WRDS = NI2_PRODLINE)
C
C---------------------
C
/*
