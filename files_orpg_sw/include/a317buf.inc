C
C RCS info
C $Author: steves $
C $Locker:  $
C $Date: 2008/04/08 15:17:06 $
C $Id: a317buf.inc,v 1.8 2008/04/08 15:17:06 steves Exp $
C $Revision: 1.8 $
C $State: Exp $
C
*.********************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A317BUF.INC
*.
*.  INCLUDE FILE VERSION: 06
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#          PROGRAMMER         NOTES
*.  ---------    --   -----         ----------------   ---------------
*.  06/01/90     00   80549         P. STALEY
*.  07/11/90     01   90819         G. JUBINSKI
*.  04/07/94     02   92119-001A00  Steve Smith
*.  04/04/95     03   NA94-35301    C. STEPHENSON        A317P6
*.  05/29/97     04   NA95-21503    B.Lee & M.Fresch   added TDA
*.  04/07/00     05   NA00-09501    Steven Smith
*.  08/01/02     06   NA02-20601    Brian Klein        Removed unused blocks      
*.********************************************************************
**A317P5
*VERSION: 2
C ***********************************************************
C *                                                         *
C *      OFFSET PARAMETER FILE FOR MESOCYLONE 3-D           *
C *        OUTPUT BUFFER IN SINGLE INDEX FORMAT.            *
C *                                                         *
C ***********************************************************
C
C *** ARRAY SIZES FOR FEATURE, AND MESOCYCLONE TABLES:
C
      INTEGER FMAX,MMAX
C
      PARAMETER (FMAX=650,MMAX=20)
C
C *** OFFSET PARAMETERS FOR HEADER DATA:
C
      INTEGER VT,NFEAT,NMES,ET,LE_FLAG
C
      PARAMETER (VT=0,NFEAT=1,NMES=2, ET=3, LE_FLAG=4)
C
C *** OFFSET PARAMETERS FOR ADAPTATION DATA:
C
      INTEGER MNF,MNM,FHT,HMT,LMT,HST,LST,MRT,FMRT,NRT,FNRT,
     1        RNGT,DIST,AZT,NPV,SPCT,TVSS
C
      PARAMETER (MNF=5,MNM=6,NPV=7,HMT=8,LMT=9,HST=10,LST=11,
     1           MRT=12,FMRT=13,NRT=14,FNRT=15,RNGT=16,DIST=17,
     2           AZT=18,FHT=19,SPCT=20,TVSS=21)
C
C *** OFFSET PARAMETERS FOR FEATURE TABLE:
C
      INTEGER FTYP,FTF,FCA,FCR,FEA,FH,FAZD,FRD,
     1        FAS,FMS,FASR,FMSR,FT,FB,FBA,FBR,FBEA,
     2        FMXT,FMXT_R,FMXT_BA,FMXT_EA,
     3        FEBR,FEMR,FEBA,FEEA
C
      PARAMETER (FTYP = TVSS+1, FTF = FTYP+FMAX,
     1           FCA = FTYP+2*FMAX,FCR = FTYP+3*FMAX,
     2           FEA = FTYP+4*FMAX, FH = FTYP+5*FMAX,
     3           FAZD = FTYP+6*FMAX,FRD = FTYP+7*FMAX,
     4           FAS = FTYP+8*FMAX,FMS = FTYP+9*FMAX,
     5           FASR = FTYP+10*FMAX,FMSR = FTYP+11*FMAX)
      PARAMETER (FT = FTYP+12*FMAX,FB = FTYP+13*FMAX,
     1           FBA = FTYP+14*FMAX,FBR = FTYP+15*FMAX,
     2           FBEA = FTYP+16*FMAX,FMXT=FTYP+17*FMAX,
     3           FMXT_R=FTYP+18*FMAX,FMXT_BA=FTYP+19*FMAX,
     4           FMXT_EA=FTYP+20*FMAX,FEBR=FTYP+21*FMAX,
     5           FEMR=FTYP+22*FMAX,FEBA=FTYP+23*FMAX,
     6           FEEA=FTYP+24*FMAX)
C
C *** OFFSET PARAMETERS FOR MESOCYCLONE TABLE:
C
      INTEGER MTF,MMAS,MAP,MRP,MEA,MH,MAD,MRD,
     1        MSR,MMSR,MT,MB,MBA,MBR,MBEA,MMXTS,MXTSR,MXTSBA,
     2        MXTSEA,MMEMBR,MMEMER,MMEMBAZ,MMEMEAZ
