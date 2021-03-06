XCOMM RCS info 
XCOMM $Author: nolitam $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2002/12/11 22:24:54 $ 
XCOMM $Id: a3cd99.inc,v 1.2 2002/12/11 22:24:54 nolitam Exp $ 
XCOMM $Revision: 1.2 $ 
XCOMM $State: Exp $ 
C@NRDP	This is adapted for being included in other files
C@NRDP      BLOCK DATA A3CD99__CONVERSION
**A3CD99	;C@NRDP
*.********************************************************************
*.                    M O D U L E  P R O L O G U E
*.
*.  MODULE NAME: A3CD99__CONVERSION
*.
*.  MODULE VERSION: 0009
*.
*.  MODULE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.       DATE          VERSION   PROGRAMMER           NOTES
*.       ----------    -------   ----------------     --------------------
*.       03 24 86      0000      PHIL STALEY
*.       10 02 86      0001      J GROMMONS           SPR # 60346,60650
*.       04 01 90      0002      STEVE ANDERSON       SPR # 90519
*.       10 10 90      0003      PAUL JENDROWSKI      SPR # 91017
*.       02 22 91      0003      JOHN DEPHILIP        SPR # 91250
*.       02 13 91      0003      PAUL JENDROWSKI      SPR # 91764
*.       12 03 91      0004      STEVE ANDERSON       SPR # 92740
*.       12/10/91      0005      ED NICHLAS           SPR 92637 PDL Removal
*.       04/24/92      0006      Toolset              SPR 91895
*.       03/25/93      0007      Toolset              SPR NA93-06801
*.       01/28/94      0008      Toolset              SPR NA94-01101
*.       03/03/94      0009      Toolset              SPR NA94-05501
*.
*.  CALLING SEQUENCE: N/A
*.
*.  MODULE FUNCTION:
*.
*.     INITIALIZE RADIAL DATA/REAL METEORLOGICAL DATA CONVERSION
*.         TABLE IMAGE
*.
*.  MODULES CALLED: NONE.
*.
*.  PARAMETERS:          (*:  G = GLOBAL, C = COMMON, P = PASSED)
*.
*.    *   INPUT  TYPE        DESCRIPTION
*.    -   -----  ----        -----------
*.
*.    *   OUTPUT  TYPE        DESCRIPTION
*.    -   ------  ----        -----------
*.    G   REF     R*4         The conversion table for reflectivity.  This
*.                            table converts from halfword integer to real
*.                            meteorological data.
*.    G   SPW     R*4         The conversion table for spectrum width.
*.    G   VEL     R*4         Table to get doppler velocity from biased values
*.                            in radial. Indexed by radial base data by
*.                            doppler modes.
*.
*.    *   ACTUAL ARGUMENTS  TYPE        DESCRIPTION
*.    -   ----------------  ----        -----------
*.
*.  DATABASE/FILE REFERENCE:  None
*.
*.  INTERNAL TABLES/WORK AREA:
*.
*.    NAME  TYPE        DESCRIPTION
*.    ----  ----        -----------
*.    IVX   I*2         Subscript used to define elements in conversion tables
*.                      for Doppler velocity modes 1 and 2.
*.    IWX   I*2         Subscript used to define elements in conversion table
*.                      for spectrum width.
*.    IZX   I*2         Subscript used to define elements in conversion table
*.                      for equivalent reflectivity.
*.
*.  GLOBAL BLOCKS REFERENCED:
*.
*.    A3CD99
*.
*.  COMMON BLOCKS REFERENCED:
*.
*.
*.  ERROR CONDITIONS:  NONE
*.
*.  ASSUMPTIONS/RESTRICTIONS:  None
*.
*.  DEVIATION FROM STANDARDS:  None
*.
*.  COMPILATION INSTRUCTIONS:
*.
*.     THIS MODULE IS COMPILED USING THE COMP09.CSS
*.
*.  LINKAGE INSTRUCTIONS:
*.
*.     THIS MODULE IS LINKED USING THE LINK09.CSS
*.
*.  MISC:  None
*.
*.*******************************************************************
C************************************************
C*                A3CD99.FTN                    *
C*                                              *
C* THE LOOK-UP TABLE CONVERTING THE INTEGER*2   *
C* BASE DATA TO REAL METEOROLOGICAL DATA        *
C*                                              *
C************************************************
C
C@NRDP        IMPLICIT NONE
C
C BLOCK DATA STATEMENT
C
C INCLUDE THE PARAMETER AND COMMON DATA STATEMENTS HERE
C
C@NRDP $INCLUDE A309.INC/G,**A3CD99 (NLIST)
C
C  CONVERSION TABLE FOR EQUIVALENT REFLECTIVITY
C
      DATA(REF(IZX),IZX=0,84) /-999.,-888.,-32.,-31.5,-31.,-30.5,-30.,
     $-29.5,-29.,-28.5,-28.,-27.5,-27.,-26.5,-26.,-25.5,-25.,-24.5,
     $-24.,-23.5,-23.,-22.5,-22.,-21.5,-21.,-20.5,-20.,-19.5,-19.,
     $-18.5,-18.,-17.5,-17.,-16.5,-16.,-15.5,-15.,-14.5,-14.,-13.5,
     $-13.,-12.5,-12.,-11.5,-11.,-10.5,-10.,-9.5,-9.,-8.5,-8.,-7.5,-7.,
     $-6.5,-6.,-5.5,-5.,-4.5,-4.,-3.5,-3.,-2.5,-2.,-1.5,-1.,-.5,0.,
     $.5,1.,1.5,2.,2.5,3.,3.5,4.,4.5,5.,5.5,6.,6.5,7.,7.5,8.,8.5,9./
      DATA(REF(IZX),IZX=85,176) /9.5,10.,10.5,11.,11.5,12.,12.5,13.,
     $13.5,14.,14.5,15.,15.5,16.,16.5,17.,17.5,18.,18.5,19.,19.5,20.,
     $20.5,21.,21.5,22.,22.5,23.,23.5,24.,24.5,25.,25.5,26.,26.5,27.,
     $27.5,28.,28.5,29.,29.5,30.,30.5,31.,31.5,32.,32.5,33.,33.5,34.,
     $34.5,35.,35.5,36.,36.5,37.,37.5,38.,38.5,39.,39.5,40.,40.5,41.,
     $41.5,42.,42.5,43.,43.5,44.,44.5,45.,45.5,46.,46.5,47.,47.5,48.,
     $48.5,49.,49.5,50.,50.5,51.,51.5,52.,52.5,53.,53.5,54.,54.5,55./
      DATA(REF(IZX),IZX=177,256) /55.5,56.,56.5,57.,57.5,58.,58.5,59.,
     $59.5,60.,60.5,61.,61.5,62.,62.5,63.,63.5,64.,64.5,65.,65.5,66.,
     $66.5,67.,67.5,68.,68.5,69.,69.5,70.,70.5,71.,71.5,72.,72.5,
     $73.,73.5,74.,74.5,75.,75.5,76.,76.5,77.,77.5,78.,78.5,79.,79.5,
     $80.,80.5,81.,81.5,82.,82.5,83.,83.5,84.,84.5,85.,85.5,86.,86.5,
     $87.,87.5,88.,88.5,89.,89.5,90.,90.5,91.,91.5,92.,92.5,93.,93.5,
     $94.,94.5,-777. /
