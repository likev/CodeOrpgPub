XCOMM RCS info 
XCOMM $Author: garyg $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2003/02/18 22:15:13 $ 
XCOMM $Id: a316.inc,v 1.4 2003/02/18 22:15:13 garyg Exp $ 
XCOMM $Revision: 1.4 $ 
XCOMM $State: Exp $ 
*.********************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A316.INC
*.
*.  INCLUDE FILE VERSION: 04
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#          PROGRAMMER         NOTES
*.  ---------    --   -----         ----------------   ---------------
*.  06/01/90     00   90318         P. STALEY
*.  07/11/90     01   90819         G. JUBINSKI
*.  11/14/91     02   92165         PAUL JENDROWSKI    **A316P1
*.  02/12/92     03   92378         PAUL JENDROWSKI    **A3162I
*.  05/04/94     04   93011-002A00  Steve Smith        **A3164P4
*.  07/21/94     04   NW-94-060-15  Steve Smith        **A3162IP
*.********************************************************************
**A3162PI
*VERSION:4
C****************FOR STORM RELATIVE MEAN RADIAL VELOCITY**************
C*                                                                   *
C*          ***** DIMENSIONAL PARAMETERS *****                       *
C* MAXSRMRV       Maximum number of products possible                *
C* NUMCTL_PARM    Number of parameters in the product control table  *
C*          ******* OFFSET PARAMETERS ********                       *
C* MSGID          Offset to message code                             *
C* BUFID          Offset to buffer ID.                               *
C* CAZ            Offset to window center azimuth                    *
C* CRNG           Offset to product center range                     *
C* STSP           Offset to storm speed (m/s*10) to remove           *
C* KSTSP          Offset to storm speed (knots*10) to remove         *
C* STDR           Offset to storm direction                          *
C* OPOINT         Offset to product buffer pointer                   *
C* GENDATE        Offset to product generation date                  *
C* GENTIME        Offset to product generation time                  *
C* MAX_AZ         Offset to the product max. azimuth                 *
C* MIN_AZ         Offset to product min. azimuth ang.                *
C* MAX_RG         Offset to the product max. range                   *
C* MIN_RG         Offset to product minimum range                    *
C* ELFLAG         Offset to flag indication first radial falls within*
C*                a window                                           *
C* INCZDEG        Offset to flag indicating that the product includes*
C*                zero degrees azm.                                  *
C* COLIX          Offset - product color lookup table                *
C* MINIBIN        Offset to min. bin to process in the input radial  *
C* MAXIBIN        Offset to max. bin # to process on the input radial*
C* BINC           Offset to bin increment for product                *
C* MAXOBIN        Offset to maximum bin number of the storm motion   *
C*                removed radial                                     *
C* MAXI2_IN_OBUF  Offset to # of I*2 words allocated for the product *
C* NEED           Offset to flag denoting the current radial is      *
C*                needed by the product                              *
C* STATUS         Offset to product status flag                      *
C* NUM_RADLS      Offset to the number of radials in the product     *
C* BUFCNT         Offset to # of I*2 words in product                *
C* NRLEB          Number of RLE bytes for this radial                *
C* MAXNEG         Offset to maximum neg. vel. found in the product   *
C* MAXPOS         Offset to maximum pos. vel. found in the product   *
C* STRMINFO       Offset to orgin of storm motion value              *
C*                                                                   *
C*          ******* ARRAY DEFINATIONS ********                       *
C* TBUFCTL(NUMCTL_PARM,MAXSRMRV)   Product control table             *
C*          ***** SUB-ARRAY DEFINATIONS ******                       *
C* TBUFCTL(MSGID,MAXSRMRV)         Product codes (55 or 56)          *
C* TBUFCTL(BUFID,MAXSRMRV)         Buffer data types (68 or 69)      *
C* TBUFCTL(CAZ,MAXSRMRV)           Window center azimuths (deg)      *
C* TBUFCTL(CRNG,MAXSRMRV)          Product center ranges (km)        *
C* TBUFCTL(STSP,MAXSRMRV)          Storm speeds to remove (m/s*10)   *
C* TBUFCTL(STDR,MAXSRMRV)          Storm directions (deg*10)            *
C* TBUFCTL(OPOINT,MAXSRMRV)        Pointers to product buffers       *
C* TBUFCTL(GENDATE,MAXSRMRV)       Product generation dates          *
C* TBUFCTL(GENTIME,MAXSRMRV)       Product generation times          *
C* TBUFCTL(MAX_AZ,MAXSRMRV)        Window max. azimuth angles (deg)  *
C* TBUFCTL(MIN_AZ,MAXSRMRV)        Window min. azmiuth angles (deg)  *
C* TBUFCTL(MAX_RG,MAXSRMRV)        Window maximum ranges (km)        *
C* TBUFCTL(MIN_RG,MAXSRMRV)        Window minimum ranges (km)        *

