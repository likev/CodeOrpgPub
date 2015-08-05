XCOMM RCS info 
XCOMM $Author: steves $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2007/03/02 22:39:30 $ 
XCOMM $Id: a308buf.inc,v 1.6 2007/03/02 22:39:30 steves Exp $ 
XCOMM $Revision: 1.6 $ 
XCOMM $State: Exp $
*.********************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A308BUF.INC
*.
*.  INCLUDE FILE VERSION: 03
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#          PROGRAMMER         NOTES
*.  ---------    --   -----         ----------------   --------------
*.  08 09 90     00   90869         PHIL STALEY
*.  08 05 92     01   NA92-14002    VALERIE HOLDEN
*.  05 03 94     02   NW-93-313-03  STEVE SMITH        USER_EBUFS SIZE
*.  05 24 95     03   NA94-26301    STEVE SMITH
*.  05 24 95     03   NA94-26303    STEVE SMITH
*.  08 14 97     04   NA95-21503    STEVE SMITH
*.  10 24 05     05   NA05-23001    Brian Klein        **A308Q3
*.********************************************************************
**A30800
*VERSION:3
C********************************************************************
C*
C*  ALERT GRID DEFINITIONS
C*
C*
C********************************************************************
C*
      INTEGER MAX_ALERT_CATS,HW_PER_CAT,NUM_ALRT_ROWS,HW_PER_ROW
      INTEGER NUM_ALRT_AREAS,MAX_CLASS1,GRID_CHNG,BIT_OFFSET
      INTEGER TOTAL_MAPS
      INTEGER NACT_SCNS, NWDS_MAX_CLASS1
      INTEGER NBUFIDS
C
C
      PARAMETER( MAX_ALERT_CATS = 10 )
      PARAMETER( HW_PER_CAT = 3      )
      PARAMETER( NUM_ALRT_ROWS = 58  )
      PARAMETER( HW_PER_ROW = 4      )
      PARAMETER( NUM_ALRT_AREAS = 2  )
      PARAMETER( MAX_CLASS1 = 41     )
      PARAMETER( NWDS_MAX_CLASS1 = 2 )
      PARAMETER( BIT_OFFSET = MAX_ALERT_CATS*HW_PER_CAT )
      PARAMETER ( TOTAL_MAPS = NUM_ALRT_AREAS*MAX_CLASS1 )
      PARAMETER (NACT_SCNS=2,NBUFIDS=9)

C
C* USER ALERT GRID CHANGE FLAG ( 1 = CHANGED, 0 = UNCHANGED )
C
      PARAMETER ( GRID_CHNG = 1 )
      INTEGER*2 USER_CHG_FLG(NUM_ALRT_AREAS,MAX_CLASS1)
C
      INTEGER*2 ALERT_GRID(MAX_ALERT_CATS*HW_PER_CAT +
     $                     NUM_ALRT_ROWS*HW_PER_ROW,
     $                     MAX_CLASS1,NUM_ALRT_AREAS )
      INTEGER LNXREF(MAX_CLASS1)
      INTEGER  UAPTR (MAX_CLASS1,NUM_ALRT_AREAS,NACT_SCNS)
      INTEGER ACTIVE_SCNS(NACT_SCNS)
      INTEGER BUFS_EXPECTED(NBUFIDS,NACT_SCNS)
      INTEGER BUFS_DONE (NBUFIDS,NACT_SCNS)
      INTEGER NUM_EBUFS(NACT_SCNS)
      INTEGER NUM_DBUFS(NACT_SCNS)
      INTEGER USER_EBUFS(NWDS_MAX_CLASS1,NBUFIDS,NACT_SCNS)
C
      COMMON /A3CD20/ ALERT_GRID,USER_CHG_FLG
      COMMON /A3CD20/ UAPTR ,ACTIVE_SCNS
      COMMON/A3CD20/ BUFS_EXPECTED,BUFS_DONE,NUM_EBUFS,NUM_DBUFS
      COMMON/A3CD20/ USER_EBUFS,LNXREF
