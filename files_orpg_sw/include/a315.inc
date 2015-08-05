XCOMM RCS info 
XCOMM $Author: ccalvert $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2004/06/30 19:20:23 $ 
XCOMM $Id: a315.inc,v 1.7 2004/06/30 19:20:23 ccalvert Exp $ 
XCOMM $Revision: 1.7 $ 
XCOMM $State: Exp $ 
*.*******************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A315.INC
*.
*.  INCLUDE FILE VERSION: 0002
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#          PROGRAMMER         NOTES
*.  ---------    --   -----         ----------------   --------------
*.  02 13 91     00   91713         DENNIS MILLER      **A315CR
*.  02 13 91     00   91713         DENNIS MILLER      **A3154P
*.  04 11 91     00   90956         DENNIS MILLER      **A315CT
*.  06 13 91     00   92083         DAVID ETRIS
*.  11 19 91     01   92505         PJ YELSHIN         **A315CD
*.  06 19 95     02 NA94-23601      MARK FRESCH        **A315CIH
*.  07/05/95     02 NA94-26302      STEVE SMITH
*.*******************************************************************
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                                   C
C     THIS FILE CONTAINS COMMON BLOCK INFORMATION LOCAL TO CPC-15   C
C                                                                   C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
**A315C3S
*VERSION: 0
C******************* STORM SEGMENTS OUTPUT BUFFER *******************
C************************** (A315C3S.CMN) ***************************
C
C** Dimensional Parameters:
C
C**  -- For Segments Attributes table:
C
C
      LOGICAL SEGHIFLG, SGBUFFLG( NSEG_REF ), BUFF_FULL
      INTEGER SEGBFPTR, SEGCTPTR, SEGAZPTR
     $     ,  SEGBF_LO, SEGBF_HI
     $     ,  SGCNT_LO, SGCNT_HI, AZMBUF_LO, AZMBUF_HI
     $     ,  RFSEGSUM( NSEG_REF ), MXSEGCNT, SEGCOUNT
C
C**  Parameter declarations
C
      PARAMETER ( SEGBF_LO = 0, SEGBF_HI = NSEG_MAX )
      PARAMETER ( SGCNT_LO = 0, SGCNT_HI = NRAD_ELV )
      PARAMETER ( AZMBUF_LO = 0, AZMBUF_HI = NRAD_ELV )
C
C** Named Common:
C
      COMMON /A315C3/ SEGHIFLG, SGBUFFLG, SEGBFPTR
     $       , SEGCTPTR, SEGAZPTR
     $       , RFSEGSUM, MXSEGCNT, SEGCOUNT, BUFF_FULL