C*                                 elevation scan lies within the    *
C*                                 window, (YES or NO).              *
C* TBUFCTL(INCZDEG,MAXSRMRV)       Flag indicating that the window   *
C*                                 includes zero deg. azimuth angle  *
C*                                 (YES or NO).                      *
C* TBUFCTL(COLIX,MAXSRMRV)         Product color lookup table index  *
C* TBUFCTL(MINIBIN,MAXSRMRV)       Bin numbers of minimum slant range*
C* TBUFCTL(MAXIBIN,MAXSRMRV)       Bin numbers or maximum slant range*
C* TBUFCTL(BINC,MAXSRMRV)          Ratios of product resolution over *
C*                                 input bin spacing in range(2 or 4)*
C* TBUFCTL(MAXOBIN,MAXSRMRV)       Max. bin numbers of the storm     *
C*                                 motion removed & compacted radal  *
C* TBUFCTL(MAXI2_IN_OBUF,MAXSRMRV) Number of I*2 words allocated for *
C*                                 each of the products              *
C* TBUFCTL(NEED,MAXSRMRV)          Flags indicating the product needs*
C*                                 the current radial (YES or NO)    *
C* TBUFCTL(STATUS,MAXSRMRV)        Product status flags,  (NOBUF,    *
C*                                 WINDOW_OPEN,RELEASED)             *
C* TBUFCTL(NUM_RADLS,MAXSRMRV)     Number of radials in the product  *
C* TBUFCTL(BUFCNT,MAXSRMRV)        Number of 2-byte words in the     *
C*                                 product buffers (running total)   *
C* TBUFCTL(NRLEB,MAXSRMRV)        Number of bytes run-length encoded *
C*                                 in the product (running total)    *
C* TBUFCTL(MAXNEG,MAXSRMRV)        Maximum neg. velocity found in the*
C*                                 in the window (running maximum)   *
C* TBUFCTL(MAXPOS,MAXSRMRV)        Maximum pos. velocity found in the*
C*                                 in the window (running maximum)   *
C*                                                                   *
C* TBUFCTL(STRMINFO,MAXSRMRV)      Flag denoting orgin of storm motion*
C*                                 -1 = algorithm, 0 = user requested*
C* TBUFCTL(KSTSP,MAXSRMRV)   Storm speed in knots*10 for header.
C*
C* TBUFCTL(ALT_CAT,MAXSRMRV)       ALERT CATEGORY
C*          ******* OTHER PARAMETERS *********                       *
C* INITVAL        Initial product control table values               *
C* YES            Flag indicating "true" conclusion                  *
C* NO             Flag indicating "false" condition                  *
C* RELEASED       Flag meaning, product buffer released              *
C* WINDOW_OPEN    Flag meaning, product window open                  *
C* NOBUF          Flag meaning, there is no buffer                   *
C* ZERO_VEL       Zero radial velocity value (scaled) ,129           *
C*********************************************************************
      INTEGER*4 RELEASED,WINDOW_OPEN,NOBUF,YES,NO,ZERO_VEL,ALT_CAT
      INTEGER*4 BUFID,BUFCNT,OPOINT,CAZ,CRNG,STSP,STDR,MSGID,MAX_AZ
      INTEGER*4 MAX_RG,MIN_AZ,MIN_RG,INCZDEG,ELFLAG,NEED,MAXI2_IN_OBUF
      INTEGER*4 NRLEB,STATUS,NUM_RADLS,GENDATE,GENTIME,MINIBIN,MAXIBIN
      INTEGER*4 MAXOBIN,BINC,MAXNEG,MAXPOS,INITVAL,COLIX,MAXSRMRV
      INTEGER*4 NUMCTL_PARM,STRMINFO,KSTSP
      PARAMETER (NUMCTL_PARM=31,MAXSRMRV=20,YES=1,NO=0,RELEASED=-1)
      PARAMETER (WINDOW_OPEN=1,NOBUF=0,ZERO_VEL=129)
      PARAMETER (MSGID=1,BUFID=2,CAZ=3,CRNG=4,STSP=5,STDR=6,OPOINT=7)
      PARAMETER (GENDATE=8,GENTIME=9,MAX_AZ=10,MIN_AZ=11,MAX_RG=12)
      PARAMETER (MIN_RG=13,ELFLAG=14,INCZDEG=15,COLIX=16,MINIBIN=17)
      PARAMETER (MAXIBIN=18,BINC=19,MAXOBIN=20,MAXI2_IN_OBUF=21)
      PARAMETER (NEED=22,STATUS=23,NUM_RADLS=24,BUFCNT=25,NRLEB=26)
      PARAMETER (MAXNEG=27,MAXPOS=28,STRMINFO=29,KSTSP=30)
      PARAMETER (ALT_CAT = 31)
      PARAMETER (INITVAL=0)
