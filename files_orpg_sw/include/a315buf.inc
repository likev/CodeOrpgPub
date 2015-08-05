XCOMM RCS info 
XCOMM $Author: steves $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2012/09/05 16:21:26 $ 
XCOMM $Id: a315buf.inc,v 1.9 2012/09/05 16:21:26 steves Exp $ 
XCOMM $Revision: 1.9 $ 
XCOMM $State: Exp $ 
*.*******************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A315BUF.INC
*.
*.  INCLUDE FILE VERSION: 0002
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#          PROGRAMMER         NOTES
*.  ---------    --   -----         ----------------   --------------
*.  04 11 91     00   90956         DENNIS MILLER      **A315CA
*.  06 13 91     00   92083         DAVID ETRIS
*.  04/05/94     01  NW-94-007-10   STEVE SMITH
*.  05/31/95     02  NA94-26302     STEVE SMITH
*.  06/08/95     02  NA94-26301     MARK FRESCH
*.*******************************************************************
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                                   C
C      THIS FILE CONTAINS OUTPUT BUFFER PARAMETER AND DEFINITION    C
C                          FILES FOR CPC-15                         C
C                                                                   C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
**A315P3S
*VERSION: 2
C********************* SEGMENTS PARAMETER FILE **********************
C
C** Offset Parameters:
C
      INTEGER   BHF, BFP, BNP, BAP, BAE, BDA, BRC, BET, BES
      PARAMETER ( BHF=0, BFP=1, BNP=2, BAP=3, BAE = 4, BDA = 5,
     $            BRC=6, BET = 7 , BES = 8 )
C
C** Buffer-size parameter:
C
      INTEGER   SIZ_SEG
      INTEGER   SIZ_FILT_REFL
      PARAMETER ( SIZ_FILT_REFL = 250000  )
      PARAMETER ( SIZ_SEG = BES + 1  )
C
C** Shared Segment A3CD09 Format Offsets
C
C** Dimensional Parameters:
C
      INTEGER   NSEG_ATR, NSEG_MAX, NSEG_NID, NSEG_REF, NRAD_ELV
     $         , NSTM_ADP
      PARAMETER ( NSEG_ATR=7, NSEG_MAX=6000, NSEG_NID=2,
     $            NSEG_REF=7, NRAD_ELV=400, NSTM_ADP = 57 )
C
C** Positional Parameters:
C
      INTEGER  SEG_LEN, SEG_MWL, SEG_XCN, SEG_YCN, SEG_MRF,
     $         SEG_FBN, SEG_LBN
      PARAMETER (SEG_LEN=1,SEG_MWL=2,SEG_XCN=3,SEG_YCN=4,SEG_MRF=5,
     $        SEG_FBN=6, SEG_LBN=7)
