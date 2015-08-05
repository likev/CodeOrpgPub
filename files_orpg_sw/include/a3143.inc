XCOMM RCS info 
XCOMM $Author: nolitam $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2002/12/11 22:24:43 $ 
XCOMM $Id: a3143.inc,v 1.3 2002/12/11 22:24:43 nolitam Exp $ 
XCOMM $Revision: 1.3 $ 
XCOMM $State: Exp $
*.******************************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A3143.INC
*.
*.  INCLUDE FILE VERSION: 1
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE        RV   SPR#         PROGRAMMER         NOTES
*.  ---------   --   ----         ---------------    -----------------
*.  04/07/97    00   NA94-08461   Tim O'Bannon       **A3143HSR
*.  07/23/97    00   NA96-18403   Elliott P. Friend  **A3143P4
*.  01/31/02    01   NA01-27501   Dennis A. Miller   **A3143P4
*.
*.******************************************************************************
**A3143HSR
*VERSION:0
C***************** HYBRID SCAN REFLECTIVITY COLOR INDEX ************************
C*************************** (A3143HSR) ****************************************
C
C    NAME              TYPE   DESCRIPTION
C    ----------        ----   --------------------------------------------------
C    HSR_COLOR_INDEX   I*4    INDEX TO THE COLOR TABLES FOR THE HYBRID SCAN
C                             REFLECTIVITY (HSR) PRODUCT.  NOTE, HSR USES THE
C                             SAME COLORS AS THE BASE REFLECTIVITY PRODUCT (#19)
C
C ******************************************************************************
C
C
      INTEGER*4 HSR_COLOR_INDEX
C
      COMMON/A3143HSR/HSR_COLOR_INDEX
C
/*
**A3143P4
*VERSION: 1
C
C** Parameter Specifications for the ASCII data layer (Layer2) of the
C**                   DIGITAL HYBID SCAN REFLECTIVITY (DHR) Product
C
C** The ASCII data layer (Layer2) is composed of 4 sub-layers corresponding to the
C** Precip Status Message,  Adaptation Data,  Supplemental Data (Preproc Alg.),  and
C** BIAS related fields respectively. Each sub-layer is preceded by a sub-layer header
C** line (Char*8) in the format of 'PSM (  )', 'ADAP(  )', 'SUPL(  )' and 'BIAS(  )'
C** respectively.
C
C---------------------
C
C** NCL_MESG  = 1 (for sub-layer header element) + HYZ_MESG
C** NCL_ADAP    = 1 (for sub-layer header element) + HYZ_ADAP + 1 (for Bias Applied Flag)
C** NCL_SUPL_PRE   = 1 (for sub-layer header element) + SSIZ_PRE
C** NCL_BIAS_ITEMS = 1 (for sub-layer header element) + N_BIAS_ITEMS
C**
C** NCHARLINE    = Total number of Char*8 CHARLINE items =
C**                          NCL_MESG + NCL_ADAP + NCL_SUPL_PRE + NCL_BIAS_ITEMS
C** NI2_PRODLINE = Size in halfwords of PRODLINE = NCHARLINE*4 (since 4 Integer*2
C**                          PRODLINE elements are needed for each Char*8 CHARLINE item)
C
      INTEGER N_BIAS_ITEMS
      INTEGER NCL_MESG, NCL_ADAP, NCL_SUPL_PRE, NCL_BIAS_ITEMS
      INTEGER NCHARLINE, NI2_PRODLINE
C
      PARAMETER (N_BIAS_ITEMS   = 11)
      PARAMETER (NCL_MESG       = 1+HYZ_MESG)
      PARAMETER (NCL_ADAP       = 1+HYZ_ADAP+1)
      PARAMETER (NCL_SUPL_PRE   = 1+SSIZ_PRE)
      PARAMETER (NCL_BIAS_ITEMS = 1+N_BIAS_ITEMS)
C
      PARAMETER (NCHARLINE      = NCL_MESG + NCL_ADAP +
     $                                   NCL_SUPL_PRE + NCL_BIAS_ITEMS)
      PARAMETER (NI2_PRODLINE   = NCHARLINE*4)
C
      INTEGER*2   PRODLINE(NI2_PRODLINE)
      CHARACTER*8 CHARLINE(NCHARLINE)
      EQUIVALENCE (CHARLINE,PRODLINE)
C
C---------------------
C
C**   NI2_ASCII_LYR_HDR =
C**         No. of I*2 Header Words in the ASCII DATA layer of the DHR product
C
C**   NI2_ASCII_LYR_WRDS =
C**         Total no. of I*2 Words in the ASCII DATA layer of the DHR product
C
      INTEGER    NI2_ASCII_LYR_HDR,    NI2_ASCII_LYR_WRDS
      PARAMETER  (NI2_ASCII_LYR_HDR=7)
      PARAMETER  (NI2_ASCII_LYR_WRDS = NI2_PRODLINE + NI2_ASCII_LYR_HDR)
C
C---------------------
C
/*