/*
**A3162I
C*********************************************************************
C*          *******  OTHER VARIABLES *********                       *
C* TNUMPROD       Total number of products set up in TBUFCTL         *
C* ONUMPROD       Outstanding number of products (unfinished)        *
C* NUMWIN         Number of window products                          *
C* NUMFUL         Number of full scan products                       *
C* VOLNUM         Volume scan number                                 *
C* ELEV_INDX      Elevation scan index of current scan               *
C* ELEV_ANG       Requested elev. angle, deg*10                      *
C* DOP_RESO       Doppler velocity resolution flag (1 or 2)          *
C* NEED_FCST      Flag indicating forcast algorithm output buffer is *
C*                needed, (true or false)                            *
C* TBUFCTL        Product control table                              *
C* INITCNT        I*2 position in product buffer to begin appending  *
C*                run-length-encoded radials                         *
C*********************************************************************
$INCLUDE A316.INC, **A3162PI (NLIST)
      INTEGER*4 TNUMPROD,ONUMPROD,NUMWIN,NUMFUL,INITCNT
      INTEGER*4 VOLNUM,ELEV_INDX,ELEV_ANG,DOP_RESO
      INTEGER*4 TBUFCTL(NUMCTL_PARM,MAXSRMRV)
      LOGICAL*4 NEED_FCST
      COMMON/A3162I/VOLNUM,ELEV_INDX,ELEV_ANG,DOP_RESO,TNUMPROD,
     $               ONUMPROD,NUMWIN,NUMFUL,TBUFCTL,INITCNT,NEED_FCST
/*
**A3164P4
*VERSION: 1
*******************COMMON DEFINITIONS FOR A3164.FTN*******************
      INTEGER*4  NROWS,NCOLS
      PARAMETER  (NROWS=40,NCOLS=55)

      INTEGER*2  TBUF(NROWS,NCOLS)
      INTEGER*2  PGNO,IPIX,JPIX,DIVIDER,NOI2
      INTEGER*2  SBLKID,GBLKID,TBLKID
      INTEGER*2  SORTED_CELLS( NHAL_MAX )

      INTEGER*4  NDX,PNDX,NOLNS,N1,N2,K1,K2,KI,IT,NP,WIND
      INTEGER*4  NMSTRMS,RDATE(3),RTIME(3)
      INTEGER*4  PRDLNG,SYMLNG,GPHLNG,TABLNG,TOTLNG
      INTEGER*4  SVIDX,SVSIDX,SVGIDX,SVTIDX
      INTEGER*4  PRDOFF,SYMBOFF,GPHOFF,TABOFF,SLOFF,SPOFF,SDOFF,
     X           GPOFF,GLOFF,GDOFF,TPROFF,TPOFF,
     X           TLOFF,TDPTOFF,GUPLIM,TUPLIM,L1,L2,L3,L4,GNOLNS,TNOLNS

      PARAMETER  (DIVIDER=-1,PRDOFF=5,SYMBOFF=60,SBLKID=1,GBLKID=2,
     X            TBLKID=3,SLOFF=5,SPOFF=3,SDOFF=2,GPOFF=5,GLOFF=2,
     X            GDOFF=3,TPROFF=4,TPOFF=64,TLOFF=2,
     X            GUPLIM=6,TUPLIM=10,L1=1,L2=2,L3=3,L4=4,NOI2=40,
     X            GNOLNS=5,TNOLNS=16)

      COMMON/A3164A/TBUF
      COMMON/A3164B/NDX,PNDX,NOLNS,N1,N2,K1,K2,KI,IT,NP,WIND,
     X              PGNO,IPIX,JPIX
      COMMON/A3164C/NMSTRMS,RDATE,RTIME
      COMMON/A3164D/PRDLNG,SYMLNG,GPHLNG,TABLNG,TOTLNG
      COMMON/A3164E/SVIDX,SVSIDX,SVGIDX,SVTIDX
      COMMON/A3164PM/GPHOFF,TABOFF,TDPTOFF
      COMMON/A316SRT/SORTED_CELLS
/*
**A3164D4
*VERSION: 0
**************DEFINITIONS*********************************************
*
*    PRDLNG    LENGTH OF PRODUCT HEADER AND DESC BLOCKS
*    SYMLNG    LENGTH OF SYMBOLOGY BLOCK
*    GPHLNG    LENGTH OF GRAPHICS BLOCK
*    TABLNG    LENGTH OF TABULAR BLOCK
*
*    PRDOFF    PRODUCT HEADER OFFSET TO PRDLNG MSW
*
*    SYMBOFF   OFFSET TO BEGINNING OF SYMBOLOGY BLOCK
*    SLOFF     OFFSET TO BEGINNING OF 1ST SYMBOLOGY LAYER
*    SPOFF     OFFSET TO BEGINNING OF SYMBOLOGY PACKET WITHIN LAYER
*    SDOFF     OFFSET TO BEGINNING OF SYMBOLOGY DATA WITHIN PACKET
*
*    GPHOFF    OFFSET TO BEGINNING OF GRAPHICS BLOCK
*    GPOFF     OFFSET TO BEGINNING OF 1ST GRAPHICS PAGE
*    GLOFF     OFFSET TO BEGINNING OF GRAPHICS LINE WITHIN PAGE
*    GDOFF     OFFSET TO BEGINNING OF CHARACTER DATA WITHIN LINE
*
*    TABOFF    OFFSET TO BEGINNING OF TABULAR BLOCK
*    TPROFF    OFFSET TO BEGINNING OF TABULAR PRODUCT BLOCK
*    TPOFF     OFFSET TO BEGINNING OF 1ST TABULAR PAGE
*    TLOFF     OFFSET TO BEGINNING OF TABULAR LINE WITHIN PAGE
*    TDPTOFF   OFFSET TO BEGINNING OF TABULAR ADAPTATION DATA
**************
*    SYMBOFF = PRDLNG
*    GPHOFF  = PRDLNG + SYMLNG
*    TABOFF  = PRDLNG + SYMLNG + GPHOFF
*    TDPTOFF = TABOFF + TABLNG(TO END OF TABULAR PAGES)
*********************************************************************
*****************A316.INC******************************************
/*
**A316C3
      INTEGER OPTRIDX
      LOGICAL BACK_POSITS,FORW_POSITS
C
      COMMON/A316C3/OPTRIDX,BACK_POSITS,FORW_POSITS
/*
**A3165B
      INTEGER*4  NDX,PNDX,NOLNS,N1,N2,K1,K2,KI,IT,NP
      INTEGER*4  GPHLNG,PLNGTH,GRFXOFF
C
      COMMON/A3165B/NDX,PNDX,NOLNS,N1,N2,K1,K2,KI,IT,NP,
     >              GPHLNG,PLNGTH,GRFXOFF
/*
**A3165C
      INTEGER*4  NMSTRMS,RDATE(3),RTIME(3)
C
      COMMON/A3165C/NMSTRMS,RDATE,RTIME
/*
