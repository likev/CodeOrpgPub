/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/09/10 20:48:03 $
 * $Id: hci_decode_product.c,v 1.36 2012/09/10 20:48:03 ccalvert Exp $
 * $Revision: 1.36 $
 * $State: Exp $
 */

/************************************************************************
 *	Module: hci_decode_product.c					*
 *	Description: This module contains a collection of routines to	*
 *		     decode an RPG product and provide functions for	*
 *		     an application to retrieve product data.		*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_decode_product.h>

static float	Radar_latitude;	/* Radar latitude (deg) */
static float	Radar_longitude;/* Radar longitude (deg) */
static int	Elevation_index;/* Index relative to current VCP
				   that corresponds to elevation
				   angle (elevation-based products) */
static int	Product_date;	/* Julian date of currently open product */
static int	Product_time; 	/* time (seconds past midnight) of product */
static short Product_vcp=-1; /* vcp of the current product */

static	short Product_code = -1;/* Product code of currently open product */
static	unsigned short	Product_type; /* Product type code */
static	int	Product_io_status = 0; /* Status of last read operation */

static Graphic_product Graphic_product_struct; /* Graphic Product Message */

/* Define storage for pointers to various blocks in product */

static	int Symbology_Block = 0;
static	int Grafattr_block  = 0;
static	int Tabular_block   = 0;

/* Define Pointer to radial and raster data structures */

static	radial_rle_t	*Radial_image = NULL;
static	raster_rle_t	*Raster_image = NULL;
static	dhr_rle_t	*Dhr_image = NULL;

/* Define Pointer to VWP data structure */
static	vwp_t	*Vwp_image = NULL;

/* Define Pointer to STI data structure */
static	sti_t	*Sti_image = NULL;

/* Define Pointer to HI data structure */
static	hi_t	*Hi_image = NULL;

/* Define Pointer to MESO data structure */
static	meso_t	*Meso_image = NULL;

/* Define Pointer to TVS data structure */
static	tvs_t	*Tvs_image = NULL;

/* Define Pointer to SWP data structure */
static	swp_t	*Swp_image = NULL;

/* Define pointer to product_pertinent data structure */
static	product_pertinent_t	*Attribute = NULL;

/* Define pointer to graphic attribute data structure */
static	graphic_attr_t	*Gtab = NULL;

/* Define pointer to tabular alphanumeric data structure */
static	tabular_attr_t	*Ttab = NULL;

/* Resolution table */
static	float Xy_azran_reso[][2] = { 
                             { 0.0, 0.0 },
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 0.0, 0.0 }, 
                             { 1.0, 1.0 },  /* Product 16 */
                             { 2.0, 1.0 },  /* Product 17 */
                             { 4.0, 1.0 },  /* Product 18 */
                             { 1.0, 1.0 },  /* Product 19 */
                             { 2.0, 1.0 },  /* Product 20 */
                             { 4.0, 1.0 },  /* Product 21 */
                             { 0.25, 1.0 }, /* Product 22 */
                             { 0.5, 1.0 },  /* Product 23 */
                             { 1.0, 1.0 },  /* Product 24 */
                             { 0.25, 1.0 }, /* Product 25 */
                             { 0.5, 1.0 },  /* Product 26 */
                             { 1.0, 1.0 },  /* Product 27 */
                             { 0.25, 1.0 }, /* Product 28 */
                             { 0.5, 1.0 },  /* Product 29 */
                             { 1.0, 1.0 },  /* Product 30 */
                             { 1.0, 2.0 },
                             { 1.0, 1.0 },
                             { 1.0, 1.0 },
                             { 1.0, 1.4 },  /* Product 34 */
                             { 1.0, 1.0 },  /* Product 35 */
                             { 4.0, 4.0 },  /* Product 36 */
                             { 1.0, 1.0 },  /* Product 37 */
                             { 4.0, 4.0 },  /* Product 38 */
                             { 1.0, 1.0 },  /* Product 39 */
                             { 4.0, 4.0 },  /* Product 40 */
                             { 4.0, 4.0 },  /* Product 41 */
                             { 4.0, 4.0 },  /* Product 42 */
                             { 1.0, 1.0 },  /* Product 43 */
                             { 0.25, 1.0 }, /* Product 44 */
                             { 0.25, 1.0 }, /* Product 45 */
                             { 0.5, 1.0 },  /* Product 46 */
                             { 4.0, 4.0 },  /* Product 47 */
                             { 0.0, 0.0 },  /* Product 48 */
                             { 0.5, 0.5 },  /* Product 49 */
                             { 1.0, 0.5 },  /* Product 50 */
                             { 1.0, 0.5 },  /* Product 51 */
                             { 1.0, 0.5 },  /* Product 52 */
                             { 1.0, 0.5 },  /* Product 53 */
                             { 0.0, 0.0 },
                             { 0.5, 1.0 },  /* Product 55 */
                             { 1.0, 1.0 },  /* Product 56 */
                             { 4.0, 4.0 },  /* Product 57 */
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },
                             { 4.0, 4.0 },  /* Product 63 */
                             { 4.0, 4.0 },  /* Product 64 */
                             { 4.0, 4.0 },  /* Product 65 */
                             { 4.0, 4.0 },  /* Product 66 */
                             { 4.0, 4.0 },  /* Product 67 */
                             { 4.0, 4.0 },  /* Product 68 */
                             { 4.0, 4.0 },  /* Product 69 */
                             { 4.0, 4.0 },  /* Product 70 */
                             { 4.0, 4.0 },  /* Product 71 */
                             { 4.0, 4.0 },  /* Product 72 */
                             { 0.0, 0.0 },  /* Product 73 */
                             { 0.0, 0.0 },  /* Product 74 */
                             { 0.0, 0.0 },  /* Product 75 */
                             { 0.0, 0.0 },
                             { 0.0, 0.0 },  /* Product 77 */
                             { 2.0, 2.0 },  /* Product 78 */
                             { 2.0, 2.0 },  /* Product 79 */
                             { 2.0, 2.0 },  /* Product 80 */
                             { 0.0, 0.0 },  /* Product 81 */
                             { 0.0, 0.0 },  /* Product 82 */
                             { 0.0, 0.0 },  /* Product 83 */
                             { 0.0, 0.0 },  /* Product 84 */
                             { 1.0, 0.5 },  /* Product 85 */
                             { 1.0, 0.5 },  /* Product 86 */
                             { 0.0, 0.0 },  /* Product 87 */
                             { 0.0, 0.0 },  /* Product 88 */
                             { 4.0, 4.0 },  /* Product 89 */
                             { 4.0, 4.0 },  /* Product 90 */
                             { 0.0, 0.0 },  /* Product 91 */
                             { 0.0, 0.0 },  /* Product 92 */
                             { 1.0, 1.0 },  /* Product 93 */
                             { 1.0, 1.0 },  /* Product 94 */
                             { 1.0, 1.0 },  /* Product 95 */
                             { 4.0, 1.0 },  /* Product 96 */
                             { 1.0, 1.0 },  /* Product 97 */
                             { 4.0, 1.0 },  /* Product 98 */
                             { 1.0, 1.0 },  /* Product 99 */
                             { 0.0, 0.0 },  /* Product 100*/
                             { 0.0, 0.0 },  /* Product 101*/
                             { 0.0, 0.0 },  /* Product 102*/
                             { 0.0, 0.0 },  /* Product 103*/
                             { 0.0, 0.0 },  /* Product 104*/
                             { 0.0, 0.0 },  /* Product 105*/
                             { 0.0, 0.0 },  /* Product 106*/
                             { 0.0, 0.0 },  /* Product 107*/
                             { 0.0, 0.0 },  /* Product 108*/
                             { 0.0, 0.0 },  /* Product 109*/
                             { 0.0, 0.0 },  /* Product 110*/
                             { 0.0, 0.0 },  /* Product 111*/
                             { 0.0, 0.0 },  /* Product 112*/
                             { 0.0, 0.0 },  /* Product 113*/
                             { 0.0, 0.0 },  /* Product 114*/
                             { 0.0, 0.0 },  /* Product 115*/
                             { 0.0, 0.0 },  /* Product 116*/
                             { 0.0, 0.0 },  /* Product 117*/
                             { 0.0, 0.0 },  /* Product 118*/
                             { 0.0, 0.0 },  /* Product 119*/
                             { 0.0, 0.0 },  /* Product 120*/
                             { 0.0, 0.0 },  /* Product 121*/
                             { 0.0, 0.0 },  /* Product 122*/
                             { 0.0, 0.0 },  /* Product 123*/
                             { 0.0, 0.0 },  /* Product 124*/
                             { 0.0, 0.0 },  /* Product 125*/
                             { 0.0, 0.0 },  /* Product 126*/
                             { 0.0, 0.0 },  /* Product 127*/
                             { 0.0, 0.0 },  /* Product 128*/
                             { 0.0, 0.0 },  /* Product 129*/
                             { 0.0, 0.0 },  /* Product 130*/
                             { 0.0, 0.0 },  /* Product 131*/
                             { 1.0, 1.0 },  /* Product 132*/
                             { 1.0, 1.0 },  /* Product 133*/
                             { 1.0, 1.0 },  /* Product 134*/
                             { 0.0, 0.0 },  /* Product 135*/
                             { 0.0, 0.0 },  /* Product 136*/
                             { 0.0, 0.0 },  /* Product 137*/
                             { 0.0, 0.0 },  /* Product 138*/
                             { 0.0, 0.0 },  /* Product 139*/
                             { 0.0, 0.0 },  /* Product 140*/
                             { 0.0, 0.0 },  /* Product 141*/
                             { 0.0, 0.0 },  /* Product 142*/
                             { 0.0, 0.0 },  /* Product 143*/
                             { 0.0, 0.0 },  /* Product 144*/
                             { 0.0, 0.0 },  /* Product 145*/
                             { 0.0, 0.0 },  /* Product 146*/
                             { 0.0, 0.0 },  /* Product 147*/
                             { 0.0, 0.0 },  /* Product 148*/
                             { 0.0, 0.0 },  /* Product 149*/
                             { 0.0, 0.0 },  /* Product 150*/
                             { 0.0, 0.0 },  /* Product 151*/
                             { 0.0, 0.0 },  /* Product 152*/
                             { 0.0, 0.0 },  /* Product 153*/
                             { 0.0, 0.0 },  /* Product 154*/
                             { 0.0, 0.0 },  /* Product 155*/
                             { 0.0, 0.0 },  /* Product 156*/
                             { 0.0, 0.0 },  /* Product 157*/
                             { 0.0, 0.0 },  /* Product 158*/
                             { 0.0, 0.0 },  /* Product 159*/
                             { 0.0, 0.0 },  /* Product 160*/
                             { 0.0, 0.0 },  /* Product 161*/
                             { 0.0, 0.0 },  /* Product 162*/
                             { 0.0, 0.0 },  /* Product 163*/
                             { 0.0, 0.0 },  /* Product 164*/
                             { 0.0, 0.0 },  /* Product 165*/
                             { 0.0, 0.0 },  /* Product 166*/
                             { 0.0, 0.0 },  /* Product 167*/
                             { 0.0, 0.0 },  /* Product 168*/
                             { 0.0, 0.0 },  /* Product 169*/
                             { 0.0, 0.0 },  /* Product 170*/
                             { 0.0, 0.0 },  /* Product 171*/
                             { 0.0, 0.0 },  /* Product 172*/
                             { 0.0, 0.0 },  /* Product 173*/
                             { 0.0, 0.0 },  /* Product 174*/
                             { 0.0, 0.0 },  /* Product 175*/
                             { 0.0, 0.0 },  /* Product 176*/
                             { 0.0, 0.0 },  /* Product 177*/
                             { 0.0, 0.0 },  /* Product 178*/
                             { 0.0, 0.0 },  /* Product 179*/
                             { 0.0, 0.0 },  /* Product 180*/
                             { 0.0, 0.0 },  /* Product 181*/
                             { 0.0, 0.0 },  /* Product 182*/
                             { 0.0, 0.0 },  /* Product 183*/
                             { 0.0, 0.0 },  /* Product 184*/
                             { 0.0, 0.0 },  /* Product 185*/
                             { 0.0, 0.0 },  /* Product 186*/
                             { 0.0, 0.0 },  /* Product 187*/
                             { 0.0, 0.0 },  /* Product 188*/
                             { 0.0, 0.0 },  /* Product 189*/
                             { 0.0, 0.0 },  /* Product 190*/
                             { 0.0, 0.0 },  /* Product 191*/
                             { 0.0, 0.0 },  /* Product 192*/
                             { 0.0, 0.0 },  /* Product 193*/
                             { 0.0, 0.0 },  /* Product 194*/
                             { 0.0, 0.0 },  /* Product 195*/
                             { 0.0, 0.0 },  /* Product 196*/
                             { 0.0, 0.0 },  /* Product 197*/
                             { 0.0, 0.0 },  /* Product 198*/
                             { 0.0, 0.0 },  /* Product 199*/
                             { 0.0, 0.0 },  /* Product 200*/
                             { 0.0, 0.0 },  /* Product 201*/
                             { 0.0, 0.0 },  /* Product 202*/
                             { 0.0, 0.0 },  /* Product 203*/
                             { 0.0, 0.0 },  /* Product 204*/
                             { 0.0, 0.0 },  /* Product 205*/
                             { 0.0, 0.0 },  /* Product 206*/
                             { 0.0, 0.0 },  /* Product 207*/
                             { 0.0, 0.0 },  /* Product 208*/
                             { 0.0, 0.0 },  /* Product 209*/
                             { 0.0, 0.0 },  /* Product 210*/
                             { 0.0, 0.0 },  /* Product 211*/
                             { 0.0, 0.0 },  /* Product 212*/
                             { 0.0, 0.0 },  /* Product 213*/
                             { 0.0, 0.0 },  /* Product 214*/
                             { 0.0, 0.0 },  /* Product 215*/
                             { 0.0, 0.0 },  /* Product 216*/
                             { 0.0, 0.0 },  /* Product 217*/
                             { 0.0, 0.0 },  /* Product 218*/
                             { 0.0, 0.0 },  /* Product 219*/
                             { 0.0, 0.0 },  /* Product 220*/
                             { 0.0, 0.0 },  /* Product 221*/
                             { 0.0, 0.0 },  /* Product 222*/
                             { 0.0, 0.0 },  /* Product 223*/
                             { 0.0, 0.0 },  /* Product 224*/
                             { 0.0, 0.0 },  /* Product 225*/
                             { 0.0, 0.0 },  /* Product 226*/
                             { 0.0, 0.0 },  /* Product 227*/
                             { 0.0, 0.0 },  /* Product 228*/
                             { 0.0, 0.0 },  /* Product 229*/
                             { 0.0, 0.0 },  /* Product 230*/
                             { 0.0, 0.0 },  /* Product 231*/
                             { 0.0, 0.0 },  /* Product 232*/
                             { 0.0, 0.0 },  /* Product 233*/
                             { 0.0, 0.0 },  /* Product 234*/
                             { 0.0, 0.0 },  /* Product 235*/
                             { 0.0, 0.0 },  /* Product 236*/
                             { 0.0, 0.0 },  /* Product 237*/
                             { 0.0, 0.0 },  /* Product 238*/
                             { 0.0, 0.0 },  /* Product 239*/
                             { 0.0, 0.0 },  /* Product 240*/
                             { 0.0, 0.0 },  /* Product 241*/
                             { 0.0, 0.0 },  /* Product 242*/
                             { 0.0, 0.0 },  /* Product 243*/
                             { 0.0, 0.0 },  /* Product 244*/
                             { 0.0, 0.0 },  /* Product 245*/
                             { 0.0, 0.0 },  /* Product 246*/
                             { 0.0, 0.0 },  /* Product 247*/
                             { 0.0, 0.0 },  /* Product 248*/
                             { 0.0, 0.0 },  /* Product 249*/
                             { 0.0, 0.0 },  /* Product 250*/
                             { 0.0, 0.0 },  /* Product 251*/
                             { 0.0, 0.0 },  /* Product 252*/
                             { 0.0, 0.0 },  /* Product 253*/
                             { 0.0, 0.0 },  /* Product 254*/
                             { 0.0, 0.0 },  /* Product 255*/
                             { 0.0, 0.0 },  /* Product 256*/
                             { 0.0, 0.0 },  /* Product 257*/
                             { 0.0, 0.0 },  /* Product 258*/
                             { 0.0, 0.0 },  /* Product 259*/
                             { 0.0, 0.0 },  /* Product 260*/
                             { 0.0, 0.0 },  /* Product 261*/
                             { 0.0, 0.0 },  /* Product 262*/
                             { 0.0, 0.0 },  /* Product 263*/
                             { 0.0, 0.0 },  /* Product 264*/
                             { 0.0, 0.0 },  /* Product 265*/
                             { 0.0, 0.0 },  /* Product 266*/
                             { 0.0, 0.0 },  /* Product 267*/
                             { 0.0, 0.0 },  /* Product 268*/
                             { 0.0, 0.0 },  /* Product 269*/
                             { 0.0, 0.0 },  /* Product 270*/
                             { 0.0, 0.0 },  /* Product 271*/
                             { 0.0, 0.0 },  /* Product 272*/
                             { 0.0, 0.0 },  /* Product 273*/
                             { 0.0, 0.0 },  /* Product 274*/
                             { 0.0, 0.0 },  /* Product 275*/
                             { 0.0, 0.0 },  /* Product 276*/
                             { 0.0, 0.0 },  /* Product 277*/
                             { 0.0, 0.0 },  /* Product 278*/
                             { 0.0, 0.0 },  /* Product 279*/
                             { 0.0, 0.0 },  /* Product 280*/
                             { 0.0, 0.0 },  /* Product 281*/
                             { 0.0, 0.0 },  /* Product 282*/
                             { 0.0, 0.0 },  /* Product 283*/
                             { 0.0, 0.0 },  /* Product 284*/
                             { 0.0, 0.0 },  /* Product 285*/
                             { 0.0, 0.0 },  /* Product 286*/
                             { 0.0, 0.0 },  /* Product 287*/
                             { 0.0, 0.0 },  /* Product 288*/
                             { 0.0, 0.0 },  /* Product 289*/
                             { 0.0, 0.0 },  /* Product 290*/
                             { 0.0, 0.0 },  /* Product 291*/
                             { 0.0, 0.0 },  /* Product 292*/
                             { 0.0, 0.0 },  /* Product 293*/
                             { 0.0, 0.0 },  /* Product 294*/
                             { 0.0, 0.0 },  /* Product 295*/
                             { 0.0, 0.0 },  /* Product 296*/
                             { 0.0, 0.0 },  /* Product 297*/
                             { 0.0, 0.0 },  /* Product 298*/
                             { 0.0, 0.0 },  /* Product 299*/
                             { 0.0, 0.0 }   /* Product 300 */
};