C
      PARAMETER (MTF = FEEA+FMAX,MMAS = MTF+MMAX,
     1           MAP = MTF+2*MMAX,MRP = MTF+3*MMAX,
     2           MEA = MTF+4*MMAX,MH = MTF+5*MMAX,
     3           MAD = MTF+6*MMAX,MRD = MTF+7*MMAX,
     4           MSR = MTF+8*MMAX,MMSR = MTF+9*MMAX)
      PARAMETER (MT = MTF+10*MMAX,MB = MTF+11*MMAX,
     1           MBA = MTF+12*MMAX,MBR = MTF+13*MMAX,
     2           MBEA = MTF+14*MMAX,MMXTS=MTF+15*MMAX,
     3  MXTSR=MTF+16*MMAX,MXTSBA=MTF+17*MMAX,MXTSEA=MTF+18*MMAX,
     4  MMEMBR=MTF+19*MMAX,MMEMER=MTF+20*MMAX,MMEMBAZ=MTF+21*MMAX,
     5  MMEMEAZ=MTF+22*MMAX)

C
C *** BUFFER SIZE OF PARAMETER FILE FOR MESO 3D:
C
      INTEGER*4 BUFSIZT5
C
      PARAMETER (BUFSIZT5 = MMEMEAZ+MMAX)