/*
**A315A3S
*VERSION: 0
C******************* STORM SEGMENTS AVERAGE TABLE *******************
C************************** (A315A3S.CMN) ***************************
C
C** Dimensional Parameters:
C
      INTEGER AVG_SIZE, MAX_AVG
      PARAMETER ( AVG_SIZE = 3840, MAX_AVG = 5 )
C
C**  Table declarations
C
      INTEGER*2 AVERAGE( AVG_SIZE ), OFFSET( MAX_AVG )
C
C** Named Common:
C
      COMMON /A315A3/ AVERAGE, OFFSET
/*
**A315C6T
*VERSION: 1
C******************** STORM TRACK OUTPUT BUFFER *********************
C************************** (A315C6T.CMN) ***************************
C
C** Dimensional Parameters:
C
C**  -- For Storm History & Correlation tables:
C
      INTEGER   NPV_MAX, NPV_MX1, NST_CHR, NST_MAX, NMF_VOL
      PARAMETER ( NPV_MAX = 13, NPV_MX1 = NPV_MAX-1 )
      PARAMETER ( NST_CHR = 2, NST_MAX = 100, NMF_VOL = 2 )
C
C** Positional Parameters:
C
      INTEGER   X_POS, Y_POS
      PARAMETER ( X_POS=1, Y_POS=2 )
C
C** Array & Variable Definitions:
C
      INTEGER  IXPRE,  IXCUR,  IXOLD,  IXNEX,  IXP,  IXC
      INTEGER  NSTMHIST( NPV_MAX ), CORLTABL( NST_MAX, NPV_MAX )
      REAL  TIMEHIST( NPV_MAX ), POSITABL( NST_CHR, NST_MAX, NPV_MAX )
C
C** Equivalences:
C
      EQUIVALENCE ( IXCUR, IXOLD )
C
C** Named Common:
C
      COMMON /A315C6T/  IXPRE, IXCUR, IXNEX, TIMEHIST, NSTMHIST
      COMMON /A315C6T/  POSITABL, CORLTABL, IXP, IXC
/*
**A315CCS
*VERSION: 0
C**************** SEGMENTS SHARED COMMON ************************
C************************ (A315CCS.CMN) *****************************
C
      LOGICAL ABORT, GOOD_RAD
      LOGICAL REF_FLAG, VEL_FLAG, WID_FLAG
      INTEGER RAD_STAT, OUTACT
C
C** Named Common:
C
      COMMON /A315CC/  ABORT, GOOD_RAD, REF_FLAG, VEL_FLAG
     $                ,WID_FLAG, RAD_STAT, OUTACT
/*
**A315CDS
*VERSION: 1
C**************** SEGMENTS DERIVED ADAPTATION DATA ******************
C************************ (A315CDS.CMN) *****************************
C
C** Dimensional Parameters for STMLWTBL and Segments Adaptable Params.
C
      INTEGER NLIQ, NRTH
      PARAMETER ( NLIQ = 256, NRTH = 7 )
C
C** Definition of Liquid Water Concentration table:
C
      REAL   STMLWTBL( 0 : NLIQ )
      LOGICAL CALC_LW_FLG
C
C** Threshholds derived directly from Adaptation Data:
C
      INTEGER   REFTHRBIAS( NRTH ), DROPRFBIAS( NRTH )
     $      , SEGLEN( NRTH ), MAXSEGNO, ADPLASTB, ADPMNZ
      INTEGER   RDROPNBR, NUMAVGBN1, REF_MIN_NDX, REF_MAX_NDX     
     $         , REFDIFBIAS, ADPNSGMX, ADPNDROP, ADPNAVGB
      REAL ADPMWGTF, ADPMULTF, ADPMCOEF, ADPSEGLN( NRTH )
C
C** Factors for converting Reflectivity from dBz to Biased values:
C
      REAL  MULTFACT, ADDFACT, OFFACT
      PARAMETER ( ADDFACT = 32.0, MULTFACT = 2.0, OFFACT = 2.0 )
C
C** Factors for partitioning SEGMAIN buffer between reflectivity thresh.
C
      REAL SUM_BUFTH, BSEG_THR( NRTH )
C
C** Named Common:
C
      COMMON /A315CD/ STMLWTBL, CALC_LW_FLG, ADPMWGTF, ADPMULTF,
     $  ADPMCOEF, ADPSEGLN, ADPNDROP, REFDIFBIAS, ADPNAVGB,
     $  ADPNSGMX, SEGLEN, MAXSEGNO, ADPLASTB, REFTHRBIAS,
     $  DROPRFBIAS, NUMAVGBN1, REF_MIN_NDX, REF_MAX_NDX,     
     $  SUM_BUFTH, BSEG_THR
C
      COMMON /A315CDM/ ADPMNZ
/*
**A315CFF
*VERSION: 1
C********************* FORECAST INTERNAL COMMON *********************
C************************** (A315CFF.CMN) ***************************
C
C** Dimensional Parameters:
C
      INTEGER   NSF_DIR, NBP_CHR
      PARAMETER ( NSF_DIR=4, NBP_CHR=3 )
C
C** Positional Parameters:
C
      INTEGER   X_DIR, Y_DIR, V_DIR, V_SPD
      PARAMETER ( X_DIR=1, Y_DIR=2, V_DIR=3, V_SPD=4 )
C
C** Flagging Parameters:
C
      REAL  INIT_MIN
      PARAMETER ( INIT_MIN = 999999.99 )
C
C** Array & Variable Definitions:
C
      INTEGER  BEST_IP, NUMERR( NST_MAX,NMF_VOL )
C
      REAL  TIMEINT, TIMEHIS2( NPV_MAX ), STMSPD( NSF_DIR,NST_MAX )
      REAL  MASSPREV( NST_MAX ), BESTPOS( NBP_CHR,NST_MAX ), BPD_MIN
      REAL  TOTERR( NST_MAX,NMF_VOL )
C
C** Named Common:
C
      COMMON /A315CFF/  TIMEINT, TIMEHIS2, STMSPD, MASSPREV
      COMMON /A315CFF/  BESTPOS, BEST_IP, BPD_MIN, TOTERR, NUMERR
/*
**A315CGS
C********************* SEGMENTS INTERNAL COMMON *********************
C************************** (A315CG.CMN) ****************************
C
C** Definitions of counters and per-radial variables & tables:
C
      INTEGER FSURBIN, NSURBIN, LSURBIN
      INTEGER BSEGCNTX, BAZM_NDX, RADTIME
      INTEGER RADSGCNT, REF_BASE_NDX
      REAL     DELTANG, SINAZ, COSAZ, RNGFCTR
      REAL   SURLEN, RADELEV, RADAZIM
C
C** Named Common:
C
      COMMON /A315CG/ RADTIME, RADAZIM, DELTANG, RADELEV,
     $      SINAZ, COSAZ, FSURBIN, NSURBIN, LSURBIN, RNGFCTR,
     $    BSEGCNTX, BAZM_NDX, RADSGCNT, REF_BASE_NDX
/*
**A315CIH
*VERSION: 0
C***************** HAIL ALGORITHM INTERNAL COMMON *******************
C
C** Variable Definitions
C
      REAL HAILTEMP(NHAL_STS-1,NHAL_MAX)
      REAL POSH, POH, MEHS, SHI
      INTEGER NHAILS
C
C** Named Common:
C
      COMMON /A315CIH/ POSH, POH, MEHS, SHI, NHAILS, HAILTEMP
/*
**A315CNCC
*VERSION: 0
C******************** COMPONENTS INTERNAL COMMON ********************
C************************** (A315CNCC.CMN) *************************
C
C** Definitions of per-elevation tables, etc.
C
      LOGICAL  NEWVOL
C
      INTEGER  LEV, NBR_STMS, NBR_STM_COMP
      INTEGER  EL_LAST, NELV_MAX
      PARAMETER (EL_LAST = 1, NELV_MAX = 25)
C
      INTEGER  NCOMPEL( NELV_MAX )
C
      REAL  ELEVTABL(NELV_MAX)
C
C** Named Common:
C
      COMMON /A315CNCC/  NEWVOL, LEV, ELEVTABL, NBR_STMS
      COMMON /A315CNCC/  NBR_STM_COMP, NCOMPEL
/*
**A315COCC
*VERSION: 1
C******************** COMPONENTS INTERNAL COMMON ********************
C************************** (A315CO.CMN) ****************************
C
C** Flagging Parameters:
C
      INTEGER  SCANCOND,  SC_FIRST, SC_MIDDLE
      PARAMETER ( SC_FIRST=1, SC_MIDDLE=2 )
C
C** Variable Definitions:
C
      INTEGER  AZ1, AZC, AZP, AZT, BAZM_NDX, EAZM_NDX
      INTEGER  RINDX, LAB, LABP, LABC, JXC, REF_MAX_NDX, REF_MIN_NDX
      INTEGER  BSEG_NXC, ESEG_NXC
C
      REAL     AZCOUNT, FIRSTAZ, DTHETA, DAZMAVG
C
      LOGICAL  ZERO_CROSS
C
C** Named Common:
C
      COMMON /A315COCC/  SCANCOND, AZ1, AZC, AZP, AZT, RINDX, LAB
      COMMON /A315COCC/  LABP, LABC, JXC, DTHETA, DAZMAVG, AZCOUNT
      COMMON /A315COCC/  BAZM_NDX, EAZM_NDX, REF_MAX_NDX, REF_MIN_NDX
      COMMON /A315COCC/  BSEG_NXC, ESEG_NXC, ZERO_CROSS
      COMMON /A315COCC/  FIRSTAZ
/*
**A315CPPC
*VERSION: 0
C******************** COMPONENTS INTERNAL COMMON ********************
C************************ (A315CPPC.PRM) ****************************
C
C** Dimensional Parameters:
C
      INTEGER  NRDCHR, NRDMAX, NRDAZS, NRDRFLEV
C******************************************************************
C
C**  NRDCHR - Maximum number of segment characteristics
C**  NRDMAX - Maximum number of segments per reflectivity threshold
C**           per radial
C**  NRDAZS - Maximum number of azimuths
C**  NRDRFLEV - Maximum number of reflectivity thresholds
C
C*******************************************************************
      PARAMETER (NRDCHR=3, NRDMAX=50, NRDAZS=3, NRDRFLEV=7)
C
C** Positional Parameters:
C
      INTEGER  RAD_FB, RAD_LB, RAD_ID
      PARAMETER (RAD_FB=1, RAD_LB=2, RAD_ID=3)
C
      INTEGER  INDX1, INDX2, INDX3
      PARAMETER (INDX1=1, INDX2=2, INDX3=3)
C
C** Flagging parameters:
C
      INTEGER  FLG_UNC, FLG_COR, FLG_FIN
      PARAMETER (FLG_UNC=0, FLG_COR=1, FLG_FIN=-1)
/*
**A315CPCC
C
C** Array & Variable definitions:
C
      INTEGER  RADSEG( NRDCHR, NRDMAX, NRDRFLEV, NRDAZS )
      INTEGER  RADFLAG( NRDRFLEV, NRDMAX )
      INTEGER  NRSEG( NRDRFLEV, NRDAZS )
C
      REAL     AZANGLE( NRDAZS )
C
C** Named Common:
C
      COMMON /A315CPCC/  RADSEG, RADFLAG, NRSEG
      COMMON /A315CPCC/  AZANGLE
/*
**A315CQPC
*VERSION: 0
C******************** COMPONENTS INTERNAL COMMON ********************
C************************ (A315CQPC.PRM) ****************************
C
C** Dimensional Parameters:
C
C
      INTEGER  NCFCHR, NCFMAX, NCFRFLEV, NIDHDR, NIDTOT
C******************************************************************
C
C**  NCFCHR - Maximum number of component characteristics
C**  NCFMAX - Maximum number of components per reflectivity threshold
C**           per elevation scan
C**  NCFRFLEV - Maximum number of reflectivity thresholds
C
C*******************************************************************
      PARAMETER (NCFCHR=16, NCFMAX=120, NCFRFLEV=7 )
C
      PARAMETER (NIDHDR=2, NIDTOT=NIDHDR+NCFMAX)
C
C** Positional Parameters:
C
C
      INTEGER  FEA_ARA, FEA_MWA, FEA_XCN, FEA_YCN, FEA_MRF,
     >         FEA_RTH, FEA_CNT, FEA_FAZ, FEA_LAZ, FEA_FZA,
     >         FEA_LZA, FEA_FBN, FEA_LBN, FEA_ZCR, FEA_RNG, FEA_AZM
      PARAMETER (FEA_ARA=1, FEA_MWA=2, FEA_XCN=3, FEA_YCN=4,
     >           FEA_MRF=5, FEA_RTH=6, FEA_CNT=7, FEA_FAZ=8,
     >           FEA_LAZ=9, FEA_FZA=10, FEA_LZA=11, FEA_FBN=12,
     >           FEA_LBN=13, FEA_ZCR=14, FEA_RNG=15, FEA_AZM=16)
/*
**A315CQCC
C******************** COMPONENTS INTERNAL COMMON ********************
C************************ (A315CQCC.CMN) ****************************
C
C** Array & Variable definitions:
C
      INTEGER  IDSTACK( NIDTOT, NCFRFLEV ), COMPCNT( NCFRFLEV )
      INTEGER  MASSRANK( NCFMAX ), COMPINDX( NCFMAX, NCFRFLEV )
      REAL     CMPFEAT( NCFCHR, NCFMAX, NCFRFLEV )
      REAL     ELVCOMP( NCFCHR, NCFMAX )
C
C** Named Common:
C
      COMMON /A315CQCC/  IDSTACK, COMPCNT, CMPFEAT, ELVCOMP
      COMMON /A315CQCC/  MASSRANK, COMPINDX
/*
**A315CRPC
*VERSION: 0
C******************** COMPONENTS INTERNAL COMMON ********************
C************************ (A315CRPC.PRM) ****************************
C
C** Dimensional Parameters:
C
      INTEGER  NVOCHR, NVOMAX, NVOELV
      PARAMETER (NVOCHR=16, NVOMAX=120, NVOELV=25 )
C
C** Positional Parameters:
C
      INTEGER  VCM_ARA, VCM_MWA, VCM_XCN, VCM_YCN, VCM_MRF,
     >         VCM_RTH, VCM_BAZ, VCM_EAZ, VCM_BRG, VCM_ERG,
     >         VCM_CRG, VCM_CAZ, VCM_ACH, VCM_ZCN, VCM_ELV, VCM_LCT
      PARAMETER (VCM_ARA=1, VCM_MWA=2, VCM_XCN=3, VCM_YCN=4,
     >           VCM_MRF=5, VCM_RTH=6, VCM_BAZ=7, VCM_EAZ=8,
     >           VCM_BRG=9, VCM_ERG=10, VCM_CRG=11, VCM_CAZ=12,
     >           VCM_ACH=13, VCM_ZCN=14, VCM_ELV=15, VCM_LCT=16)
/*
**A315CRCC
C******************** COMPONENTS INTERNAL COMMON ********************
C************************ (A315CRCC.CMN) ****************************
C
C** Array & Variable definitions:
C
      INTEGER  NVCOMP( NVOELV )
      REAL     VOLCOMP( NVOCHR, NVOMAX, NVOELV )
C
C** Named Common:
C
      COMMON /A315CRCC/  VOLCOMP, NVCOMP
/*
**A315CSCC
*VERSION: 0
C******************** COMPONENTS INTERNAL COMMON ********************
C************************** (A315CSCC.CMN) **************************
C
C** Array & Variable Definitions:
C
      REAL  DLTAR1, DLTAR2, DLTAR3, PHI, TWOIRRE, SURLEN
      REAL  MWA, MXC, MYC, SLRANGE, SLRANG2, SINPHI, COSPHI
      REAL  COSPHI_DLTAR3
C
C** Data Constants:
C
      PARAMETER (SURLEN = 1000.0, DLTAR1 = 1.0)
      PARAMETER (DLTAR2 = DLTAR1*DLTAR1, DLTAR3 = DLTAR2*DLTAR1)
      PARAMETER (TWOIRRE = 1.0/(2.0*1.21*6371.0))
C
C** Named Common:
C
      COMMON  /A315CSCC/  PHI, COSPHI_DLTAR3, COSPHI, SINPHI
      COMMON /A315CSCC/   MWA, MXC, MYC, SLRANGE, SLRANG2
/*
**A315CTT
*VERSION: 1
C********************* TRACKING INTERNAL COMMON *********************
C************************** (A315CTT.CMN) ***************************
C
      REAL SPEEDINIT
      PARAMETER (SPEEDINIT = -1.0)
C
C** Dimensional Parameters:
C
      INTEGER   NST_LAB, NLAB_HDR, NLAB_TOT
      PARAMETER ( NST_LAB = 260, NLAB_HDR = 2 )
      PARAMETER ( NLAB_TOT = NST_LAB + NLAB_HDR )
C
C** Array & Variable Definitions:
C
      LOGICAL  STARTUP
      REAL     COSPD
      INTEGER  NPVOL, STRMLABL( NLAB_TOT )
C
C** Named Common:
C
      COMMON /A315CTT/  STARTUP, COSPD, NPVOL, STRMLABL
/*
**A315CUPC
*VERSION: 0
C******************** CENTROIDS INTERNAL COMMON ********************
C************************ (A315CUPC.PRM) ****************************
C
C** Dimensional Parameters:
C
      INTEGER  NCELLCHR, NCMPMAX, NCELLMAX
C******************************************************************
C
C**  NCELLCHR - Maximum number of potential storm cell
C**             characteristics
C**  NCMPMAX - Maximum number of components per potential storm cell
C**  NCELLMAX - Maximum number of potential storm cells
C
C*******************************************************************
      PARAMETER (NCELLCHR=17, NCMPMAX=20, NCELLMAX=130 )
C
C** Positional Parameters:
C
      INTEGER  CCM_ARA, CCM_MWA, CCM_XCN, CCM_YCN, CCM_MRF,
     >         CCM_RTH, CCM_BAZ, CCM_EAZ, CCM_BRG, CCM_ERG,
     >         CCM_CRG, CCM_CAZ, CCM_LCB, CCM_ZCN, CCM_ELV, CCM_LCT,
     >         CCM_PVIL
      PARAMETER (CCM_ARA=1, CCM_MWA=2, CCM_XCN=3, CCM_YCN=4,
     >           CCM_MRF=5, CCM_RTH=6, CCM_BAZ=7, CCM_EAZ=8,
     >           CCM_BRG=9, CCM_ERG=10, CCM_CRG=11, CCM_CAZ=12,
     >           CCM_LCB=13, CCM_ZCN=14, CCM_ELV=15, CCM_LCT=16,
     >           CCM_PVIL=17)
/*
**A315CUCC
C******************** CENTROIDS INTERNAL COMMON ********************
C************************ (A315CUCC.CMN) ****************************
C
C** Array & Variable definitions:
C
      INTEGER  NBR_COMP( NCELLMAX ), NBR_DEL_CELL
      REAL     CELLCOMP( NCELLCHR, NCMPMAX, NCELLMAX )
C
C** Named Common:
C
      COMMON /A315CUCC/  CELLCOMP, NBR_COMP, NBR_DEL_CELL
/*
**A315CVPC
*VERSION: 0
C******************** CENTROIDS INTERNAL COMMON ********************
C************************ (A315CVPC.PRM) ****************************
C
C** Dimensional Parameters:
C
      INTEGER  NSTRMCHR, NSTRMMAX
C******************************************************************
C
C**  NSTRMCHR - Maximum number of storm cell characteristics
C**  NSTRMMAX - Maximum number of storm cells
C
C*******************************************************************
      PARAMETER (NSTRMCHR=11, NSTRMMAX=130)
C
C** Positional Parameters:
C
      INTEGER  STC_XCN, STC_YCN, STC_ZCN, STC_MWA, STC_MRF,
     >         STC_MZH, STC_VIL, STC_BAS, STC_TOP, STC_LCT,
     >         STC_LCB
      PARAMETER (STC_XCN=1, STC_YCN=2, STC_ZCN=3, STC_MWA=4,
     >           STC_MRF=5, STC_MZH=6, STC_VIL=7, STC_BAS=8,
     >           STC_TOP=9, STC_LCT=10, STC_LCB=11)
/*
**A315CVCC
C******************** CENTROIDS INTERNAL COMMON ********************
C************************ (A315CVCC.CMN) ****************************
C
C** Array & Variable definitions:
C
      INTEGER  STORM_ID( NSTRMMAX ), CELL_CNT
C
      REAL     STRMCELL( NSTRMCHR, NSTRMMAX ), BEG_VOL_TIME
C
C** Named Common:
C
      COMMON /A315CVCC/  STRMCELL, STORM_ID, CELL_CNT
      COMMON /A315CVCC/  BEG_VOL_TIME
/*
**A315CWCC
C**********COMPONENTS/CENTROIDS DERIVED ADAPTATION DATA*************
C************************ (A315CWCC.CMN) ****************************
C
C** Array & Variable definitions:
C
      INTEGER  ADPSTMMX
C
      REAL     DELTAZM, ADPRDSTH_SQR( MAX_SEARCH_RADIUS )
      REAL     HALF_ADPBWDTH, ADPHZMRG_SQR, ADPHZDEL_SQR
      REAL     TWICE_ADPHZDEL_SQR, ADPBWDTH, ADPVILMX
C
C** Named Common:
C
      COMMON /A315CWCC/  ADPRDSTH_SQR, HALF_ADPBWDTH, ADPHZMRG_SQR
      COMMON /A315CWCC/  ADPHZDEL_SQR, TWICE_ADPHZDEL_SQR, ADPSTMMX
      COMMON /A315CWCC/  ADPBWDTH, ADPVILMX, DELTAZM
/*