/* Product color levels table */
static	int Data_level_tab[] =
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /*   0 */
      8,  8,  8, 16, 16, 16,  8,  8,  8, 16, 16, 16,  8,  8,  8,  0,  /*  16 */
    256,  0,  8,  8,  8, 16, 16,  0,  0, 16,  0, 16, 16,  8, 16,  0,  /*  32 */
      6, 16, 16, 16,  8,  8,  0, 16, 16, 16,  0,  0,  0,  0,  0,  8,  /*  48 */
      8,  8,  8,  8,  8,  8,  8,  8,  8,  0,  9,  0,  0,  0, 16, 16,  /*  64 */
     16,  0,  8,  9,  8,  8,  8, 16,  0,  8,  8,  0,  0,256,256,  8,  /*  80 */
      8, 16, 16,256,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /*  96 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 112 */
      0,  0,  0,  0, 11, 12,256,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 128 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 144 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 160 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 176 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 192 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 208 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 224 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 240 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 256 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 272 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 288 */
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}; 

/* Function prototypes */

static int decode_header( short * );
static int decode_radial_rle( short * );
static int decode_raster_rle( short * );
static void decode_data_level( short, char * );
static void level_to_ASCII( float, unsigned char, char * );
static void convert_time( int, int *, int *, int * );
static void free_memory( short );
static void decode_tabular_block( short * );
static void decode_text_packet_8( short *, char **, int *, int * );
static void product_dependent( short * );
static void degrees_minutes_seconds( float, int *, int *, int * );
static void decode_vwp( short *, short, int );
static void decode_sti( short *, short, int );
static void decode_hi( short *, short, int );
static void decode_meso( short *, short, int );
static void decode_tvs( short *, short, int );
static void decode_swp( short *, short, int );
static void decode_dhr( short * );
void decode_grafattr_block( short * );
static int local_endian_type();

/************************************************************************
 *	Description: This function decodes the product data pointed to	*
 *		     by the input product data and puts the result in	*
 *		     various internal tables for access.		*
 *									*
 *	Input:  product_data - pointer to product data buffer		*
 *	Output: NONE							*
 *	Return: Product code of product					*
 ************************************************************************/

int
hci_decode_product(
short	*product_data
)
{
	int	status;
	int	product_offset;
	int	block1_size;
	int	block1_num_layers;
	int	layer1_size = 0;
static	char	*uncompress = NULL;

/*	Check to see if product previously decoded.  If so, free all	*
 *	memory associated with it before decoding new product.		*/

	if ((Product_code >= 16) &&
	    (Product_code <= ORPGPAT_MAX_PRODUCT_CODE)) {

 	    free_memory(Product_code);
	    Product_code = -1;

	}

/*	Decode product header, and return the offset to the symbology	* 
 *	block.								*/

	product_offset = decode_header (product_data);

/*	Validate product code.						*/ 

	if ((Product_code < 16) ||
	    (Product_code > ORPGPAT_MAX_PRODUCT_CODE)) {
   
	    return Product_code;

	}

/*	Check to see if the product is compressed.  If so, uncompress	*
	it.								*/

	if (ORPGPAT_get_compression_type (
		ORPGPAT_get_prod_id_from_code (Product_code)) > 0) {

	    if (uncompress != NULL)
		free (uncompress);

/* 	This function receives the address of the ORPG product header.	*/
	    uncompress = 
               RPGC_decompress_product ((void *) (((char *) product_data) - sizeof(Prod_header)));

/*	    NOTE: We don't have to free the old product_data here since	*
 *	    it is free'd later in the calling routine.			*/

            if( uncompress != NULL )
	       product_data = (short *) (((char *) uncompress) + sizeof(Prod_header));

	}
 
/*	Calculate the size of the symbology block			*/

	block1_size = ((product_data [product_offset + BLOCK1_SIZE_OFFSET] << 16) |
		      (product_data [product_offset + BLOCK1_SIZE_OFFSET+1] & 0xffff))/2;

/*	Calculate the number of layers in the symbology block		*/

	block1_num_layers = product_data [product_offset + NUM_LAYERS_OFFSET];

	if (block1_num_layers == 1) {

	    layer1_size = ((product_data [product_offset + LAYER1_SIZE_OFFSET] << 16) |
			  (product_data [product_offset + LAYER1_SIZE_OFFSET +1] & 0xffff))/2; 

	} else if (block1_num_layers >         1 && 
		   Product_code      != DHR_TYPE &&
		   Product_code      != DBV_TYPE &&
		   Product_code      != DVL_TYPE &&
		   Product_code      !=  DR_TYPE &&
		   Product_code      !=  DV_TYPE &&
		   Product_code      !=  SS_TYPE &&
		   Product_code      != SPD_TYPE &&
		   Product_code      != FTM_TYPE &&
		   Product_code      != PUP_TYPE &&
		   ORPGPAT_get_format_type (ORPGPAT_get_prod_id_from_code (Product_code)) != FORMAT_TYPE_HIRES_RADIAL ) {

	    HCI_LE_error("More than 1 layer in the symbology block" );
	    HCI_LE_error("Tool does not support multiple layers." );
	    return -1;

	} else if (block1_num_layers == 0) {

	    layer1_size = 0;

	}
  
/*	Extract product type, and decode accordingly			*/

	product_offset += IMAGE_TYPE_OFFSET; 
	Product_type = product_data [product_offset]; 

	if (Product_type == RADIAL_TYPE) { 
   
/*	    Product is a radial run-length-encoded type			*/

	    status = decode_radial_rle (&product_data [product_offset]); 
      
	} else if ((Product_type == RASTER_TYPE) ||
		   (Product_type == 0xBa07 )){

/*	    Product is a raster run-length-encoded type			*/

	    Product_type = RASTER_TYPE;

	    status = decode_raster_rle (&product_data[ product_offset]); 

	} else if (Product_code == CLD_TYPE) {
   
/*	    Product is a radial run-length-encoded type			*/

	    status = decode_radial_rle (&product_data [product_offset]); 
	    Product_type = CLD_TYPE;

	} else if (Product_code == CLR_TYPE) {
   
/*	    Product is a radial run-length-encoded type			*/

	    status = decode_radial_rle (&product_data [product_offset]); 
	    Product_type = CLR_TYPE;

	} else if (Product_code == DBV_TYPE) {

/*	    If this is the DBV product, decode it			*/

	    decode_dhr (&product_data[ product_offset]);
	    Product_type = DBV_TYPE;

	} else if (Product_code == DHR_TYPE) {

/*	    If this is the DHR product, decode it			*/

	    decode_dhr (&product_data[ product_offset]);
	    Product_type = DHR_TYPE;

	} else if (Product_code == DVL_TYPE) {

/*	    If this is the DVL product, decode it			*/

	    decode_dhr (&product_data[ product_offset]);
	    Product_type = DVL_TYPE;

	} else if (Product_code == DV_TYPE) {

/*	    If this is the DV product, decode it			*/

	    decode_dhr (&product_data[ product_offset]);
	    Product_type = DV_TYPE;

	} else if (Product_code == DR_TYPE) {

/*	    If this is the DR product, decode it			*/

	    decode_dhr (&product_data[ product_offset]);
	    Product_type = DR_TYPE;

	} else if (Product_code == VWP_TYPE) {

/*	    If this is the VWP product, decode it			*/

	    decode_vwp (product_data, 
			product_offset, 
			layer1_size);
	    Product_type = VWP_TYPE;

	} else if (Product_code == STI_TYPE) {

/*	    If this is the STI product, decode it			*/

	    decode_sti (product_data, 
			product_offset, 
			layer1_size);
	    Product_type = STI_TYPE;

	} else if (Product_code == HI_TYPE) {

/*	    If this is the HI product, decode it			*/

	    decode_hi ( product_data, 
			product_offset, 
			layer1_size);
	    Product_type = HI_TYPE;

	} else if (Product_code == MESO_TYPE) {

/*	    If this is the MESO product, decode it			*/

	    decode_meso (product_data, 
			 product_offset, 
			 layer1_size);
	    Product_type = MESO_TYPE;

	} else if (Product_code == TVS_TYPE) {

/*	    If this is the TVS product, decode it			*/

	    decode_tvs (product_data, 
			product_offset, 
			layer1_size);
	    Product_type = TVS_TYPE;

	} else if (Product_code == SWP_TYPE) {

/*	    If this is the SWP product, decode it			*/

	    decode_swp( product_data, 
			product_offset, 
			layer1_size );
	    Product_type = SWP_TYPE;

	} else if (Product_code == SS_TYPE  ||
	           Product_code == SPD_TYPE ||
	           Product_code == FTM_TYPE ||
	           Product_code == PUP_TYPE) {

/*	    If this is the SPD product, do nothing.			*/

	    Product_type = STAND_ALONE_TYPE;

	    decode_tabular_block (product_data);

	} else {

	    switch (ORPGPAT_get_format_type (ORPGPAT_get_prod_id_from_code (Product_code))) {

		case FORMAT_TYPE_RADIAL :
   
/*		    Product is a radial run-length-encoded type		*/

		    status = decode_radial_rle (&product_data [product_offset]); 
		    Product_type = GENERIC_RADIAL_TYPE;
		    break;

		case FORMAT_TYPE_HIRES_RADIAL :
   
/*		    Product is a high resolution radial type		*/

		    decode_dhr (&product_data [product_offset]); 
		    Product_type = GENERIC_HIRES_RADIAL_TYPE;
		    break;

		case FORMAT_TYPE_RASTER :
   
/*		    Product is a raster run-length-encoded type		*/

		    status = decode_raster_rle (&product_data [product_offset]); 
		    Product_type = GENERIC_RASTER_TYPE;
		    break;

		default :

		    HCI_LE_error("Unknown Product Type: Packet Code = %2x", Product_type);
		    return -1;

	    }
	}

	return Product_type;

}

/************************************************************************
 *	Description: This function reads product data from a regular	*
 *		     Concurrent format file.				*
 *									*
 *	Input:  file_name - Pointer to filename string (include path)	*
 *	Output: product_data - pointer to product data			*
 *	Return: number of bytes read					*
 ************************************************************************/