/*
**A317PD
*VERSION: 1
C ******************************************************************
C *                                                                *
C *    PARAMETER OFFSET FILE FOR MESOCYCLONE 3D OUTPUT             *
C *              BUFFER IN DOUBLE INDEX FORMAT.                    *
C * (PARAMETER FILE 'A317P5' MUST BE INCLUDED TO DEFINE THIS FILE) *
C *                                                                *
C ******************************************************************
C
C *** OFFSET PARAMETERS FOR HEADER DATA:
C
      INTEGER HDO
C
      PARAMETER (HDO=VT)
C
C *** OFFSET PARAMETERS FOR ADAPTATION PARAMETERS:
C
      INTEGER IAO,AO
C
      PARAMETER (IAO=MNF,AO=HMT)
C
C *** OFFSET PARAMETERS FOR FEATURE TABLE:
C
      INTEGER IFO,FO
C
      PARAMETER (IFO=FTYP,FO=FCA)
C
C *** OFFSET PARAMETERS FOR MESOCYCLONE TABLE:
C
      INTEGER IMO,MO
C
      PARAMETER (IMO=MTF,MO=MMAS)
/*
**A317DD
*VERSION: 3
C
C ***********************************************************
C *                                                         *
C *       DEFINITION FILE FOR MESOCYCLONE 3-D               *
C *       OUTPUT BUFFER IN DOUBLE INDEX FORMAT.             *
C *                                                         *
C ***********************************************************
C
C *** TASK HEADER PARAMETERS:
C
      INTEGER*4 VTIM,NUMFEAT,NUMMES,ETIM,LEF
C
      PARAMETER (VTIM=1,NUMFEAT=2,NUMMES=3,ETIM=4,LEF=5)
C
C *** ADAPTATION PARAMETERS FOR MESOCYCLONE PROCESSING:
C
      INTEGER*4 NPVTHR,MAXNFEAT,MAXNMES
C
      PARAMETER (MAXNFEAT=1,MAXNMES=2,NPVTHR=3)
C
      INTEGER HMTHR,LMTHR,HSTHR,LSTHR,MRTHR,FMRTHR,
     1     NRTHR,FNRTHR,RNGTHR,DISTHR,AZTHR,FHTHR
C
      PARAMETER (HMTHR=1,LMTHR=2,HSTHR=3,LSTHR=4,
     1           MRTHR=5,FMRTHR=6,NRTHR=7,FNRTHR=8,
     2           RNGTHR=9,DISTHR=10,AZTHR=11,FHTHR=12)
C
C *** FEATURE ATTRIBUTE PARAMETER:
C
      INTEGER*4 FTYPE
C
      PARAMETER (FTYPE=1)
C
      INTEGER FCAPOS,FCRPOS,FELANG,FAZDIAM,
     1    FRDIAM,FHGT,FAVST,FMAXST,
     2    FAVSPDR,FMAXSPDR,FTOP,FBOT,FBOTAZ,FBOTRNG,FBOTELA,
     3    FMAXTS,FMAXTS_RANGE,FMAXTS_BEGAZ,FMAXTS_ENDAZ,
     4    FEMBR,FEMER,FEMBAZ,FEMEAZ
C
      PARAMETER (FCAPOS=1,FCRPOS=2,FELANG=3,FHGT=4,
     1           FAZDIAM=5,FRDIAM=6,
     2           FAVST=7,FMAXST=8,FAVSPDR=9,FMAXSPDR=10,
     3           FTOP=11,FBOT=12,FBOTAZ=13,FBOTRNG=14,
     4           FBOTELA=15,
     5           FMAXTS=16,FMAXTS_RANGE=17,FMAXTS_BEGAZ=18,
     6   FMAXTS_ENDAZ=19,FEMBR=20,FEMER=21,FEMBAZ=22,FEMEAZ=23)
C
C *** MESOCYCLONE ATTRIBUTE PARAMETERS:
C
      INTEGER*4 MESTVSF
C
      PARAMETER (MESTVSF=1)
C
      INTEGER MAZPOS,MRPOS,MELANG,MHGT,
     1     MAZDIAM,MRDIAM,
     2     MMAXAS,MSPDR,MMAXSPDR,MTOP,MBOT,MBOTAZ,MBOTRNG,MBOTELA,
     3     MMAXTS,MMAXTS_RANGE,MMAXTS_BEGAZ,MMAXTS_ENDAZ,
     4     MEMBR,MEMER,MEMBAZ,MEMEAZ
C
      PARAMETER (MMAXAS=1,MAZPOS=2,MRPOS=3,MELANG=4,MHGT=5,
     1           MAZDIAM=6,MRDIAM=7,MSPDR=8,MMAXSPDR=9,
     2           MTOP=10,MBOT=11,MBOTAZ=12,MBOTRNG=13,MBOTELA=14,
     3           MMAXTS=15,MMAXTS_RANGE=16,MMAXTS_BEGAZ=17,
     4           MMAXTS_ENDAZ=18,MEMBR=19,MEMER=20,MEMBAZ=21,
     5           MEMEAZ=22)
C
C *** PARAMETER VALUES FOR MAXIMUM NUMBER OF ATTRIBUTES IN TABLES:
C
      INTEGER MAXHD,MAXIADPT,MAXADPT,IFTR_MAXATR,FTR_MAXATR,
     1        IMES_MAXATR,MES_MAXATR
C
      PARAMETER (MAXHD=5,MAXIADPT=3,MAXADPT=14,IFTR_MAXATR=2,
     1           FTR_MAXATR=23,IMES_MAXATR=1,MES_MAXATR=22)
C
C *** TASK HEADER TABLE:
C
      INTEGER HEADER(MAXHD)
C
C *** ADAPTATION PARAMETER TABLES:
C
      INTEGER IADAPT(MAXIADPT)
C
      REAL ADAPT(MAXADPT)
C
C *** FEATURE ATTRIBUTE TABLES:
C
      INTEGER IFTR(FMAX,IFTR_MAXATR)
C
      REAL FTR(FMAX,FTR_MAXATR)
C
C *** MESOCYCLONE ATTRIBUTE TABLES:
C
      INTEGER IMESO(MMAX,IMES_MAXATR)
C
      REAL MESO(MMAX,MES_MAXATR)
/*
**A317P2
*VERSION:0
C**************************************************************
C*                   VAD ALERT                               **
C*                   PARAMETERS                              **
C*                    A317AL2P                               **
C**************************************************************
C
C* Offset parameters
C
C* PARG = Pointer to the lowest level vad range, km (INT*4)
C* PABA = Pointer to the beginning azimuth angle processed, deg(INT*4)
C* PAEA = Pointer to the ending azimuth angle processed, deg (INT*4)
C* PAHT = Pointer to the height of the vad analysis, feet (INT*4)
C* PASP = Pointer to the speed at the lowest height, knots (INT*4)
C* PADR = Pointer to the wind direction at the lowest height, deg(I*4)
C* PAMS = Pointer to the missing data flag, (INT*4)
C*
C* Dimensional parameters
C*
C* LEN_VAD_ALERT = Length (full words) of the vad alert buffer
C
      INTEGER PARG,PABA,PAEA,PAHT,PASP,PADR,PAMS,LEN_VAD_ALERT
      PARAMETER (PARG=1,PABA=2,PAEA=3,PAHT=4,PASP=5,PADR=6,PAMS=7)
      PARAMETER (LEN_VAD_ALERT=7)
/*
**A317D2
*VERSION:0
C**************************************************************
C*                   VAD ALERT                               **
C*                DEFINITION FILE                            **
C*                    A317AL2D                               **
C**************************************************************
C
C* Array and variable definitions.
C
      INTEGER*4 VAD_ALERT(LEN_VAD_ALERT)
/*
**A317VP
*VERSION:1
C***************************************************************
C*                      VAD OUTPUT BUFFER
C*                      PARAMETER FILE
C***************************************************************
C*
C*    THE FOLLOWING PARAMETERS DEFINE DIMENSIONS AND INDICES TO
C*    VAD_DATA
C*
C*    VAD_DAT    = NUMBER OF DATA ITEMS FOR VAD
C*      VAD_HTG  = HEIGHT VALUES IN METERS FOR EACH ELEVATION
C*      VAD_RMS  = RMS VALUES FOR EACH ELEVATION
C*      VAD_HWD  = WIND DIRECTION IN DEGREES FOR EACH ELEVATION
C*      VAD_SHW  = WIND SPEED IN M/S FOR EACH ELEVATION
C*
C*    THE FOLLOWING DEFINE OFFSETS TO VADTMHGT BUFFER
C*
C*    VPDT = OFFSET TO THE DATE OF THE CURRENT VOLUME SCAN
C*    VPCV = OFFSET TO THE CURREN VOLUME SCAN NUMBER
C*    VPTI = OFFSET TO THE TIMES
C*    VPND = OFFSET TO THE MISSING DATA VALUE
C*    VPDA = OFFSET TO THE TABLE OF HEIGHT, RMS, DIRECTION AND
C*           SPEED VALUES
C*
      INTEGER   VAD_DAT, VAD_NAZIMUTHS, VAD_AMBIG_RANGES
      PARAMETER (VAD_DAT=13, VAD_NAZIMUTHS = 400)
      PARAMETER (VAD_AMBIG_RANGES = 3)
      INTEGER VAD_HTG, VAD_RMS, VAD_HWD, VAD_SHW, VAD_NRADS
      INTEGER VAD_AZM, VAD_REF, VAD_VEL, VAD_CF1, VAD_CF2, VAD_CF3
      INTEGER VAD_ARP, VAD_ART, VAD_HT_PARAMS, VAD_AZM_PARAMS
      INTEGER VAD_SLR, VAD_LCN, VAD_ARL
      INTEGER MAX_VAD_VOLS, VAD_AR_PARAMS
      PARAMETER (VAD_HTG = 1, VAD_RMS = 2, VAD_HWD = 3, VAD_SHW = 4)
      PARAMETER (VAD_NRADS=5, VAD_AZM = 1, VAD_REF = 2, VAD_VEL = 3)
      PARAMETER (VAD_CF1 = 6, VAD_CF2 = 7, VAD_CF3 =  8)
      PARAMETER (VAD_ARP = 9, VAD_SLR = 10, VAD_LCN = 11, VAD_ART = 1)
      PARAMETER (VAD_HT_PARAMS = 11, VAD_AZM_PARAMS = 3, VAD_ARL = 2)
      PARAMETER (VAD_AR_PARAMS = 2, MAX_VAD_VOLS = 11)
      INTEGER   VPDT,VPCV,VPTI,VPND,VPDA, VPDAZ, VPDAR
      PARAMETER (VPDT=0,VPCV=1,VPTI=2,VPND=VPTI+MAX_VAD_VOLS)
      PARAMETER (VPDA=VPND+1)
      PARAMETER (VPDAZ = VPDA + MAX_VAD_HTS * VAD_HT_PARAMS)
      PARAMETER (VPDAR=VPDAZ+MAX_VAD_HTS*VAD_NAZIMUTHS*VAD_AZM_PARAMS)
      INTEGER   VAD_OBUF_SZ
      PARAMETER (VAD_OBUF_SZ = VPDA + MAX_VAD_HTS * (VAD_HT_PARAMS +
     +(VAD_AZM_PARAMS*VAD_NAZIMUTHS)+(VAD_AR_PARAMS*VAD_AMBIG_RANGES)))
C*
/*
**A317VD
*VERSION:1
C****************************************************************
C*                         VAD OUTPUT BUFFER
C*                         DEFINITION FILE
C****************************************************************
      INTEGER   VAD_DATE,VAD_CVOL,VAD_TIME(MAX_VAD_VOLS)
      REAL      VAD_MISS,VAD_DATA_HTS(VAD_HT_PARAMS,MAX_VAD_HTS)
      REAL      VAD_DATA_AZM(VAD_AZM_PARAMS,VAD_NAZIMUTHS,MAX_VAD_HTS)
      REAL      VAD_DATA_AR(VAD_AR_PARAMS,VAD_AMBIG_RANGES,MAX_VAD_HTS)
/*
**A317P3T
*VERSION: 0000
C********************* 1DTDA PARAMETER FILE **********************
C
C** Offset Parameters:
C
C*******************************************************************
C  OFFSET     TO       DEFINITION
C  ------  ---------   ---------
C  TBHF    TDAHIFLG    LOGICAL FLAG TO INDICATE WHICH HALF OF THE
C                      TDA1D ATTRIBUTES ARRAY WAS UPDATED MOST RECENT
C  TBPVIP  TDABFPTR    POINTER TO 1ST GOOD PATTERN VECTOR INDEX W/IN
C                      THE TDA1D ATTRIBUTES ARRAY (0 or 3000)
C  TBRIP   TDARIPTR    POINTER TO 1ST GOOD RADIAL INDEX WITHIN THE
C                      TDA1D ATTRIBUTES ARRAY (1 or 401)
C  TBAZP   TDAAZPTR    POINTER TO 1ST GOOD AZIMUTHAL INDEX W/IN THE
C                      TDA1D ATTRIBUTES ARRAY (1 or 401)
C  TBAE    AVG_ELEV    AVG. ELEV ANGLE W/IN THE ELEVATION SCAN (deg)
C  TBPVC   PVCOUNT     # OF PATTERN VECTORS IN THE ELEVATION SCAN
C  TBET    ELV_TIME    BEGINNING TIME OF THE ELEVATION SCAN (msec)
C  TBESF   ELEVSTAT    FLAG OF ELEV. SCAN SEQUENCE W/IN THE VOLUME
C                      SCAN. 1=LAST ELEVATION SCAN, 0=OTHER
C  TBRC    RADCOUNT    # OF RADIALS IN THE ELEVATION SCAN
C  TBBS    DBINSIZE    SIZE OF A DOPPLE BIN (meters)
C
C*******************************************************************
      INTEGER TBHF, TBPVIP, TBRIP, TBAZP, TBAE, TBPVC,
     >        TBET, TBESF, TBRC, TBBS
      PARAMETER ( TBHF=0, TBPVIP=1, TBRIP=2, TBAZP=3)
      PARAMETER(TBAE=4, TBPVC=5, TBET=6, TBESF=7, TBRC=8, TBBS = 9 )
C
C** Buffer-size parameter:
C
      INTEGER   SIZ_TDA
      PARAMETER ( SIZ_TDA = TBBS + 1  )
C
C** Shared TDA1D A3CD09 Format Offsets
C
C** Dimensional Parameters:
C
C*******************************************************************
C
C   N1D_ATR   # OF PATTERN VECTOR ATTRIBUTES IN THE TDAMAIN ARRAY
C   N1D_MAX   MAX # OF PATTERN VECTORS IN (HALF OF) THE TDAMAIN ARRAY
C   N1D_NID   # OF HALVES TO THE TDA1D ATTRIBUTE ARRAY
C   NRAD_ELEV MAX # OF RADIALS IN AN ELEVATION SCAN
C
C*******************************************************************
C
      INTEGER   N1D_ATR, N1D_MAX, N1D_NID, NRAD_ELEV
      INTEGER   PV_DV, PV_BIN, PV_SHR, PV_BAZ
      PARAMETER ( N1D_ATR=4, N1D_MAX=3000, N1D_NID=2, NRAD_ELEV=400)
C
C** Positional Parameters:
C
C*******************************************************************
C
C   PV_DV   PATTERN VECTOR DELTA VELOCITY (m/s)
C   PV_BIN  PATTERN VECTOR BIN #
C   PV_SHR  PATTERN VECTOR SHEAR (m/s/km)
C   PV_BAZ  PATTERN VECTOR BEGINNING AZIMUTH (deg)
C
C*******************************************************************
C
      PARAMETER (PV_DV=1, PV_BIN=2, PV_SHR=3, PV_BAZ=4)
/*
**A317PTAD
*VERSION: 0000
C******************************************************************C
C                                                                  C
C    A317PTAD    -  PARAMETER FILE DEFINING LENGTHS AND OFFSETS    C
C                   FOR TDA ADAPTATION DATA                        C
C     Created on 4/2/97 by R. Lee                                  C
C                                                                  C
C******************************************************************C
C
C** Total size of TDA Adaptation Data Buffer:
C
      INTEGER*4   NTDA_AD
      PARAMETER ( NTDA_AD = 30 )
C
C** Maximum number of Delta Velocity Levels:
C
C     INTEGER*4   MAX_DV_LEVELS
C     PARAMETER ( MAX_DV_LEVELS = 6 )
C
C** Offset to beginning of primary field of local Adaptation Data:
C
      INTEGER*4   OFF_TDA
      PARAMETER ( OFF_TDA = 1 )
C
C** Offsets within primary field of local Adaptation Data:
C
      INTEGER*4   TDA_DV1, TDA_DV2, TDA_DV3, TDA_DV4
      INTEGER*4   TDA_DV5, TDA_DV6
      INTEGER*4   TDA_MDV
C
      INTEGER*4   TDA_MFAR, TDA_MPVR, TDA_CR1, TDA_CR2
      INTEGER*4   TDA_CRR, TDA_MTBH, TDA_MTBE
C
      INTEGER*4   TDA_MPVH, TDA_MPVF, TDA_MTED, TDA_MLDV
      INTEGER*4   TDA_MTDV, TDA_MVRD, TDA_MVAD, TDA_MRV
C
      INTEGER*4   TDA_SAD, TDA_MFPD, TDA_MN2D, TDA_MN3D
      INTEGER*4   TDA_MNPV, TDA_MNET, TDA_MNT, TDA_MADH
C
      PARAMETER ( TDA_DV1 = OFF_TDA )
      PARAMETER ( TDA_DV2 = OFF_TDA+1 )
      PARAMETER ( TDA_DV3 = OFF_TDA+2 )
      PARAMETER ( TDA_DV4 = OFF_TDA+3 )
      PARAMETER ( TDA_DV5 = OFF_TDA+4 )
      PARAMETER ( TDA_DV6 = OFF_TDA+5 )
      PARAMETER ( TDA_MDV = OFF_TDA+6 )
      PARAMETER ( TDA_MFAR = OFF_TDA+7 )
      PARAMETER ( TDA_MPVR = OFF_TDA+8 )
      PARAMETER ( TDA_CR1 = OFF_TDA+9 )
      PARAMETER ( TDA_CR2 = OFF_TDA+10 )
      PARAMETER ( TDA_CRR = OFF_TDA+11 )
      PARAMETER ( TDA_MTBH = OFF_TDA+12 )
      PARAMETER ( TDA_MTBE = OFF_TDA+13 )
      PARAMETER ( TDA_MPVH = OFF_TDA+14 )
      PARAMETER ( TDA_MPVF = OFF_TDA+15 )
      PARAMETER ( TDA_MTED = OFF_TDA+16 )
      PARAMETER ( TDA_MLDV = OFF_TDA+17 )
      PARAMETER ( TDA_MTDV = OFF_TDA+18 )
      PARAMETER ( TDA_MVRD = OFF_TDA+19 )
      PARAMETER ( TDA_MVAD = OFF_TDA+20 )
      PARAMETER ( TDA_MRV = OFF_TDA+21 )
      PARAMETER ( TDA_SAD = OFF_TDA+22 )
      PARAMETER ( TDA_MFPD = OFF_TDA+23 )
      PARAMETER ( TDA_MN2D = OFF_TDA+24 )
      PARAMETER ( TDA_MN3D = OFF_TDA+25 )
      PARAMETER ( TDA_MNPV = OFF_TDA+26 )
      PARAMETER ( TDA_MNET = OFF_TDA+27 )
      PARAMETER ( TDA_MNT = OFF_TDA+28 )
      PARAMETER ( TDA_MADH = OFF_TDA+29 )
/*
**A317LOCK
*VERSION: 0000
C************** TDA1D BUFFER LOCK SIZING DEFINITIONS **************
C
C** Array & Variable Definitions:
C
      INTEGER*4 LOW, HIGH
      PARAMETER (LOW = 1, HIGH = 2)
      LOGICAL   UNAVAILABLE, AVAILABLE
      PARAMETER (UNAVAILABLE = .TRUE., AVAILABLE = .FALSE.)
/*
**A317CLOK
*VERSION: 0000
C****************** TDA1D BUFFER LOCK DEFINITIONS *****************
C
C** Array & Variable Definitions:
C
C
      LOGICAL*4 TDA_BUF_LOCK(HIGH)
C
      COMMON/A317LOCK/ TDA_BUF_LOCK
/*
**A317CTAD
*VERSION: 0000
C***************** TDA ALGORITHMS ADAPTATION DATA ******************
C*************************** (A317CTAD) ****************************
C
C** Array Definitions (equivalent arrays):
C
C     INTEGER*4  TDAADAP( 0:NTDA_AD-1 )
C     REAL     TADADAP( 0:NTDA_AD-1 )
      INTEGER*4  TDAADAP( NTDA_AD )
      REAL     TADADAP( NTDA_AD )
C
C** TDA 1D Variable Definitions (for equivalencing to arrays):
C
      INTEGER*4  ADPMDV,ADPMPVR, ADPMRV, ADPMNPV
C
C** TDA 2D Variable Definitions (for equivalencing to arrays):
C
      INTEGER*4 ADPDV1, ADPDV2, ADPDV3, ADPDV4, ADPDV5, ADPDV6
      INTEGER*4 ADPCRR, ADPMPVF, ADPMFPD, ADPMN2D
C
      REAL  ADPMFAR, ADPMVRD, ADPMVAD, ADPCR1, ADPCR2, ADPMPVH
C
C** TDA 3D Variable Definitions (for equivalencing to arrays):
C
      INTEGER*4  ADPMLDV, ADPMTDV
      INTEGER*4  ADPMN3D, ADPMNET, ADPMNT
C
      REAL    ADPMTED, ADPMTBH, ADPMTBE, ADPSAD, ADPMADH
C
C** Equivalences:
C
C** --Integer & Real arrays...
C
      EQUIVALENCE ( TDAADAP(1), TADADAP(1) )
C
C** --Individual elements & Integer array...
C
      EQUIVALENCE ( TDAADAP(TDA_MDV),ADPMDV )
      EQUIVALENCE ( TDAADAP(TDA_MPVR),ADPMPVR )
      EQUIVALENCE ( TDAADAP(TDA_MRV),ADPMRV )
      EQUIVALENCE ( TDAADAP(TDA_MNPV),ADPMNPV )
      EQUIVALENCE ( TDAADAP(TDA_DV1),ADPDV1 )
      EQUIVALENCE ( TDAADAP(TDA_DV2),ADPDV2 )
      EQUIVALENCE ( TDAADAP(TDA_DV3),ADPDV3 )
      EQUIVALENCE ( TDAADAP(TDA_DV4),ADPDV4 )
      EQUIVALENCE ( TDAADAP(TDA_DV5),ADPDV5 )
      EQUIVALENCE ( TDAADAP(TDA_DV6),ADPDV6 )
      EQUIVALENCE ( TDAADAP(TDA_CRR),ADPCRR )
      EQUIVALENCE ( TDAADAP(TDA_MN2D),ADPMN2D )
      EQUIVALENCE ( TDAADAP(TDA_MPVF),ADPMPVF )
      EQUIVALENCE ( TDAADAP(TDA_MFPD),ADPMFPD )
      EQUIVALENCE ( TDAADAP(TDA_MN3D),ADPMN3D )
      EQUIVALENCE ( TDAADAP(TDA_MLDV),ADPMLDV )
      EQUIVALENCE ( TDAADAP(TDA_MNET),ADPMNET )
      EQUIVALENCE ( TDAADAP(TDA_MNT),ADPMNT )
      EQUIVALENCE ( TDAADAP(TDA_MTDV),ADPMTDV )
C
C** --Individual elements & Real array...
C
      EQUIVALENCE ( TADADAP(TDA_MPVH),ADPMPVH )
      EQUIVALENCE ( TADADAP(TDA_MFAR),ADPMFAR )
      EQUIVALENCE ( TADADAP(TDA_MVRD),ADPMVRD )
      EQUIVALENCE ( TADADAP(TDA_MVAD),ADPMVAD )
      EQUIVALENCE ( TADADAP(TDA_CR1),ADPCR1 )
      EQUIVALENCE ( TADADAP(TDA_CR2),ADPCR2 )
      EQUIVALENCE ( TADADAP(TDA_MTED),ADPMTED )
      EQUIVALENCE ( TADADAP(TDA_MTBH),ADPMTBH )
      EQUIVALENCE ( TADADAP(TDA_MTBE),ADPMTBE )
      EQUIVALENCE ( TADADAP(TDA_SAD),ADPSAD )
      EQUIVALENCE ( TADADAP(TDA_MADH),ADPMADH )
C
C** Named Common:
C
      COMMON /A317CTAD/  TDAADAP
C
/*
**A317TP9
*VERSION: 0000
C********************* TDA 2D&3D PARAMETER FILE *********************
C**************************** (A317TP9) *****************************
C
C** TVSATTR Output Buffer Dimensional Parameters
C
C*******************************************************************
C
C   TVFEAT_CHR  # OF TVS FEATURE ATTRIBUTES IN TVS_MAIN
C   TVFEAT_MAX  MAXIMUM # OF TVS FEATURES (TVSs + ETVSs) IN TVS_MAIN
C   NTDA_ADP    # OF TDA ADAPTABLE PARAMETERS IN THE TVSATTR BUFFER
C
C*******************************************************************
C
      INTEGER*4 TVFEAT_CHR, TVFEAT_MAX, NTDA_ADP
      PARAMETER ( TVFEAT_CHR=13, TVFEAT_MAX=50, NTDA_ADP=30 )
C
C***  POINTERS FOR TVSATTR_MAIN ARRAY AND TVS3D ARRAY
C
C**  PARAMETER DEFINITIONS FOR TVS FEATURES
C
C*******************************************************************
C
C  TV_TYP  TVS FEATURE TYPE: TVS=1, ETVS=2
C  TV_AZM  AZIMUTH OF FEATURE BASE (deg)
C  TV_RAN  SLANT RANGE OF FEATURE BASE (km)
C  TV_LDV  LOW-LEVEL DELTA VELOCITY (m/s)
C  TV_ADV  AVERAGE DELTA VELOCITY (m/s)
C  TV_MDV  MAXIMUM DELTA VELOCITY (m/s)
C  TV_MVH  HEIGHT OF MAXIMUM DELTA VELOCITY (km)
C  TV_DEP  DEPTH OF FEATURE, NEGATIVE MEANS TOP OR BASE IS ON HIGHEST
C          OR LOWEST ELEVATION SCAN, RESPECTIVELY (km)
C  TV_BAS  BASE HEIGHT (km)
C  TV_TOP  TOP HEIGHT (km)
C  TV_SHR  MAXIMUM SHEAR (m/s/km)
C  TV_SRH  HEIGHT OF MAXIMUM SHEAR (km)
C
C*******************************************************************
C
      INTEGER TV_TYP, TV_AZM, TV_RAN, TV_LDV, TV_ADV
      INTEGER TV_MDV, TV_MVH, TV_DEP, TV_BEL, TV_BAS
      INTEGER TV_TOP, TV_SHR, TV_SRH
C
      PARAMETER (TV_TYP = 1, TV_AZM = 2, TV_RAN = 3, TV_LDV= 4)
      PARAMETER (TV_ADV = 5, TV_MDV = 6, TV_MVH = 7, TV_DEP = 8)
      PARAMETER (TV_BAS = 9, TV_BEL =10, TV_TOP =11, TV_SHR =12)
      PARAMETER (TV_SRH =13)
C
C** Offset Parameters:
C
      INTEGER*4 TVT, TVD, TNT, TNE, TAM, TAD
      PARAMETER ( TVT=0, TVD=1, TNT=2, TNE=3, TAM=4,
     $            TAD = TAM + TVFEAT_CHR * TVFEAT_MAX)
C
C** Buffer-size parameter:
C** Must add ONE to size since Buffer Offsets start at ZERO.
C
      INTEGER*4   SIZ_TVS
      PARAMETER ( SIZ_TVS = TAD + NTDA_ADP + 1)
/*
**A317D4
*VERSION: 0
C**************** TDA 2D & 3D OUTPUT DEFINITION FILE ****************
C
C** Array & Variable Definitions:
C
C*******************************************************************
C
C   VOLTIME   BEGINNING OF VOLUME SCAN TIME (msecs since midnight)
C   VOLDATE   BEGINNING OF VOLUME SCAN DATE (Julian)
C   NTVFEAT   # OF TVSes; NEGATIVE MEANS MAX. ADAPTABLE WAS EXCEEDED
C   NETFEAT   # OF ETVSes; NEGATIVE MEANS MAX. ADAPTABLE WAS EXCEEDED
C   TVS_MAIN  TVS FEATURE ATTRIBUTE ARRAY
C
C*******************************************************************
C
      INTEGER  VOLTIME, VOLDATE, NTVFEAT, NETFEAT
      REAL    TVS_MAIN( TVFEAT_CHR, TVFEAT_MAX )
/*
