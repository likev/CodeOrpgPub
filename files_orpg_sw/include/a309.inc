*.*******************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A309.INC
*.
*.  INCLUDE FILE VERSION: 0016
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#          PROGRAMMER         NOTES
*.  ---------    --   -----         ----------------   -------------
*.  06 01 90     00   90613         P. STALEY
*.  06 01 90     00   90615         P. STALEY
*.  07 11 90     01   90819         G. JUBINSKI
*.  08 02 90     01   90869         STEVE ANDERSON
*.  08 23 90     01   90936         M.L. ECKERLE
*.  09 07 90     01   90952         PAUL JENDROWSKI
*.  09 24 90     01   90995         M.L. ECKERLE
*.  09 25 90     01   90778         C. BAUMHER
*.  11 01 90     02   91079         C. BAUMHER         **A3CD07
*.  11 09 90     02   91122         C. BAUMHER         **A3CD07
*.  11 13 90     02   90925         MEI OU             **A3CD07
*.  12 04 90     02   91434         VALERIE HOLDEN     **A3CD07
*.  12 04 90     02   91434         VALERIE HOLDEN     **A3PM07
*.  01 07 91     02   90888         MEI  OU            **A3CD07
*.  01 10 91     02   91148         VALERIE HOLDEN     **A3CD07
*.  01 21 91     02   91386         JOSEPH WHEELER     **A3CD07
*.  02 08 91     02   91671         VALERIE HOLDEN     **A3CD07
*.  02 19 91     02   90921         STAN MANOSKI       **A3CD40
*.  02 19 91     02   90921         STAN MANOSKI       **A3PM00
*.  03 25 91     02   91794         BOB TACH           **A3CD04
*.  06 13 91     02   92083         DAVID ETRIS
*.  08 20 91     03   91088         MEI   OU           **A3PM08
*.  08 29 91     03   92361         STEVE ANDERSON     **A3PM00
*.  10 03 91     03   92144         PENNY J YELSHIN    **A3CD07
*.  11 01 91     04   92410         EDWARD WENDOSKI    **A3PM50
*.  11 01 91     04   92598         EDWARD WENDOSKI    **A3PM50
*.  11 15 91     04   92650         MEI  OU            **A3CD07
*.  03 12 92     05   92553         MEI  OU            **A3CD07
*.  03 20 92     06   92160         MEI  OU            **A3CD07
*.  03 20 92     06   92840         PENNY J YELSHIN    **A3PM50
*.  04 27 92     06   93012         VALERIE HOLDEN     **A3CD07
*.  04 27 92     06   92583         MEI  OU            **A3CD07
*.  05 20 92     06   92440         PJ YELSHIN         **A3PM00
*.  05 20 92     06   92801         PJ YELSHIN         **A3PM00
*.  06 12 92     07   93013         DENNIS MILLER      **A309P1
*.  06 12 92     07   93013         DENNIS MILLER      **A309P2
*.  06 12 92     07   93013         DENNIS MILLER      **A309P3
*.  06 12 92     07   93013         DENNIS MILLER      **A309C1
*.  06 25 92     07   17502         VALERIE HOLDEN     **A3CD05**
*.  07 03 92     08   92625         MARK FRESCH        **A3CD07
*.  07 14 92     08   93011         BRADLEY SUTKER     PM18,CD05,CD07,CD18
*.  07 27 92     08   92462         D. WALKER          PM07
*.  07 31 92     08   NA92-20201    P. PISANI          **A3CD07
*.  08 05 92     08   NA92-14002    VALERIE HOLDEN     **A3CD07
*.  08 13 92     08   NA92-22303    MEI  OU            **A3CD07
*.  10 15 92     09   NA90-92835    BOB TACH           **A3CD40
*.  10 21 92     09   NA92-30902    BOB TACH           **A3CD40
*.  06 25 93     10   NA93-17501    B. SUTKER          PM00,PM17,CD07
*.  11 23 93     11   NA93-32802    B. SUTKER          **A3CD07
*.  04 28 94     12   NW-94-048-06  J. WHEELER         **A3CD69
*.  04 29 94     12   NW-93-300-05  S. SMITH           **A3CD97
*.  09 21 94     12   NW-94-075-05  S. SMITH           **A3CD07
*.  10 26 94     12   NW-94-048-29  S. SMITH           **A3PM00
*.  03 31 95     13   NA94-11252    S. SMITH           **A3CD07
*.  03 31 95     13   NA94-35402    S. SMITH          PM00,PM04,CD07
*.  04 04 95     13   NA94-35301    C. STEPHENSON     PM06,CD05,CD07
*.  04 04 95     13   NA93-19301    C. STEPHENSON     PM14
*.  04 11 95     13   NA94-35402    MIKE ISTOK        PM06 & PM07
*.  05 24 95     13   NA94-26302    STEVE SMITH       A3CD08
*.  05 24 95     13   NA94-26301    MARK FRESCH       A3PM00
*.  06 01 95     13   NA94-08462    MIKE ISTOK        PM00,CD03,CD07
*.  06 01 95     13   NA94-08459    MIKE ISTOK        PM00
*.  08 23 95     13   NA94-33206    MIKE ISTOK        PM00
*.  11 29 95     13   NA94-08462    DALE HARRIS       A3CD07
*.  02 03 97     14   NA96-17804    CHERYL STEPHENSON PM00
*.  02 03 97     14   NA95-33502    CAS & AG          PM00,PM20,CD07
*.  02 03 97     14   NA96-33504    CHERYL STEPHENSON PM00,PM20
*.  02 03 97     14   NA94-08461    CHERYL STEPHENSON PM00
*.  05/02/97     14   NA94-34702    DALE HARRIS       A3PM69,A3CD69
*.  06/20/97     14   NA95-21503    BOB LEE           A3CD09BL
*.  06/20/97     14   NA95-21503    STEVE D SMITH     REMOVE A3CD10
*.  08/25/97     14   NA94-08461    CHERYL STEPHENSON ADD A3PM10
*.  10/16/97     14   NA95-33502    ALOK GAUTAM        A3CD07
*.  05/28/98     14   NA95-27002    C. STEPHENOSN      MAXNBQ_ENT
*.  08/15/01     15   NA00-19201    C. Stephenson      A3PM00
*.  07/31/02     16   NA02-15801    Jihong Liu         A3PM00  
*.
*.*******************************************************************
**A3PM00
*VERSION:7
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C              USE: PARAMETER FILE CONTAINING PARAMETERIZED    C
C                   VALUES IDENTIFYING THE TYPE OF DATA        C
C                   CONTAINED IN A BUFFER, AND ALSO            C
C                   PARAMETERIZED VALUES FOR RELEASING AND     C
C                   DESTROYING, OR RELEASING AND FORWARDING    C
C                   A TASK'S OUTPUT BUFFER.                    C
C**************************************************************
C**************************************************************
C
C    NAME             TYPE    DESCRIPTION
C    ---------------  ------  --------------------------------------
C    INPUT_AVAIL      INT*4   Defines the parameter trap value that
C                             indicates to a task that input data is
C                             available.
C
C    FORWARD          INT*4   Defines a mnemonic used as input to
C                             A31216__REL_OUTBUF.  Indicates "pass
C                             data to processor tasks".
C
C    DESTROY          INT*4   Defines a mnemonic used as input to
C                             A31216__REL_OUTBUF.  Indicates "do not
C                             pass data to other tasks".
C
C    NORMAL           INT*4   Defines a mnemonic used as output from
C                             A31211__GET_INBUF /A31215__GET_OUTBUF.
C                             Indicates normal completion of requested
C                             operation.
C
C    TERMINATE        INT*4   Defines a mnemonic used as output from
C                             A31211__GET_INBUF /A31215__GET_OUTBUF.
C                             Indicates "terminate processing".
C
C    NOT_REQD         INT*4   Defines a mnemonic used as output form
C                             A31211__GET_INBUF.
C                             Indicates data type for which output
C                             buffer was requested is not required.
C
C    NO_BLOCKS        INT*4   DEFINES A MNEMONIC USED AS OUTPUT FROM
C                             A31231__.  INDICATES THAT NO EMPTY
C                             BLOCKS ARE AVAILABLE FOR ALLOCATION.
C
C    NO_DATA          INT*4   Defines a mnemonic used as output from
C                             A31211__GET_INBUF.  Indicates "requested
C                             data is not available".
C
C    ANY_TYPE         INT*4   Defines a mnemonic used as input to
C                             A31211__GET_INBUF.  Indicates "any of
C                             the possible task input data types is
C                             desired".
C
C    NO_TYPE          INT*4   Defines a mnemonic used as output from
C                             A31224__GET_QUEUE_INDEX.
C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
C  Mnemonics for various controls
C
      INTEGER REPORT_CPU
      PARAMETER (REPORT_CPU = 18)
      INTEGER    SHUT_DOWN
      PARAMETER (SHUT_DOWN = 19)
      INTEGER    INPUT_AVAIL
      PARAMETER (INPUT_AVAIL = 20)
C
C  Keywords for REL_OUTBUF
C
      INTEGER  FORWARD,DESTROY
      PARAMETER  (FORWARD = 100, DESTROY = 101)
C
C  Keywords for extending arguments for REL_OUTBUF
C  
      INTEGER  RPG_EXTEND_ARGS
      PARAMETER  (RPG_EXTEND_ARGS = Y'10000')
C
C  Status Values From GET_INBUF, GET_OUTBUF AND THE LIKE.
C
      INTEGER  NORMAL,TERMINATE,NOT_REQD,NO_DATA,NO_BLOCKS,NO_MEM,
     *         FAULT_ME
      PARAMETER (NORMAL = 0, TERMINATE = 1, NOT_REQD = 1, NO_DATA = 2,
     *           NO_BLOCKS = 3, NO_MEM = 4, FAULT_ME = 99 )
C
C  Buffer Data Content Identifiers
C
      INTEGER ANY_TYPE, NO_TYPE
      PARAMETER (ANY_TYPE = 0, NO_TYPE = 0)