int
hci_read_concurrent_product_file (
char	*file_name, 
short	**product_data)
{
	short	message_header[9];
	int	fd;
	int	product_size;
	int	status;
	off_t	offset = 0;

/*	Open file for read only						*/

	fd = open (file_name, O_RDONLY);

	if (fd == -1) {

	    HCI_LE_error("read_concurrent_product_file () failed" );
	    return fd;

	}

/*	Read product message header					*/

	read (fd, message_header, 18);

/*	Extract the size of this product file, in bytes			*/

	product_size = (message_header [4] << 16) |
		       (message_header [5] & 0xffff);

/*	Allocate a buffer for the product data				*/

	*product_data = malloc( product_size );
	if( *product_data == NULL )
	{
	  HCI_LE_error("Product_data malloc failed for %d bytes", product_size);
	  HCI_task_exit( HCI_EXIT_FAIL );
	}
   
/*	Read in the product data from the product file			*/

	offset = lseek( fd, offset, SEEK_SET ); 
	status = read( fd, *product_data, product_size );

/*	Check status of read operation.  If error, return error.	*
 *	Otherwise, return number of bytes read.				*/

	if (status == -1) {

	    HCI_LE_error("read_concurrent_product_file (): Error On Product Read." );

	} 
   
	return product_size;

}

/************************************************************************
 *	Description: This function decodes the product header and	*
 *		     builds graphical/tabular/attributes blocks.	*
 *									*
 *	Input:  product_data - pointer to product data.		*
 *	Output: NONE							*
 *	Return: offset to symbology block				*
 ************************************************************************/