C
C
C  CONVERSION TABLE FOR DOPPLER VELOCITY MODE 1
C
      DATA(VEL(IVX,1),IVX=0,76) /-999.,-888.,-63.5,-63.,-62.5,-62.,
     $-61.5,-61.,-60.5,-60.,-59.5,-59.,-58.5,-58.,-57.5,-57.,-56.5,-56.,
     $-55.5,-55.,-54.5,-54.,-53.5,-53.,-52.5,-52.,-51.5,-51.,-50.5,-50.,
     $-49.5,-49.,-48.5,-48.,-47.5,-47.,-46.5,-46.,-45.5,-45.,-44.5,-44.,
     $-43.5,-43.,-42.5,-42.,-41.5,-41.,-40.5,-40.,-39.5,-39.,-38.5,-38.,
     $-37.5,-37.,-36.5,-36.,-35.5,-35.,-34.5,-34.,-33.5,-33.,-32.5,-32.,
     $-31.5,-31.,-30.5,-30.,-29.5,-29.,-28.5,-28.,-27.5,-27.,-26.5/
      DATA(VEL(IVX,1),IVX=77,168)/-26.,-25.5,-25.,-24.5,-24.,-23.5,
     $-23.,-22.5,-22.,-21.5,-21.,-20.5,-20.,-19.5,-19.,-18.5,-18.,-17.5,
     $-17.,-16.5,-16.,-15.5,-15.,-14.5,-14.,-13.5,-13.,-12.5,-12.,-11.5,
     $-11.,-10.5,-10.,-9.5,-9.,-8.5,-8.,-7.5,-7.,-6.5,-6.,-5.5,-5.,-4.5,
     $-4.,-3.5,-3.,-2.5,-2.,-1.5,-1.,-.5,0.,.5,1.,1.5,2.,2.5,3.,3.5,4.,
     $4.5,5.,5.5,6.,6.5,7.,7.5,8.,8.5,9.,9.5,10.,10.5,11.,11.5,12.,12.5,
     $13.,13.5,14.,14.5,15.,15.5,16.,16.5,17.,17.5,18.,18.5,19.,19.5/
      DATA(VEL(IVX,1),IVX=169,256)/20.,20.5,21.,21.5,22.,22.5,23.,23.5,
     $24.,24.5,25.,25.5,26.,26.5,27.,27.5,28.,28.5,29.,29.5,30.,30.5,
     $31.,31.5,32.,32.5,33.,33.5,34.,34.5,35.,35.5,36.,36.5,37.,37.5,
     $38.,38.5,39.,39.5,40.,40.5,41.,41.5,42.,42.5,43.,43.5,44.,44.5,
     $45.,45.5,46.,46.5,47.,47.5,48.,48.5,49.,49.5,50.,50.5,51.,51.5,
     $52.,52.5,53.,53.5,54.,54.5,55.,55.5,56.,56.5,57.,57.5,58.,58.5,
     $59.,59.5,60.,60.5,61.,61.5,62.,62.5,63.,-777./