/*
**A308P3
*VERSION:3
C******************************************************************
C*                    COMBINED ATTRIBUTES                         *
C*                       PARAMETERS                               *
C*                         A308P3                                 *
C******************************************************************
C
C
C  The following parameters define dimensions and indices to CAT_FEAT:
C
C            CAT_MXSTMS: Maximum no. of Storm Cells allowed in buffer
C            CAT_NF:     Number of Features per Storm Cell
C               CAT_SID  = Index for Storm ID
C               CAT_TVS  = Index for TVS  flag
C               CAT_POH  = Index for Probability Of Hail
C               CAT_POSH = Index for Probability Of Severe Hail
C               CAT_MEHS = Index for Maximum Expected Hail Size
C               CAT_HAIL = Index for Hail Potential Label
C               CAT_TYPE = Index for Storm Cell Type ("New"/"Cont")
C               CAT_MDA  = Index for Mesocyclone Detection Algorithm
C
      INTEGER  CAT_MXSTMS, CAT_NF
      INTEGER  CAT_SID, CAT_TVS, CAT_POH, CAT_POSH
      INTEGER  CAT_MEHS, CAT_HAIL, CAT_TYPE, CAT_MDA
C
      PARAMETER (CAT_MXSTMS=100, CAT_NF=8)
      PARAMETER (CAT_SID=1, CAT_TVS=2, CAT_POH=3)
      PARAMETER (CAT_POSH=4, CAT_MEHS=5, CAT_HAIL=6, CAT_TYPE=7)
      PARAMETER (CAT_MDA=8)
C
C
C  The following parameters define dimensions and indices to COMB_ATT:
C
C            CAT_DAT:  Number of Attributes per Storm Cell
C               CAT_AZ    = Index for Azimuth of Centroid
C               CAT_RNG   = Index for Slant Range of Centroid
C               CAT_HCN   = Index for Height (ARL) of Centroid
C               CAT_ELCN  = Index for Elevation of Centroid
C               CAT_SBS   = Index for Height (ARL) of Storm Cell Base
C               CAT_MXZ   = Index for Max Reflectivity of Storm Cell
C               CAT_HMXZ  = Index for Height of Max Reflectivity
C               CAT_ELMXZ = Index for Elevation of Max Reflectivity
C               CAT_VIL   = Index for Cell Based VIL
C               CAT_STP   = Index for Storm Top Height (ARL)
C               CAT_FDIR  = Index for Forecast Storm Cell Direction
C               CAT_FSPD  = Index for Forcast Storm Cell Speed
C               CAT_AZTVS = Index for Azimuth of TVS Base
C               CAT_RNTVS = Index for Range of TVS Base
C               CAT_ELTVS = Index for Elevation of TVS Base
C               CAT_AZMDA = Index for Azimuth of MDA feature Base
C               CAT_RNMDA = Index for Range of MDA feature Base
C               CAT_ELMDA = Index for Elevation of MDA feature Base
C               CAT_SRMDA = Index for Strength Rank of MDA feature
C
      INTEGER  CAT_DAT
      INTEGER  CAT_AZ, CAT_RNG, CAT_HCN, CAT_ELCN
      INTEGER  CAT_SBS, CAT_MXZ, CAT_HMXZ, CAT_ELMXZ
      INTEGER  CAT_VIL, CAT_STP, CAT_FDIR, CAT_FSPD
      INTEGER  CAT_AZTVS, CAT_RNTVS, CAT_ELTVS, CAT_AZMDA 
      INTEGER  CAT_RNMDA, CAT_ELMDA, CAT_SRMDA
C
      PARAMETER (CAT_DAT=19)
      PARAMETER (CAT_AZ=1, CAT_RNG=2, CAT_HCN=3, CAT_ELCN=4)
      PARAMETER (CAT_SBS=5, CAT_MXZ=6, CAT_HMXZ=7, CAT_ELMXZ=8)
      PARAMETER (CAT_VIL=9, CAT_STP=10, CAT_FDIR=11, CAT_FSPD=12)
      PARAMETER (CAT_AZTVS=13, CAT_RNTVS=14, CAT_ELTVS=15)
      PARAMETER (CAT_AZMDA=16, CAT_RNMDA=17, CAT_ELMDA=18, CAT_SRMDA=19)
C
C
C  The following parameters define FORCST_POSITS:
C
C            MAX_FPOSITS: Max # of Forecast Positions per storm Cell
C            FOR_DAT:     No. of Descriptors per forecast position
C                 CAT_FX  = X Index of the forecast position
C                 CAT_FY  = Y Index of the forecast position
C
      INTEGER  FOR_DAT, MAX_FPOSITS
      INTEGER  CAT_FX,  CAT_FY
C
      PARAMETER (CAT_FX=1,  CAT_FY=2)
      PARAMETER (FOR_DAT=2, MAX_FPOSITS=4)
C
C  THE FOLLOWING PARAMETERS DEFINE THE RADAR CODED MESSAGE PARAMETERS
C  FOR 2D UNCORRELATED SHEAR AND TVS
C
C      CAT_RCM:      NUMBER OF TOTAL DATA FIELDS FOR RCM
C         RCM_TVS    INDEX FOR TOTAL NUMBER OF TVS
C         RCM_MDA    INDEX FOR TOTAL NUMBER OF MDA FEATURES
C
C      CAT_NTVS:     NUMBER OF ATTRIBUTES PER TVS
C         CAT_TVSAZ  INDEX FOR AZIMUTH OF TVS BASE
C         CAT_TVSRN  INDEX FOR RANGE OF TVS BASE
C         CAT_TVSEL  INDEX FOR ELEVATION OF TVS BASE
C
C      CAT_NMDA:     NUMBER OF ATTRIBUTES PER MDA FEATURE
C         CAT_MDAAZ  INDEX FOR AZIMUTH OF MDA FEATURE BASE
C         CAT_MDARN  INDEX FOR RANGE OF MDA FEATURE BASE
C         CAT_MDAEL  INDEX FOR ELEVATION OF MDA FEATURE BASE
C         CAT_MDASR  INDEX FOR STRENGTH RANK OF MDA FEATURE
C
      INTEGER CAT_RCM,RCM_TVS,RCM_2D,RCM_MDA
      PARAMETER (CAT_RCM=2,RCM_TVS=1,RCM_MDA=2)
C*
      INTEGER CAT_NTVS,CAT_TVSAZ,CAT_TVSRN,CAT_TVSEL
      PARAMETER (CAT_NTVS=3,CAT_TVSAZ=1,CAT_TVSRN=2,CAT_TVSEL=3)
C*
      INTEGER CAT_NMDA,CAT_MDAAZ,CAT_MDARN,CAT_MDAEL,CAT_MDASR
      PARAMETER (CAT_NMDA=4,CAT_MDAAZ=1,CAT_MDARN=2,CAT_MDAEL=3)
      PARAMETER (CAT_MDASR=4)

C  The following parameter is a flag indicating No Data in a field:
C
      REAL  NODATA
      PARAMETER (NODATA=-999.99)
C
C  The following parameters define the offsets from the top
C  of the Combined Attributes buffer for the preceding data:
C
C  (The data exists in the output buffer in the following order:
C     CAT_NUM_STORMS
C     CAT_FEAT
C     COMB_ATT
C     NUM_FPOSITS
C     FORCST_POSITS
C     CAT_NUM_RCM
C     CAT_TVST
C     CAT_MDAT)
C
      INTEGER CNS, CFEA, CATT, CNFP, CNFST, CNRCM, CNTVS
      INTEGER CNMDA
      PARAMETER (CNS=0, CFEA=1 )
      PARAMETER (CATT=CFEA+CAT_NF*CAT_MXSTMS)
      PARAMETER (CNFP=CATT+CAT_DAT*CAT_MXSTMS)
      PARAMETER (CNFST=CNFP+1)
      PARAMETER (CNRCM = CNFST + (FOR_DAT * MAX_FPOSITS * CAT_MXSTMS))
      PARAMETER (CNTVS = CNRCM + CAT_RCM)
      PARAMETER (CNMDA = CNTVS + (CAT_NTVS * CAT_MXSTMS))
C
C The following parameter yields the total output buffer size (I*4):
C
      INTEGER CAT_OBUF_SIZ
      PARAMETER (CAT_OBUF_SIZ = CNMDA + CAT_NMDA * CAT_MXSTMS)
/*
**A308D3
*VERSION:3
C******************************************************************
C*                        COMBINED ATTRIBUTES                    **
C*                        DEFINITIONS FILE                       **
C*                            A308D3                             **
C******************************************************************
C
C
      INTEGER  CAT_NUM_STORMS
      INTEGER  CAT_FEAT( CAT_NF,CAT_MXSTMS )
      REAL     COMB_ATT( CAT_DAT,CAT_MXSTMS )
      INTEGER  NUM_FPOSITS
      REAL     FORCST_POSITS( FOR_DAT,MAX_FPOSITS,CAT_MXSTMS )
      INTEGER  CAT_NUM_RCM(CAT_RCM)
      REAL     CAT_TVST(CAT_NTVS,CAT_MXSTMS)
      REAL     CAT_MDAT(CAT_NMDA,CAT_MXSTMS)
/*
**A308Q3
*VERSION: 2
C    These are parameters used within the Combined Attributes task.
C
C  Parameters for sizing and flagging STORM_FEATS:
      INTEGER  FEAT_TYPES, CATMXSTM
      PARAMETER( CATMXSTM=100, FEAT_TYPES = 2)
      INTEGER  TVS_TYPE, MESO_TYPE, MDA_TYPE
      PARAMETER( TVS_TYPE = 1, MDA_TYPE = 2 )
      INTEGER  MESDAT
      PARAMETER( MESDAT=1 )
C
C  Parameters for sizing and indexing TIBUF:
      INTEGER  NIBUFS, CI, FI, HI, MI, TI, MDA
      PARAMETER (NIBUFS=6,CI=1,FI=2,HI=3,MI=4,TI=5,MDA=6)
C
C  Parameters for sizing and indexing BASECHARS:
      INTEGER  NBCHAR, MESB_AZ, MESB_RN, MESB_EL,
     $                 TVSB_AZ, TVSB_RN, TVSB_EL,
     $                 MDAB_AZ, MDAB_RN, MDAB_EL,
     $                 MDA_SR
      PARAMETER (NBCHAR=10, MESB_AZ=1, MESB_RN=2, MESB_EL=3,
     $                      TVSB_AZ=4, TVSB_RN=5, TVSB_EL=6,
     $                      MDAB_AZ=7, MDAB_RN=8, MDAB_EL=9,
     $                      MDA_SR=10)
/*
**A308C3
*VERSION: 1
C    This is the Common Block for the Combined Attributes task.
C
      INTEGER  STORM_FEATS, TIBUF, CAT_NSTMS, CAT_IDX
      REAL  BASECHARS
      COMMON /A308C3/ STORM_FEATS(FEAT_TYPES, CATMXSTM)
      COMMON /A308C3/ TIBUF(NIBUFS),CAT_NSTMS, CAT_IDX
      COMMON /A308C3/ BASECHARS(NBCHAR, CATMXSTM)
/*
**A308P4
*VERSION:0
C**************************************************
C*                    A308P4                      *
C*     ALERT PROCESSING: LOWEST ELEVATION         *
C*         VELOCITY OUTPUT PARAMETERS             *
C*                                                *
C**************************************************
C
C NACOL = The number of columns in the Alert grid.
C NAROW = The number of rows in the Alert grid.
C ABOXHGHT = The height (y-dir) of an Alert box, in km.
C ABOXWDTH = The width (x-dir) of an Alert box, in km.
C OMAX = The offset to the maximum velocity value.
C OMAXI = The offset to the max velocity I position.
C OMAXJ = The offset to the max velocity J position.
C OMODE = The Doppler velocity weather mode offset.
C OGRID = The offset to the lowest el. velocity Alert grid.
C NVBXES = The total number of low el vel boxes.
C GRSTART = THE ALERT GRID START POINT OF THE 4 X 4 GRID.
C GREND = THE ALERT GRID END POINT OF THE 4 X 4 GRID.
C
      INTEGER NACOL,NAROW,ABOXHGHT,ABOXWDTH
      PARAMETER (NACOL=58,NAROW=58,ABOXHGHT=16,ABOXWDTH=16)
      INTEGER OMAX,OMAXI,OMAXJ,OGRID,NVBXES,OMODE
      PARAMETER (NVBXES=NACOL*NAROW)
      PARAMETER (OMAX=0,OMAXI=1,OMAXJ=2,OMODE=3,OGRID=4)
C
      INTEGER GRSTART,GREND
      PARAMETER ( GRSTART=15,GREND=44 )
/*
**A308Z4
*VERSION:0
C
C************************************************
C*                  A308Z4                      *
C*    ALERT PROCESSING: LOWEST ELEVATION        *
C*        VELOCITY OUTPUT DEFINITIONS           *
C*                                              *
C************************************************
C
C MAPVEL = The Cart. mapped maximum lowest elevations velocity
C          values on the Alert grid.
C MAXMAX = The maximum value on the Alert grid.
C MAXI   = The I position of the max velocity value.
C MAXJ   = The J position of the max velocity value.
C
      INTEGER*2 MAPVEL(NACOL,NAROW)
      INTEGER MAXMAX,MAXI,MAXJ,DOPMODE
/*