static int
decode_header(
short	*product_data
)
{

	int	source_id;
	int	height;
	int	day;
	int	month;
	int	year;
	int	i;
	int	j;
	int	hours;
	int	degrees;
	int	minutes;
	int	seconds;
	float	latitude;
	float	longitude;
	size_t	length;
	short	vcp;
	short	mode;
	short	elevation;
	int	fullword;

/*	Fill Graphic Product Struct					*/

	memcpy( &Graphic_product_struct, product_data, sizeof( Graphic_product_struct ));
  
/*	Allocate space for product pertinent attributes			*/ 

	Attribute = malloc (sizeof (product_pertinent_t));
	if( Attribute == NULL )
	{
	  HCI_LE_error("Attribute malloc failed for %d bytes", sizeof(product_pertinent_t));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Allocate space for annotations data				*/

	for (i=0;i<PROD_NUM_ANNOTATE_LINES;i++) {

	    Attribute->text [i] = malloc ( MAX_ANNOTATE_LINE_LEN + 1 );
	    if( Attribute->text[i] == NULL )
	    {
	      HCI_LE_error("Attribute->text[%d] malloc failed for %d bytes", i, 21*sizeof(char)+1);
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }
      
/*	    Initialize data to blanks					*/

	    for (j=0;j<MAX_ANNOTATE_LINE_LEN;j++) {

		Attribute->text [i][j] = ' ';

	    }
            Attribute->text[i][MAX_ANNOTATE_LINE_LEN] = '\0';
	}

	Attribute->number_of_lines = -1;

/*	Extract product code						*/

	Product_code = product_data [MESSAGE_CODE_OFFSET];
        HCI_LE_log("Setting Product_code: %d", Product_code );

	if ((Product_code >= 16) &&
	    (Product_code <= ORPGPAT_MAX_PRODUCT_CODE)){

	    char *title;

/*	    Build product ID and product resolution text string		*/

	    length = strlen (ORPGPAT_get_description (
			     ORPGPAT_get_prod_id_from_code (Product_code),
			     STRIP_NOTHING));

	    title = calloc (length+1,1);

	    strcpy (title, ORPGPAT_get_description (
			   ORPGPAT_get_prod_id_from_code (Product_code),
			   STRIP_NOTHING));

	    for (i=0;i<length;i++) {
         
		Attribute->product_name [i] = title[i];

	    }

	    Attribute->product_name [length] = '\0';

	}
   
/*	Extract volume scan time of product				*/

	fullword = (product_data [TIME_OFFSET] << 16) |
		   (product_data [TIME_OFFSET+1] & 0xffff);

	Product_time = fullword;

	convert_time (fullword, &hours, &minutes, &seconds );  

	Attribute->number_of_lines++;

	sprintf( Attribute->text[PROD_TIME_INDEX], "%2.2d:%2.2d", hours, minutes ); 
   
/*	Extract volume scan date of product				*/

	Product_date = product_data [DATE_OFFSET];;

	calendar_date (product_data [DATE_OFFSET], &day, &month,
                    &year );

	Attribute->number_of_lines++;

	sprintf( Attribute->text[PROD_DATE_INDEX], "%2.2d/%2.2d/%2.2d", month, day, year %100 );

/*	Extract the source ID of the product				*/

	source_id = product_data [SOURCE_ID_OFFSET];

	Attribute->number_of_lines++;

	sprintf( Attribute->text[PROD_ID_INDEX], "%3d", source_id );

/*	Extract radar latitude						*/

	fullword = (product_data [LATITUDE_OFFSET] << 16) |
		   (product_data [LATITUDE_OFFSET+1] & 0xffff);
	latitude = ((float) fullword)/1000.0;
	Radar_latitude = latitude;

/*	Convert to degrees, minutes, seconds format			*/

	degrees_minutes_seconds( latitude, 
                            &degrees, 
                            &minutes, 
                            &seconds );
   
/*	Combine source id and latitude in one line			*/

	Attribute->number_of_lines++;

	sprintf( Attribute->text[PROD_LAT_INDEX], "%2.2d/%2.2d/%2.2d N", 
            degrees, minutes, seconds ); 

/*	Extract the radar height MSL					*/

	height = product_data [RADAR_HEIGHT_OFFSET];

	Attribute->number_of_lines++;

	sprintf( Attribute->text[PROD_ELEV_HEIGHT_INDEX], "%4d FT", height ); 

/*	Extract radar longitude						*/

	fullword = (product_data [LONGITUDE_OFFSET] << 16) |
		   (product_data [LONGITUDE_OFFSET+1] & 0xffff);
	longitude = ((float) fullword)/1000.0;
	Radar_longitude = longitude;

/*	Convert longitude to degrees, minutes, seconds format		*/

	degrees_minutes_seconds( longitude, 
                            &degrees, 
                            &minutes, 
                            &seconds );

/*	Combine radar height and longitude in one line			*/

	Attribute->number_of_lines++;

	if (degrees < 0) {

	    degrees = -degrees;
	    sprintf( Attribute->text[PROD_LON_INDEX], "%3.2d/%2.2d/%2.2d W", 
		degrees, minutes, seconds ); 

	} else {

	    sprintf( Attribute->text[PROD_LON_INDEX], "%2.2d/%2.2d/%2.2d E", 
		degrees, minutes, seconds ); 

	}


/*	Extract operational vcp						*/

	vcp  = product_data [VCP_OFFSET];

	Product_vcp = vcp;

	Attribute->number_of_lines++;

	sprintf( Attribute->text[PROD_VCP_INDEX], "%d",vcp); 

/*	Extract operational mode					*/

	mode = product_data[MODE_OFFSET];

	/* Since this gets set via the UMC (librpg) the definition
	   is reversed to support the (legacy) PUP. */

	Attribute->number_of_lines++;

	if (mode == CLEAR_AIR_MODE) {

	    sprintf( Attribute->text[PROD_MODE_INDEX], "B"); 

	} else if (mode == PRECIPITATION_MODE) {

	    sprintf( Attribute->text[PROD_MODE_INDEX], "A");

	} else {

	    sprintf( Attribute->text[PROD_MODE_INDEX], "M");

 	}

/*	Extract elevation angle of product				*/

	elevation = product_data [ELEVATION_OFFSET];

	Elevation_index = elevation;

	Attribute->number_of_lines++;

	Attribute->elevation = ORPGVCP_get_elevation_angle( vcp, elevation ); 

	if (Attribute->elevation >= -0.500000) {

	    sprintf( Attribute->text[PROD_CUT_ANGLE_INDEX], "%.1f", 
		Attribute->elevation);

	}

/*	Add product dependent data					*/

	product_dependent( product_data );

/*	Determine the number of data levels				*/

	Attribute->num_data_levels = 
		Data_level_tab [product_data [MESSAGE_CODE_OFFSET]];
  
/*	Assign data levels. Ignore DHR product.				*/

	if ((Product_code != DHR_TYPE) &&
	    (Product_code != DBV_TYPE) &&
	    (Product_code != DVL_TYPE) &&
	    (Product_code != DR_TYPE ) &&
	    (Product_code != DV_TYPE ) &&
	    (ORPGPAT_get_format_type (
		ORPGPAT_get_prod_id_from_code (Product_code))
			  != FORMAT_TYPE_HIRES_RADIAL)) {

	    for (i=0;i<Attribute->num_data_levels;i++) {

		Attribute->data_levels[i] = 
			malloc (MAX_LEVEL_LENGTH*sizeof (char));
		if( Attribute->data_levels[i] == NULL )
		{
	          HCI_LE_error("Attribute->data_levels[%d] malloc failed for %d bytes", i, MAX_LEVEL_LENGTH*sizeof(char));
		  HCI_task_exit( HCI_EXIT_FAIL );
		}

		decode_data_level (product_data [DATA_LEVEL_OFFSET + i], 
				   Attribute->data_levels[i] );
	    }
	}

/*	Set the resolutions for this radial product			*/
/*	NOTE: By default we get the product resolution from the		*
 *	internal table.  This is because the current ICD does not	*
 *	support full resolution information for all product types.	*
 *	It is expected for the new generic types that the product	*
 *	resolution will be defined in the product attributes table.	*
 *	For regular radial products this does not have to be done.	*
 *	For high resolution radial products this must be done.		*/

	Attribute->x_reso = 
		Xy_azran_reso [product_data [MESSAGE_CODE_OFFSET]][0]; 
	Attribute->y_reso = 
		Xy_azran_reso [product_data [MESSAGE_CODE_OFFSET]][1];

/*	Extract offset to product symbology block			*/

	Symbology_Block = (product_data [OFFSET_TO_SYMBOLOGY] << 16) |
			  (product_data [OFFSET_TO_SYMBOLOGY+1] & 0xffff);

/*	Extract offset to product graphic alphanumeric block		*/

	Grafattr_block = (product_data [OFFSET_TO_GRAFATTR] << 16) |
			 (product_data [OFFSET_TO_GRAFATTR+1] & 0xffff);

/*	Extract offset to product tabular alphanumeric block		*/

	Tabular_block = (product_data [OFFSET_TO_TABULAR] << 16) |
			(product_data [OFFSET_TO_TABULAR+1] & 0xffff);

/*	Treat the SS and SPD products as a special case.  The offset to	*
 *	the tabular alphanumeric block is stored in the symbology	*
 *	block.								 */

	if ((Product_code == SS_TYPE  || Product_code == SPD_TYPE ||
	     Product_code == FTM_TYPE || Product_code == PUP_TYPE)  && 
	    Tabular_block == 0) {

	    Tabular_block = Symbology_Block;
	    Grafattr_block = 0;

	}

/*	Return offset to symbology block				*/

	return Symbology_Block;

}

/************************************************************************
 *	Description: This function decodes run length encoded radial	*
 *		     data and writes the output to the internal		*
 *		     Radial_image structure for easy access.		*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: NONE							*
 *	Return: status (normal = 0)					*
 ************************************************************************/

static int
decode_radial_rle (
short	*product_data
)
{ 

	int	num_halfwords_radial;
	int	start_index;
	int	bin;
	int	i;
	int	j;
	int	k;
	float	total_width;
	unsigned char	run;
	unsigned char	color;   
	unsigned short	ushort_value;
    
/*	Allocate memory for the radial_rle struture			*/

	Radial_image = malloc( sizeof( radial_rle_t ) );
	if( Radial_image == NULL )
	{
	  HCI_LE_error("Radial_image malloc failed for %d bytes", sizeof(radial_rle_t));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}
 

/*	Extract the number of data elements, range interval, and number	*
 *	of radials							*/

	Radial_image->data_elements = 
		product_data [NUMBER_OF_BINS_OFFSET];
	Radial_image->range_interval = 
 		product_data [RANGE_INTERVAL_OFFSET]/1000.0;
	Radial_image->number_of_radials = 
		product_data [NUMBER_OF_RADIALS_OFFSET];

/*	Allocate memory for the radial data				*/

	for(i=0;i<Radial_image->number_of_radials;i++) {
 
	    Radial_image->radial_data[i] = malloc( 920*sizeof(short) );
	    if( Radial_image->radial_data[i] == NULL )
	    {
	      HCI_LE_error("Radial_image->radial_data[%d] malloc failed for %d bytes", i, 920*sizeof(short));
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }
 
	}

/*	Allocate memory for the azimuth data				*/

	Radial_image->azimuth_angle = 
		malloc( Radial_image->number_of_radials*sizeof(float) );
	if( Radial_image->azimuth_angle == NULL )
	{
	  HCI_LE_error("Radial_image->azimuth_angle malloc failed for %d bytes", Radial_image->number_of_radials*sizeof(float));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Allocate memory for the azimuth width data			*/

	Radial_image->azimuth_width = 
		malloc( Radial_image->number_of_radials*sizeof(float) );
	if( Radial_image->azimuth_width == NULL )
	{
	  HCI_LE_error("Radial_image->azimuth_width malloc failed for %d bytes", Radial_image->number_of_radials*sizeof(float));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	start_index = RADIAL_RLE_OFFSET;
	total_width = 0.0;

	for (i=0;i<Radial_image->number_of_radials;i++) {
 
/*	    Get the number of halfwords this radial			*/

	    num_halfwords_radial = product_data [start_index];
      
/*	    Get the radial azimuth and azimuth delta			*/

	    Radial_image->azimuth_angle[i] = 
		product_data [start_index + AZIMUTH_ANGLE_OFFSET]/10.0;
	    Radial_image->azimuth_width[i] = 
		product_data [start_index + AZIMUTH_DELTA_OFFSET]/10.0;

	    total_width = Radial_image->azimuth_width[i] + total_width;

/* 	    initialize start_index and bin				*/

	    start_index += RADIAL_RLE_HEADER_OFFSET;
	    bin = 0;

	    for (j=0;j< num_halfwords_radial;j++,start_index++) {
          
		ushort_value = product_data [start_index];

/*		Extract first nibble of RLE data			*/

		run =   (unsigned char)((ushort_value & 0xf000) >> 12);
		color = (unsigned char)((ushort_value & 0x0f00) >> 8);

		for (k=0;k<run;k++,bin++) {

		    Radial_image->radial_data[i][bin] = color; 

		} 

/*		Extract second nibble of RLE data			*/

		run =   (unsigned char)((ushort_value & 0x00f0) >> 4);
		color = (unsigned char)(ushort_value & 0x000f);

		for (k=0;k<run;k++,bin++) {

		    Radial_image->radial_data[i][bin] = color;

		}  
	    } 
	}

	if (ORPGPAT_get_format_type (
		ORPGPAT_get_prod_id_from_code (Product_code)) >= 0) {

	    Attribute->x_reso = Radial_image->range_interval;
	    Attribute->y_reso = total_width/Radial_image->number_of_radials;

	}

	return 0;

}

/************************************************************************
 *	Description: This function decodes run length encoded raster	*
 *		     data and writes the output to the internal		*
 *		     Raster_image structure for easy access.		*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: NONE							*
 *	Return: status (normal = 0)					*
 ************************************************************************/

static int
decode_raster_rle(
short	*product_data
)
{

	int	num_halfwords_row;
	int	start_index;
	int	cell;
	int	i;
	int	j;
	int	k;
	unsigned short	ushort_value;
	unsigned char	run;
	unsigned char	color;

/*	Allocate memory for the raster_rle struture			*/

	Raster_image = malloc( sizeof( raster_rle_t ) );
	if( Raster_image == NULL )
	{
	  HCI_LE_error("Raster_image malloc failed for %d bytes", sizeof(raster_rle_t));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Extract the i,j start coordinates, the x,y scale factors, and i	*
 *	number of rows							*/

	Raster_image->x_start = product_data [I_START_OFFSET];
	Raster_image->y_start = product_data [J_START_OFFSET];
	Raster_image->x_scale = product_data [X_SCALE_OFFSET];
	Raster_image->y_scale = product_data [Y_SCALE_OFFSET];
	Raster_image->number_of_rows = product_data [NUMBER_OF_ROWS_OFFSET];
	Raster_image->number_of_columns = Raster_image->number_of_rows;
   
/*	Allocate memory for the raster data				*/

	for (i=0;i<Raster_image->number_of_rows;i++) {

	    Raster_image->raster_data[i] = 
		malloc( Raster_image->number_of_columns*sizeof(short) );
	    if( Raster_image->raster_data[i] == NULL )
	    {
	      HCI_LE_error("Raster_image->raster_data[%d] malloc failed for %d bytes", i, Raster_image->number_of_columns*sizeof(short));
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }

	}

	start_index = RASTER_RLE_OFFSET;

	for(i=0;i<Raster_image->number_of_rows;i++) {

/*	    Get the number of halfwords this row			*/

	    num_halfwords_row = product_data [start_index]/2;

/*	    initialize start_index and bin				*/

	    start_index += RASTER_RLE_HEADER_OFFSET;
	    cell = 0;

	    for (j=0;j< num_halfwords_row;j++,start_index++) {

		ushort_value = product_data [start_index];

/*		Extract first nibble of RLE data			*/

		run =  ((ushort_value & 0xf000) >> 12);
		color = ((ushort_value & 0x0f00) >> 8);

		for (k=0;k<run;k++,cell++) {

		    Raster_image->raster_data[i][cell] = color;

		}

/*		Extract second nibble of RLE data			*/

		run =  ((ushort_value & 0x00f0) >> 20);
		color = ((ushort_value & 0x000f) >> 16);

		for (k=0;k<run;k++,cell++) {

		    Raster_image->raster_data[i][cell] = color;

		}
	    }
	}

	return 0;

}

/************************************************************************
 *	Description: This function decodes Digital hybrid Reflectivity	*
 *		     data and writes the output to the internal		*
 *		     Dhr_image structure for easy access.		*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void
decode_dhr(
short	*product_data
)
{ 
	int	num_halfwords_radial;
	int	start_index;
	int	bin;
	int	i;
	int	j;
	float	total_width;
	unsigned short	ushort_value;
   
/*	Allocate memory for the radial_rle struture			*/

	Dhr_image = malloc( sizeof( dhr_rle_t ) );
	if( Dhr_image == NULL )
	{
	  HCI_LE_error("Dhr_image malloc failed for %d bytes", sizeof(dhr_rle_t));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}
 
/*	Extract number of data elements, range interval, and number of 	*
 *	radials								*/

	Dhr_image->data_elements  = product_data [NUMBER_OF_BINS_OFFSET];
	Dhr_image->range_interval = Attribute->x_reso;
	Dhr_image->number_of_radials = product_data [NUMBER_OF_RADIALS_OFFSET];

/*	Allocate memory for the radial data				*/

	for (i=0;i<Dhr_image->number_of_radials;i++) {

	    Dhr_image->radial_data[i] = malloc( Dhr_image->data_elements*sizeof(short) );
	    if( Dhr_image->radial_data[i] == NULL )
	    {
	      HCI_LE_error("Dhr_image->radial_data[%d] malloc failed for %d bytes", i, Dhr_image->data_elements*sizeof(short));
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }

	}

/*	Allocate memory for the azimuth data				*/

	Dhr_image->azimuth_angle = 
		malloc( Dhr_image->number_of_radials*sizeof(float) );
	if( Dhr_image->azimuth_angle == NULL )
	{
	  HCI_LE_error("Dhr_image->azimuth_angle malloc failed for %d bytes", Dhr_image->number_of_radials*sizeof(float));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Allocate memory for the azimuth width data			*/

	Dhr_image->azimuth_width = 
		malloc( Dhr_image->number_of_radials*sizeof(float) );
	if( Dhr_image->azimuth_width == NULL )
	{
	  HCI_LE_error("Dhr_image->azimuth_width malloc failed for %d bytes", Dhr_image->number_of_radials*sizeof(float));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	start_index = RADIAL_RLE_OFFSET;
	total_width = 0.0;

	for (i=0;i<Dhr_image->number_of_radials;i++) {

/*	    Get the number of halfwords this radial			*/

	    num_halfwords_radial = product_data [start_index]/2;
      
/*	    Get the radial azimuth and azimuth delta			*/

	    Dhr_image->azimuth_angle[i] = product_data [start_index + AZIMUTH_ANGLE_OFFSET]/10.0;
	    Dhr_image->azimuth_width[i] = product_data [start_index + AZIMUTH_DELTA_OFFSET]/10.0;

	    total_width = Dhr_image->azimuth_width[i] + total_width;

/*	    initialize start_index and bin				*/

	    start_index += RADIAL_RLE_HEADER_OFFSET;
	    bin = 0;

	    for (j=0;j<num_halfwords_radial;j++,start_index++) {

		ushort_value = product_data [start_index];

/*		Extract the colors					*/

		Dhr_image->radial_data[i][bin++] = (ushort_value & 0xff00) >> 8; 
		Dhr_image->radial_data[i][bin++] = (ushort_value & 0x00ff);

	    } 
	}

/*	If this is a new product who's format is defined in the product	*
 *	attributes table, then we need to try and get it's resolution	*
 *	from there (if the x_azi_res and y_ran_res fields are defined).	*/

	if (ORPGPAT_get_format_type (
		ORPGPAT_get_prod_id_from_code (Product_code)) > 0) {

	    if (ORPGPAT_get_resolution (
		ORPGPAT_get_prod_id_from_code (Product_code),
		ORPGPAT_X_AZI_RES) > 0) {

		Attribute->y_reso = ORPGPAT_get_resolution (
			ORPGPAT_get_prod_id_from_code (Product_code),
			ORPGPAT_X_AZI_RES) / 10.0;

	    } else {

		Attribute->y_reso = total_width/Dhr_image->number_of_radials;

	    }

	    if (ORPGPAT_get_resolution (
		ORPGPAT_get_prod_id_from_code (Product_code),
		ORPGPAT_Y_RAN_RES) > 0) {

		Attribute->x_reso = ORPGPAT_get_resolution (
			ORPGPAT_get_prod_id_from_code (Product_code),
			ORPGPAT_Y_RAN_RES) / 1000.0;

	    } else {

		Attribute->x_reso = 230/Dhr_image->data_elements;

	    }
	}

	return;
}

/************************************************************************
 *	Description: This function decodes Storm Tracking Index		*
 *		     data and writes the output to theinternal		*
 *		     Sti_image structure for easy access.		*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void
decode_sti(
short	*product_data,
short	product_offset, 
int	layer1_size
)
{
	int	last_index;
	short	block_size;
	short	packet_type;
	short	packet_size;
	short	i;
	short	j;
	short	k;
static	short	four_km = 4;
   
/*	Allocate space for sti stucture					*/

	Sti_image = malloc( sizeof( sti_t ) );
	if( Sti_image == NULL )
	{
	  HCI_LE_error("Sti_image malloc failed for %d bytes", sizeof(sti_t));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Initialize the number of storms and number of past and		*
 *	forecast positions.						*/

	Sti_image->number_storms = -1;

	for (i=0;i<100;i++) {

	    Sti_image->number_past_pos[i]   = -1;
	    Sti_image->number_forcst_pos[i] = -1;

	}

/*	Process the Symbology Block.					*/

	last_index = product_offset + layer1_size;

	while (product_offset < last_index) {

/*	    Extract the packet type.					*/

	    packet_type = product_data [product_offset];

	    if (packet_type == 15) {

/*		This is a Storm ID packet. Increment # of storms.	*/

		Sti_image->number_storms++;
		i = Sti_image->number_storms;
   
/*		Set the the Storm ID					*/

		Sti_image->equiv.storm_id [Sti_image->number_storms][2] =
			'\0';
		Sti_image->equiv.storm_id_hw [Sti_image->number_storms][0] = 
			product_data [product_offset + PACKET15_STORMID];

/*		Set the x and y positions.				*/

		Sti_image->curr_xpos [Sti_image->number_storms] = 
			product_data [product_offset + PACKET15_XPOS]/four_km;
		Sti_image->curr_ypos [Sti_image->number_storms] = 
			product_data [product_offset + PACKET15_YPOS]/four_km;

/*		Update product offset for next pass.			*/

		packet_size    = product_data [product_offset + 1]/2;
		product_offset = product_offset + packet_size + 2;

	    } else if (packet_type == 23) {
     
/*		Extract packets within this packet.			*/

		packet_size = product_data [product_offset + 1]/2;
		block_size  = product_offset + packet_size + 2;
    
/*		Move offset to start of first packet			*/

		product_offset += 2;

/*		Decode all packets within this block of packets.	*/

		while (product_offset < block_size) { 
        
		    packet_type = product_data [product_offset];

/*		    If special symbol packet, get x and y positions	*/

		    if (packet_type == SPECIAL_SYMBOL_PACKET) {

			Sti_image->number_past_pos[i]++;
			j = Sti_image->number_past_pos[i];
			Sti_image->past_xpos[i][j] = 
				product_data [product_offset + PACKET2_XPOS]/four_km;
			Sti_image->past_ypos[i][j] = 
				product_data [product_offset + PACKET2_YPOS]/four_km;
		    }

/*		    Update product offset.				*/

		    packet_size    = product_data [product_offset + 1]/2;
		    product_offset = product_offset + packet_size + 2;

		}
      
	    } else if (packet_type == 24) {
     
/*		Extract packets within this packet.			*/

		packet_size = product_data [product_offset + 1]/2;
		block_size  = product_offset + packet_size + 2;

/*		Move offset to start of first packet			*/

		product_offset += 2;

/*		Decode all packets within this block of packets.	*/

		while (product_offset < block_size) { 
        
		    packet_type = product_data [product_offset];

/*		    If special symbol packet, get x and y positions	*/

		    if (packet_type == SPECIAL_SYMBOL_PACKET) {

			Sti_image->number_forcst_pos[i]++;
			k = Sti_image->number_forcst_pos[i];
			Sti_image->forcst_xpos[i][k] = 
				product_data [product_offset + PACKET2_XPOS]/four_km;
			Sti_image->forcst_ypos[i][k] = 
				product_data [product_offset + PACKET2_YPOS]/four_km;

		    }

/*		    Update product offset.				*/

		    packet_size    = product_data [product_offset + 1]/2;
		    product_offset = product_offset + packet_size + 2;

		}
      
	    } else if (packet_type == 25) {

/*		This is a slow mover.					*/
         
/*		Set number of past and forecast positions to flag value.*/

		Sti_image->number_past_pos[i]   = SLOW_MOVER; 
		Sti_image->number_forcst_pos[i] = SLOW_MOVER; 

	    } else {

/*		This is a don't care packet type.			*/

/*		Update the product offset for next pass.		*/

		packet_size    = product_data [product_offset + 1]/2;
		product_offset = product_offset + packet_size + 2;

	    }
	}
}

/************************************************************************
 *	Description: This function decodes Hail Index data and writes	*
 *		     the output to the internal Hi_image structure for	*
 *		     easy access.					*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void
decode_hi (
short	*product_data,
short	product_offset, 
int	layer1_size
)
{
	int	last_index;
	short	packet_type;
	short	packet_size;
	short	prob_hail;
	short	prob_svr;
static	short	four_km = 4;
   
/*	Allocate space for hi stucture					*/

	Hi_image = malloc( sizeof( hi_t ) );
	if( Hi_image == NULL )
	{
	  HCI_LE_error("Hi_image malloc failed for %d bytes", sizeof(hi_t));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Initialize the number of storms.				*/

	Hi_image->number_storms = -1;

/*	Process the Symbology Block.					*/

	last_index = product_offset + layer1_size;

	while (product_offset < last_index) {

/*	    Extract the packet type.					*/

	    packet_type = product_data [product_offset];

	    if (packet_type == 15) {

/*		Set the the Storm ID					*/

		Hi_image->equiv.storm_id [Hi_image->number_storms][2] = '\0';
		Hi_image->equiv.storm_id_hw [Hi_image->number_storms][0] = 
			product_data [product_offset + PACKET15_STORMID];

/*		Set the x and y positions.				*/

		Hi_image->curr_xpos [Hi_image->number_storms] = 
			product_data [product_offset + PACKET15_XPOS]/four_km;
		Hi_image->curr_ypos [Hi_image->number_storms] = 
			product_data [product_offset + PACKET15_YPOS]/four_km;

/*		Update product offset for next pass.			*/

		packet_size    = product_data [product_offset + 1]/2;
		product_offset = product_offset + packet_size + 2;

	    } else if (packet_type == 19) {
     
/*		Extract packets within this packet.			*/

/*		This is a Storm ID packet. Increment # of storms.	*/

		Hi_image->number_storms++;

/*		Extract the probabilities and maximum hail size.	*/

		prob_hail = product_data [product_offset + PACKET19_POH];
		prob_svr  = product_data [product_offset + PACKET19_PSH];

		if ((prob_hail > 0 )||
		    (prob_svr  > 0 )) {
         
/*		    Both probabilities > 0.				*/

		    Hi_image->prob_hail [Hi_image->number_storms] = prob_hail;
		    Hi_image->prob_svr  [Hi_image->number_storms] = prob_svr;
		    Hi_image->hail_size [Hi_image->number_storms] = 
			product_data [product_offset + PACKET19_MHS];

		} else {

/*		    Ignore this storm.					*/

		    Hi_image->number_storms--;

	        }

/*		Update product offset.					*/

		packet_size    = product_data [product_offset + 1]/2;
		product_offset = product_offset + packet_size + 2;

	    } else {

/*		This is a don't care packet type.			*/

/*		Update the product offset for next pass.		*/

		packet_size    = product_data [product_offset + 1]/2;
		product_offset = product_offset + packet_size + 2;

	    }
	}
}

/************************************************************************
 *	Description: This function decodes Mesocyclone product data	*
 *		     and writes the output to the internal Meso_image	*
 *		     structure for easy access.				*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void
decode_meso (
short	*product_data,
short	product_offset, 
int	layer1_size
)
{
	int	last_index;
	int	number_symbols;
	int	cnt;
	int	i;
	short	packet_type;
	short	packet_size;
static	short	four_km = 4;
   
/*	Allocate space for meso stucture				*/

	Meso_image = malloc( sizeof( meso_t ) );
	if( Meso_image == NULL )
	{
	  HCI_LE_error("Meso_image malloc failed for %d bytes", sizeof(meso_t));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Initialize the number of mesocyclones and 3D shears.		*/

	Meso_image->number_mesos = -1;
	Meso_image->number_3Dshears = -1;

/*	Process the Symbology Block.					*/

	last_index = product_offset + layer1_size;

	while (product_offset < last_index) {

/*	    Extract the packet type.					*/

	    packet_type = product_data [product_offset];

	    if (packet_type == 15) {

/*		Set the the Storm ID					*/

		Meso_image->equiv.storm_id [Meso_image->number_mesos][2] =
			'\0';
		Meso_image->equiv.storm_id_hw [Meso_image->number_mesos][0] =
			product_data [product_offset + PACKET15_STORMID];

/*		Update product offset for next pass.			*/

		packet_size    = product_data [product_offset + 1]/2;
		product_offset = product_offset + packet_size + 2;

	    } else if (packet_type == 3) {
     
/*		This is a mesocyclone packet.				*/

/*		Increment # of mesocyclones.				*/

		Meso_image->number_mesos++;

/*		Extract the mesocyclone position.			*/

		Meso_image->meso_xpos [Meso_image->number_mesos] = 
			product_data [product_offset + PACKET3_XPOS]/four_km;
		Meso_image->meso_ypos [Meso_image->number_mesos] = 
			product_data [product_offset + PACKET3_YPOS]/four_km;
   
/*		Extract the mesocyclone size.				*/

		Meso_image->meso_size [Meso_image->number_mesos] =
			product_data [product_offset + PACKET3_SIZE]/four_km;

/*		Update product offset.					*/

		packet_size    = product_data [product_offset + 1]/2;
		product_offset = product_offset + packet_size + 2;

	    } else if (packet_type == 11) {
     
/*		This is a 3D shear packet.				*/

/*		Extract the packet size.  If there are multiple symbols	*
 *		in this packet, each symbol requires 3 halfwords to	*
 *		define.  The packet has two halfwords of overhead.	*/

		packet_size = product_data [product_offset + 1]/2;

/*		Calculate the number of symbols.			*/

		number_symbols = packet_size / HW_PER_MESO_SYMBOL;

		for (cnt=0;cnt<number_symbols;cnt++) {

/*		    Increment # of 3D shears.				*/

		    Meso_image->number_3Dshears++;

/*		    Define packet offset for this symbol.		*/

		    i = cnt*HW_PER_MESO_SYMBOL;

/*		    Extract the 3D shear position.			*/

		    Meso_image->a3D_xpos [Meso_image->number_3Dshears] = 
			product_data [product_offset+PACKET3_XPOS+i]/four_km;
		    Meso_image->a3D_ypos [Meso_image->number_3Dshears] = 
			product_data [product_offset+PACKET3_YPOS+i]/four_km;

/*		    Extract the 3D shear size.				*/

		    Meso_image->a3D_size [Meso_image->number_3Dshears] =
			product_data [product_offset+PACKET3_SIZE+i]/four_km;

		}

/*		Update product offset.					*/

		product_offset = product_offset + packet_size + 2;

	    } else {

/*		This is a don't care packet type.			*/

/*		Update the product offset for next pass.		*/

		packet_size    = product_data [product_offset + 1]/2;
		product_offset = product_offset + packet_size + 2;

	    }
	}
}

/************************************************************************
 *	Description: This function decodes Tornado detection data	*
 *		     and writes the output to the internal Tvs_image	*
 *		     structure for easy access.				*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void
decode_tvs (
short	*product_data,
short	product_offset, 
int	layer1_size
)
{
	int	last_index;
	short	packet_type;
	short	packet_size;
static	short	four_km = 4;
   
/*	Allocate space for tvs stucture					*/

	Tvs_image = malloc( sizeof( tvs_t ) );
	if( Tvs_image == NULL )
	{
	  HCI_LE_error("Tvs_image malloc failed for %d bytes", sizeof(tvs_t));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Initialize the number of TVSs.					*/

	Tvs_image->number_tvs = -1;

/*	Process the Symbology Block.					*/

	last_index = product_offset + layer1_size;

	while (product_offset < last_index) {

/*	    Extract the packet type.					*/

	    packet_type = product_data [product_offset];

	    if (packet_type == 15) {

/*		Set the the Storm ID					*/

		Tvs_image->equiv.storm_id    [Tvs_image->number_tvs][2] = '\0';
		Tvs_image->equiv.storm_id_hw [Tvs_image->number_tvs][0] = 
			product_data [product_offset + PACKET15_STORMID];

/*		Update product offset for next pass.			*/

		packet_size    = product_data [product_offset + 1]/2;
		product_offset = product_offset + packet_size + 2;
      
	    } else if (packet_type == 12) {
     
/*		This is a TVS packet.					*/

/*		Increment # of TVSs.					*/

		Tvs_image->number_tvs++;

/*		Extract the TVS position.				*/

		Tvs_image->tvs_xpos [Tvs_image->number_tvs] = 
			product_data [product_offset + PACKET12_XPOS]/four_km;
		Tvs_image->tvs_ypos [Tvs_image->number_tvs] = 
			product_data [product_offset + PACKET12_YPOS]/four_km;
   
/*		Update product offset.					*/

		packet_size    = product_data [product_offset + 1]/2;
		product_offset = product_offset + packet_size + 2;

	    } else {

/*		This is a don't care packet type.			*/

/*		Update the product offset for next pass.		*/

		packet_size    = product_data [product_offset + 1]/2;
		product_offset = product_offset + packet_size + 2;

	    }
	}       
}

/************************************************************************
 *	Description: This function decodes VWP wind product data	*
 *		     and writes the output to the internal Vwp_image	*
 *		     structure for easy access.				*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void
decode_vwp (
short	*product_data,
short	product_offset, 
int	layer1_size
)
{
/*	Constants extracted from VWP product generator for 		*
 *	calculating time						*/

	float	time_const1 = 90.33;
	float	time_const2 = 38.334;
	float	max_volume_scans = 10.0;
	float	time;
   
/*	Constants extracted from VWP product generator for 		*
 *	calculating height						*/

	float	height_const1 = 10.0;
	float	height_const2 = 460.0;

	int	dummy_arg = -1;
	int	last_index;
	int	done;
	int	i;
	int	j;
	size_t	length;
	short	packet_type;
	short	block4_length;
	short	height;
	short	height_counter = -1;
	short	time_counter = -1;
	char	*packet_text;

/*	Allocate memory for vwp product data				*/

	Vwp_image = malloc( sizeof( vwp_t ) );
	if( Vwp_image == NULL )
	{
	  HCI_LE_error("Vwp_image malloc failed for %d bytes", sizeof(vwp_t));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Initialize the time and height strings				*/

	for (i=0;i<30;i++) {

	    Vwp_image->heights[i][0] = '\0';

	}

	for (i=0;i<11;i++) {

	    Vwp_image->times[i][0] = '\0';

	}

/*	Initialize the number of times and number of heights to 	*
 *	missing								*/

	Vwp_image->number_of_times   = -1;
	Vwp_image->number_of_heights = -1;

/*	Initialize the color for all possible times and heights to 0	*
 *	Initialize the heights and times to all possible values		*
 *	Initialize the speed and direction to missing value		*/

	for (i=0;i<30;i++) { 

	    for (j=0;j<11;j++) { 

		Vwp_image->barb[i][j].color     = 0;
		Vwp_image->barb[i][j].time      = j;
		Vwp_image->barb[i][j].height    = i;
		Vwp_image->barb[i][j].direction = -666;
		Vwp_image->barb[i][j].speed     = -666;

	    }
	} 

/*	Decode all pertinent data packets in this product		*/

	done        = 0;
	last_index  = product_offset + layer1_size;
	packet_type = product_data [product_offset];

	while (!done && product_offset < last_index) {

	    if (packet_type == 0x8) {

		decode_text_packet_8 (&product_data[ product_offset ], 
                               &packet_text,
                               &dummy_arg,
                               &dummy_arg );

		length = strlen (packet_text);

		if ((length == 4) &&
		    (strcmp ("TIME", packet_text))) {

/*		    This must be a time field				*/

		    Vwp_image->number_of_times++; 
		    strcpy(Vwp_image->times [Vwp_image->number_of_times],
			packet_text );
		    Vwp_image->times [Vwp_image->number_of_times][4] = '\0';

		} else if ((length == 2) &&
			   (strcmp("ND", packet_text))) {

/*		    This must be a height field				*/

		    Vwp_image->number_of_heights++; 
		    strcpy (Vwp_image->heights [Vwp_image->number_of_heights],
			packet_text );
		    Vwp_image->heights [Vwp_image->number_of_heights][2] = '\0';

		}

/*		Update the product_offset				*/

		product_offset = product_offset +
			product_data [product_offset+PACKET_SIZE_OFFSET]/2 + 2;

/*		Extract next packet type for next pass			*/   

		packet_type = product_data [product_offset]; 

		free (packet_text);
         
	    } else if (packet_type == 0xA) {

/*		Find the length of this packet and add to product_offset*
 *		Currently, we are ignoring this packet code		 */

		product_offset = product_offset + 
			product_data [product_offset+PACKET_SIZE_OFFSET]/2 + 2; 

/*		Extract next packet type for next pass			*/   

		packet_type = product_data [product_offset]; 

	    } else if (packet_type == 0x4) {

/*		This is wind barb data					*/
    
/*		Find length of data block n number of shorts		*/

		block4_length = 
			product_data [product_offset + 1]/2; 

		product_offset += BLOCK4_OVERHEAD; 

		while (block4_length > 0) {

/*		    Calculate time_counter and height_counter from pixel*
 *		    location data					*/ 

		    time = (float) product_data [product_offset+PACKET4_TIME_OFFSET]; 
		    time_counter = (int) ((time - time_const1)/time_const2 + 0.5); 
		    time_counter = max_volume_scans - time_counter;

		    if ((time_counter <  0) ||
			(time_counter > 10)) {

			HCI_LE_error("decode_vwp (): time_counter out of range!" );
			return;

		    }

		    height = product_data [product_offset + PACKET4_HEIGHT_OFFSET]; 
		    height_counter = (Vwp_image->number_of_heights + 1 ) - 
                             (((float)height-height_const1)/height_const2 *
                             (float)(Vwp_image->number_of_heights+2));

		    if ((height_counter <  0) ||
			(time_counter   > 29)) {

			HCI_LE_error("decode_vwp (): height_counter out of range!" );
			return;

		    }
       
/*		    Store color, time, height, direction, and speed	*/

		    Vwp_image->barb [height_counter][time_counter].color = 
			product_data [product_offset + PACKET4_COLOR_OFFSET]; 
		    Vwp_image->barb [height_counter][time_counter].time = 
			time_counter; 
		    Vwp_image->barb [height_counter][time_counter].height = 
			height_counter; 
		    Vwp_image->barb [height_counter][time_counter].direction = 
			product_data [product_offset + PACKET4_DIRECTION_OFFSET]; 
		    Vwp_image->barb [height_counter][time_counter].speed = 
			product_data [product_offset + PACKET4_SPEED_OFFSET]; 

/*		    Update the product_offset				*/

		    product_offset = product_offset + PACKET4_SIZE;

/*		    Update the number of shorts remaining in this block */

		    block4_length -= PACKET4_SIZE;
 
		}  

/*		Extract next packet type for next pass			*/   

		packet_type = product_data [product_offset]; 

	    } else {

		done = 1;

	    }

	}

	return;
}

/************************************************************************
 *	Description: This function decodes Severe Wx Probability data	*
 *		     and writes the output to the internal Swp_image	*
 *		     structure for easy access.				*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
decode_swp (
short	*product_data,
short	product_offset, 
int	layer1_size
)
{
	int	last_index;
static	short	four_km = 4;
	short	packet_type;
	short	packet_size;
   
/*	Allocate space for swp stucture					*/

	Swp_image = malloc( sizeof( swp_t ) );
	if( Swp_image == NULL )
	{
	  HCI_LE_error("Swp_image malloc failed for %d bytes", sizeof(swp_t));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Initialize the number of storms.				*/

	Swp_image->number_storms = -1;

/*	Process the Symbology Block.					*/

	last_index  = product_offset + layer1_size;
	packet_size = product_data [product_offset + 1];

	while (product_offset < last_index) {

/*	    Extract the packet type.					*/

	    packet_type = product_data [product_offset];

	    if (packet_type == 8) {

/*		Increment the number of storms.				*/

		Swp_image->number_storms++;
		if( Swp_image->number_storms <= ORPGPAT_MAX_PRODUCT_CODE )
		{
 		  HCI_LE_error("Swp_image->number_storms (%d) <= ORPGPAT_MAX_PRODUCT_CODE (%d)", Swp_image->number_storms, ORPGPAT_MAX_PRODUCT_CODE);
		  HCI_task_exit( HCI_EXIT_FAIL );
		}

/*		Set the value of the text string.			*/

		Swp_image->text_value [Swp_image->number_storms] = 
			product_data [product_offset + PACKET8_TEXT_VALUE];

/*		Set the x and y positions.				*/

		Swp_image->xpos [Swp_image->number_storms] = 
			product_data [product_offset + PACKET8_XPOS]/four_km;
		Swp_image->ypos [Swp_image->number_storms] = 
			product_data [product_offset + PACKET8_YPOS]/four_km;

/*		Set the SWP value.					*/

		Swp_image->equiv.swp_hw [Swp_image->number_storms][0] = 
			product_data [product_offset + PACKET8_TEXT_START];
         
		Swp_image->equiv.swp [Swp_image->number_storms][2] = '\0';

/*		Update product offset for next pass.			*/

		packet_size    = product_data [product_offset + 1];
		product_offset = product_offset + packet_size/2 + 2;

	    } else {

/*		This is a don't care packet type.			*/
 
		HCI_LE_error("decode_swp (): Unknown packet type %d",packet_type);

/*		Update the product offset for next pass.		*/

		packet_size    = product_data [product_offset + 1]/2;
		product_offset = product_offset + packet_size + 2;

	    }
	}
}

/************************************************************************
 *	Description: This function decodes data level data for DHR	*
 *		     type products.					*
 *									*
 *	Input:  level - halfword containing lower and upper level	*
 *	Output: decoded_level - array conaining ASCII level info	*
 *	Return: NONE							*
 ************************************************************************/

static void
decode_data_level (
short	level,
char	*decoded_level
)
{
	unsigned short	*ushort_ptr; 
	unsigned char	upperbyte;
	unsigned char	lowerbyte;  
static	float	scale_factor = 1.0;
	int	i;
	char	qualifier;
   
/*	Initialize string to blanks and null terminate			*/

	for (i=0;i<MAX_LEVEL_LENGTH;i++) {

	    decoded_level[i] = ' ';

	}

	decoded_level[6] = '\0';

/*	If level is less than zero, then the least significant byte 	*
 *	has special meaning						*/

	ushort_ptr = (unsigned short*)&level;
	upperbyte = (((*ushort_ptr) & 0xff00) >> 8);
	lowerbyte = ((*ushort_ptr) & 0x00ff);

	if (level < 0) {

	    switch (lowerbyte) {

		case 1:

		    decoded_level[4] = 'T';
		    decoded_level[5] = 'H';
		    break;

		case 2:

		    decoded_level[4] = 'N';
		    decoded_level[5] = 'D';
		    break;
   
		case 3:

		    decoded_level[4] = 'R';
		    decoded_level[5] = 'F';
		    break;

		default:

		    break;

	    }
   
/*	Most significant bit is not set, so most significant byte is	*
 *	numeric value							*/

	} else {

/*	    Add qualifier if required					*/ 
    
	    qualifier = ' ';

	    if (upperbyte & 0x08) {

		qualifier = '>';
      
	    } else if (upperbyte & 0x04) {

		qualifier = '<';

	    } else if (upperbyte & 0x02) {

		qualifier = '+';

	    } else if (upperbyte & 0x01) {

		qualifier = '-';  

	    }

/* 	    Determine if a scale factor bit is set			*/

	    if (upperbyte & 0x20) {

		scale_factor = 100.0;

	    } else if (upperbyte & 0x10) {

		scale_factor = 10.0;

	    } else {

		scale_factor = 1.0;

	    }

	    level_to_ASCII (scale_factor, lowerbyte, decoded_level); 
     
/*	    Add qualifier to data level, if required			*/

	    for (i=0;i<6;i++) {

		if (decoded_level[ i ] != ' ') {

		    decoded_level [i-1] = qualifier;
		    break;
		}
	    } 
	}
}

/************************************************************************
 *	Description: This function converts a binary data level to	*
 *		     an ASCII string.					*
 *									*
 *	Input:  scale_factor - scale_factor to apply to level		*
 *		level        - data level				*
 *	Output: string      - pointer to ASCII decoded level		*
 *	Return: NONE							*
 ************************************************************************/

void
level_to_ASCII (
float		scale_factor,
unsigned char	level,
char		*string
)
{
	unsigned char	digit = 0;
	short		i = 5;
	short		check_for_blank;
   
/*	Convert data level to ASCII text string				*/   

	check_for_blank = 0; 

	do {

/*	    Get next digit to be converted				*/

	    digit = (level % 10);

/*	    If check for blank flag is set, clear it			*/

	    if (check_for_blank) {
	    
		check_for_blank = 0;

	    }

/*	    Convert next digit to ASCII and update data level		*/

	    string [i--] = digit + 0x30;
	    level /= 10;

/*	    Add decimal point if number is scaled			*/

	    if ((scale_factor == 10.0) &&
	        (i == 4 )) {

		string[i] = '.';
		i--;
		check_for_blank = 1;

	    } else if ((scale_factor == 100.0) &&
		       (i >= 3)) {

		if (i == 3) {

		    string[i] = '.';
		    i--;

		}

		check_for_blank = 1;

	    }

/*	Continue until digit is converted				*/

	}
	
	while( level != 0 || check_for_blank );

}

/************************************************************************
 *	Description: This function extracts the hour, minute, and	*
 *		     second value from seconds past midnight.		*
 *									*
 *	Input:  timevalue - seconds past midnight			*
 *	Output: hours - hour						*
 *		minutes - minute					*
 *		second  - second					*
 *	Return: NONE							*
 ************************************************************************/

static void
convert_time (
int	timevalue,
int	*hours,
int	*minutes,
int	*seconds
)
{
/*	Extract the number of hours					*/

	*hours = timevalue/3600;

/*	Extract the number of minutes					*/

	timevalue = timevalue - (*hours)*3600;
	*minutes  = timevalue/60;

/*	Extract the number of seconds					*/

	*seconds = timevalue - *minutes*60;

}

/************************************************************************
 *	Description: This function frees internal memory allocated by	*
 *		     decoding a product.				*
 *		     NOTE: This is done automatically by the decode	*
 *		     product function.					*
 *									*
 *	Input:  product_code - products product code			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void
free_memory (
short	product_code
)
{
	int	i;
	int	j;

/*	Free memory for product annotations data			*/

	if (Attribute != NULL) {
     
	    if ((product_code != DHR_TYPE) &&
		(product_code != DBV_TYPE) &&
		(product_code != DVL_TYPE) &&
		(product_code != DR_TYPE ) &&
		(product_code != DV_TYPE ) &&
		(Product_type != GENERIC_HIRES_RADIAL_TYPE)) {

		for (i=0;i<Attribute->num_data_levels;i++) {
 
		    free( Attribute->data_levels[i] );

		}
	    }
/*
	    for (i=0;i<10;i++) {

		free( Attribute->text[i] );

	    }
*/
	    free( Attribute );
	    Attribute = NULL;

	}

/*	Free memory if product had graphic attributes			*/

	if (Gtab != NULL) {
     
	    for (i=0;i<= Gtab->number_of_lines;i++) {

		free( Gtab->text[i] );

	    }

	    free( Gtab );
	    Gtab = NULL;

	}

/*	Free memory if product had tabular alphanumeric			*/

	if (Ttab != NULL) {
     
	    for (i=0;i<Ttab->number_of_pages;i++) {

		for (j=0;j<=Ttab->number_of_lines [i];j++) {

/*		Free memory for text this page and line.		*/

		    free (Ttab->text [i][j]);

		}

/*		Free memory for line this page.				*/

		free (Ttab->text[i]);

	     }

	    free (Ttab->number_of_lines);

/*	    Free memory for text.					*/

	    free (Ttab->text);
	    free (Ttab);
	    Ttab = NULL;
      
	}

/*	Free memory for radial image data				*/

	if (Radial_image != NULL) {

	    free( Radial_image->azimuth_angle );
	    free( Radial_image->azimuth_width );

	    for (i=0;i<Radial_image->number_of_radials;i++) {
 
		free( Radial_image->radial_data[i] );

	    } 

	    free( Radial_image );
	    Radial_image = NULL;

	}

/*	Free memory for dhr image data					*/

	if (Dhr_image != NULL) {

	    free( Dhr_image->azimuth_angle );
	    free( Dhr_image->azimuth_width );

	    for (i=0;i<Dhr_image->number_of_radials;i++) {

		free( Dhr_image->radial_data[i] );

	    } 

	    free( Dhr_image );
	    Dhr_image = NULL;

	}

/*	Free memory for raster image					*/

	if (Raster_image != NULL) {

	    for (i=0;i<Raster_image->number_of_rows;i++) {

		free( Raster_image->raster_data[i] );

	    }

	    free( Raster_image );
	    Raster_image = NULL;

	}

/*	Free memory for vwp image					*/

	if (Vwp_image != NULL) {
   
	    free( Vwp_image );
	    Vwp_image = NULL;
   
	}

/*	Free memory for sti image					*/

	if (Sti_image != NULL) {
   
	    free( Sti_image );
	    Sti_image = NULL;
   
	}

/*	Free memory for hi image					*/

	if (Hi_image != NULL) {
   
	    free( Hi_image );
	    Hi_image = NULL;
   
	}

/*	Free memory for meso image					*/

	if (Meso_image != NULL) {
   
	    free( Meso_image );
	    Meso_image = NULL;
   
	}

/*	Free memory for tvs image					*/

	if (Tvs_image != NULL) {
   
	    free( Tvs_image );
	    Tvs_image = NULL;
   
	}

/*	Free memory for swp image					*/

	if (Swp_image != NULL) {
   
	    free( Swp_image );
	    Swp_image = NULL;
   
	}
}

/************************************************************************
 *	Description: This function decodes the graphics attributes	*
 *		     block.  Graphics attributes data are maintained	*
 *		     internally.					*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
decode_grafattr_block (
short	*product_data
)
{

	char	*packet_text;
	char	*old_string;
	char	*new_string;
	int	length_of_block;
   
/*	Initialize variables to NULL					*/

	int	number_of_pages = 0;   
	int	page_number = 0;
	int	page_length = 0;
	int	page_offset = 0;
	int	length_offset = 0;
	int	line_number;
	int	col_number;
	int	packet_code;
	int	packet_length;
	int	packet_code_offset;
	int	packet_length_offset;
	int	bytes_remaining;
	int	new_string_length;
	int	i;
	int	j;

/*	Verify this block is ID 2					*/

	if (product_data [BLOCK_ID_OFFSET] != 2) {
	
	    return;

	}

/*	If graphic alphanumeric block, allocate storage for text data	*/

	Gtab = malloc( sizeof( graphic_attr_t ) );
	if( Gtab == NULL )
	{
	  HCI_LE_error("Gtab malloc failed for %d bytes", sizeof(graphic_attr_t));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Initialize the number of lines of table to -1			*/

	Gtab->number_of_lines = -1;
	line_number = -1;

/*	Extract the number of pages and page length in bytes		*/

	number_of_pages = product_data [NUM_PAGES_OFFSET];

	length_of_block = (product_data [BLOCK_LENGTH_OFFSET] << 16) |
			  (product_data [BLOCK_LENGTH_OFFSET+1] & 0xffff);

/*	Initialize offsets						*/

	page_offset          = FIRST_PAGE_OFFSET;
	length_offset        = FIRST_PAGE_LENGTH_OFFSET;
	packet_code_offset   = FIRST_PACKET_OFFSET;
	packet_length_offset = FIRST_PACKET_LENGTH_OFFSET;

	for (i=0;i<number_of_pages;i++) {
 
/*	    Set the page number and page length in bytes		*/

	    page_number = product_data [page_offset];
	    page_length = product_data [length_offset];
     
/*	    Set the number of bytes remaining to the page length	*/

	    bytes_remaining = page_length;

	    while (bytes_remaining > 0) {
      
/*		Extract packets code and packet length.			*/

		packet_code   = product_data [packet_code_offset];
		packet_length = product_data [packet_length_offset];
         
/*		Currently only care about text packets (packet code 8)	*/

		if (packet_code == 8) {
    
		    decode_text_packet_8( &product_data[ packet_code_offset], 
                                  &packet_text,
                                  &line_number,
                                  &col_number );
            
/*		    Calculate the line number this page.		*/

		    line_number = (i*LINES_PER_PAGE) + line_number; 
     
		    if (Gtab->number_of_lines < line_number) {

/*			Increment the number of lines of text data	*/

			Gtab->number_of_lines = Gtab->number_of_lines + 1;
            
/*			Assign text pointer to structure		*/

			Gtab->text [Gtab->number_of_lines] = packet_text;

		    } else {
  
/*			Extract the pointer to the original text string.*/ 

			old_string = Gtab->text[ line_number ];

/*			Calculate the length of the new string.		*/

			new_string_length = strlen( old_string ) + 
                                   strlen( packet_text );

/*			Allocate storage for the new string.		*/

			new_string = malloc( new_string_length*sizeof(char)+1 );
			if( new_string == NULL )
			{
	  		  HCI_LE_error("new_string malloc failed for %d bytes", new_string_length*sizeof(char)+1);
			  HCI_task_exit( HCI_EXIT_FAIL );
			}

/*			Concatenate the strings, and NULL terminate the	*
 *			string.						 */

			new_string[0] = '\0';
			strcat( new_string,
				old_string );

			new_string[col_number] = '\0';
			strcat( new_string, 
				packet_text );

			for (j=new_string_length-1;j>0;j--) {

			    if ((new_string[j] >= '0') &&
				(new_string[j] <= 'z')) {

				new_string[ j+1 ] = '\0';
				break;

			    }
			}

/*			Assign new string to Gtab structure and free old*
 *			strings.					*/

			Gtab->text[ line_number ] = new_string;
               
			free( old_string );
			free( packet_text );

		    }
		}

/*		Update bytes remaining in this data block		*/

		bytes_remaining -= ( packet_length + PACKET_OVERHEAD_BYTES );

/*		Update word offsets to packet code and packet length	*/

		packet_code_offset = packet_code_offset + 
                              (packet_length+1)/2 + 
                              2;
		packet_length_offset = packet_code_offset + 
                                1;

	    }

/*	    Update word offsets for page number and page length		*/

	    page_offset = page_offset + 
			(page_length+1)/2 + 
			PAGE_HEADER_DATA_SIZE;
	    length_offset = page_offset + 
			1;

/*	Update word offsets for packet code and packet length		*/

	    packet_code_offset = length_offset + 
			1;
	    packet_length_offset = packet_code_offset + 
			1;

	}
}

/************************************************************************
 *	Description: This function decodes the tabular data		*
 *		     block.  Tabular data are maintained		*
 *		     internally.					*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void
decode_tabular_block (
short	*product_data
)
{
	int	block_offset;
	int	char_this_line;
	int	i;
	int	lines_this_page;
	short	*data;

	data = &product_data[Tabular_block];

/*	Verify this block is ID 3					*/

	if ((data [BLOCK_ID_OFFSET] != 3) &&
	    (Product_type != STAND_ALONE_TYPE)) {

	    return;

	}

/*	If tabular alphanumeric block, allocate storage for text data	*/

	Ttab = malloc( sizeof( tabular_attr_t ) );
	if( Ttab == NULL )
	{
 	  HCI_LE_error("Ttab malloc failed for %d bytes", sizeof(tabular_attr_t));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Initialize the number of pages.					*/

	if (Product_type != STAND_ALONE_TYPE) {

	    Ttab->number_of_pages = data [TABULAR_PAGE_OFFSET];

	} else{

	    Ttab->number_of_pages = data [SA_TABULAR_PAGE_OFFSET];

	}
 
/*	Allocate storage for lines per page.				*/

	Ttab->number_of_lines = 
		malloc( Ttab->number_of_pages * sizeof( int ) );
	if( Ttab->number_of_lines == NULL )
	{
 	  HCI_LE_error("Ttab->number_of_lines malloc failed for %d bytes", Ttab->number_of_pages*sizeof(int));
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	Allocate storage for text data.					*/

	Ttab->text = malloc( sizeof(char ***) * Ttab->number_of_pages );
	if( Ttab->text == NULL )
	{
 	  HCI_LE_error("Ttab->text malloc failed for %d bytes", sizeof(char ***) * Ttab->number_of_pages);
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	if( Product_type != STAND_ALONE_TYPE ) {

	    block_offset = TABULAR_PAGE_OFFSET + 1;

	} else {

	    block_offset = SA_TABULAR_PAGE_OFFSET + 1;

	}

	for (i=0;i<Ttab->number_of_pages;i++) {
  
/*	    Allocate storage for up to 17 lines per page.		*/

	    Ttab->text[ i ] = malloc( 17 * sizeof( char ** ) );
	    if( Ttab->text[i] == NULL )
	    {
	      HCI_LE_error("Ttab->text[%d] malloc failed for %d bytes", i, 17*sizeof(char **));
	      HCI_task_exit( HCI_EXIT_FAIL );
	    }

/*	    Initialize the number of lines this page to -1.		*/

	    Ttab->number_of_lines[ i ] = lines_this_page =  -1;

	    while (data [block_offset] != -1 ){
        
/*		Allocate storage for char_this_line.			*/ 

		lines_this_page++; 
		Ttab->text[ i ][ lines_this_page ] = 
		malloc( 81 * sizeof( char ) );
		if( Ttab->text[i][lines_this_page] == NULL )
		{
		  HCI_LE_error("Ttab->text[%d][%d] malloc failed for %d bytes", i, lines_this_page, 81*sizeof(char));
		  HCI_task_exit( HCI_EXIT_FAIL );
		}

/*		Extract the number of characters this line.		*/

		char_this_line = data [block_offset];

/*		Update block offset.					*/

		block_offset++;

/*		Copy line out of product data.				*/

		strncpy( Ttab->text[ i ][ lines_this_page ],
			(char *) &data[ block_offset ],
			char_this_line ); 

/*		NULL terminate the line.				*/

		Ttab->text[ i ][ lines_this_page ][ char_this_line ] = '\0';

/*		Update block offset, then go to next line.		*/

		block_offset += (char_this_line + 1)/2;

	    }

/*	    Update the number of lines this page.			*/

	    Ttab->number_of_lines[ i ] = lines_this_page;

/*	    Increment block offset to start at next page.		*/

	    block_offset++;

	}  
}

/************************************************************************
 *	Description: This function decodes packet 8 text.		*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: packet_text  - pointer to text data array		*
 *		line_number  - row text contained in			*
 *		col_number   - column text contained in			*
 *	Return: NONE							*
 ************************************************************************/

static void
decode_text_packet_8 (
short	*product_data, 
char	**packet_text,
int	*line_number,
int	*col_number
)
{
static	short	j_pix_start = 0;
static	short	i_pix_start = 0;
	short	j_pix;
	short	i_pix;
	short	string_length;
	unsigned short	halfword;
	int	i;
	int	j;
   
/*	Get the length of the text data only.				*/

	string_length = product_data [PACKET8_LENGTH] - 
                   PACKET8_HEADER_LENGTH;
   
/*	Allocate storage for text data.					*/ 

	*packet_text = malloc( string_length * sizeof( char )+1 );
	if( *packet_text == NULL )
	{
 	  HCI_LE_error("*packet_text malloc failed for %d bytes", string_length*sizeof(char)+1);
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

/*	If this is the first time called for this product, perform 	*
 *	some initialization.						*/

	if (*line_number == -1) {

	    *line_number = 0;

/*	    Extract the pixel start positions.				*/

	    j_pix_start = product_data [PACKET8_YPOS];
	    i_pix_start = product_data [PACKET8_XPOS];

	}

/*	Extract the pixel locations for row and column computation.	*/

	j_pix = product_data [PACKET8_YPOS];
	i_pix = product_data [PACKET8_XPOS];
   
/*	Calculate which row this belongs to.				*/

	*line_number = (int) ( (float) (j_pix - j_pix_start)/10.0 + 0.5 );
	*col_number = (int) ( (float) (i_pix - i_pix_start)/7.0 + 0.5 );

/*	Extract the text data.						*/

	for (i=0,j=0;i<(string_length+1)/2;i++,j+=2) {

	    halfword = product_data [PACKET8_TEXT_START + i];  
	    (*packet_text)[j]   = (halfword & 0xff00) >> 8;
	    (*packet_text)[j+1] = (halfword & 0x00ff);

	}

/*	Pad the text data with a NULL terminator.			*/

	(*packet_text)[ string_length ] = '\0';
 
}

/************************************************************************
 *	Description: This function decodes product dependent data.	*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void
product_dependent (
short	*product_data
)
{
	int	int_range;
	int	int_azimuth;
	int	day;
	int	month;
	int	year;
	int	hours;
	int	minutes;

/*	Set the azimuth/range center to 0/0				*/

	Attribute->center_azimuth = 0.0;
	Attribute->center_range = 0.0;

/*	Set the full screen flag					*/

	Attribute->full_screen = 1;

/*	NULL terminate the units array					*/

	Attribute->units[0] = '\0';
	Attribute->units[5] = '\0';

/*	Get product code						*/

	Product_code = product_data [MESSAGE_CODE_OFFSET];
	Attribute->product_code = Product_code;
  
	if ((Product_code >= 16) &&
	    (Product_code <= 21)) {

/*	    Process Base Reflectivity					*/

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX= %3d dBZ", product_data [46]);
  
/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "  dBZ" );

	} else if ((Product_code == DHR_TYPE) ||
		   (Product_code == DR_TYPE)) {

/*	    Assign units for color bar	*/

	    sprintf( Attribute->units, "  dBZ" );

	} else if ((Product_code == DBV_TYPE) ||
		   (Product_code == DV_TYPE)) {

/*	    Assign units for color bar	*/

	    sprintf( Attribute->units, "  m/s" );

	} else if (Product_code == DVL_TYPE) {

/*	    Assign units for color bar	*/

	    sprintf( Attribute->units, "kg/m2" );

	} else if ((Product_code >= 22) &&
		   (Product_code <= 27)) {

/*	    Process Base Velocity					*/

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX= %4d KT %3d KT", product_data [46],
		product_data [47]);
  
/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "  KTS" );

	} else if ((Product_code >= 28) &&
		   (Product_code <= 30)) {

/*	    Process Base Spectrum Width					*/

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX= %3d KT", product_data [46]);
  
/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "  KTS" );

	} else if ((Product_code >= 35) &&
		   (Product_code <= 38)) {

/*	    Process Composite Reflectivity and contour			*/

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX= %3d dBZ", product_data [46]);

/*	     Assign units for color bar					*/

	    sprintf( Attribute->units, "  dBZ" );

	} else if ((Product_code == 41) ||
		   (Product_code == 42)) {

/*	    Process echo tops and echo tops contour			*/

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ], 
		"MAX=  %2d KFT", product_data [46]);

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "  KFT" );

	} else if (Product_code == 43) {

/*	    Process severe weather reflectivity				*/ 

/*	    Clear the full screen flag					*/

	    Attribute->full_screen = 0;
      
	    Attribute->number_of_lines++;
	    Attribute->center_azimuth = (float) product_data [26]/10.0;
	    Attribute->center_range   = (float) product_data [27]/10.0;

	    int_azimuth = (int) (Attribute->center_azimuth + 0.5 );
	    int_range   = (int) (Attribute->center_range + 0.5 );

	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ], 
		"CNTR= %3d DEG  %3d NM", int_azimuth, int_range );

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ], 
		"MAX=  %2d dBZ", product_data [46]);

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_SUPP2_INDEX ], 
		"HEIGHT:  %2d KFT AGL", product_data [48]);

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "  dBZ" );

	} else if (Product_code == 44)  {

/*	    Process severe weather velocity				*/ 
 
/*	    Clear the full screen flag					*/

	    Attribute->full_screen = 0;

	    Attribute->number_of_lines++;
	    Attribute->center_azimuth = (float) product_data [26]/10.0;
	    Attribute->center_range   = (float) product_data [27]/10.0;

	    int_azimuth = (int) (Attribute->center_azimuth + 0.5 );
	    int_range   = (int) (Attribute->center_range + 0.5 );
	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ], 
		"CNTR= %3d DEG  %3d NM", int_azimuth, int_range );

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ], 
		"MAX=  %3d KTS", product_data [46]);

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_SUPP2_INDEX ], 
		"HEIGHT:  %2d KFT AGL", product_data [48]);

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "  KTS" );

	} else if (Product_code == 45) {

/*	    Process severe weather width				*/ 

/*	    Clear the full screen flag					*/

	    Attribute->full_screen = 0;

	    Attribute->number_of_lines++;
	    Attribute->center_azimuth = (float) product_data [26]/10.0;
	    Attribute->center_range   = (float) product_data [27]/10.0;

	    int_azimuth = (int) (Attribute->center_azimuth + 0.5 );
	    int_range   = (int) (Attribute->center_range + 0.5 );
	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ], 
		"CNTR= %3d DEG  %3d NM", int_azimuth, int_range );

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ], 
		"MAX=  %2d KTS", product_data [46]);

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_SUPP2_INDEX ], 
		"HEIGHT:  %2d KFT AGL", product_data [48]);

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "  KTS" );

	} else if (Product_code == 46) {

/*	    Process severe weather shear				*/ 

/*	    Clear the full screen flag					*/

	    Attribute->full_screen = 0;

	    Attribute->number_of_lines++;
	    Attribute->center_azimuth = (float) product_data [26]/10.0;
	    Attribute->center_range   = (float) product_data [27]/10.0;

	    int_azimuth = (int) (Attribute->center_azimuth + 0.5 );
	    int_range   = (int) (Attribute->center_range + 0.5 );
	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ], 
		"CNTR= %3d DEG  %3d NM", int_azimuth, int_range );

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_SUPP2_INDEX ], 
		" %3d E-3/S  %2dE-3/S", product_data [46],
		product_data [47]);

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_SUPP3_INDEX ], 
		"HEIGHT:  %2d KFT AGL", product_data [48]);

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "E-3/S" );

	} else if (Product_code == 48) {

/*	    Process vertical  wind profile				*/ 

/*	    Clear the full screen flag					*/

	    Attribute->full_screen = 0;

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ], 
		"MAX=%3d DEG  %3d KT", product_data [47], 
		product_data [46]);

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ], 
		"ALT: %5d FT", product_data [48]*10 );

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "RMS" );

	} else if (Product_code == 55) {

/*	    Process storm relative velocity region			*/ 

/*	    Clear the full screen flag					*/

	    Attribute->full_screen = 0;

	    Attribute->number_of_lines++;
	    Attribute->center_azimuth = (float) product_data [26]/10.0;
	    Attribute->center_range   = (float) product_data [27]/10.0;

	    int_azimuth = (int) (Attribute->center_azimuth + 0.5 );
	    int_range   = (int) (Attribute->center_range + 0.5 );
	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ],
		"CNTR= %3d DEG  %3d NM", int_azimuth, int_range );
 
	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX= %4d KT %3d KT", product_data [46],
		product_data [47]);

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_SUPP2_INDEX ],
		"SRM:%3dDEG %3d KT", product_data [51]/10,
		product_data [50]/10 );

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "  KTS" );

	} else if (Product_code == 56) {

/*	    Process storm relative velocity map				*/ 
 
	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX= %4d KT %3d KT", product_data [46],
		product_data [47]);

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ],
		"SRM:%3dDEG %3d KT", product_data [51]/10,
		product_data [50]/10 );

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "  KTS" );

	} else if (Product_code == 57) {

/*	    Process VIL							*/

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX=  %2d KG/M2", product_data [46]);

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "KG/M2" );

	} else if (Product_code == 58) {

/*	    Process STI							*/

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ],
		"%3d IDENTIFIED STMS", product_data [46]);

	} else if (Product_code == 59) {

/*	    Process HI							*/
   
/*	    Nothing to do!						*/

	} else if ((Product_code == 63) ||
		   (Product_code == 65)) {

/*	    Process Layer 1 Composite Reflectivity Max or Ave		*/

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX=  %2d dBZ", product_data [46]);
      
	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ],
		"ALT= SFC- %2d KFT", product_data [48]);

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "  KFT" );

	} else if ((Product_code == 64) ||
		   (Product_code == 66)) {

/*	    Process Layer 2 Composite Reflectivity Max or Ave		*/
    
	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX=  %2d dBZ", product_data [46]);
 
	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ],
		"ALT=  %2d- %2d KFT", product_data [47],
		product_data [48]);
   