C
/*
**A315P5C
*VERSION: 0
C********************* CENTROIDS PARAMETER FILE *********************
C*************************** (A315P5C.PRM) **************************
C
C** Dimensional Parameters:
C
      INTEGER   NSTM_CHR, NSTM_MAX, NLEV_MAX, NSTK_CHR, NSTK_MAX
      INTEGER   NCMP_MAX, NCEN_ADP
      PARAMETER ( NSTM_CHR=14, NSTM_MAX=100, NLEV_MAX=25, NSTK_CHR=3)
      PARAMETER ( NCMP_MAX = 200 )
      PARAMETER ( NSTK_MAX= NCMP_MAX * NLEV_MAX, NCEN_ADP=57 )
C
C** Positional Parameters:
C
C  STM_AZM - Index to centroid's azimuth in centroid array.
C  STM_RAN - Index to centroid's range in centroids array.
C  STM_XCN - Index to storm cell x-position in an array of centroid data.
C  STM_YCN - Index to storm cell y-position in an array of centroid data.
C  STM_ZCN - Index to storm cell z-position in an array of centroid data.
C  STM_MRF - Index to maximum refelctivity in centroids array.
C  STM_RFH - Index to height in centroids array.
C  STM_VIL - Index to cell based VIL in array of centroid data.
C  STM_NCP - Index to number of componets in centroid array.
C  STM_ENT - Index to first componet in centroid array.
C  STM_TOP - Index to storm top in array of centroid data.
C  STM_LCT - Index to flag value indicating storm top is from highest 
C            elevation cut in centroid array.
C  STM_BAS - Index to centroid's base in centroid array.
C  STM_LCB - Index to flag value indicating storm base is from lowest 
C            elevation cut in centroid array.

      INTEGER  STM_AZM, STM_RAN, STM_XCN, STM_YCN, STM_ZCN
     $        ,STM_MRF, STM_RFH, STM_VIL, STM_NCP, STM_ENT
     $        ,STM_TOP, STM_LCT, STM_BAS, STM_LCB
C
      PARAMETER(STM_AZM=1, STM_RAN=2, STM_XCN=3, STM_YCN=4, STM_ZCN=5
     $        ,STM_MRF=6, STM_RFH=7, STM_VIL=8, STM_NCP=9, STM_ENT=10
     $        ,STM_TOP=11, STM_LCT=12, STM_BAS=13, STM_LCB=14 )
C
      INTEGER STK_ZCN, STK_MRF, STK_PTR
      PARAMETER( STK_ZCN=1, STK_MRF=2, STK_PTR=3 )
C
C** Offset Parameters:
C
      INTEGER   BVT, BNS, BNK, BST, BSK, BSA
      PARAMETER ( BVT=0, BNS=1, BNK=2, BST=3
     $           ,BSK = BST + NSTM_CHR * NSTM_MAX
     $           ,BSA = BSK + NSTK_CHR * NSTK_MAX )
C
C** Buffer-size parameter:
C
      INTEGER   SIZ_CEN
      PARAMETER ( SIZ_CEN = BSA + NCEN_ADP )
/*
**A315P7F
*VERSION: 2
C********************** FORECAST PARAMETER FILE ********************
C*************************** (A315P7F.PRM) *************************
C
C** Dimensional Parameters:
C
      INTEGER   NSTF_IDT, NSTF_MOT, NSTF_FOR, NSTF_BAK, NSTF_MPV,
     $                    NSTF_INT, NSTF_MAX, NSTF_ADP
      PARAMETER ( NSTF_IDT=3, NSTF_MOT=8, NSTF_FOR=2, NSTF_BAK=2,
     $            NSTF_MPV=13, NSTF_INT=4, NSTF_MAX=100, NSTF_ADP=57 )
C
C** Positional Parameters:
C
C**  -- Storm Cell ID-Type table:
C
      INTEGER   STF_ID,  STF_TYP,  STF_NVL
      PARAMETER ( STF_ID=1, STF_TYP=2, STF_NVL=3 )
C
C**  -- Storm Cell Motion table:
C
      INTEGER  STF_X0,  STF_Y0,  STF_XSP, STF_YSP, STF_SPD, STF_DIR
     $        ,STF_ERR, STF_MFE
      PARAMETER ( STF_X0=1, STF_Y0=2, STF_XSP=3, STF_YSP=4, STF_SPD=5
     $        ,STF_DIR=6, STF_ERR=7, STF_MFE=8)
C
C**  -- Storm Cell Forward table:
C
      INTEGER   STF_XF, STF_YF
      PARAMETER ( STF_XF=1, STF_YF=2 )
C
C**  -- Storm Cell Backward table:
C
      INTEGER   STF_XB, STF_YB
      PARAMETER ( STF_XB=1, STF_YB=2 )
C
C** Flagging Parameters:
C
      INTEGER   TYP_CON,   TYP_NEW,   TYP_CHG
      PARAMETER ( TYP_CON=0, TYP_NEW=1, TYP_CHG=2 )
C
      REAL      UNDEF
      PARAMETER ( UNDEF=-999.99 )
C
C** Offset Parameters:
C
      INTEGER   BNT, BNO, BVS, BVD, BSI, BSM, BSF, BSB, BFA
      PARAMETER ( BNT=0, BNO=1, BVS=2, BVD=3, BSI=4
     $           ,BSM = BSI + NSTF_IDT * NSTF_MAX
     $           ,BSF = BSM + NSTF_MOT * NSTF_MAX
     $           ,BSB = BSF + NSTF_FOR * NSTF_INT * NSTF_MAX
     $           ,BFA = BSB + NSTF_BAK * NSTF_MPV * NSTF_MAX)
C
C** Buffer-size parameter:
C
      INTEGER   SIZ_FOR
      PARAMETER ( SIZ_FOR = BFA + NSTF_ADP )
/*
**A315P8T
*VERSION: 0
C***************HAIL TREND/STRUCTURE PARAMETER FILE ****************
C************************** (A315P8T.PRM) ***************************
C
C** Dimensional Parameters:
C
      INTEGER   NSTR_MAX
      PARAMETER ( NSTR_MAX=100 )
C
C** Offset Parameters:
C
      INTEGER   BNR, BRI
      PARAMETER ( BNR=0, BRI=1 )
C
C** Buffer-size parameter:
C
      INTEGER   SIZ_STR
      PARAMETER ( SIZ_STR = BRI + NSTR_MAX)
/*
**A315P9H
*VERSION: 0
C*********************** HAIL PARAMETER FILE ************************
C************************** (A315P9H.PRM) ***************************
C
C** Dimensional Parameters:
C
      INTEGER   NHAL_STS, NHAL_MAX, NHAL_ADP
      PARAMETER ( NHAL_STS=6, NHAL_MAX=100, NHAL_ADP=32 )
C
C** Positional Parameters within HAILSTATS:
C
      INTEGER H_POH
      INTEGER H_PSH
      INTEGER H_MHS
      INTEGER H_RNG
      INTEGER H_AZM
      INTEGER H_CID
      PARAMETER ( H_POH = 1)
      PARAMETER ( H_PSH = H_POH + 1)
      PARAMETER ( H_MHS = H_POH + 2)
      PARAMETER ( H_RNG = H_POH + 3)
      PARAMETER ( H_AZM = H_POH + 4)
      PARAMETER ( H_CID = H_POH + 5)
C
C** Flagging Hail Label Parameters for the RCM:
C
      INTEGER   LAB_POS,   LAB_PRB,   LAB_NEG,   LAB_UNK
      PARAMETER ( LAB_POS=1, LAB_PRB=2, LAB_NEG=3, LAB_UNK=4 )
C
C** Flag value for UNKNOWN or cells out of range
C
      INTEGER UNKNOWN
      PARAMETER (UNKNOWN = -999)
C
C** Offset Parameters within the Output Buffer (HAILATTR):
C
      INTEGER   BNH, BHS, BHL, BHA
      PARAMETER  ( BNH = 0
     $            ,BHS = BNH + 1
     $            ,BHL = BHS + NHAL_STS * NHAL_MAX
     $            ,BHA = BHL + NHAL_MAX )
C
C** Buffer-size parameter:
C
      INTEGER   SIZ_HAL
      PARAMETER ( SIZ_HAL = BHA + NHAL_ADP )
/*
**A315PSAD
*VERSION: 2
C******************************************************************C
C                                                                  C
C    A315PSAD    -  PARAMETER FILE DEFINING LENGTHS AND OFFSETS    C
C                   FOR STORMS ADAPTATION DATA                     C
C                                                                  C
C******************************************************************C
C                                                                  C
C** Total size of STORMS Adaptation Data Buffer:
C
      INTEGER   NSTA_ADP
      PARAMETER ( NSTA_ADP = 57 )
C
C** Maximum number of Reflectivity Levels:
C
      INTEGER   MAX_REF_LEVELS
      PARAMETER ( MAX_REF_LEVELS = 7 )
C
C** Maximum number of search radii.
C
      INTEGER   MAX_SEARCH_RADIUS
      PARAMETER ( MAX_SEARCH_RADIUS = 3 )
C
C** Offset to beginning of primary field of local Adaptation Data:
C
      INTEGER   OFF_STA
      PARAMETER ( OFF_STA = 1 )
C
C** Offsets within primary field of local Adaptation Data:
C
      INTEGER   STA_RLVL
C
      INTEGER   STA_REF1, STA_REF2, STA_REF3, STA_REF4
      INTEGER   STA_REF5, STA_REF6, STA_REF7
C
      INTEGER   STA_SGL1, STA_SGL2, STA_SGL3, STA_SGL4
      INTEGER   STA_SGL5, STA_SGL6, STA_SGL7
C
      INTEGER   STA_CPA1, STA_CPA2, STA_CPA3, STA_CPA4
      INTEGER   STA_CPA5, STA_CPA6, STA_CPA7
C
      INTEGER   STA_NDRO, STA_RDIF, STA_NAVG, STA_MWTF
      INTEGER   STA_MULF, STA_MCOF, STA_SGMX, STA_RGMX
      INTEGER   STA_OVLP, STA_AZMD, STA_SRD1, STA_SRD2
      INTEGER   STA_SRD3, STA_DDEL, STA_HDEL, STA_ELMR
      INTEGER   STA_HEMR, STA_HOMR, STA_MNSG, STA_CMPX
      INTEGER   STA_STMX, STA_VILX, STA_DFDI, STA_DFSP
      INTEGER   STA_MAXT, STA_PVOL, STA_COSP, STA_SPMN
      INTEGER   STA_ALER, STA_FINT, STA_NFOR, STA_ERIN
      INTEGER   STA_RSGM, STA_MXPC, STA_MXDS
C
      PARAMETER ( STA_REF1 = OFF_STA )
      PARAMETER ( STA_REF2 = OFF_STA+1 )
      PARAMETER ( STA_REF3 = OFF_STA+2 )
      PARAMETER ( STA_REF4 = OFF_STA+3 )
      PARAMETER ( STA_REF5 = OFF_STA+4 )
      PARAMETER ( STA_REF6 = OFF_STA+5 )
      PARAMETER ( STA_REF7 = OFF_STA+6 )
      PARAMETER ( STA_SGL1 = OFF_STA+7 )
      PARAMETER ( STA_SGL2 = OFF_STA+8 )
      PARAMETER ( STA_SGL3 = OFF_STA+9 )
      PARAMETER ( STA_SGL4 = OFF_STA+10 )
      PARAMETER ( STA_SGL5 = OFF_STA+11 )
      PARAMETER ( STA_SGL6 = OFF_STA+12 )
      PARAMETER ( STA_SGL7 = OFF_STA+13 )
      PARAMETER ( STA_CPA1 = OFF_STA+14 )
      PARAMETER ( STA_CPA2 = OFF_STA+15 )
      PARAMETER ( STA_CPA3 = OFF_STA+16 )
      PARAMETER ( STA_CPA4 = OFF_STA+17 )
      PARAMETER ( STA_CPA5 = OFF_STA+18 )
      PARAMETER ( STA_CPA6 = OFF_STA+19 )
      PARAMETER ( STA_CPA7 = OFF_STA+20 )
      PARAMETER ( STA_RLVL = OFF_STA+21 )
      PARAMETER ( STA_NDRO = OFF_STA+22 )
      PARAMETER ( STA_RDIF = OFF_STA+23 )
      PARAMETER ( STA_NAVG = OFF_STA+24 )
      PARAMETER ( STA_MWTF = OFF_STA+25 )
      PARAMETER ( STA_MULF = OFF_STA+26 )
      PARAMETER ( STA_MCOF = OFF_STA+27 )
      PARAMETER ( STA_SGMX = OFF_STA+28 )
      PARAMETER ( STA_RGMX = OFF_STA+29 )
      PARAMETER ( STA_OVLP = OFF_STA+30 )
      PARAMETER ( STA_AZMD = OFF_STA+31 )
      PARAMETER ( STA_SRD1 = OFF_STA+32 )
      PARAMETER ( STA_SRD2 = OFF_STA+33 )
      PARAMETER ( STA_SRD3 = OFF_STA+34 )
      PARAMETER ( STA_DDEL = OFF_STA+35 )
      PARAMETER ( STA_HDEL = OFF_STA+36 )
      PARAMETER ( STA_ELMR = OFF_STA+37 )
      PARAMETER ( STA_HEMR = OFF_STA+38 )
      PARAMETER ( STA_HOMR = OFF_STA+39 )
      PARAMETER ( STA_MNSG = OFF_STA+40 )
      PARAMETER ( STA_CMPX = OFF_STA+41 )
      PARAMETER ( STA_STMX = OFF_STA+42 )
      PARAMETER ( STA_VILX = OFF_STA+43 )
      PARAMETER ( STA_DFDI = OFF_STA+44 )
      PARAMETER ( STA_DFSP = OFF_STA+45 )
      PARAMETER ( STA_MAXT = OFF_STA+46 )
      PARAMETER ( STA_PVOL = OFF_STA+47 )
      PARAMETER ( STA_COSP = OFF_STA+48 )
      PARAMETER ( STA_SPMN = OFF_STA+49 )
      PARAMETER ( STA_ALER = OFF_STA+50 )
      PARAMETER ( STA_FINT = OFF_STA+51 )
      PARAMETER ( STA_NFOR = OFF_STA+52 )
      PARAMETER ( STA_ERIN = OFF_STA+53 )
      PARAMETER ( STA_RSGM = OFF_STA+54 )
      PARAMETER ( STA_MXPC = OFF_STA+55 )
      PARAMETER ( STA_MXDS = OFF_STA+56 )
/*
**A315PHAD
*VERSION: 0
C******************************************************************C
C
C    PARAMETER FILE DEFINING LENGTHS AND OFFSETS
C             FOR HAIL ADAPTATION DATA
C
C** Offset to beginning of primary field of local Adaptation Data:
C
      INTEGER   OFF_HA
      PARAMETER ( OFF_HA = 1 )
C
C** Offsets within primary field of local Adaptation Data:
C
      INTEGER HA_H0, HA_H20, HA_KE1, HA_KE2, HA_KE3,
     $       HA_PSC, HA_PSO, HA_HSC, HA_HSE, HA_RWL,
     $       HA_RWU, HA_WTC, HA_WTO, HA_PO1, HA_PO2,
     $       HA_PO3, HA_PO4, HA_PO5, HA_PO6, HA_PO7,
     $   HA_PO8, HA_PO9, HA_PO0, HA_MRP, HA_RHL, HA_SHL, HA_XRG
C
C** Offsets to Hail Temperature Altitude Time/Date Stamp
C
      INTEGER HA_THR, HA_TMN, HA_TDA, HA_TMO, HA_TYR
C
      PARAMETER ( HA_H0 = OFF_HA   )
      PARAMETER ( HA_H20 = OFF_HA+1 )
      PARAMETER ( HA_KE1 = OFF_HA+2 )
      PARAMETER ( HA_KE2 = OFF_HA+3 )
      PARAMETER ( HA_KE3 = OFF_HA+4 )
      PARAMETER ( HA_PSC = OFF_HA+5 )
      PARAMETER ( HA_PSO = OFF_HA+6 )
      PARAMETER ( HA_HSC = OFF_HA+7 )
      PARAMETER ( HA_HSE = OFF_HA+8 )
      PARAMETER ( HA_RWL = OFF_HA+9 )
      PARAMETER ( HA_RWU = OFF_HA+10 )
      PARAMETER ( HA_WTC = OFF_HA+11 )
      PARAMETER ( HA_WTO = OFF_HA+12 )
      PARAMETER ( HA_PO1 = OFF_HA+13 )
      PARAMETER ( HA_PO2 = OFF_HA+14 )
      PARAMETER ( HA_PO3 = OFF_HA+15 )
      PARAMETER ( HA_PO4 = OFF_HA+16 )
      PARAMETER ( HA_PO5 = OFF_HA+17 )
      PARAMETER ( HA_PO6 = OFF_HA+18 )
      PARAMETER ( HA_PO7 = OFF_HA+19 )
      PARAMETER ( HA_PO8 = OFF_HA+20 )
      PARAMETER ( HA_PO9 = OFF_HA+21 )
      PARAMETER ( HA_PO0 = OFF_HA+22 )
      PARAMETER ( HA_MRP = OFF_HA+23 )
      PARAMETER ( HA_RHL = OFF_HA+24 )
      PARAMETER ( HA_SHL = OFF_HA+25 )
      PARAMETER ( HA_XRG = OFF_HA+26 )
      PARAMETER ( HA_THR = OFF_HA+27 )
      PARAMETER ( HA_TMN = OFF_HA+28 )
      PARAMETER ( HA_TDA = OFF_HA+29 )
      PARAMETER ( HA_TMO = OFF_HA+30 )
      PARAMETER ( HA_TYR = OFF_HA+31 )
/*
**A315CSAD
*VERSION: 2
C***************** SCIT ALGORITHMS ADAPTATION DATA ******************
C*************************** (A315CSAD) ****************************
C
C** Array Definitions (equivalent arrays):
C
      INTEGER  STRMADAP( NSTA_ADP )
      REAL     STMRADAP( NSTA_ADP )
C
C** Storm Cell Components Variable Definitions (for equivalencing
C** to arrays):
C
      INTEGER  ADPSEGMN, ADPOVLAP, ADPNLVLS, ADPCMPMX
      INTEGER  ADPRSGMX, ADPMPCMP, ADPMDSTM
      INTEGER  ADPRFLEV(MAX_REF_LEVELS)
C
      REAL     ADPDELAZ, ADPARETH(MAX_REF_LEVELS)
C
C** Storm Cell Centroids Variable Definitions (for equivalencing
C** to arrays):
C
      REAL    ADPHZMRG, ADPELMRG, ADPHTMRG, ADPHZDEL
      REAL    ADPDPDEL, ADPRDSTH(MAX_SEARCH_RADIUS)
C
C** Tracking and Forecast Variable Definitions (for equivalencing
C** to arrays):
C
      INTEGER  ADPNPVOL, ADPNFORC, ADPFCINT, ADPALWER
      INTEGER  ADPERINT, ADPDFDIR, ADPMXTIM
C
      REAL     ADPCOSPD, ADPDFSPD, ADPSPDMN
C
C** Equivalences:
C
C** --Integer & Real arrays...
C
      EQUIVALENCE ( STRMADAP(1), STMRADAP(1) )
C
C** --Individual elements & Integer array...
C
      EQUIVALENCE ( STRMADAP(STA_MNSG),ADPSEGMN )
      EQUIVALENCE ( STRMADAP(STA_OVLP),ADPOVLAP )
      EQUIVALENCE ( STRMADAP(STA_RLVL),ADPNLVLS )
      EQUIVALENCE ( STRMADAP(STA_CMPX),ADPCMPMX )
      EQUIVALENCE ( STRMADAP(STA_REF1),ADPRFLEV(1) )
      EQUIVALENCE ( STRMADAP(STA_PVOL),ADPNPVOL )
      EQUIVALENCE ( STRMADAP(STA_NFOR),ADPNFORC )
      EQUIVALENCE ( STRMADAP(STA_FINT),ADPFCINT )
      EQUIVALENCE ( STRMADAP(STA_ALER),ADPALWER )
      EQUIVALENCE ( STRMADAP(STA_ERIN),ADPERINT )
      EQUIVALENCE ( STRMADAP(STA_DFDI),ADPDFDIR )
      EQUIVALENCE ( STRMADAP(STA_MAXT),ADPMXTIM )
      EQUIVALENCE ( STRMADAP(STA_RSGM),ADPRSGMX )
      EQUIVALENCE ( STRMADAP(STA_MXPC),ADPMPCMP )
      EQUIVALENCE ( STRMADAP(STA_MXDS),ADPMDSTM )
C
C** --Individual elements & Real array...
C
      EQUIVALENCE ( STMRADAP(STA_AZMD),ADPDELAZ )
      EQUIVALENCE ( STMRADAP(STA_CPA1),ADPARETH(1) )
      EQUIVALENCE ( STMRADAP(STA_SRD1),ADPRDSTH(1) )
      EQUIVALENCE ( STMRADAP(STA_HOMR),ADPHZMRG )
      EQUIVALENCE ( STMRADAP(STA_ELMR),ADPELMRG )
      EQUIVALENCE ( STMRADAP(STA_HEMR),ADPHTMRG )
      EQUIVALENCE ( STMRADAP(STA_HDEL),ADPHZDEL )
      EQUIVALENCE ( STMRADAP(STA_DDEL),ADPDPDEL )
      EQUIVALENCE ( STMRADAP(STA_COSP),ADPCOSPD )
      EQUIVALENCE ( STMRADAP(STA_DFSP),ADPDFSPD )
      EQUIVALENCE ( STMRADAP(STA_SPMN),ADPSPDMN )
C
C** Named Common:
C
      COMMON /A315CSAD/  STRMADAP
/*
**A315PTRD
C
C** Cell Trend and Volume Scan List
C
      INTEGER  NSTF_HKT, NSTF_HKV, NSTF_NTF, NSTF_NTV
      PARAMETER ( NSTF_HKT=1, NSTF_HKV=2, NSTF_NTF=8, NSTF_NTV=10 )
C
C**  -- Storm Cell Trend Data list and Volume Scan list:
C
      INTEGER*4 PER_CELL
      INTEGER*4 TREND_SIZE
      INTEGER*4 TREND_SIZE4
      INTEGER*4 VOLUME_SIZE
      INTEGER*4 VOLUME_SIZE4
C
      PARAMETER (PER_CELL = NSTF_HKT + (NSTF_NTV*NSTF_NTF))
      PARAMETER (TREND_SIZE = PER_CELL*NSTF_MAX)
      PARAMETER (TREND_SIZE4 = TREND_SIZE/2 + 1)
      PARAMETER (VOLUME_SIZE = NSTF_HKV+NSTF_NTV)
      PARAMETER (VOLUME_SIZE4 = VOLUME_SIZE/2 + 1)
C
C**  --Offsets used in volume time list.
C
      INTEGER*4 TIMEOFF
      INTEGER*4 NVOL_CVOL
C
      PARAMETER (NVOL_CVOL = 1)
      PARAMETER (TIMEOFF = 1)
C
C**  --Offsets used in cell trend list.
C
      INTEGER*4 NVOLOFF
      INTEGER*4 CTOPOFF
      INTEGER*4 CBASEOFF
      INTEGER*4 MAXZHOFF
      INTEGER*4 POHOFF
      INTEGER*4 POSHOFF
      INTEGER*4 VILOFF
      INTEGER*4 MAXZOFF
      INTEGER*4 CHGTOFF
C
      PARAMETER (NVOLOFF = 0)
      PARAMETER (CTOPOFF = 0)
      PARAMETER (CBASEOFF = CTOPOFF + NSTF_NTV)
      PARAMETER (MAXZHOFF = CBASEOFF + NSTF_NTV)
      PARAMETER (POHOFF = MAXZHOFF + NSTF_NTV)
      PARAMETER (POSHOFF = POHOFF + NSTF_NTV)
      PARAMETER (VILOFF = POSHOFF + NSTF_NTV)
      PARAMETER (MAXZOFF = VILOFF + NSTF_NTV)
      PARAMETER (CHGTOFF = MAXZOFF + NSTF_NTV)
C
C**  --Trend codes in trend data list:
C
      INTEGER*4 CELL_TOP_CODE
      INTEGER*4 CELL_BASE_CODE
      INTEGER*4 MAXZ_HGT_CODE
      INTEGER*4 POH_CODE
      INTEGER*4 POSH_CODE
      INTEGER*4 VIL_CODE
      INTEGER*4 MAXZ_CODE
      INTEGER*4 CENTROID_HGT_CODE
C
      PARAMETER (CELL_TOP_CODE = 1)
      PARAMETER (CELL_BASE_CODE = 2)
      PARAMETER (MAXZ_HGT_CODE = 3)
      PARAMETER (POH_CODE = 4)
      PARAMETER (POSH_CODE = 5)
      PARAMETER (VIL_CODE = 6)
      PARAMETER (MAXZ_CODE = 7)
      PARAMETER (CENTROID_HGT_CODE = 8)
C
      INTEGER*4 MAX_LABELS
      PARAMETER (MAX_LABELS = 260)
C
      INTEGER*4 TPBASOFF
      PARAMETER (TPBASOFF = 1000)
/*
**A315CTRD
C
C**  Cell trend list definition.
C
      INTEGER*2 CELL_TREND_DATA(TREND_SIZE)
      INTEGER*4 CELL_TREND_DATA4(TREND_SIZE4)
      EQUIVALENCE (CELL_TREND_DATA4(1), CELL_TREND_DATA(1))
C
      INTEGER*4 CELL_POINTERS(NSTF_MAX+2)
      INTEGER*2 CELL_TREND_INDEX(MAX_LABELS)
C
      INTEGER*4 VOLUME_COUNTER
      INTEGER*4 TIMEPTR
      INTEGER*2 VOLUME_TIMES(VOLUME_SIZE)
      INTEGER*4 VOLUME_TIMES4(VOLUME_SIZE4)
      EQUIVALENCE (VOLUME_TIMES4(1), VOLUME_TIMES(1))
C
      COMMON/A315TRND/ CELL_TREND_DATA4, CELL_POINTERS
      COMMON/A315TRND/ VOLUME_COUNTER, TIMEPTR
      COMMON/A315TRND/ VOLUME_TIMES, CELL_TREND_INDEX
/*
**A315CHH
*VERSION: 0
C***************** HAIL ALGORITHM ADAPTATION DATA *******************
C
C** Variable Definitions
C
      REAL HT0_MSL, HT20_MSL, HKE_COF1, HKE_COF2, HKE_COF3, HS_COF,
     $   HS_EXP, POSH_COF, WT_OFS, WT_COF,
     $   POHHDTH1, POHHDTH2, POHHDTH3, POHHDTH4, POHHDTH5,
     $   POHHDTH6, POHHDTH7, POHHDTH8, POHHDTH9, POHHDTH0
C
      INTEGER UL_HKE_REF, LL_HKE_REF, MR_POH_TH, RCM_PSTV, RCM_PRBL,
     $     POSH_OFS, MX_HL_RNG
C
C** Hail Temperature Altitude Time/Date Stamp Variables
C
      INTEGER TMP_ALT_HR_STMP, TMP_ALT_MIN_STMP, TMP_ALT_DAY_STMP,
     $     TMP_ALT_MON_STMP, TMP_ALT_YR_STMP
C
C** Derived Adaptation Data
C
      REAL UL_LL_HKE_REF, HT0_ARL, HT20_ARL, HT20_HT0,
     $     P_OFS, P_COF, WT, HKE_CF23
C
C** Named Common:
C
      COMMON /A315CHH/ UL_HKE_REF,LL_HKE_REF,HT0_MSL,HT20_MSL,
     $     HKE_COF1,HKE_COF2,HKE_COF3,HS_COF,HS_EXP,POSH_OFS,
     $     POSH_COF,WT_OFS,WT_COF,RCM_PSTV,RCM_PRBL,MR_POH_TH,
     $     POHHDTH1,POHHDTH2,POHHDTH3,POHHDTH4,POHHDTH5,POHHDTH6,
     $     POHHDTH7,POHHDTH8,POHHDTH9,POHHDTH0,MX_HL_RNG
C
      COMMON /A315CHH/ TMP_ALT_HR_STMP, TMP_ALT_MIN_STMP,
     $   TMP_ALT_DAY_STMP, TMP_ALT_MON_STMP, TMP_ALT_YR_STMP
C
C** Derived Adaptation Data
C
      COMMON /A315CHH/ UL_LL_HKE_REF,HT0_ARL,HT20_ARL,HT20_HT0,
     $     HKE_CF23,P_OFS,P_COF,WT
/*
**A315D4
*VERSION: 0
C**************** COMPONENTS OUTPUT DEFINITION FILE *****************
C*************************** (A315D4.DEF) ***************************
C
C** Array & Variable Definitions:
C
      INTEGER  ELEVSTAT, NUMCOMP
      INTEGER  COMPADAP( NCMP_ADP )
C
      REAL   ELEVTIME, ELEVANG
      REAL   COMPMAIN( NCMP_CHR, NCMP_MAX )
/*
**A315D5
*VERSION: 0
C***************** CENTROIDS OUTPUT DEFINITION FILE *****************
C*************************** (A315D5.DEF) ***************************
C
C** Array & Variable Definitions:
C
      INTEGER  NSTORMS, NCOMPSTK
      INTEGER  CENTADAP( NCEN_ADP )
C
      REAL    STORMAIN( NSTM_CHR, NSTM_MAX )
      REAL   COMPSTAK( NSTK_CHR, NSTK_MAX )
/*
**A315D7
*VERSION: 0
C****************** FORECAST OUTPUT DEFINITION FILE *****************
C*************************** (A315D7.DEF) ***************************
C
C** Array & Variable Definitions:
C
      INTEGER  NTOTPRED, NCONPRED
      INTEGER  STORMIDTYP( NSTF_IDT, NSTF_MAX )
      INTEGER  FORCADAP( NSTF_ADP )
C
      REAL   AVGSPEED,   AVGDIREC
      REAL   STORMOTION( NSTF_MOT, NSTF_MAX )
      REAL   STORMFORW ( NSTF_FOR, NSTF_INT, NSTF_MAX )
      REAL   STORMBACK ( NSTF_BAK, NSTF_MPV, NSTF_MAX )
/*
**A315D8
*VERSION: 0
C***************** TREND OUTPUT DEFINITION FILE ******************
C*************************** (A315D8.DEF) ***************************
C
C** Array & Variable Definitions:
C
      INTEGER  NUMSTORM, STORMID( NSTR_MAX )
C
/*
**A315D9
*VERSION: 1
C******************** HAIL OUTPUT DEFINITION FILE *******************
C*************************** (A315D9.DEF) ***************************
C
C** Array & Variable Definitions:
C
      INTEGER  NUMHAIL
      REAL  HAILSTATS( NHAL_STS, NHAL_MAX )
      INTEGER  HAILABEL( NHAL_MAX )
      INTEGER  HAILADAP( NHAL_ADP )
      REAL  HAIL_RADAP( NHAL_ADP )
C
/*
**A315LOCK
*VERSION: 1
C****************** SEGMENT BUFFER LOCK DEFINITIONS *****************
C
C** Array & Variable Definitions:
C
      INTEGER*4 LOW, HIGH
      PARAMETER (LOW = 1, HIGH = 2)
      LOGICAL   UNAVAILABLE, AVAILABLE
      PARAMETER (UNAVAILABLE = .TRUE., AVAILABLE = .FALSE.)
C
      LOGICAL*4 SEG_BUF_LOCK(HIGH)
C
      COMMON/A315LOCK/ SEG_BUF_LOCK
/*
