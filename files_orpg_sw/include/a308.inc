XCOMM RCS info 
XCOMM $Author: ryans $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2006/02/28 16:54:30 $ 
XCOMM $Id: a308.inc,v 1.10 2006/02/28 16:54:30 ryans Exp $ 
XCOMM $Revision: 1.10 $ 
XCOMM $State: Exp $
*.*******************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A308.INC
*.
*.  INCLUDE FILE VERSION: 03
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#      PROGRAMMER         NOTES
*.  ---------    --   -----     ----------------   -----------------
*.  04 25 91     00   92083     D. C. ERICKSON     ADD PROLOGUE
*.  10 22 92     01  NA92-28001 MIKE ISTOK         RCM LFM
*.  05 03 95     02  NA94-26301 STEVE SMITH        HAIL
*.  05 03 95     02  NA94-26302 STEVE SMITH        SCIT
*.  06/01/95     02  NA94-32101 MIKE ISTOK         ADD HYBRSCAN TO RCM
*.  11/16/05     03  NA05-23001 Brian Klein        ADD MDA TO ALERTS
*.
*.*******************************************************************
**A308C1
*VERSION:1
      INTEGER   UAM_PTRS (MAX_CLASS1,NACT_SCNS)
      INTEGER*2  TBUF(NROWS,TLINES)
      INTEGER NDX(MAX_CLASS1,NACT_SCNS),IT(MAX_CLASS1,NACT_SCNS)
      INTEGER NP(MAX_CLASS1,NACT_SCNS)
      COMMON/A308C1/ UAM_PTRS,NDX,IT,NP,TBUF