/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "  KFT" );
  
	} else if ((Product_code == 89) ||
		   (Product_code == 90)) {

/*	    Process Layer 3 Composite Reflectivity Max or Ave		*/

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX=  %2d dBZ", product_data [46]);
  
	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ],
		"ALT=  %2d- %2d KFT", product_data [47],
		product_data [48]);
 
/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "  KFT" );

	} else if ((Product_code == 78) ||
		   (Product_code == 79)) {

/*	    Process storm total precipitation				*/

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX=  %.2f IN", product_data [46]/10.0 );
      
	    Attribute->number_of_lines++;
	    calendar_date (product_data [49], &day, &month, &year );

	    hours   = product_data [50]/60;
	    minutes = product_data [50] - hours*60;

	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ],
		"END=%2.2d/%2.2d/%2.2d %2.2d:%2.2d", month, day, year,
		hours, minutes );

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "   IN" );

	} else if (Product_code == 80) {

/*	    Process storm total precipitation				*/

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX=  %.2f IN", product_data [46]/10.0 );
      
	    Attribute->number_of_lines++;
	    calendar_date (product_data [47], &day, &month, &year );

 	    hours   = product_data [48]/60;
	    minutes = product_data [48] - hours*60;

	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ],
		"BEG=%2.2d/%2.2d/%2.2d %2.2d:%2.2d", month, day, year,
		hours, minutes );
      
	    Attribute->number_of_lines++;
	    calendar_date (product_data [49], &day, &month, &year );

	    hours   = product_data [50]/60;
	    minutes = product_data [50] - hours*60;

	    sprintf( Attribute->text[ PROD_DATA_SUPP2_INDEX ],
		"END=%2.2d/%2.2d/%2.2d %2.2d:%2.2d", month, day, year,
		hours, minutes );

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "   IN" );

	} else if (Product_code == 87) {

/*	    Process combined shear					*/ 

	    Attribute->number_of_lines++;
	    sprintf( Attribute->text[ PROD_DATA_MAX_INDEX ],
		"MAX= %3d E-4/S", product_data [46]);

	    Attribute->number_of_lines++;
	    Attribute->center_azimuth = (float) product_data [47]/10.0;
	    Attribute->center_range   = (float) product_data [48]/10.0;

	    int_azimuth = (int) (Attribute->center_azimuth + 0.5 );
	    int_range   = (int) (Attribute->center_range + 0.5 );

	    sprintf( Attribute->text[ PROD_DATA_SUPP1_INDEX ],
		"POS= %3d DEG %3dNM", int_azimuth, int_range );

/*	    Assign units for color bar					*/

	    sprintf( Attribute->units, "E-3/S" );

	} else {

	    sprintf( Attribute->units, " --- " );

	}
}