C
C
C  CONVERSION TABLE FOR DOPPLER VELOCITY MODE 2
C
      DATA(VEL(IVX,2),IVX=0,78)/-999.,-888.,-127.,-126.,-125.,-124.,
     $-123.,-122.,-121.,-120.,-119.,-118.,-117.,-116.,-115.,-114.,-113.,
     $-112.,-111.,-110.,-109.,-108.,-107.,-106.,-105.,-104.,-103.,-102.,
     $-101.,-100.,-99.,-98.,-97.,-96.,-95.,-94.,-93.,-92.,-91.,-90.,
     $-89.,-88.,-87.,-86.,-85.,-84.,-83.,-82.,-81.,-80.,-79.,-78.,-77.,
     $-76.,-75.,-74.,-73.,-72.,-71.,-70.,-69.,-68.,-67.,-66.,-65.,-64.,
     $-63.,-62.,-61.,-60.,-59.,-58.,-57.,-56.,-55.,-54.,-53.,-52.,-51./
      DATA(VEL(IVX,2),IVX=79,176)/-50.,-49.,-48.,-47.,-46.,-45.,-44.,
     $-43.,-42.,-41.,-40.,-39.,-38.,-37.,-36.,-35.,-34.,-33.,-32.,-31.,
     $-30.,-29.,-28.,-27.,-26.,-25.,-24.,-23.,-22.,-21.,-20.,-19.,-18.,
     $-17.,-16.,-15.,-14.,-13.,-12.,-11.,-10.,-9.,-8.,-7.,-6.,-5.,-4.,
     $-3.,-2.,-1.,0.,1.,2.,3.,4.,5.,6.,7.,8.,9.,10.,11.,12.,13.,14.,15.,
     $16.,17.,18.,19.,20.,21.,22.,23.,24.,25.,26.,27.,28.,29.,30.,31.,
     $32.,33.,34.,35.,36.,37.,38.,39.,40.,41.,42.,43.,44.,45.,46.,47./
      DATA(VEL(IVX,2),IVX=177,256) /48.,49.,50.,51.,52.,53.,54.,55.,56.,
     $57.,58.,59.,60.,61.,62.,63.,64.,65.,66.,67.,68.,69.,70.,71.,72.,
     $73.,74.,75.,76.,77.,78.,79.,80.,81.,82.,83.,84.,85.,86.,87.,88.,
     $89.,90.,91.,92.,93.,94.,95.,96.,97.,98.,99.,100.,101.,102.,103.,
     $104.,105.,106.,107.,108.,109.,110.,111.,112.,113.,114.,115.,116.,
     $117.,118.,119.,120.,121.,122.,123.,124.,125.,126.,-777./
C
C
C  CONVERSION TABLE FOR SPECTRUM WIDTH
C
      DATA(SPW(IWX),IWX=0,256) /-999.,-888.,127*0.0,
     $ 0.,.5,1.,1.5,2.,2.5,3.,3.5,4.,
     $4.5,5.,5.5,6.,6.5,7.,7.5,8.,8.5,9.,9.5,107*10.,-777./
C
C@NRDP      END
/*		;C@NRDP