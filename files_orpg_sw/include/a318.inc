XCOMM RCS info 
XCOMM $Author: garyg $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2003/02/19 22:51:36 $ 
XCOMM $Id: a318.inc,v 1.5 2003/02/19 22:51:36 garyg Exp $ 
XCOMM $Revision: 1.5 $ 
XCOMM $State: Exp $ 
*.********************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A318.INC
*.
*.  INCLUDE FILE VERSION: 02
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#          PROGRAMMER         NOTES
*.  ---------    --   -----         ----------------   ---------------
*.  06/01/90     00   80272         P. STALEY
*.  07/11/90     01   90819         G. JUBINSKI
*.  08/07/90     01   90928         M. ISTOK
*.  08/14/90     01   90884         RICH MIHOCI
*.  04/05/95     02   NA94-35301    C. STEPHENSON       **A318PA
*.********************************************************************
**A318P1
*VERSION:0003
C
C      PARAMETERS USED BY HIGH RES SHEAR PRODUCT TASK
C
      INTEGER BEG_EL,END_EL,INTERMED,GOOD,BAD
      INTEGER ALLPRODS,NOBUF,RELEASED,INITVAL,YES,NO
      INTEGER RADSHR,AZISHR,SEVWEA,SHR_BUF_SIZ,HALF_RES_SIZ
      INTEGER BIASVAL,BLOTRES,RNGFOLD
      INTEGER MISSNDAT
      INTEGER WINDOW_OPEN,EST_RADIAL_RLE
C
      PARAMETER (BEG_EL=1   , END_EL=2  , INTERMED=3,
     *           GOOD    =1   , BAD     =2  , ALLPRODS=0 ,
     *           NOBUF   =0   , RELEASED=3  , INITVAL  =0 ,
     *           YES     =1   , NO      =0   ,
     *            WINDOW_OPEN=2)
C
C  EST_RADIAL_RLE IS AN ESTIMATE OF THE 16 BITS WORDS TO
C  RUN LENGTH ENCODE 1 RADIAL OF SHEAR FOR THE 50 KM WINDOW.
C  THIS IS A WORST CASE ESTIMATE WITH NO COMPACTION TAKEN INTO ACCOUNT.
C
      PARAMETER(EST_RADIAL_RLE=50)
C
      PARAMETER (RADSHR  =33  , AZISHR  =34 , SEVWEA  =46,
     *           SHR_BUF_SIZ=920, BIASVAL=102,BLOTRES=0,
     *           RNGFOLD=01, MISSNDAT=256,
     *           HALF_RES_SIZ=460)
C
C    THE PRODUCT BUFFER SIZES IS A FUNCTION OF THE RANGE OF
C    THE CENTER OF THE WINDOW, THE FOLLOWING PARAMETERS ARE
C    THE NUMBER OF I*4 WORDS .  THE SIZES ARE SET INTO THE
C    CONTROL TABLE BY SUBROUTINE A318G2.
C
      INTEGER BUFSIZ1,BUFSIZ2,BUFSIZ3,BUFSIZ4
      PARAMETER(BUFSIZ1=10000,BUFSIZ2=13000,BUFSIZ3=3750)
      PARAMETER(BUFSIZ4=1600)