/************************************************************************
 *	Description: This function extracts degrees, minutes, and	*
 *		     seconds from a latitude/longitude.			*
 *									*
 *	Input:  lotlong - latitude or loongitude data.			*
 *	Output: deg - degrees						*
 *		min - minutes						*
 *		sec - seconds						*
 *	Return: NONE							*
 ************************************************************************/

static void
degrees_minutes_seconds (
float	latlong,
int	*deg,
int	*min,
int	*sec
)
{
/*	Extract the number of degrees					*/

	*deg = (int) latlong;

/*	Extract the number of minutes					*/

	latlong -= (float) *deg;

	if (latlong < 0.0) {
	
	    latlong = -latlong;

	} 

	*min = (int) ( latlong*60.0 );

/*	Extract the number of seconds					*/

	latlong -= ( (*min)/60.0 );
	*sec = (int) ( latlong*3600.0 );

}

/************************************************************************
 *	Description: This function extracts month, day, and year from	*
 *		     a julian date.					*
 *									*
 *	Input:  date - julian date.					*
 *	Output: ds - month						*
 *		dm - day						*
 *		dy - year (last 2 digits)				*
 *	Return: NONE							*
 ************************************************************************/

void
calendar_date (
short	date,
int	*dd,
int	*dm,
int	*dy
)
{

	int	l;
	int	n;
	int	julian;

/*	Convert modified julian to type integer				*/

	julian = date;

/*	Convert modified julian to year/month/day			*/

	julian += 2440587;
	l = julian + 68569;
	n = 4*l/146097;
	l = l -  (146097*n + 3)/4;
	*dy = 4000*(l+1)/1461001;
	l = l - 1461*(*dy)/4 + 31;
	*dm = 80*l/2447;
	*dd= l -2447*(*dm)/80;
	l = *dm/11;
	*dm = *dm+ 2 - 12*l;
	*dy = 100*(n - 49) + *dy + l;
	*dy = (*dy - 1900)%100;
 
	return;
}