C
C  Buffer data types
C
      INTEGER ALRTMSG
      INTEGER BREF19
      INTEGER BREF16
      INTEGER BREF20
      INTEGER BREF17
      INTEGER BREF21
      INTEGER BREF18
      INTEGER BSPC28
      INTEGER BSPC29
      INTEGER BSPC30
      INTEGER BVEL25
      INTEGER BVEL22
      INTEGER BVEL26
      INTEGER BVEL23
      INTEGER BVEL27
      INTEGER BVEL24
      INTEGER SPARE17
      INTEGER SPARE18
      INTEGER SPARE19
      INTEGER BASEVGD
      INTEGER COMBATTR
      INTEGER VADTMHGT
      INTEGER CENTATTR
      INTEGER CRP37
      INTEGER CRP35
      INTEGER CRP38
      INTEGER CRP36
      INTEGER SEGATTR
      INTEGER FILTERED_REFL_ELEV
      INTEGER SPARE28
      INTEGER ETPRODD
      INTEGER ETTAB
      INTEGER ALL_TYPES
      INTEGER FREETXT
      INTEGER FTXTMSG
      INTEGER HAILCAT
      INTEGER TRENDATR
      INTEGER HPLOTS
      INTEGER RFAVLYR1
      INTEGER RFAVLYR2
      INTEGER RFMXLYR1
      INTEGER RFMXLYR2
      INTEGER MES2DATR
      INTEGER MESOATTR
      INTEGER MESOPROD
      INTEGER ALRTPROD
      INTEGER RMXAPLYR
      INTEGER RMXAPPG
      INTEGER TDA1DATR
      INTEGER HEADINFO
      INTEGER HAILATTR
      INTEGER STRUCDAT
      INTEGER TRFRCATR
      INTEGER STMTRDAT
      INTEGER PRCIPMSG
      INTEGER PRODXFRM
      INTEGER RADARMSG   ;* NOTE:  Keep this .... may want to resurrect at some point
      INTEGER BASEDATA
      INTEGER REFLDATA
      INTEGER ITWSDBV
      INTEGER SEVWTH46
      INTEGER SEVWTH43
      INTEGER SEVWTH45
      INTEGER SEVWTH44
      INTEGER SPARE63
      INTEGER SPARE64
      INTEGER TVSATTR
      INTEGER SRMRVMAP
      INTEGER SRMRVREG
      INTEGER CPC2MSG
      INTEGER CPC10MSG
      INTEGER CPC20LOG
      INTEGER CPC20MSG
      INTEGER RAWDATA_REFLDATA
      INTEGER RAWDATA_COMBBASE
      INTEGER POSEDRCM
      INTEGER TVSPROD
      INTEGER VADPARAM
      INTEGER VCS54
      INTEGER VCS53
      INTEGER VCS52
      INTEGER VILPROD
      INTEGER VILTABL
      INTEGER SPARE88
      INTEGER HYUSPACC
      INTEGER HYBRDREF
      INTEGER HYBRGREF
      INTEGER SCRATCH
      INTEGER MESSAGE
      INTEGER CRPG
      INTEGER COMBBASE
      INTEGER CRCG230
      INTEGER CRCG460
      INTEGER BREF8BIT
      INTEGER BVEL8BIT
      INTEGER APRODUCT
      INTEGER HYBRSCAN
      INTEGER HYUSPBUF
      INTEGER HYACCSCN
      INTEGER HYADJSCN
      INTEGER HY1HRACC
      INTEGER HY3HRACC
      INTEGER HYSTMTOT
      INTEGER HY1HRDIG
      INTEGER HYSUPPLE
      INTEGER VADVER2
      INTEGER VCSR8
      INTEGER VCSV8
      INTEGER SPARE113
      INTEGER SPARE114
      INTEGER SPARE115
      INTEGER RFAVLYR3
      INTEGER RFMXLYR3
      INTEGER CPC19MSG
      INTEGER CFCPROD
      INTEGER HYDIGSTM 
      INTEGER BASEDATA_ELEV
      INTEGER REFLDATA_ELEV
      INTEGER COMBBASE_ELEV
      INTEGER CRPGAPE
      INTEGER CRPAPE97
      INTEGER CRPAPE95
      INTEGER CRPAPE98
      INTEGER CRPAPE96
      INTEGER PRFOVLY
      INTEGER RAWDATA      
      INTEGER MESORUPROD
      INTEGER MESORUATTR
      INTEGER MDAPROD     
      INTEGER OSWACCUM   
      INTEGER OSDACCUM  
      INTEGER SSWACCUM 
      INTEGER SSDACCUM   
      INTEGER USWACCUM  
      INTEGER USDACCUM 
      INTEGER SAAACCUM   
      INTEGER TVSATR_RU  
      INTEGER MDA1D     
      INTEGER MDA2D    
      INTEGER MDA3D     
      INTEGER MDATTNN  
      INTEGER DQA         
      INTEGER RECCLREF   
      INTEGER RECCLDOP  
      INTEGER PRFSEL
      INTEGER STATPROD



      PARAMETER (ALRTMSG=1)
      PARAMETER (BREF19=2)
      PARAMETER (BREF16=3)
      PARAMETER (BREF20=4)
      PARAMETER (BREF17=5)
      PARAMETER (BREF21=6)
      PARAMETER (BREF18=7)
      PARAMETER (BSPC28=8)
      PARAMETER (BSPC29=9)
      PARAMETER (BSPC30=10)
      PARAMETER (BVEL25=11)
      PARAMETER (BVEL22=12)
      PARAMETER (BVEL26=13)
      PARAMETER (BVEL23=14)
      PARAMETER (BVEL27=15)
      PARAMETER (BVEL24=16)
      PARAMETER (SPARE17=17)
      PARAMETER (SPARE18=18)
      PARAMETER (SPARE19=19)
      PARAMETER (BASEVGD=20)
      PARAMETER (COMBATTR=21)
      PARAMETER (CENTATTR=22)
      PARAMETER (CRP37=23)
      PARAMETER (CRP35=24)
      PARAMETER (CRP38=25)
      PARAMETER (CRP36=26)
      PARAMETER (SEGATTR=27)
      PARAMETER (FILTERED_REFL_ELEV=400)
      PARAMETER (SPARE28=28)
      PARAMETER (ETPRODD=29)
      PARAMETER (ETTAB=30)
      PARAMETER (FREETXT=31)
      PARAMETER (FTXTMSG=32)
      PARAMETER (HAILCAT=33)
      PARAMETER (TRENDATR=34)
      PARAMETER (HPLOTS=35)
      PARAMETER (RFAVLYR1=36)
      PARAMETER (RFAVLYR2=37)
      PARAMETER (RFMXLYR1=38)
      PARAMETER (RFMXLYR2=39)
      PARAMETER (MES2DATR=40)
      PARAMETER (MESOATTR=41)
      PARAMETER (MESOPROD=42)
      PARAMETER (ALRTPROD=43)
      PARAMETER (RMXAPLYR=44)
      PARAMETER (RMXAPPG=45)
      PARAMETER (TDA1DATR=46)
      PARAMETER (HEADINFO=47)
      PARAMETER (HAILATTR=48)
      PARAMETER (STRUCDAT=49)
      PARAMETER (TRFRCATR=50)
      PARAMETER (STMTRDAT=51)
      PARAMETER (PRCIPMSG=52)
      PARAMETER (PRODXFRM=53)
      PARAMETER (RAWDATA=54)
      PARAMETER (BASEDATA=55)
      PARAMETER (HYUSPACC=56)
      PARAMETER (HYBRDREF=57)
      PARAMETER (HYBRGREF=58)
      PARAMETER (SEVWTH46=59)
      PARAMETER (SEVWTH43=60)
      PARAMETER (SEVWTH45=61)
      PARAMETER (SEVWTH44=62)
      PARAMETER (SPARE63=63)
      PARAMETER (SPARE64=64)
      PARAMETER (TVSATTR= 65)
      PARAMETER (SRMRVMAP=68)
      PARAMETER (SRMRVREG=69)
      PARAMETER (CPC2MSG =70)
      PARAMETER (CPC20LOG=75)
      PARAMETER (CPC20MSG=76)
      PARAMETER (RAWDATA_REFLDATA=77)
      PARAMETER (RAWDATA_COMBBASE=78)
      PARAMETER (REFLDATA=79)
      PARAMETER (TVSPROD=80)
      PARAMETER (VADPARAM=81)
      PARAMETER (VCS54=82)
      PARAMETER (VCS53=83)
      PARAMETER (VCS52=84)
      PARAMETER (VILPROD=85)
      PARAMETER (VILTABL=86)
      PARAMETER (ITWSDBV=87)
      PARAMETER (SPARE88=88)
      PARAMETER (SCRATCH=89)
      PARAMETER (MESSAGE=90)
      PARAMETER (CRPG=91)
      PARAMETER (CRCG230=92)
      PARAMETER (CRCG460=93)
      PARAMETER (BREF8BIT=94)
      PARAMETER (ALL_TYPES=95)
      PARAMETER (COMBBASE=96)
      PARAMETER (VADTMHGT=97)
      PARAMETER (CPC10MSG=98)
      PARAMETER (BVEL8BIT=99)
      PARAMETER (APRODUCT = 100)
      PARAMETER (HYBRSCAN = 101)
      PARAMETER (HYUSPBUF = 102)
      PARAMETER (HYACCSCN = 103)
      PARAMETER (HYADJSCN = 104)
      PARAMETER (HY1HRACC = 105)
      PARAMETER (HY3HRACC = 106)
      PARAMETER (HYSTMTOT = 107)
      PARAMETER (HY1HRDIG = 108)
      PARAMETER (HYSUPPLE = 109)
      PARAMETER (VADVER2  = 110)
      PARAMETER (VCSR8    = 111)
      PARAMETER (VCSV8    = 112)
      PARAMETER (SPARE113 = 113)
      PARAMETER (SPARE114 = 114)
      PARAMETER (SPARE115 = 115)
      PARAMETER (RFAVLYR3 = 116)
      PARAMETER (RFMXLYR3 = 117)
      PARAMETER (CPC19MSG = 118)
      PARAMETER (CFCPROD  = 119)
      PARAMETER (CRPGAPE = 122)
      PARAMETER (CRPAPE97 = 123)
      PARAMETER (CRPAPE95 = 124)
      PARAMETER (CRPAPE98 = 125)
      PARAMETER (CRPAPE96 = 126)
      PARAMETER (PRFOVLY = 127)
      PARAMETER (POSEDRCM = 129)
      PARAMETER (RADARMSG = 130)      
      PARAMETER (RECCLREF = 132)
      PARAMETER (RECCLDOP = 133)
      PARAMETER (HYDIGSTM = 138)
      PARAMETER (MESORUPROD = 139)
      PARAMETER (MDAPROD = 141)
      PARAMETER (OSWACCUM = 144)
      PARAMETER (OSDACCUM = 145)
      PARAMETER (SSWACCUM = 146)
      PARAMETER (SSDACCUM = 147)
      PARAMETER (USWACCUM = 150)
      PARAMETER (USDACCUM = 151)
      PARAMETER (STATPROD = 152 )
      PARAMETER (SAAACCUM = 280)
      PARAMETER (TVSATR_RU = 290)
      PARAMETER (MDA1D = 292)
      PARAMETER (MDA2D = 293)
      PARAMETER (MDA3D = 294)
      PARAMETER (MDATTNN = 295)
      PARAMETER (MESORUATTR = 296)
      PARAMETER (DQA = 297)
      PARAMETER (BASEDATA_ELEV = 301)
      PARAMETER (REFLDATA_ELEV = 302)
      PARAMETER (COMBBASE_ELEV = 303)
      PARAMETER (PRFSEL = 304 )