/*
**A318C1
*VERSION:02
C
C     DEFINE THE INTERNAL CONTROL TABLE FOR HIGH RES SHEAR
C
      INTEGER MAXSHRS,VOLNUM,ELEV_INDX,ELEV_ANG
      INTEGER NUMCTL_PARM,MIN_AZ,MAX_AZ,MIN_RNG,MAX_RNG,CAZ
      INTEGER CRNG,BUFID,MSGID,OPOINT,ELFLAG,BUFCNT,STATUS
      INTEGER MARK,MAXNEG,MAXPOS,NRLEB,NUM_RADLS,ALT_CAT
      INTEGER GENDATE,GENTIME,COLIX,MAXI2_IN_OBUF
      INTEGER NUMPRODS,NUM_RADSHR_PRODS,NUM_TANSHR_PRODS
      PARAMETER(NUMCTL_PARM=22,MAXSHRS=30)
C  INDEX VALUES INTO CONTROL TABLE
      PARAMETER(MIN_AZ  = 1  , MAX_AZ  = 2  ,MIN_RNG  = 3,
     *          MAX_RNG = 4  , CAZ     = 5  ,CRNG     = 6,
     *          BUFID   = 7  , MSGID   = 8  ,OPOINT   = 9,
     *          ELFLAG  =10  , BUFCNT  =11  ,STATUS   =12,
     *          MAXNEG  =13  , MAXPOS  =14  ,NRLEB    =15,
     *          MARK    =16  , NUM_RADLS=17 ,GENDATE  =18,
     *          GENTIME =19  ,  COLIX=20,  MAXI2_IN_OBUF=21)
      PARAMETER(ALT_CAT=22)
C
      INTEGER TBUFCTL(NUMCTL_PARM,MAXSHRS),BUFS(2),BUF_COUNT
      INTEGER WEAMODE
      COMMON/A318GC/TBUFCTL,VOLNUM,ELEV_INDX,ELEV_ANG,WEAMODE,
     *              BUF_COUNT,BUFS,NUMPRODS,
     *              NUM_TANSHR_PRODS,NUM_RADSHR_PRODS
/*
**A318C7
* VERSION: 0003
*
      INTEGER*2 LABELS, GRIDS, CURVE, NYQUIST
      PARAMETER (GRIDS = 8, LABELS = 9, CURVE = 10, NYQUIST = 11)
      INTEGER HTIDX, CVOL, VOLN, IPTR, OPTR, V2_ALT, CLRIDX, CURVCP
      INTEGER ELANG
      REAL MISSING, MIN_VELOC, MAX_VELOC, PPKT, PPDEG, MTKTS
      REAL XMIN, XMAX, YMIN, YMAX, CF1, CF2, CF3
      REAL VEL0PIX, AZ0PIX, HCHSZ, SHW, HWD, SLRAN, RMS, AHWS
      PARAMETER (MTKTS = 1.94384, HCHSZ = 7.0)
      PARAMETER (VEL0PIX = 234.0, AZ0PIX = 71.0, PPDEG = 1.219)
      PARAMETER (XMIN =  72.0, XMAX = 510.0)
      PARAMETER (YMIN =  0.0, YMAX = 467.0)
*
      COMMON /A318C7/ HTIDX, CVOL, VOLN, IPTR, OPTR, V2_ALT, MISSING
      COMMON /A318C7/ CLRIDX, MIN_VELOC, MAX_VELOC, PPKT, CURVCP
      COMMON /A318C7/ ELANG, SHW, HWD, SLRAN, RMS, AHWS
      COMMON /A318C7/ CF1, CF2, CF3
*
/*
**A318PA
*VERSION: 02
      REAL DOR, DXM, DYM, DXS, DYS, FLV, TPOW, THCS, NTH
      REAL MAX_SHEAR, AZIM_MS, RANGE_MS
      INTEGER NROW, NCOL, INC1, ELEV, ELEV_INDX
      INTEGER MSR, NFL, NSV
      PARAMETER(INC1 = 1)
C
      COMMON /A3188A/ DOR, DXM, DYM, DXS, DYS, FLV,
     $              MSR, NFL, MAX_SHEAR, AZIM_MS,
     $              RANGE_MS, NCOL, NROW, NSV, NTH, TPOW, THCS, ELEV,
     $              ELEV_INDX
/*
**A318TVS
*VERSION: 00
C**********************************************************
C*                                                        *
C* X,Y COORDINATES FOR TVS PRODUCT AND RELATED STORM IDS  *
C*                                                        *
C**********************************************************
C     LOCAL COMMON:
C
      INTEGER*4  FEATMX
      PARAMETER (FEATMX = 50)
      INTEGER*2 TVS_STM_ID(FEATMX)
      REAL TVS_XP(FEATMX),TVS_YP(FEATMX),TVS_LAB_XP(FEATMX)
      REAL TVS_LAB_YP(FEATMX)
      COMMON/A318TVS/TVS_XP,TVS_YP,TVS_LAB_XP,TVS_LAB_YP,TVS_STM_ID
C
/*
**A318TVS1
*VERSION: 00
C***********************************************************
C*                                                         *
C* BUFFER INDEX, FLAGS AND FEATURE COUNT FOR TVS PRODUCT   *
C*                                                         *
C***********************************************************
      INTEGER*4 TVSIDX, ITVS, IETVS, FEATCNT
      LOGICAL*4 BFLGTVS,BFLGETVS
      COMMON /TVSINDEX/ TVSIDX, ITVS, IETVS, BFLGTVS, BFLGETVS
     > ,FEATCNT
C
/*