/************************************************************************
 *	Description: This function returns the product type associated	*
 *		     with the active product.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: product type						*
 ************************************************************************/

int
hci_product_type ()
{
	return (int) Product_type;
}

/************************************************************************
 *	Description: This function returns the number of radials in	*
 *		     a radial product.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: number of radials					*
 ************************************************************************/

int
hci_product_radials ()
{
	if (Radial_image != (radial_rle_t *) NULL) {

	    return (int) Radial_image->number_of_radials;

	} else if (Dhr_image != (dhr_rle_t *) NULL) {

	    return (int) Dhr_image->number_of_radials;

	} else {

	    return -1;

	}
}

/************************************************************************
 *	Description: This function returns the number of data elements	*
 *		     in a radial product.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: number of data elements					*
 ************************************************************************/

int
hci_product_data_elements ()
{
	if (Radial_image != (radial_rle_t *) NULL) {

	    return (int) Radial_image->data_elements;

	} else if (Dhr_image != (dhr_rle_t *) NULL) {

	    return (int) Dhr_image->data_elements;

	} else {

	    return -1;

	}
}

/************************************************************************
 *	Description: This function returns the range interval (km)	*
 *		     of a radial product.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: range interval (km)					*
 ************************************************************************/

float
hci_product_range_interval ()
{
	if (Radial_image != (radial_rle_t *) NULL) {

	    return (float) Radial_image->range_interval;

	} else if (Dhr_image != (dhr_rle_t *) NULL) {

	    return (float) Dhr_image->range_interval;

	} else {

	    return -1.0;

	}
}