/*
**A308C2
*VERSION:0
      INTEGER STRMDIR(CAT_MXSTMS)
      INTEGER FORDIR(CAT_MXSTMS)
      INTEGER FPOSDIR(MAX_FPOSITS,CAT_MXSTMS)
C
      COMMON/A308C2/ STRMDIR,FPOSDIR,FORDIR
/*
C
C   COMMON SHARED ONLY BE A3081A AND A3081C. THIS BLOCK
C   HOLDS THE ALERTING PARAMETERS NEEDED BY A30817
C
**A308C3
*VERSION:0
      REAL STMSPD,STMDIR,ELEVANG,AZ,RAN,EXVAL
      INTEGER EXVAL1,THRESHOLD
      CHARACTER*4 STMID
      COMMON/A308C3/AZ,RAN,ELEVANG,STMID,STMSPD,STMDIR,
     *              EXVAL,EXVAL1,THRESHOLD
/*
*********************************************************************
**A3085CA
*VERSION:0
C************************************************
C*                A3085CA                       *
C*  ALERT PROCESSING: LOCAL COMMON TABLE DEF.   *
C************************************************
C REFTBLE = LOOK-UP TABLE FOR FINDING START INDEX OF
C            4 X 4 REF GRID WITHIN A PARTICULAR ALERT BOX.
C
      INTEGER TSZE,GRSTZ,GRENZ,JZ,IZ
      PARAMETER (TSZE=2,GRSTZ=8,GRENZ=51 )
      INTEGER ITYPE,JTYPE
      PARAMETER (JTYPE=1,ITYPE=2)
C
      INTEGER*2 REFTBLE(TSZE,GRSTZ:GRENZ)
      COMMON /A3085CA/ REFTBLE
/*
**A3085CB
*VERSION:0
C************************************************
C*                A3085CB                       *
C*  ALERT PROCESSING: LOCAL COMMON TABLE DEF.   *
C************************************************
C VETTBLE = LOOK-UP TABLE FOR FINDING START INDEX OF
C            4 X 4 GRID WITHIN A PARTICULAR ALERT BOX.
C
      INTEGER TSZE,GRST,GREN,JV,IV
      PARAMETER (TSZE=2,GRST=15,GREN=44)
      INTEGER ITYPE,JTYPE
      PARAMETER (JTYPE=1,ITYPE=2)
C
      INTEGER*2 VETTBLE(TSZE,GRST:GREN)
C
      COMMON /A3085CB/ VETTBLE
/*
*********************************************************************
**A308C4
*VERSION:0
C
C*************************************************
C*                  A308C4                       *
C*   ALERT PROCESSING: LOWEST ELEVATION          *
C*          VELOCITY COMMON BLOCK                *
C*                                               *
C**************************************************
C
C COLFACT = A Cartesian mapping column factor.
C ROWFACT = A Cartesian mapping row factor.
C COLEL = A Cartesian mapping column factor constant with
C         a constant elevation angle.
C ROWEL = A Cartesian mapping row factor constant with a
C         constant elevation angle.
C
      INTEGER COLFACT,ROWFACT
      REAL COLEL,ROWEL
      COMMON /A308C4/ COLFACT,ROWFACT,COLEL,ROWEL
/*
**A3081PM
*VERSION: 3
*********************************************************************
*   PARAMETERS
*   ==========
*******************COMMON PARAMETERS FOR A3081.FTN********************
      INTEGER*2  DIVIDER,NOI2,MBLKID,MSGCODE,ALMLNG,ALTLNG

      INTEGER*4  PRDOFF,MSGOFF,MLOFF,MUPLIM,MNOLNS,MPOFF,NOLNS
      INTEGER*4  ALPOFF,ALMOFF,ALBOFF,ALLOFF
      INTEGER*4  DTYP1,DTYP2,DTYP3,DTYP4,DTYP6,DTYP7,DTYP8,DTYP9
      INTEGER*4  DTYP10,DTYP11,DTYP12,DTYP13,DTYP14,DTYP25,DTYP26
      INTEGER*4  DTYP15,DTYP16
      INTEGER*4  DTYP27,DTYP28,DTYP29,DTYP30,DTYP31,DTYP32
      INTEGER*4  DTYP999,N3,N4,N5,N6,N8,N9,N10
      INTEGER*4  NEW_ALERT,OLD_ALERT,NO_ALERT,END_ALERT
      INTEGER EXPECTED
      INTEGER*4  NROWS,NCOLS,BROWS,BCOLS,SFACT,TLINES
      REAL       BSZKM,RRAD,STFACT
C
      PARAMETER  (NROWS=40,NCOLS=7,BROWS=4,BCOLS=58,BSZKM=16.0)
      PARAMETER  (RRAD=0.017453,SFACT=16,STFACT=0.5,TLINES=20)
      PARAMETER  (NO_ALERT=0,NEW_ALERT=1,OLD_ALERT=2,END_ALERT=3)
      PARAMETER   (DIVIDER=-1,PRDOFF=5,MSGOFF=60,MBLKID=1,MLOFF=1)
      PARAMETER  (MPOFF=2,NOLNS=8,DTYP1=1,DTYP2=2,DTYP3=3,DTYP4=4)
      PARAMETER  (DTYP6=6,DTYP7=7,DTYP8=8,DTYP9=9,DTYP10=10)
      PARAMETER  (DTYP11=11,DTYP12=12,DTYP13=13,DTYP14=14,DTYP25=25)
      PARAMETER  (DTYP15=15, DTYP16=16)
      PARAMETER  (DTYP26=26,DTYP27=27,DTYP28=28,DTYP29=29,DTYP30=30)
      PARAMETER  (DTYP31=31,DTYP32=32,DTYP999=999)
      PARAMETER  (N3=3,N4=4,N5=5,N6=6,N8=8,N9=9)
      PARAMETER  (N10=10,MUPLIM=10,NOI2=40,MNOLNS=16,ALMLNG=10)
      PARAMETER  (ALTLNG=16)
      PARAMETER  (ALPOFF=6,ALMOFF=5,ALBOFF=9,ALLOFF=10,MSGCODE=9)
      PARAMETER(EXPECTED=1)

/*
**A3081DS
*VERSION: 4
*********************************************************************
*   DATA STATEMENTS
*   ===============
*******************COMMON DATA FOR A3081.FTN**************************

      CHARACTER*25  CUNITS(2,50)

C
C  For those strings ending in Y, adding a blank character to
C  fix a compiler problem.
C
      DATA  CUNITS(1,1)/'VELOCITY '/
      DATA  CUNITS(1,2)/'COMP REFLECTIVITY '/
      DATA  CUNITS(1,3)/'ECHO TOPS'/
      DATA  CUNITS(1,4)/'SWP'/
      DATA  CUNITS(1,6)/'VIL'/
      DATA  CUNITS(1,7)/'VAD'/
      DATA  CUNITS(1,8)/'MAX HAIL SIZE'/
      DATA  CUNITS(1,9)/'MESO'/
      DATA  CUNITS(1,10)/'TVS'/
      DATA  CUNITS(1,11)/'MAX STORM REFL'/
      DATA  CUNITS(1,12)/'PROB HAIL'/
      DATA  CUNITS(1,13)/'PROB SVR HAIL'/
      DATA  CUNITS(1,14)/'STORM TOP'/
      DATA  CUNITS(1,15)/'MAX 1-HR. RAINFALL'/
      DATA  CUNITS(1,16)/'MDA STRNGTH RANK'/
      DATA  CUNITS(1,25)/'FORECAST MAX HAIL SIZE'/
      DATA  CUNITS(1,26)/'FORECAST MESO'/
      DATA  CUNITS(1,27)/'FORECAST TVS'/
      DATA  CUNITS(1,28)/'FORECAST MAX STORM REFL'/
      DATA  CUNITS(1,29)/'FORECAST PROB HAIL'/
      DATA  CUNITS(1,30)/'FORECAST PROB SVR HAIL'/
      DATA  CUNITS(1,31)/'FORECAST STORM TOP'/
      DATA  CUNITS(1,32)/'FORECAST MDA STRNGTH RANK'/
      DATA  CUNITS(2,1)/'KNOTS'/
      DATA  CUNITS(2,2)/'DBZ'/
      DATA  CUNITS(2,3)/'K-FEET'/
      DATA  CUNITS(2,4)/'PROB %'/
      DATA  CUNITS(2,6)/'KG/M**2'/
      DATA  CUNITS(2,7)/'KNOTS'/
      DATA  CUNITS(2,8)/'INCHES'/
      DATA  CUNITS(2,11)/'DBZ'/
      DATA  CUNITS(2,12)/'PROB %'/
      DATA  CUNITS(2,13)/'PROB %'/
      DATA  CUNITS(2,14)/'K-FEET'/
      DATA  CUNITS(2,15)/'INCHES'/
      DATA  CUNITS(2,25)/'INCHES'/
      DATA  CUNITS(2,28)/'DBZ'/
      DATA  CUNITS(2,29)/'PROB %'/
      DATA  CUNITS(2,30)/'PROB %'/
      DATA  CUNITS(2,31)/'K-FEET'/
/*
*********************************************************************
**A308P2
*VERSION:2
C******************************************************************
C*                    RADAR CODED MESSAGE                        **
C*                       PARAMETERS                              **
C*                         A308P2                                **
C******************************************************************
C*    NROWS     INT*4    NUMBER OF ROW IN THE RCM LFM GRID
C*    NCOLS     INT*4    NUMBER OF COLUMNS IN THE RCM LFM GRID
C*    END       INT*4    FLAG INDICATING THAT THE RCM ENCODING IS
C*                       BEING DONE AT EITHER END OF A LFM GRID ROW
C*    MID       INT*4    FLAG LIKE 'END' BUT MEANING WE'RE IN THE
C*                       MIDDLE OF A LFM GRID ROW
C*    NACOLS    INT*4    NUMBER OF COLUMNS AVAILABLE FOR RCM PRODUCT
C*    NAROWS    INT*4    NUMBER OF ROWS PER PAGE FOR RCM PRODUCT
C*    BTH       INT*4    BIASED REFLECTIVITY VALUE WHICH IS THE
C*                       CORRESPONDS TO LESS THAN 12 DBZ
C*    COUNT_INT INT*4    COUNTER FOR NUMBER OF INTENSITIES
C*    NIBUF     INT*4    NUMBER OF INPUT BUFFERS
C*    PG        INT*4    IDENTIFIES POLAR GRID INPUT BUFFER
C*    CA        INT*4    IDENTIFIES COMBINED ATTRIBUTES INPUT BUFFER
C*    ET        INT*4    IDENTIFIES ECHO TOPS INPUT BUFFER
C*    VW        INT*4    IDENTIFIES VAD WINDS INPUT BUFFER
C*    HY        INT*4    IDENTIFIES HYBRSCAN INPUT BUFFER
C*    GIBUF     INT*4    NUMBER OF INPUT AND OUTPUT BUFFERS
C*    MAX_STORM INT*4    MAXIMUM NUMBER OF STORMS
C*    TIBF      INT*4    TABLE OF INPUT BUFFER POINTERS INTO MEM
C*    VB        INT*4    VALID BUFFER ARRAY
C*    RCMOFF    INT*4    OFFSET IN PARTA FOR RADNE OR RADOM
C*    OPMODEOFF INT*4    OFFSET IN PARTA FOR OPERATIONAL LINE
C*    RCMIDX    INT*4    RADAR CODED MESSAGE INDEX
C*    SAVE_BYTE INT*4    SAVED BYTE VALUE FOR OFFSETS
C*    MSG_SIZ1  INT*4    LENGTH OF MESSAGE FOR #83
C*
C******************************************************************
      INTEGER*4 NROWS,NCOLS,END,MID,NACOLS,NAROWS,BTH
      INTEGER*4 COUNT_INT,NIBUF,PG,CA,ET,VW,HY,GIBUF,MAX_STORM
      PARAMETER (NIBUF=5,PG=1,CA=2,ET=3,VW=4,HY=5,GIBUF=7)
      PARAMETER (MAX_STORM=20)
      INTEGER*4 TIBF,VB,RCMOFF,OPMODEOFF,RCMIDX,SAVE_BYTE,MSG_SIZ1
      PARAMETER (NROWS=100,NCOLS=100,END=-1,MID=1)
      PARAMETER (NACOLS=70,NAROWS=17,BTH=0)
      COMMON  /A308P2/ COUNT_INT,VB(GIBUF),RCMOFF,OPMODEOFF,RCMIDX,
     $                 TIBF(NIBUF),SAVE_BYTE,MSG_SIZ1
/*
**A308P1
*VERSION: 1
********************* A308P1 ****************************
*                                                       *
*  THIS ELEMENT CONTAINS THE PARAMETER OFFSET INTO      *
*  THE MEM BUFFERS THAT CONTAIN THE COPIES OF THE       *
*  USER ALERT GRIDS AND AREA DEFINITIONS WHICH ARE      *
*  USED BY THE ALERTING TASK.                           *
*                                                       *
*********************************************************
      INTEGER GDOFF,ACOFF,THOFF,PRQOFF,CSTATOFF
C
C  GDOFF IS THE OFFSET TO THE ALERT GRID .
C  ACOFF IS THE OFFSET TO THE ACTIVE CATEGORIES TABLE
C  THOFF IS THE OFFSET TO THE THRESHOLDS OF THE CATEGORIES
C  PRQOFF IS THE OFFSET TO THE PRODUCT REQUEST FLAG PER CATEGORY
C  CSTATOFF IS THE OFF SET TO THE CURRENT STATUS DIRECTORY
C  PSTATOFF IS THE OFFSET TO THE PREVIOUS STATUS TABLE
C
      PARAMETER (GDOFF=0,ACOFF=116,THOFF=121,PRQOFF=126)
      PARAMETER (CSTATOFF=131)
C
/*