/*
**A3PM01
*VERSION: 2
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                              C
C         PARAMETER FILE A3PM01                                C
C              WRITTEN: FEB. 8, 1985                           C
C              USE: PARAMETER FILE CONTAINING PARAMETERIZED    C
C                   VALUES DEFINING OFFSETS INTO THE PRODUCT   C
C                   HEADER FOR PRODUCTS,OFFSETS INTO THE       C
C                   DISPLAY HEADER FOR RADIALLY FORMATTED      C
C                   AND RASTER FORMATTED PRODUCTS, THE         C
C                   PARAMETERIZED RPG ID NUMBER FOR THIS       C
C                   PARTICULAR RPG, AND SOME OTHER             C
C                   MISCELLANEOUS PARAMETERS USED IN PRODUCT   C
C                   FORMATTING.                                C
C                                                              C
C**************************************************************C
C**************************************************************C
C                                                              C
C              DESCRIPTION OF OFFSET POSITIONS DEFINED BY      C
C                 THESE PARAMETERS:                            C
C                                                              C
C                 PRODUCT HEADER OFFSET DESCRIPTIONS:          C
C                                                              C
C                   MESCDOFF - MESSAGE CODE                    C
C                   DTMESOFF - DATE OF MESSAGE                 C
C                   TMSWOFF - TIME OF MESSAGE (MSW)            C
C                   TLSWOFF - TIME OF MESSAGE (LSW)            C
C                   LGMSWOFF - LENGTH OF MESSAGE               C
C                                             (MSW)            C
C                   LGLSWOFF - LENGTH OF MESSAGE               C
C                                             (LSW)            C
C                   SRCIDOFF - SOURCE ID-NUMBER                C
C                   DSTIDOFF - DESTINATION ID-NUMBER           C
C                   NBLKSOFF - NUMBER OF BLOCKS IN PRODUCT     C
C                   DIV1OFF - FIRST DIVIDER                    C
C                   LTMSWOFF - LATITUDE OF RADAR (MSW)         C
C                   LTLSWOFF - LATITUDE OF RADAR (LSW)         C
C                   LNMSWOFF - LONGITUDE OF RADAR (MSW)        C
C                   LNLSWOFF - LONGITUDE OF RADAR (LSW)        C
C                   RADHGTOFF - HEIGHT OF RADAR                C
C                   PRDCODOFF - PRODUCT CODE                   C
C                   WTMODOFF - WEATHER MODE                    C
C                   VCPOFF   - VOLUME COVERAGE PATTERN         C
C                   SQNUMOFF - SEQUENCE NUMBER                 C
C                   VSNUMOFF - VOLUME SCAN NUMBER              C
C                   VSDATOFF - VOLUME SCAN DATE                C
C                   VSTMSWOFF - VOLUME SCAN TIME (MSW)         C
C                   VSTLSWOFF - VOLUME SCAN TIME (LSW)         C
C                   GDPRDOFF - GENERATION DATE OF PRODUCT      C
C                   GTMSWOFF - GENERATION TIME OF PRODUCT      C
C                                    (MSW)                     C
C                   GTLSWOFF - GENERATION TIME OF PRODUCT      C
C                                    (LSW)                     C
C                   AZWINOFF - AZIMUTH OF WINDOW CENTER        C
C                   RNWINOFF - RANGE OF WINDOW CENTER          C
C                   ELINDOFF - ELEVATION INDEX                 C
C                   EAZALOFF - ELEVATION, AZIMUTH, OR ALTITUDE C
C                   DL1OFF - DATA LEVEL 1                      C
C                   DL2OFF - DATA LEVEL 2                      C
C                   DL3OFF - DATA LEVEL 3                      C
C                   DL4OFF - DATA LEVEL 4                      C
C                   DL5OFF - DATA LEVEL 5                      C
C                   DL6OFF - DATA LEVEL 6                      C
C                   DL7OFF - DATA LEVEL 7                      C
C                   DL8OFF - DATA LEVEL 8                      C
C                   DL9OFF - DATA LEVEL 9                      C
C                   DL10OFF - DATA LEVEL 10                    C
C                   DL11OFF - DATA LEVEL 11                    C
C                   DL12OFF - DATA LEVEL 12                    C
C                   DL13OFF - DATA LEVEL 13                    C
C                   DL14OFF - DATA LEVEL 14                    C
C                   DL15OFF - DATA LEVEL 15                    C
C                   DL16OFF - DATA LEVEL 16                    C
C                   MDL1OFF - MAXIMUM DATA LEVEL 1             C
C                   MDL2OFF - MAXIMUM DATA LEVEL 2             C
C                   MDL3OFF - MAXIMUM DATA LEVEL 3             C
C                   MDL4OFF - MAXIMUM DATA LEVEL 4             C
C                   STSPDOFF - STORM SPEED                     C
C                   STDIROFF - STORM DIRECTION                 C
C                   CALCONMSW- OFFSET TO CALIBRATION CONS.(MSW)C
C                   CALCONLSW- OFFSET TO CALIBRATION CONS.(LSW)C
C                   CNTINTOFF - CONTOUR INTERVAL               C
C                   NMAPSOFF - NUMBER OF MAPS                  C
C                   OPRMSWOFF - OFFSET TO PRODUCT (MSW)        C
C                   OPRLSWOFF - OFFSET TO PRODUCT (LSW)        C
C                   OGMSWOFF - OFFSET TO G. ATTRIBUTES (MSW)   C
C                   OGLSWOFF - OFFSET TO G. ATTRIBUTES (LSW)   C
C                   OTADMSWOFF - OFFSET TO TABULAR/ADAPT (MSW) C
C                   OTADLSWOFF - OFFSET TO TABULAR/ADAPT (LSW) C
C                                                              C
C                 PRODUCT BLOCK OFFSETS                        C
C                                                              C
C                   DIV2OFF - SECOND DIVIDER                   C
C                   BLOCKIDOFF - PRODUCT BLOCK ID              C
C                   LRMSWOFF - LENGTH OF PRODUCT BLOCK PORTION C
C                                OF PRODUCT (MSW)              C
C                   LRLSWOFF - LENGTH OF PRODUCT BLOCK PORTION C
C                                OF PRODUCT (LSW)              C
C                   NLYROFF - NUMBER OF LAYERS                 C
C                   LYRDIVOFF - LAYER DIVIDER                  C
C                   LYRLMSWOFF - LAYER LENGTH (MSW)            C
C                   LYRLLSWOFF - LAYER LENGTH )LSW)            C
C                                                              C
C                 DISPLAY HEADER OFFSET DESCRIPTIONS           C
C                   (FOR RADIALLY FORMATTED PRODUCTS):         C
C                                                              C
C                   OPCDOFF - OPERATION CODE                   C
C                   FRBOFF - RANGE TO FIRST BIN                C
C                   NRBOFF - NUMBER OF RANGE BINS              C
C                   ICNTOFF - I CENTER OF SWEEP                C
C                   JCNTOFF - J CENTER OF SWEEP                C
C                   SFACOFF - SCALE FACTOR                     C
C                   RADOFF - NUMBER OF RADIALS                 C
C                                                              C
C                                                              C
C                 DISPLAY HEADER OFFSET DESCRIPTIONS           C
C                   (FOR RASTER FORMATTED PRODUCTS):           C
C                                                              C
C                   FLAGSOF1 -FIRST OPCODE/FLAGS OFFSET
C                   FLAGSOF2-SECOND FLAGS OFFSET
C                   FLAGSOF3 -THIRD  FLAGS OFFSET
C                   ICDSTOFF - I COORDINATE START
C                   JCDSTOFF - J COORDINATE START
C                   XSCFACINT- X SCALE FACTOR INTEGER
C                   XSCFACFRA- X SCALE FACTOR FRACTION
C                   YSCFACINT- Y SCALE FACTOR INTEGER
C                   YSCFACFRA- Y SCALE FACTOR FRACTION
C                   NRWSOFF  - NUMBER OF ROWS OFFSET
C                   PACKDESC - PACKING DESCRIPTOR OFFSET
C                                                              C
C                 ADDITIONAL PARMETERS:                        C
C                                                              C
C                   PHEADLNG - NUMBER OF I*2 WORDS IN          C
C                              PRODUCT HEADER (ITS LENGTH)     C
C                   PCODSTR - START OF PRODUCT                 C
C                   PHBYTES- NUMBER OF BYTES IN HEADER         C
C                   DHBYTES- NUMBER OF BYTES IN RADIAL HEADER  C
C                   RASTER_HEADER -NUMBER OF HALF WORKS IN      C
C                                  RASTER PACKET HEADER         C
C                   BLKOVRHD-BYTES OF OVERHEAD IN HEADER       C
C                            BLOCKS                            C
C                   RASTA_BEGIN - PRODUCT BUFFER BEGINNING     C
C                                 INDEX FOR STORING RASTER     C
C                                 RUN-LENGTH ENCODED DATA.     C
C                                 TO BE USED FOR RASTER        C
C                                 PRODUCTS ONLY!               C
C                    RASFLG1 - VALUE TO BE USED IN OFFSET:FLAGSOF1
C                    RASFLG2 - VALUE TO BE USED IN OFFSET:FLAGSOF2
C                    RASFLG3 - VALUE TO BE USED IN OFFSET:FLAGSOF3
C                    RAS_PACK_DES - PRESET TO RASTER PACKING DESC.
C                                                              C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
      INTEGER MESCDOFF,DTMESOFF,TMSWOFF,TLSWOFF,
     1        LGMSWOFF,LGLSWOFF,SRCIDOFF,DSTIDOFF,
     2        NBLKSOFF,DIV1OFF,LTMSWOFF,LTLSWOFF,
     3        LNMSWOFF,LNLSWOFF,RADHGTOFF,PRDCODOFF,
     4        WTMODOFF,VCPOFF,SQNUMOFF,VSNUMOFF

      PARAMETER(MESCDOFF=1,DTMESOFF=2,TMSWOFF=3,TLSWOFF=4,
     1          LGMSWOFF=5,LGLSWOFF=6,SRCIDOFF=7,DSTIDOFF=8,
     2          NBLKSOFF=9,DIV1OFF=10,LTMSWOFF=11,LTLSWOFF=12,
     3          LNMSWOFF=13,LNLSWOFF=14,RADHGTOFF=15,PRDCODOFF=16,
     4          WTMODOFF=17,VCPOFF=18,SQNUMOFF=19,VSNUMOFF=20)

      INTEGER VSDATOFF,VSTMSWOFF,VSTLSWOFF,GDPRDOFF,
     1        GTMSWOFF,GTLSWOFF,AZWINOFF,RNWINOFF,
     2        ELINDOFF,EAZALOFF,DL1OFF,DL2OFF,
     3        DL3OFF,DL4OFF,DL5OFF,DL6OFF,
     4        DL7OFF,DL8OFF,DL9OFF,DL10OFF

      PARAMETER(VSDATOFF=21,VSTMSWOFF=22,VSTLSWOFF=23,
     1          GDPRDOFF=24,GTMSWOFF=25,GTLSWOFF=26,AZWINOFF=27,
     2         RNWINOFF=28,ELINDOFF=29,EAZALOFF=30,DL1OFF=31,
     3         DL2OFF=32,DL3OFF=33,DL4OFF=34,DL5OFF=35,
     4         DL6OFF=36,DL7OFF=37,DL8OFF=38,DL9OFF=39,
     5         DL10OFF=40)

      INTEGER DL11OFF,DL12OFF,DL13OFF,DL14OFF,DL15OFF,
     1       DL16OFF,MDL1OFF,MDL2OFF,MDL3OFF,MDL4OFF,
     2       STSPDOFF,STDIROFF,CNTINTOFF,NMAPSOFF,
     3       OPRMSWOFF,OPRLSWOFF,OGMSWOFF,OGLSWOFF,
     4       OTADMSWOFF,OTADLSWOFF,DIV2OFF,BLOCKIDOFF,
     5       LRMSWOFF,LRLSWOFF,CALCONMSW,CALCONLSW

      PARAMETER(DL11OFF=41,DL12OFF=42,DL13OFF=43,DL14OFF=44,
     1         DL15OFF=45,DL16OFF=46,MDL1OFF=47,MDL2OFF=48,
     2         MDL3OFF=49,MDL4OFF=50,STSPDOFF=51,STDIROFF=52,
     3         CNTINTOFF=53,NMAPSOFF=54,OPRMSWOFF=55,OPRLSWOFF=56,
     4         OGMSWOFF=57,OGLSWOFF=58,OTADMSWOFF=59,OTADLSWOFF=60,
     5         DIV2OFF=61,BLOCKIDOFF=62,LRMSWOFF=63,LRLSWOFF=64,
     6         CALCONMSW=51,CALCONLSW=52)
C
C    THE CALIBRATION OFFSETS AND STORM SPEED AND DIR ARE THE
C    SAME VALUES BY DESIGN.
C
C
C      THE FOLLOWING NAMES CAN BE USED FOR THE INDEX TO THE
C      PRODUCT DEPENDENT PARAMETERS. THESE ARE ALSO DEFINED ABOVE
C      WITH SPECIFIC NAMES THAT CAN BE USED ALSO.
C
      INTEGER PDPRAM1,PDPRAM2,PDPRAM3,PDPRAM4,PDPRAM5,
     *        PDPRAM6,PDPRAM7,PDPRAM8,PDPRAM9,PDPRAM10
C
      PARAMETER(PDPRAM1=27,PDPRAM2=28,PDPRAM3=30,PDPRAM4=47,
     *          PDPRAM5=48,PDPRAM6=49,PDPRAM7=50,PDPRAM8=51,
     *          PDPRAM9=52,PDPRAM10=53 )
C

      INTEGER NLYROFF,LYRDIVOFF,LYRLMSWOFF,LYRLLSWOFF,
     1       OPCDOFF,FRBOFF,NRBOFF,ICNTOFF,JCNTOFF,
     2       SFACOFF,RADOFF

      PARAMETER(NLYROFF=65,LYRDIVOFF=66,LYRLMSWOFF=67,LYRLLSWOFF=68,
     1         OPCDOFF=69,FRBOFF=70,NRBOFF=71,ICNTOFF=72,
     2         JCNTOFF=73,SFACOFF=74,RADOFF=75)


      INTEGER FLAGSOF1,FLAGSOF2,FLAGSOF3,ICDSTOFF,JCDSTOFF,
     *        XSCFACINT,XSCFACFRA,YSCFACINT,YSCFACFRA,NRWSOFF,
     *        PACKDESC

      PARAMETER(FLAGSOF1=69,FLAGSOF2=70,FLAGSOF3=71,ICDSTOFF=72,
     *          JCDSTOFF=73,XSCFACINT=74,XSCFACFRA=75,
     *          YSCFACINT=76,YSCFACFRA=77,NRWSOFF=78,PACKDESC=79)
C
      INTEGER RASFLG1,RASFLG2,RASFLG3,RAS_PACK_DES
C
      PARAMETER(RASFLG1=X'BA07',RASFLG2=X'8000',RASFLG3=X'00C0',
     *          RAS_PACK_DES=2)

      INTEGER PHEADLNG,PCODSTR,PHBYTES,DHBYTES,BLKOVRHD,
     1        RASTA_BEGIN,RASTER_HEADER

      PARAMETER(PHEADLNG=60,PCODSTR=69)
      PARAMETER(PHBYTES=120,DHBYTES=14,BLKOVRHD=16,
     1          RASTA_BEGIN=80,RASTER_HEADER=11)
/*
**A3PM02
*VERSION: 0
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                              C
C         PARAMETER FILE A3PM02                                C
C              WRITTEN: FEB. 8, 1985                           C
C              USE: PARAMETER FILE CONTAINING PARAMETERIZED    C
C                   VALUES DEFINING THE STATUS OF A RADIAL OF  C
C                   BASE RADAR DATA.                           C
C                                                              C
C**************************************************************C
C**************************************************************C
C                                                              C
C              DESCRIPTION OF RADIAL SATUS FLAG VARIABLE       C
C              NAMES: INCLUDING RADIAL EVALUATION (GOOD OR BAD)C
C                                                              C
C                   GOODBEL - GOOD BEGINNING OF ELEVATION CUT  C
C                   BADBEL - BAD BEGINNING OF ELEVATION CUT    C
C                   GOODBVOL - GOOD BEGINNING OF VOLUME SCAN   C
C                   BADBVOL - BAD BEGINNING OF VOLUME SCAN     C
C                   GENDEL - GOOD END OF ELEVATION CUT         C
C                   BENDEL - BAD END OF ELEVATION CUT          C
C                   GENDVOL - GOOD END OF VOLUME SCAN          C
C                   BENDVOL - BAD END OF VOLUME SCAN           C
C                   GOODINT - GOOD INTERMEDIATE RADIAL         C
C                   BADINT - BAD INTERMEDIATE RADIAL           C
C                   PGENDEL - PSUEDO GOOD END OF ELEVATION CUT C
C                   PGENDVOL - PSUEDO GOOD END OF VOLUME SCAN  C
C                   PBENDEL - PSUEDO BAD END OF ELEVATION CUT  C
C                   PBENDVOL - PSUEDO BAD END OF VOLUME SCAN   C
C                   GOODTHRLO - VALUE OF LOWEST GOOD STATUS    C
C                   GOODTHRHI - VALUE OF HIGHEST GOOD STATUS   C
C                                                              C
C              DESCRIPTION OF RADIAL SATUS FLAG VARIABLE       C
C              NAMES: STATUS (I.E. SEQUENCE IN SCAN) ONLY      C
C                   BEG_ELEV - BEGINNING OF ELEVATION SCAN     C
C                   INT_ELEV - WITHIN ELEVATION SCAN           C
C                   END_ELEV - END OF ELEVATION SCAN           C
C                   BEG_VOL  - BEGINNING OF VOLUME SCAN        C
C                   END_VOL  - END OF VOLUME SCAN              C
C                   PSEND_ELEV - PSEUDO END OF ELEVATION SCAN  C
C                   PSEND_VOL  - PSEUDO END OF VOLUME SCAN     C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
      INTEGER GOODBEL,BADBEL,GOODBVOL,BADBVOL,GENDEL,BENDEL,GENDVOL,
     1        BENDVOL,GOODINT,BADINT,PGENDEL,PGENDVOL,PBENDEL,PBENDVOL
     2       ,GOODTHRLO,GOODTHRHI
C
      PARAMETER (GOODBEL=X'00',BADBEL=X'80',GOODBVOL=X'03',
     1           BADBVOL=X'83',GENDEL=X'02',BENDEL=X'82',
     2           GENDVOL=X'04',BENDVOL=X'84',GOODINT=X'01',
     3           BADINT=X'81',PGENDEL=X'08',PGENDVOL=X'09',
     4           PBENDEL=X'88',PBENDVOL=X'89',
     5           GOODTHRLO=X'00',GOODTHRHI=X'09')
C
C** Radial Status flags:
C
      INTEGER  BEG_ELEV, INT_ELEV, END_ELEV, BEG_VOL, END_VOL
     $        ,PSEND_ELEV, PSEND_VOL
C
      PARAMETER ( BEG_ELEV=X'00', INT_ELEV=X'01', END_ELEV=X'02'
     $           ,BEG_VOL =X'03',                 END_VOL =X'04'
     $           ,PSEND_ELEV=X'08',             PSEND_VOL =X'09' )
/*
**A3PM04
*VERSION: 3
C*********************************************************
C
C       PARAMETERS THAT DEFINE THE INDEX INTO THE COLOR
C       LEVEL TABLE  COLDAT FOR ALL THE COLOR TABLES IN
C       THE RPG.
C
C       REFNC16     REFLECT. 16 LEVEL NON-CLEAR AIR
C       REFCL16     REFLECT. 16 LEVEL CLEAR AIR
C       REFNC8      REFLECT.  8 LEVEL NON-CLEAR AIR
C       REFCL8      REFLECT.  8 LEVEL CLEAR AIR
C       VELNC161    VELOCITY 16 LEVEL NON-CLEAR RESOLUTION 1
C       VELCL161    VELOCITY 16 LEVEL CLEAR AIR RESOLUTION 1
C       VELNC81     VELOCITY  8 LEVEL NON-CLEAR RESOLUTION 1
C       VELCL81     VELOCITY  8 LEVEL CLEAR AIR RESOLUTION 1
C       VELNC162    VELOCITY 16 LEVEL NON-CLEAR RESOLUTION 2
C       VELCL162    VELOCITY 16 LEVEL CLEAR AIR RESOLUTION 2
C       VELNC82     VELOCITY  8 LEVEL NON-CLEAR RESOLUTION 2
C       VELCL82     VELOCITY  8 LEVEL CLEAR AIR RESOLUTION 2
C       SPCL8       SP. WDTH  8 LEVEL CLEAR AIR
C       SPNC8       SP. WDTH  8 LEVEL NON CLEAR AIR
C       RCMCL       RAD.CODED MSG 7 LEVEL CL AIR
C       RCMNC       RAD.CODED MSG 7 LEVEL NON CL AIR
C       HRSNC16     HI R SHR 16 LEVEL NON CLEAR AIR
C       HRSCL16     HI R SHR 16 LEVEL NON CLEAR AIR
C       ECNC16      ECHO TOP 16 LEVEL NON CLEAR AIR
C       ECCL16      ECHO TOP 16 LEVEL CLEAR AIR
C       VADRMS      VAD COLOR TABLE
C       ALYCRFNC    LYR Z AV. 8 LEVEL NON CLEAR AIR
C       ALYCRFCL    LYRZ AV.  8 LEVEL CLEAR AIR
C       WERNC       WK ECHO   5 LEVEL NON CLEAR AIR
C       WERCL       WK ECHO   5 LEVEL CLEAR AIR
C       VILNC       V.I.L    16 LEVEL NON CLEAR AIR
C       VILCL       V.I.L    16 LEVEL CLEAR AIR
C       RCMIGP      RADAR CODED MESSAGE 9 LEVEL (INT. GRAPHIC PRODUCT)
C       LCTNC       LYR TUR    8 LEVEL NON CLEAR AIR
C       LCTCL       LYR TUR    8 LEVEL CLEAR AIR
C       PPHRLYNC     PRECIP 1&3 HOUR PRODUCTS -NON CLEAR
C       PPHRLYCL     PRECIP 1&3 HOUR PRODUCTS -CLEAR AIR
C       STMTOTNC     STORM TOTAL NON CLEAR
C       STMTOTCL     STORM TOTAL CLEAR AIR (SAME TABLE NOW)
C       PPQTRLFM     1/4 LFM PRECIP.RATE GRID FOR HYDROMET
C*******************************************************
C
      INTEGER   REFNC16 ,  REFCL16 ,  REFNC8  ,  REFCL8  ,
     *          VELNC161,  VELCL161,  VELNC81 ,  VELCL81 ,
     *          VELNC162,  VELCL162,  VELNC82 ,  VELCL82
      INTEGER   SPCL8   ,  SPNC8   ,  RCMCL   ,  RCMNC   ,
     *          HRSNC16 ,  HRSCL16 ,  ECNC16  ,  ECCL16  ,
     *          ALYCRFNC,  ALYCRFCL,
     *          WERNC   ,  WERCL   ,  VILNC   ,  VILCL
      INTEGER   RCMIGP  ,  LCTNC   ,  LCTCL
      INTEGER PPHRLYNC,PPHRLYCL,STMTOTNC,STMTOTCL,PPQTRLFM
      INTEGER CSHRCL,CSHRNC, VADRMS,  VADV2TBL
      INTEGER SRRCL161,SRRNC161,SRRCL162,SRRNC162
      INTEGER CLUTFILC
C***********************************************************
      PARAMETER (REFNC16 = 1)
      PARAMETER (REFCL16 = 2)
      PARAMETER (REFNC8  = 3)
      PARAMETER (REFCL8  = 3)
      PARAMETER (VELNC161= 4)
      PARAMETER (VELCL161=26)
      PARAMETER (VELNC81 = 5)
      PARAMETER (VELCL81 =27)
      PARAMETER (VELNC162= 6)
      PARAMETER (VELCL162=28)
      PARAMETER (VELNC82 =7)
      PARAMETER (VELCL82 =29)
      PARAMETER (SPCL8   =8)
      PARAMETER (SPNC8   =8)
      PARAMETER (CSHRCL = 9)
      PARAMETER (CSHRNC = 9)
      PARAMETER (ECNC16  =10)
      PARAMETER (ECCL16  =10)
      PARAMETER (HRSNC16 =11)
      PARAMETER (HRSCL16 =11)
      PARAMETER (VADRMS = 13)
      PARAMETER (WERNC   =14)
      PARAMETER (WERCL   =14)
      PARAMETER (SRRNC161 = 15)
      PARAMETER (SRRCL161 = 15)
      PARAMETER (SRRNC162 = 6 )
      PARAMETER (SRRCL162 = 6 )
      PARAMETER (VILNC   =16)
      PARAMETER (VILCL   =16)
      PARAMETER (ALYCRFCL=17)
      PARAMETER (ALYCRFNC=18)
      PARAMETER (LCTNC   =19)
      PARAMETER (LCTCL   =19)
      PARAMETER (RCMCL   =20)
      PARAMETER (RCMNC   =20)
      PARAMETER (RCMIGP  =21)
      PARAMETER (PPHRLYNC =22)
      PARAMETER (PPHRLYCL =22)
      PARAMETER (STMTOTNC =23)
      PARAMETER (STMTOTCL =23)
      PARAMETER (PPQTRLFM = 24 )
      PARAMETER (VADV2TBL = 25)
      PARAMETER (CLUTFILC = 30)
/*
**A3PM06
*VERSION:0001
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C  USE: Define parameters used to define and dimension the     C
C       Scan Summary Table.
C***************************************************************
C***************************************************************
C
C     Parameters used for Scan Summary Table definitions:
C
      INTEGER MAX_VSCANS
      INTEGER SCAN_PARAMS
      INTEGER SCAN_DATE
      INTEGER SCAN_TIME
      INTEGER SCAN_MODE
      INTEGER SCAN_VCP
      INTEGER SCAN_NUM_ELV
      INTEGER SCAN_SB
      INTEGER SCAN_SUPER_RES
C
      PARAMETER (MAX_VSCANS=80)
      PARAMETER (SCAN_PARAMS=13)
      PARAMETER (SCAN_DATE=1)
      PARAMETER (SCAN_TIME=2)
      PARAMETER (SCAN_MODE=3)
      PARAMETER (SCAN_VCP = 4)
      PARAMETER (SCAN_NUM_ELV = 5)
      PARAMETER (SCAN_SB = 6)
      PARAMETER (SCAN_SUPER_RES = 7)
C
/*
**A3PM07
*VERSION:2
C*********************************************************************
C*
C*         INTER-TASK COMMUNICATION PARAMETERS DEFINED
C*
C*********************************************************************
C*
      INTEGER INPUT_AVAILABLE
      INTEGER START_OF_VOLUME_SCAN

      INTEGER PROD_MEM_SHED 
      INTEGER PROD_CPU_SHED 
      INTEGER PROD_TASK_FAILED 
      INTEGER PROD_TASK_NOT_LOADED 
      INTEGER PROD_CUST_MEM_SHED 
      INTEGER PROD_DISABLED_MOMENT 

      PARAMETER( PROD_MEM_SHED = 8)
      PARAMETER( PROD_CPU_SHED = 9)
      PARAMETER( PROD_TASK_FAILED = 10)
      PARAMETER( PROD_TASK_NOT_LOADED = 11)
      PARAMETER( PROD_CUST_MEM_SHED = 12)
      PARAMETER( PROD_DISABLED_MOMENT = 13)
C*
      PARAMETER(INPUT_AVAILABLE=20)
      PARAMETER(START_OF_VOLUME_SCAN=21)
C*
C*********************************************************************
/*
**A3PM10
*VERSION: 0
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
C   SIZING FOR LARGEST PRODUCT PUP CAN RECEIVE
C   WHICH IS 80K BYTES
C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
      INTEGER SZ_LRG_PROD_BYTE
      INTEGER SZ_LRG_PROD_WORD
      PARAMETER (SZ_LRG_PROD_BYTE = 81920)
      PARAMETER (SZ_LRG_PROD_WORD = SZ_LRG_PROD_BYTE/4)
/*
**A3PM13
*VERSION: 0
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
C***  DIMENSIONS OF CRITICAL RANGE-POINTS TABLES:
C***  (FOR CARTESIAN HOLE-FILLING (SMOOTHING) OPERATION)
C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
C*** INDICES OF CRITICAL RANGES WITHIN TABLES:
C        LC         -  LEFT CRITICAL RANGE
C        LB         -  LEFT BEGINNING RANGE
C        RB         -  RIGHT BEGINNING RANGE
C        RC         -  RIGHT CRITICAL RANGE
C        G230_R1X1  - GRID 0-230KM; RESOLUTION 1X1; TOTAL 464X464
C        G460_R4X4  - GRID 0-460KM; RESOLUTION 4X4; TOTAL 232X232
C        G230_R2X2  - GRID 0-230KM; RESOLUTION 2X2; TOTAL 232X232
C                     (NOTE: [0-460; 4X4] GRID & [0-230; 2X2] GRID
C                      HAVE SAME CRITICAL-RANGE DEFINITIONS, AND
C                      INDEX INTO SAME TABLE).
C
      INTEGER  MAXBOX, NCRITPTS, NGRIDTYP
      PARAMETER ( MAXBOX=464, NCRITPTS=4, NGRIDTYP=2 )
      INTEGER  LC, LB, RB, RC
      PARAMETER( LC=1, LB=2, RB=3, RC=4 )
      INTEGER  G230_R1X1, G460_R4X4, G230_R2X2
      PARAMETER( G230_R1X1=1, G460_R4X4=2, G230_R2X2=2 )
/*
**A3PM14
*VERSION: 0
$INCLUDE orpgdat.inc,**ORPGDAT
C
C   THIS ELEMENT CONTAINS PARAMETERS FOR THE RAIN GAGE PROCESSING
C
      INTEGER NUM_RGDACS
      PARAMETER (NUM_RGDACS=2)
C
C  DEFINE  THE LOGICAL UNIT FOR THE RAIN GAGE DISK FILE
C
      INTEGER RGLU
C$INSERTC     ORPG Issue 6
C$INSERT      PARAMETER (RGLU=GAGEDATA)
C$DELETE
      PARAMETER (RGLU=2)
C$ENDDELETE      
C
C  DEFINE THE NUMBER OF BYTES IN THE OUTGOING STATUS MSG
C
      INTEGER BYTES_STATMSG
      PARAMETER(BYTES_STATMSG=38)
C
C  DEFINE SIZES OF TABLES IN COMMON BLOCK A305C1
C
      INTEGER*4  GAGE_ITEMS, MAXGAGES,PASSITMS
      PARAMETER (GAGE_ITEMS=7,MAXGAGES=50,PASSITMS=2)
C
C  INDEX OF ITEMS WITHIN 'GAGE_ID' TABLE:
C
      INTEGER*4  GAGEID1,GAGEID2,GAGELAT,GAGELON
      INTEGER*4  GAGEAZ,GAGERNG,GAGERPT
      PARAMETER (GAGEID1=1,GAGEID2=2,GAGELAT=3,GAGELON=4)
      PARAMETER (GAGEAZ=5,GAGERNG=6,GAGERPT=7)
C
C
C
C  DEFINE THE SIZES AND PARAMETERS FOR COMMON C3
C
      INTEGER NREP_PARMS
      INTEGER NREP_GAGE
      INTEGER NGAGES_USER
      INTEGER TYPE_INCRMENT,TYPE_ACCUM
      PARAMETER(TYPE_INCRMENT=1,TYPE_ACCUM=2)
      INTEGER RDATEIX,RTIMMSB,RTIMLSB,RTYPIX,RDURIX,RVALIX
      PARAMETER(RDATEIX=1,RTIMMSB=2,RTIMLSB=3,RTYPIX=4,RDURIX=5)
      PARAMETER(RVALIX=6)
      PARAMETER(NREP_PARMS=6,NREP_GAGE=50,NGAGES_USER=50)
C
C  THE FOLLOWING VARIABLES ARE USED AS WORKCODES FOR THE
C  ROUTINES THAT READ AND WRITE THE GAGE DATA TO DISC.
C
      INTEGER GREAD,GWRITE
      PARAMETER (GREAD=1,GWRITE=2)
C
C
/*
**A3PM50
*VERSION:02
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                 INCLUDE BLOCK A3PM50.INC                     C
C                                                              C
C   Parameter file containing parameterized values identifying C
C   the scaling factors needed to convert from English values  C
C   to Metric values, or to convert from Metric values to Eng- C
C   lish values.                                               C
C                                                              C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
C    NAME             TYPE    DESCRIPTION
C    ---------------  ------  --------------------------------------
C    FT_TO_KM          REAL   Defines the parameter value for the   *
C                             scaling factor for Feet to Kilometers.*
C                                                                   *
C    FT_TO_M           REAL   Defines the parameter value for the   *
C                             scaling factor for feet to meters.    *
C                                                                   *
C    IN_TO_MM          REAL   Defines the parameter value for the   *
C                             scaling factor for Inch to Millimeters*
C                                                                   *
C    KM_TO_FT          REAL   Defines the parameter value for the   *
C                             scaling factor for Kilometer to Feet. *
C                                                                   *
C    KM_TO_NM          REAL   Defines the parameter value for the   *
C                             scaling factor for Kilometer to Naut- *
C                             ical miles.                           *
C                                                                   *
C    KTS_TO_MPS        REAL   Defines the parameter value for the   *
C                             scaling factor for Knots to Meters per*
C                             second.                               *
C                                                                   *
C    M_TO_FT           REAL   Defines the parameter value for the   *
C                             scaling factor for Meter to Feet.     *
C                                                                   *
C    M_TO_KFT          REAL   Defines the parameter value for the   *
C                             scaling factor for Meter to Kilo-     *
C                             feet.                                 *
C                                                                   *
C    MM_TO_IN          REAL   Defines the parameter value for the   *
C                             scaling factor for Millimeter to      *
C                             Inches.                               *
C                                                                   *
C    MPS_TO_KTS        REAL   Defines the parameter value for the   *
C                             scaling factor for Meters Per Second  *
C                             to Knots.                             *
C                                                                   *
C    NM_TO_KM          REAL   Defines the parameter value for the   *
C                             scaling factor for Nautical miles to  *
C                             Kilometers. (International units)     *
C                                                                   *
C    NM_TO_M           REAL   Defines the parameter value for the   *
C                             scaling factor for Nautical miles to  *
C                             Meters.  (International units)        *
C                                                                   *
C    PI_CONST          REAL   Defines the parameter value for the   *
C                             PI_CONST variable.                    *
C                                                                   *
C    ONE_RADIAN        REAL   Defines the parameter value for the   *
C                             ONE_RADIAN variable.                  *
C                                                                   *
C********************************************************************
C
C   Global Scaling Variables
C
      REAL FT_TO_KM, IN_TO_MM, KM_TO_FT
      REAL KM_TO_NM, KTS_TO_MPS, M_TO_FT
      REAL M_TO_KFT, MM_TO_IN, MPS_TO_KTS
      REAL NM_TO_KM, NM_TO_M, FT_TO_M
      REAL M_TO_KM
C
C   Global Scaling Parameters
C
      PARAMETER (FT_TO_KM = 0.0003048)
      PARAMETER (FT_TO_M  = 0.30479999)
      PARAMETER (IN_TO_MM = 25.4)
      PARAMETER (KM_TO_FT = 3280.84)
      PARAMETER (KM_TO_NM = 0.53996)
      PARAMETER (KTS_TO_MPS = 0.5144562)
      PARAMETER (M_TO_FT = 3.28084)
      PARAMETER (M_TO_KFT = 0.00328084)
      PARAMETER (MM_TO_IN = 0.03937008)
      PARAMETER (MPS_TO_KTS = 1.9438)
      PARAMETER (NM_TO_KM = 1.852)
      PARAMETER (NM_TO_M  = 1852.0)
      PARAMETER (M_TO_KM = 0.001)

C
C    RADIAN & PI VARIABLES
C
      REAL ONE_RADIAN, PI_CONST
      PARAMETER (ONE_RADIAN = 0.01745329)
      PARAMETER (PI_CONST = 3.141593)

/*
**A3CD00
*VERSION:0
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C  USE: Common data file for declaring the area defining the
C       maximum amount of memory available in the RPG system
C       for buffer sharing and for declaration of the shared
C       memory array
C**************************************************************
C**************************************************************
C
C    NAME             TYPE    DESCRIPTION
C    ---------------  ------  --------------------------------------
C    MEM              INT*4   ARRAY IN RPG SHARED MEMORY SET ASIDE
C                             FOR BUFFER SHARING.
C
C    WCOFFSET         INT*4   DEFINES THE MAXIMUM INDEX OF MEM. THE
C                             VALUE IS CHOSEN TO ACCOMODATE COMPILE-
C                             TIME CONSTANT REFERENCES TO THE
C                             LARGEST OFFSET INTO AN ALLOCATED
C                             BUFFER.
C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
      INTEGER WCOFFSET
      PARAMETER (WCOFFSET=2)
C
      INTEGER MEM(WCOFFSET)
C
      COMMON /A3CD00/ MEM
/*
**A3CD05
*VERSION:0001
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C  USE: Define the Product Index and the Scan Summary Table    C
C       to be accessed by the various tasks of the RPG.        C
C***************************************************************
C***************************************************************
C
C*    Spot Blank Field in Scan Summary Table definition:
C
      INTEGER*2 VOL_SPOT_BLANK
      PARAMETER (VOL_SPOT_BLANK = 0)
C
C*    Spot Blanking Enabled in product definition:
C
      INTEGER*2 SBON
      PARAMETER (SBON = 1)
C
C*    Scan Summary Table definition:
      INTEGER*4 SUMMARY(SCAN_PARAMS,0:MAX_VSCANS)      
C
      COMMON /A3CD05/ SUMMARY
C
/*
**A3CD06
*VERSION: 0000
C !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
C
C----------------------------------------------------------
C   VOLUME STATUS VARIABLES
C   (see gen_stat_msg.h for definition of variables)
C----------------------------------------------------------
C
      INTEGER*4 MAX_CUTS
      PARAMETER( MAX_CUTS = 25 )

      INTEGER*4 RPGVSEQNUM
      INTEGER*4 RPGVSTIME
      INTEGER*4 RPGVSDATE
      INTEGER*4 RPGINITVOL
      INTEGER*4 RPGPVSTAT
      INTEGER*4 RPGXVOLD
      INTEGER*4 RPGVSNUM
      INTEGER*4 RPGWMODE
      INTEGER*4 RPGDUALPOL
      INTEGER*4 RPGVCP
      INTEGER*4 RPGVCPID
      INTEGER*4 RPGNELEVS
      INTEGER*4 RPGSRCUTS
      INTEGER*2 RPGVCPSUP
      INTEGER*2 RPGSPARE
      INTEGER*2 ELEV_ANG(MAX_CUTS)
      INTEGER*2 ELEV_IND(MAX_CUTS)
      INTEGER*2 CURR_VCP_TAB( VCPSIZ )
C
C------------------------------------------------------------------
C     COMMON BLOCK DEFINITION FOR VOLSTAT (see also gen_stat_msg.h)
C--------------------------------------------------------------------
C
      COMMON /VOLSTAT/ RPGVSEQNUM, RPGVSTIME, RPGVSDATE
      COMMON /VOLSTAT/ RPGINITVOL, RPGPVSTAT, RPGXVOLD
      COMMON /VOLSTAT/ RPGVSNUM, RPGWMODE, RPGDUALPOL 
      COMMON /VOLSTAT/ RPGVCP, RPGVCPID, RPGNELEVS 
      COMMON /VOLSTAT/ ELEV_ANG, ELEV_IND, RPGSRCUTS
      COMMON /VOLSTAT/ RPGVCPSUP, RPGSPARE, CURR_VCP_TAB
C
/*
**A3CD07
*VERSION: 0007
C !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
C
C   Task ID to which Status Change Notification Parameter is queued
      CHARACTER*8 STATUS_TASK
      PARAMETER (STATUS_TASK = 'STATMON ')
C
      INTEGER CNDISKFILE
      PARAMETER( CNDISKFILE = 2700 )

      INTEGER RDA_PRIMARY
      PARAMETER( RDA_PRIMARY = 0 )

C----------------------------------------------------------
C  Following are the definitions of the various elements that
C  comprise the RPG "State File".  
C----------------------------------------------------------
C----------------------------------------------------------
C----------------------------------------------------------
C----------------------------------------------------------
C   STATE FILE VARIABLES
C----------------------------------------------------------
C
C TIME_GDB_UPD        Time of last gage database update
C                     (secs since 00:00:00,01/01/70).
C
      INTEGER*4 TIME_GDB_UPD
C
C     The following is to support the Clutter Filter Control Product
C
      LOGICAL NW_MAP_AVAIL, BYPASS_MAP_AVAIL
      INTEGER*2 BM_GENDATE, BM_GENTIME
C
C***  DEFINE DATA THAT WILL CONTAIN THE MODIFIED JULIAN DATE AND
C***  HOUR (0-23) OF THE MOST RECENT 1 HOUR RAINFALL PERIOD IN THE
C***  30 HOUR DATABASE USED FOR THE USP PRODUCT.
C
      INTEGER*2 LAST_DATE_HRDB,LAST_TIME_HRDB
      LOGICAL   STP_EDIT_FLAG, OHP_EDIT_FLAG
C
C------------------------------------------------------------------
C
C--------------------------------------------------------------------
C
      COMMON /STATFIL/ NW_MAP_AVAIL
      COMMON /STATFIL/ BYPASS_MAP_AVAIL
      COMMON /STATFIL/ BM_GENDATE
      COMMON /STATFIL/ BM_GENTIME
C
      COMMON /A3CD07/ LAST_DATE_HRDB
      COMMON /A3CD07/ LAST_TIME_HRDB
      COMMON /A3CD07/ STP_EDIT_FLAG
      COMMON /A3CD07/ OHP_EDIT_FLAG
/*
**A3CD08
*VERSION: 0
C
C   CONTAINS ASCII VALUES FOR STORM ID-S.
C
C
      CHARACTER*2 CHARIDTABLE(260)
/*
**A3CD08D
      DATA (CHARIDTABLE(I),I=1,63)/'A0','B0','C0','D0','E0','F0',
     1   'G0','H0','I0','J0','K0','L0','M0','N0','O0','P0','Q0','R0',
     2   'S0','T0','U0','V0','W0','X0','Y0','Z0','A1','B1','C1','D1',
     3   'E1','F1','G1','H1','I1','J1','K1','L1','M1','N1','O1','P1',
     4   'Q1','R1','S1','T1','U1','V1','W1','X1','Y1','Z1','A2','B2',
     5   'C2','D2','E2','F2','G2','H2','I2','J2','K2'/
      DATA(CHARIDTABLE(I),I=64,126)/'L2','M2','N2','O2','P2','Q2',
     1   'R2','S2','T2','U2','V2','W2','X2','Y2','Z2','A3','B3','C3',
     2   'D3','E3','F3','G3','H3','I3','J3','K3','L3','M3','N3','O3',
     3   'P3','Q3','R3','S3','T3','U3','V3','W3','X3','Y3','Z3','A4',
     4   'B4','C4','D4','E4','F4','G4','H4','I4','J4','K4','L4','M4',
     5   'N4','O4','P4','Q4','R4','S4','T4','U4','V4'/
      DATA(CHARIDTABLE(I),I=127,189)/'W4','X4','Y4','Z4','A5','B5',
     1   'C5','D5','E5','F5','G5','H5','I5','J5','K5','L5','M5','N5',
     2   'O5','P5','Q5','R5','S5','T5','U5','V5','W5','X5','Y5','Z5',
     3   'A6','B6','C6','D6','E6','F6','G6','H6','I6','J6','K6','L6',
     4   'M6','N6','O6','P6','Q6','R6','S6','T6','U6','V6','W6','X6',
     5   'Y6','Z6','A7','B7','C7','D7','E7','F7','G7'/
      DATA(CHARIDTABLE(I),I=190,260)/'H7','I7','J7','K7','L7','M7',
     1   'N7','O7','P7','Q7','R7','S7','T7','U7','V7','W7','X7','Y7',
     2   'Z7','A8','B8','C8','D8','E8','F8','G8','H8','I8','J8','K8',
     3   'L8','M8','N8','O8','P8','Q8','R8','S8','T8','U8','V8','W8',
     4   'X8','Y8','Z8','A9','B9','C9','D9','E9','F9','G9','H9','I9',
     5   'J9','K9','L9','M9','N9','O9','P9','Q9','R9','S9','T9','U9',
     6   'V9','W9','X9','Y9','Z9'/
/*
**A3CD09
*VERSION:0
C*********************************************************************
C
C  USE: Common block for Image file shared by algorithm SEGMENTS
C       and products WEAK ECHO REGION & STORM RELATIVE MEAN RADIAL
C       VELOCITY. Data values are filled by the algorithm, for use
C       by the procucts.
C
C*********************************************************************
C
C    NAME             TYPE    DESCRIPTION
C    ---------------  ------  ----------------------------------------
C    NUMSTRM          INT*4   Total no. of storms this volume scan
C
C    TIMETAG          REAL*4  Time of beginning of vol scan (Julian),
C                                       in milliseconds past midnight
C    AVGSTSPD         REAL*4  Average speed of all storms in scan
C                                                              (m/s)
C    AVGSTDIR         REAL*4  Average direction of all storms in scan
C                                                       (degrees from)
C    STRMID( )        INT*4   Array containing ID-labels of all storms
C
C    STRMOVE( , )     REAL*4  Array containing X & Y positions and
C                                          vector motion of each storm
C    LOKID            LOG:*4  Test & set bit for locking/unlocking
C                            shared common block (for allowing writes)
C---------------------------------------------------------------------
C
C** Dimensional Parameters for internal components table:
C
      INTEGER   NSTR_INX, NSTR_MOV, NSTR_TOT
      PARAMETER ( NSTR_INX=2, NSTR_MOV=12, NSTR_TOT=100 )
C
C** Positional Parameters for internal components table:
C
C   STR_XPO - Positional Storm Parameter Index for X Position of the Storm.
C   STR_YPO - Positional Storm Parameter Index for Y Position of the Storm.
C   STR_DIR - Positional Storm Parameter Index for the Vector Motion of 
C             the Storm.
C   STR_SPD - Positional Storm Parameter Index for the Relative Speed of 
C             the Storm.
C   STR_XSP - Positional Storm Parameter Index for X Component of Speed of 
C             the Storm.
C   STR_YSP - Positional Storm Parameter Index for Y Component of Speed of 
C             the Storm.
C   STR_AZM - Positional Storm Parameter Index for Azimuth Position of 
C             the Storm.
C   STR_RAN - Positional Storm Parameter Index for Range Position of 
C             the Storm.
C   STR_MRF - Positional Storm Parameter Index for Maximum Reflectivity of 
C             the Storm.
C   STR_MRF - Positional Storm Parameter Index for Height of Maximum Reflectivity
C             of the Storm.
C   STR_VIL - Positional Storm Parameter Index for Cell-based VIL of the Storm.
C   STR_TOP - Positional Storm Parameter Index for Top of the Storm.

C
      INTEGER   STR_XPO, STR_YPO, STR_XSP, STR_YSP, STR_SPD, STR_DIR
      INTEGER   STR_AZM, STR_RAN, STR_MRF, STR_RFH, STR_VIL, STR_TOP
      PARAMETER ( STR_XPO=1, STR_YPO=2, STR_XSP=3, STR_YSP=4,
     $            STR_SPD=5, STR_DIR=6, STR_AZM=7, STR_RAN=8,
     $            STR_MRF=9, STR_RFH=10, STR_VIL=11, STR_TOP=12 )
C
C** Array & Variable Definitions:
C
      INTEGER  NUMSTRM, STRMID( NSTR_TOT )
C
      LOGICAL  LOKID
C
      REAL    TIMETAG, AVGSTSPD, AVGSTDIR
      REAL    STRMOVE( NSTR_MOV, NSTR_TOT )
C
C** Named Common:
C
      COMMON /A3CD09/  NUMSTRM, TIMETAG, AVGSTSPD, AVGSTDIR
     $                 ,STRMID, STRMOVE, LOKID
/*
**A3CD09DZ
      REAL SEGMAIN( NSEG_ATR, NSEG_MAX * 2 )
      REAL SEGAZIM( NRAD_ELV * 2 )
      REAL SEGBUF_TH( NSEG_REF )
      INTEGER*2 SEGINDX( NSEG_NID, NSEG_REF, NRAD_ELV * 2 )
      COMMON/SGMTS09/ SEGMAIN,SEGINDX,SEGAZIM,SEGBUF_TH
/*
**A3CD09BL
C*********************************************************************
C
C  USE: Common block for Image file shared by TDA 1D task and
C       TDA 2D (&3D) task.  It contains arrays with the pattern
C       vector attributes and pointers to the pattern vectors by
C       radial number.  Plus an array of all the azimuth angles in
C       the elevation scan, in degrees.
C
C*********************************************************************
      REAL TDAMAIN( N1D_ATR, N1D_MAX * 2)
      REAL PV_AZIM( NRAD_ELEV * 2 )
      INTEGER*2 PV_INDX( N1D_NID, NRAD_ELEV * 2 )
      COMMON/PVECS09/ TDAMAIN,PV_INDX,PV_AZIM
/*
**A3CD11
*VERSION: 0
C
C *** LOCAL COPY OF ADAPTATION DATA FOR MESO 2D AND 3D TASKS
C *** WHICH IS EVENTUALLY PASSED TO 'MEM' BUFFER AT END OF VOLUME:
C
      INTEGER*4 IMXNFEAT,IMXNMES,INPVTHR
C
      REAL IHMTHR,ILMTHR,IHSTHR,ILSTHR,IMRTHR,IFMRTHR,
     1     INRTHR,IFNRTHR,IRNGTHR,IDISTHR,IAZTHR,IAZTHR1,IFHTHR
C
      COMMON /A3CD11/ IMXNFEAT,IMXNMES,INPVTHR,IHMTHR,ILMTHR,IHSTHR,
     1                ILSTHR,IMRTHR,IFMRTHR,INRTHR,IFNRTHR,IRNGTHR,
     2                IDISTHR,IAZTHR,IAZTHR1,IFHTHR
/*
**A3CD70
*VERSION:0
C
C  THIS ELEMENT IS NO LONGER REQUIRED FOR COMPILATION.  THE INDIVIDUAL
C  ELEMENTS OF A309ADPT.INC EACH HAVE A COMMON BLOCK
C  DEFINITION FOR THE RELATED CATEGORY OF ADAPTATION DATA.
C  H O W E V E R :  THE NAME A3CD70 IS STILL USED FOR THE SHARED
C                   SEGMENT WHICH CONTAINS ADAPTATION DATA.
C
/*
**A3CD80C1
*VERSION: 0
C
C  THE GAGE-ID,GAGE-ORDER,AND THE GAGE_REL_DISTANCE TABLES
C  THE START_GID AND END_GID VARIABLES ARE USED TO CALCULATE THE
C  LENGTH OF THE BLOCK FOR THE FILE WRITE PROCEDURE AND SHOULD
C  ALWAYS REMAIN AT THE BEGINNING AND END OF THE BLOCK.
C
C
      INTEGER START_GID,END_GID
      INTEGER GAGE_PASS(PASSITMS)
      INTEGER GAGE_PASSFLG
      INTEGER   GAGE_ID(GAGE_ITEMS,MAXGAGES)
      INTEGER   WGAGE_ID(GAGE_ITEMS,MAXGAGES)
      INTEGER*2 GAGE_ORDER(MAXGAGES)
      INTEGER*2 GAGE_REL_DIST(MAXGAGES,MAXGAGES)
C
      COMMON/A3CD80C1/ START_GID
      COMMON/A3CD80C1/ GAGE_PASS,GAGE_PASSFLG
      COMMON/A3CD80C1/ GAGE_ID,GAGE_ORDER,GAGE_REL_DIST
      COMMON/A3CD80C1/ WGAGE_ID
      COMMON/A3CD80C1/ END_GID
C
/*
**A3CD80C3
*VERSION: 0
C
C  THIS IS THE COMMON BLOCK THAT IS USED TO MAINTAIN THE
C  PRECIPITATION STATUS DATA AS NON-ASCII DATA. THIS IS THE
C  SOURCE OF DATA FOR THE MODULE THAT FORMATS THE PRECIPITATION
C  STATUS MESSAGE TO BE TRANSMITTED.
C
      INTEGER START_STAT,END_STAT
      INTEGER*4 DATE_DETECT_RAN
      INTEGER*4 TIME_DETECT_RAN
      INTEGER*4 DATE_LAST_PRECIP
      INTEGER*4 TIME_LAST_PRECIP
      INTEGER*4 CUR_PRECIP_CAT
      INTEGER*4 LAST_PRECIP_CAT
      INTEGER*4 NUM_GAGES
      INTEGER*4 DATE_GDB_UPDATE
      INTEGER*4 TIME_GDB_UPDATE
      INTEGER*4 GDB_UPDATE_PENDING
C
      COMMON/A3CD80C3/ START_STAT
      COMMON/A3CD80C3/ DATE_DETECT_RAN,TIME_DETECT_RAN,
     *               DATE_LAST_PRECIP,TIME_LAST_PRECIP,
     *               CUR_PRECIP_CAT,LAST_PRECIP_CAT,
     *               NUM_GAGES,
     *               DATE_GDB_UPDATE,TIME_GDB_UPDATE,
     *               GDB_UPDATE_PENDING
      COMMON/A3CD80C3/END_STAT
C
/*
**A3CD80C2
*VERSION: 0
C
C  THIS IS THE COMMON BLOCK THAT HAS THE GAUGE REPORTS.
C  NOTE THAT THE TABLE IS DEFINED SUCH THAT ONLY THE
C  REPORTS FROM ONE SINGLE USER ARE IN THE TABLES
C  AT A TIME.
C  THE TABLES ARE KEPT UP-TO-DATE ON DISC.
C
      INTEGER START_REPORTS
      INTEGER END_REPORTS
      INTEGER*2 GAGE_REPORTS
C
      COMMON /A3CD80C2/ START_REPORTS
      COMMON /A3CD80C2/ GAGE_REPORTS(NREP_PARMS,NREP_GAGE,NGAGES_USER)
      COMMON /A3CD80C2/ END_REPORTS
C
/*
**A3CD97
*VERSION: 0002
*
      INTEGER*4 LEN_EWTAB, NPARMS, BASEHGT, HGTINC, NEPARAMS
      INTEGER*2 MTTABLE_INT
      REAL MTTABLE
      PARAMETER (LEN_EWTAB = 70)
      PARAMETER (NEPARAMS = 2)
      PARAMETER (NPARMS  = 2)
      PARAMETER (MTTABLE = 32767.0)
      PARAMETER (MTTABLE_INT = 32767)
      PARAMETER (HGTINC = 1000)
*
      REAL EWTAB( LEN_EWTAB, NPARMS )
      LOGICAL ENVWNDFLG
      INTEGER*2 NEWNDTAB( NEPARAMS, LEN_EWTAB )
      INTEGER*4 SOUND_TIME
      LOGICAL*4 VALID_SOUNDINGS
*
      COMMON /A3CD97/ ENVWNDFLG,EWTAB,BASEHGT,NEWNDTAB
      COMMON /A3CD97/ SOUND_TIME, VALID_SOUNDINGS
      INTEGER*4 WNDSPD, WNDDIR, NCOMP, ECOMP
      PARAMETER (WNDDIR = 1)
      PARAMETER (WNDSPD = 2)
      PARAMETER (NCOMP  = 1)
      PARAMETER (ECOMP  = 2)
*
/*
**A3CD98
*VERSION: 0
*
C***  PRECIPTIATION PRE-PROCESSING:
*
      INTEGER LOWOCC, HIGHOCC, NUMAZ, NUMEL, NDELIMS, NSECTSM, NLEVS
      INTEGER NDESCR
      PARAMETER (LOWOCC = 0)
      PARAMETER (HIGHOCC = 5)
      PARAMETER (NUMAZ   = 3600)
      PARAMETER (NUMEL   = 40)
      PARAMETER (NDELIMS = 4)
      PARAMETER (NSECTS  = 10)
      PARAMETER (NLEVS   = 4)
      PARAMETER (NDESCR  = 4)
*
      INTEGER*2 OCCODE(LOWOCC:HIGHOCC, NUMAZ, NUMEL)
      INTEGER*2 EL_AZ_RAN(NDELIMS, NSECTS, NLEVS)
      INTEGER*2 SECT_DESCR( NDESCR, NLEVS )
      COMMON /A3CD98/OCCODE, EL_AZ_RAN, SECT_DESCR
/*
**A3CD99
*VERSION:1
C*************************************************
C*                 A3CD99.CMN                    *
C*                                               *
C* PARAMETER AND COMMON STATEMENTS FOR THE BASE  *
C* DATA LOOK-UP TABLE (A3CD99.BLK) THAT IS USED  *
C* TO CONVERT THE I*2 RADIAL BASE DATA           *
C* (VALUES FROM 2-255) TO METEOROLOGICALLY       *
C* MEANINGFUL VALUES IN REAL*4 FOR ALGORITHM     *
C* PROCESSING                                    *
C*                                               *
C*************************************************
C
C N_MAXVAL = THE MAXIMUM VALUE IN EACH LOOK-UP TABLE.
C N_MODES = THE NUMBER OF DOPPLER MODES.
C
C REF(0:N_MAXVAL) = THE TABLE VALUE AT POSITION 0 TO N_MAXVAL FOR
C REFLECTIVITY, IN DBZE.
C VEL(0:N_MAXVAL,N_MODES) = THE TABLE VALUE AT POSITION 0 TO
C N_MAXVAL, MODE 1 OR 2, FOR DOPPLER VELOCITY, IN M/S.
C SPW(0:N_MAXVAL) = THE TABLE VALUE AT POSITION 0 TO N_MAXVAL FOR
C SPECTRUM WIDTH, IN M/S.
C
      INTEGER*2 MODE,N_MAXVAL,N_MODES
      PARAMETER(N_MAXVAL=256,N_MODES=2)
C
      REAL REF(0:N_MAXVAL),VEL(0:N_MAXVAL,1:N_MODES),SPW(0:N_MAXVAL)
#IFDEF LINUX
      INTEGER*4 IZX,IVX,IWX
#ELSE
      INTEGER*2 IZX,IVX,IWX
#ENDIF
C
      COMMON /A3CD99/ REF,SPW,VEL
/*