/************************************************************************
 *	Description: This function returns a pointer to the azimuth	*
 *		     data in a radial product.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to azimuth data					*
 ************************************************************************/

float
*hci_product_azimuth_data ()
{
	if (Radial_image != (radial_rle_t *) NULL) {

	    return (float *) Radial_image->azimuth_angle;

	} else if (Dhr_image != (dhr_rle_t *) NULL) {

	    return (float *) Dhr_image->azimuth_angle;

	} else {

	    return (float *) NULL;

	}
}

/************************************************************************
 *	Description: This function returns a pointer to the azimuth	*
 *		     data in a radial product.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to azimuth data					*
 ************************************************************************/

unsigned short
**hci_product_radial_data ()
{
	if (Radial_image != (radial_rle_t *) NULL) {

	    return (unsigned short **) Radial_image->radial_data;

	} else if (Dhr_image != (dhr_rle_t *) NULL) {

	    return (unsigned short **) Dhr_image->radial_data;

	} else {

	    return (unsigned short **) NULL;

	}
}

/************************************************************************
 *	Description: This function returns a pointer to the azimuth	*
 *		     width data in a radial product.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to azimuth width data				*
 ************************************************************************/

float
*hci_product_azimuth_width ()
{
	if (Radial_image != (radial_rle_t *) NULL) {

	    return (float *) Radial_image->azimuth_width;

	} else if (Dhr_image != (dhr_rle_t *) NULL) {

	    return (float *) Dhr_image->azimuth_width;

	} else {

	    return (float *) NULL;

	}
}

/************************************************************************
 *	Description: This function returns the product resolution	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: product resolution (km)					*
 ************************************************************************/

float
hci_product_resolution ()
{
	return (float) Xy_azran_reso [Product_code][0];
}

/************************************************************************
 *	Description: This function returns the product elevation index	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: elevation index						*
 ************************************************************************/

int
hci_product_elevation_index ()
{
	return (int) Elevation_index;
}

/************************************************************************
 *	Description: This function returns the product vcp	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: vcp						*
 ************************************************************************/

short
hci_product_vcp ()
{
	return (short) Product_vcp;
}

/************************************************************************
 *	Description: This function returns the product elevation	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: elevation (deg)						*
 ************************************************************************/

float
hci_product_elevation ()
{
	return (float) Attribute->elevation;
}

/************************************************************************
 *	Description: This function returns the product code		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: product code						*
 ************************************************************************/

int
hci_product_code ()
{
	return (int) Product_code;
}

/************************************************************************
 *	Description: This function returns the product date		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: product date (julian)					*
 ************************************************************************/

int
hci_product_date ()
{
	return (int) Product_date;
}

/************************************************************************
 *	Description: This function returns the product time		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: product time (milliseconds past midnight)		*
 ************************************************************************/

int
hci_product_time ()
{
	return (int) Product_time;
}

/************************************************************************
 *	Description: This function returns the number of data levels	*
 *		     in a product.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: number of data levels					*
 ************************************************************************/

int
hci_product_data_levels ()
{
	if (Product_code >= 16) {

	    return (int) Data_level_tab [Product_code];

	} else {

	    return -1;

	}
}

/************************************************************************
 *	Description: This function extracts the sement and channel	*
 *		     numbers in a CFC product.				*
 *									*
 *	Input:  product_data - pointer to product data			*
 *	Output: seg_num - pointer to segment number			*
 *		channel - pointer to channel number			*
 *	Return: NONE							*
 ************************************************************************/

void
hci_get_seg_channel (
short	*product_data,
int	*seg_num,
int	*channel
)
{
	short	param1;

	param1 = product_data [PARAM_1];

/*	Extract the segment number for this CFC product		*/

	if ( ((short) 0x02 & param1) ) {

	    *seg_num = 1;	/* Elevation segment 1 (LOW).	*/

	} else if ( ((short) 0x04 & param1) ) {

	    *seg_num = 2;	/* Elevation segment 2 (HIGH).	*/

	} else {

	    *seg_num = -1;	/* Invalid elevation segment.	*/

	}

/*	Extract the channel number.  NOTE:  Channel 1 is surveillance	*
 *	channel.  Channel 2 is Doppler channel.				*/

	if ( ((short) 0x01 & param1) ) {

	    *channel = 2;	/* Doppler channel	*/

	} else {

	    *channel = 1;	/* Surveillance channel	*/

	}

	return;

}

/************************************************************************
 *	Description: This function returns the number of levels	in the	*
 *		     product attributes table.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: number of levels					*
 ************************************************************************/

int
hci_product_attribute_num_data_levels (
)
{
	if (Attribute != (product_pertinent_t *) NULL) {

	    return Attribute->num_data_levels;

	} else {

	    return -1;

	}
}

/************************************************************************
 *	Description: This function returns the number of lines	in the	*
 *		     product attributes table.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: number of lines						*
 ************************************************************************/

int
hci_product_attribute_number_of_lines (
)
{
	if (Attribute != (product_pertinent_t *) NULL) {

	    return Attribute->number_of_lines;

	} else {

	    return -1;

	}
}

/************************************************************************
 *	Description: This function returns a pointer to the product	*
 *		     attributes level data.				*
 *									*
 *	Input:  level - attributes table level				*
 *	Output: NONE							*
 *	Return: pointer to attribute data level string			*
 ************************************************************************/

char
*hci_product_attribute_data_level (
int	level
)
{
	if (Attribute != (product_pertinent_t *) NULL) {

	    return (char *) Attribute->data_levels [level];

	} else {

	    return (char *) NULL;

	}
}

/************************************************************************
 *	Description: This function returns a pointer to the product	*
 *		     attributes text.					*
 *									*
 *	Input:  level - attributes table index				*
 *	Output: NONE							*
 *	Return: pointer to attribute string				*
 ************************************************************************/

char
*hci_product_attribute_text (
int	indx
)
{
	if (Attribute != (product_pertinent_t *) NULL) {

	    return (char *) Attribute->text [indx];

	} else {

	    return (char *) NULL;

	}
}

/************************************************************************
 *	Description: This function returns a pointer to the product	*
 *		     attributes units.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to attribute units string			*
 ************************************************************************/

char
*hci_product_attribute_units (
)
{
	if (Attribute != (product_pertinent_t *) NULL) {

	    return (char *) Attribute->units;

	} else {

	    return (char *) NULL;

	}
}

/************************************************************************
 *	Description: This function returns the number of pages in the	*
 *		     tabular block.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: number of pages						*
 ************************************************************************/

int
hci_product_tabular_pages (
)
{
	if (Ttab != (tabular_attr_t *) NULL) {

	    return Ttab->number_of_pages;

	} else {

	    return -1;

	}
}

/************************************************************************
 *	Description: This function returns the number of lines in the	*
 *		     tabular block.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: number of lines						*
 ************************************************************************/

int
hci_product_tabular_lines (
int	page
)
{
	if (Ttab != (tabular_attr_t *) NULL) {

	    return Ttab->number_of_lines [page];

	} else {

	    return -1;

	}
}

/************************************************************************
 *	Description: This function returns a pointer to a line in the	*
 *		     tabular block.					*
 *									*
 *	Input:  page - page in tabular block				*
 *		line - line in tabular block				*
 *	Output: NONE							*
 *	Return: pointer to tabular block line				*
 ************************************************************************/

char
*hci_product_tabular_line (
int	page,
int	line
)
{
	if (Ttab != (tabular_attr_t *) NULL) {

	    return (char *) Ttab->text[page][line];;

	} else {

	    return (char *) NULL;

	}
}

/************************************************************************
 *	Description: This function returns param_1 in Graphic product	*
 ************************************************************************/

short hci_product_param_1()
{
  return Graphic_product_struct.param_1;
}

/************************************************************************
 *	Description: This function returns param_2 in Graphic product	*
 ************************************************************************/

short hci_product_param_2()
{
  return Graphic_product_struct.param_2;
}
/************************************************************************
 *	Description: This function returns param_3 in Graphic product	*
 ************************************************************************/

short hci_product_param_3()
{
  return Graphic_product_struct.param_3;
}

/************************************************************************
 *	Description: This function returns param_4 in Graphic product	*
 ************************************************************************/

short hci_product_param_4()
{
  return Graphic_product_struct.param_4;
}

/************************************************************************
 *	Description: This function returns param_5 in Graphic product	*
 ************************************************************************/

short hci_product_param_5()
{
  return Graphic_product_struct.param_5;
}

/************************************************************************
 *	Description: This function returns param_6 in Graphic product	*
 ************************************************************************/

short hci_product_param_6()
{
  return Graphic_product_struct.param_6;
}

/************************************************************************
 *	Description: This function returns param_7 in Graphic product	*
 ************************************************************************/

short hci_product_param_7()
{
  return Graphic_product_struct.param_7;
}

/************************************************************************
 *	Description: This function returns param_8 in Graphic product	*
 ************************************************************************/

short hci_product_param_8()
{
  return Graphic_product_struct.param_8;
}

/************************************************************************
 *	Description: This function returns param_9 in Graphic product	*
 ************************************************************************/

short hci_product_param_9()
{
  return Graphic_product_struct.param_9;
}

/************************************************************************
 *	Description: This function returns param_10 in Graphic product	*
 ************************************************************************/

short hci_product_param_10()
{
  return Graphic_product_struct.param_10;
}

/************************************************************************
 *      Description: This function reads a specified product from the   *
 *                   RPG products database.  Memory is automatically    *
 *                   allocated and product information is available     *
 *                   via various hci_product functions.                 *
 ************************************************************************/

int
hci_load_product_data (
int     prod_code,
int     vol_time,
short   *params
)
{
        char    *lb_data;
        int     status;
        static  int     product_type;
        static  short   *product_data = NULL;
        int     ret;

        status = 0;

/*      Try to fetch the specified product from the ORPG Products       *
 *      database.  If the product was found, then decode it and return  *
 *      a 1.  Otherwise, return a 0.                                    */

        ret = ORPGDBM_read (prod_code,
                      &lb_data,
                      vol_time,
                      params);

        Product_io_status = status;
        if (ret <= 0) {

            status = 0;

        } else {

            product_data = (short *) lb_data;
            product_data = product_data+(sizeof (Prod_header)/2);

            if (local_endian_type () == ORPG_LITTLE_ENDIAN) {

                UMC_icd_to_product (lb_data + sizeof (Prod_header),
                        ret - sizeof (Prod_header));

            }

            product_type = hci_decode_product (product_data);

            status = 1;
            free (lb_data);

        }

        return status;
}

/************************************************************************
 *      Description: This function returns the status of the last read. *
 ************************************************************************/

int
hci_load_product_io_status (
)
{
        return (Product_io_status);
}

/************************************************************************
 *      Description: This function returns the endian type of the       *
 *                   local system.                                      *
 ************************************************************************/

static int
local_endian_type (
)
{
        int     num;
        char    *buf;

        buf = (char *) &num;
        num = 1;

        if ((int) buf [0] == 0) {

            return ORPG_BIG_ENDIAN;

        } else {

            return ORPG_LITTLE_ENDIAN;

        }
}

