/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/09/21 16:53:02 $
 * $Id: hci_fonts.c,v 1.16 2009/09/21 16:53:02 ccalvert Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_fonts.c						*
 *									*
 *	Description: This module contains functions for controlling	*
 *		     font properties in HCI applications.		*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_font.h>

/*	Local definitions	*/

#define	MAX_NUM_SIZES	20

/*	Static variables.						*/

static	int	Scaled = MEDIUM; /* Scaled font size */

static	XmFontList  Fontlist  [SCALED+1]; /* Fontlist storage */
static	Font	    Xfont     [SCALED+1]; /* Font storage */
static	XFontStruct *Fontinfo [SCALED+1]; /*Font Info storage */
static	char	    new_fontname [ 128 ]; /*name of adjusted font */
static	int	    courier_point_sizes[ MAX_NUM_SIZES ]; /* pt sizes avail */
static	int	    num_courier_sizes = 0; /* num unique sizes available */
static	int	    helvetica_point_sizes[ MAX_NUM_SIZES ]; /* pts avail */
static	int	    num_helvetica_sizes = 0; /* num unique sizes available */

static	char		List_fontname   [] = "-*-courier-bold-r-*-*-*-100-100-100-*-*-iso8859-1";
static	char		Small_fontname  [] = "-*-helvetica-bold-r-*-*-*-80-100-100-*-*-iso8859-1";
static	char		Medium_fontname [] = "-*-helvetica-bold-r-*-*-*-100-100-100-*-*-iso8859-1";
static	char		Large_fontname  [] = "-*-helvetica-bold-r-*-*-*-120-100-100-*-*-iso8859-1";
static	char		Extra_large_fontname  [] = "-*-helvetica-bold-r-*-*-*-140-100-100-*-*-iso8859-1";

static Display *display;

/*	Function prototypes.						*/

static void get_adjusted_fontname( int font, float adj_factor );
static void get_point_size_info();
static int  sort_point_sizes( const void *, const void * );


/************************************************************************
 *	Description: This function initializes all of the fonts used	*
 *		     in HCI applications.  All of the font information	*
 *		     is defined and stored for easy access.		*
 *									*
 *	Input:  display - Pointer to Display information		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_initialize_fonts (
Display		*passed_in_display
)
{
    static int initialized = 0;
    XmFontListEntry	list_entry;
    char	point [8];
    char	size  [8];

    if (initialized)
	return;
    initialized = 1;

    display = passed_in_display;

    get_point_size_info();

    /* Read the list font properties from HCI gui data message. */

    sprintf (point,"%3.3d", HCI_DEFAULT_FONT_POINT);
    sprintf (size,"%3.3d", HCI_DEFAULT_FONT_SIZE);

    List_fontname [24] = point [0];
    List_fontname [25] = point [1];
    List_fontname [26] = point [2];
    List_fontname [28] = size [0];
    List_fontname [29] = size [1];
    List_fontname [30] = size [2];
    List_fontname [32] = size [0];
    List_fontname [33] = size [1];
    List_fontname [34] = size [2];

/*	List (proportionate) font	*/

	list_entry    = XmFontListEntryLoad (display,
				List_fontname,
				XmFONT_IS_FONT,
				"LIST");
	Fontinfo [LIST] = XLoadQueryFont (display, List_fontname);
	Fontlist [LIST] = XmFontListAppendEntry (NULL, list_entry);
	Xfont    [LIST] = XLoadFont (display, List_fontname);
	XmFontListEntryFree (&list_entry);

/*	Small font	*/

	list_entry    = XmFontListEntryLoad (display,
				Small_fontname,
				XmFONT_IS_FONT,
				"SMALL");
	Fontinfo [SMALL] = XLoadQueryFont (display, Small_fontname);
	Fontlist [SMALL] = XmFontListAppendEntry (NULL, list_entry);
	Xfont    [SMALL] = XLoadFont (display, Small_fontname);
	XmFontListEntryFree (&list_entry);

/*	Medium font	*/

	list_entry    = XmFontListEntryLoad (display,
				Medium_fontname,
				XmFONT_IS_FONT,
				"MEDIUM");
	Fontinfo [MEDIUM] = XLoadQueryFont (display, Medium_fontname);
	Fontlist [MEDIUM] = XmFontListAppendEntry (NULL, list_entry);
	Xfont    [MEDIUM] = XLoadFont (display, Medium_fontname);
	XmFontListEntryFree (&list_entry);

/*	Large font	*/

	list_entry    = XmFontListEntryLoad (display,
				Large_fontname,
				XmFONT_IS_FONT,
				"LARGE");
	Fontinfo [LARGE] = XLoadQueryFont (display, Large_fontname);
	Fontlist [LARGE] = XmFontListAppendEntry (NULL, list_entry);
	Xfont    [LARGE] = XLoadFont (display, Large_fontname);
	XmFontListEntryFree (&list_entry);

/*	Extra Large font	*/

	list_entry    = XmFontListEntryLoad (display,
				Extra_large_fontname,
				XmFONT_IS_FONT,
				"EXTRA_LARGE");
	Fontinfo [EXTRA_LARGE] = XLoadQueryFont (display, Extra_large_fontname);
	Fontlist [EXTRA_LARGE] = XmFontListAppendEntry (NULL, list_entry);
	Xfont    [EXTRA_LARGE] = XLoadFont (display, Extra_large_fontname);
	XmFontListEntryFree (&list_entry);
}

/************************************************************************
 *	Description: This function returns a pointer to the font info	*
 *		     data for the specified font index.			*
 *									*
 *	Input:  font - font index (LIST, SMALL, MEDIUM, LARGE,...)	*
 *	Output: NONE							*
 *	Return: pointer to font information				*
 ************************************************************************/

XFontStruct
*hci_get_fontinfo (
int	font
)
{
	switch (font) {

	    case LIST :

		return	(XFontStruct *) Fontinfo [LIST];
		break;

	    case SMALL :

		return	(XFontStruct *) Fontinfo [SMALL];
		break;

	    case MEDIUM :

		return	(XFontStruct *) Fontinfo [MEDIUM];
		break;

	    case LARGE :

		return	(XFontStruct *) Fontinfo [LARGE];
		break;

	    case EXTRA_LARGE :

		return	(XFontStruct *) Fontinfo [EXTRA_LARGE];
		break;

	    case SCALED :

		return	(XFontStruct *) Fontinfo [Scaled];
		break;

	    default:

		return	(XFontStruct *) Fontinfo [LIST];
		break;

	}
}

/************************************************************************
 *	Description: This function returns a pointer to the font info	*
 *		     data for the specified font index and adjustment	*
 *		     factor. Function hci_free_fontinfo_adj MUST BE	*
 *		     CALLED afterward, or a memory leak will occur.	*
 *									*
 *	Input:  font - font index (LIST, SMALL, MEDIUM, LARGE,...)	*
 *		adj_factor - ratio to adjust font			*
 *	Output: NONE							*
 *	Return: pointer to font information				*
 ************************************************************************/

XFontStruct
*hci_get_fontinfo_adj (
int	font,
float	adj_factor
)
{
	/* User should check to make sure return value isn't NULL. */

	get_adjusted_fontname( font, adj_factor );
        return (XFontStruct *) XLoadQueryFont (display, new_fontname);
}

/************************************************************************
 *	Description: This function frees the pointer returned from the	*
 *		     hci_get_fontinfo_adj function.			*
 *									*
 *	Input:  font_struct - pointer to XFontStruct struct		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_free_fontinfo_adj (
XFontStruct	*font_struct
)
{
	XFreeFont (display, font_struct);
}

/************************************************************************
 *	Description: This function returns the XmFontList information	*
 *		     for the specified font index.  This information is	*
 *		     used to set Motif widget font properties.		*
 *									*
 *	Input:  font - font index (LIST, SMALL, MEDIUM, LARGE,...)	*
 *	Output: NONE							*
 *	Return: font list information					*
 ************************************************************************/

XmFontList
hci_get_fontlist (
int	font
)
{

	switch (font) {

	    case LIST :

		return	Fontlist [LIST];
		break;

	    case SMALL :

		return	Fontlist [SMALL];
		break;

	    case MEDIUM :

		return	Fontlist [MEDIUM];
		break;

	    case LARGE :

		return	Fontlist [LARGE];
		break;

	    case EXTRA_LARGE :

		return	Fontlist [EXTRA_LARGE];
		break;

	    case SCALED :

		return	Fontlist [Scaled];
		break;

	    default:

		return	Fontlist [LIST];
		break;

	}
}

/************************************************************************
 *	Description: This function returns the Font information for	*
 *		     the specified font index.  This information is	*
 *		     used to set X Window font properties.		*
 *									*
 *	Input:  font - font index (LIST, SMALL, MEDIUM, LARGE,...)	*
 *	Output: NONE							*
 *	Return: font information					*
 ************************************************************************/

Font
hci_get_font (
int	font
)
{

	switch (font) {

	    case LIST :

		return	Xfont [LIST];
		break;

	    case SMALL :

		return	Xfont [SMALL];
		break;

	    case MEDIUM :

		return	Xfont [MEDIUM];
		break;

	    case LARGE :

		return	Xfont [LARGE];
		break;

	    case EXTRA_LARGE :

		return	Xfont [EXTRA_LARGE];
		break;

	    case SCALED :

		return	Xfont [Scaled];
		break;

	    default:

		return	Xfont [LIST];
		break;

	}
}

/************************************************************************
 *	Description: This function returns the Font information for	*
 *		     the specified font index and adjustment factor.	*
 *		     This information is used to set X Window font	*
 *		     properties.					*
 *									*
 *	Input:  font - font index (LIST, SMALL, MEDIUM, LARGE,...)	*
 *		adj_factor - ratio to adjust font			*
 *	Output: NONE							*
 *	Return: font information					*
 ************************************************************************/

Font
hci_get_font_adj (
int	font,
float	adj_factor
)
{
	get_adjusted_fontname( font, adj_factor );
	return XLoadFont (display, new_fontname);
}

/************************************************************************
 *	Description: This function sets the scaled font to the		*
 *		     specified font index.				*
 *									*
 *	Input:  font - font index (SMALL, MEDIUM, LARGE, etc.)		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_set_font (
int	font,
int	size
)
{
	if (size <= 13) {

	    Scaled = SMALL;

	} else if (size < 16) {

	    Scaled = MEDIUM;

	} else if (size < 18) {

	    Scaled = LARGE;

	} else {

	    Scaled = EXTRA_LARGE;

        }
}

/************************************************************************
 *	Description: This function creates a new fontname by adjusting	*
 *		     the point size in the current fontname.		*
 *									*
 *	Input:	font     - font index (LIST, SMALL, MEDIUM, LARGE, ...)	*
 *		adj_factor - ratio to adjust font			*
 *	Output: NONE							*
 *	Return: name of new font					*
 ************************************************************************/

static void
get_adjusted_fontname (
int	font,
float	adj_factor )
{
  char temp_fontname [128];
  char new_fontname_head [60];
  char *new_fontname_tail = "-100-100-*-*-iso8859-1";
  char *str_tok;
  char old_point_size_token[4];
  char new_point_size_token[4];
  int old_point_size;
  int new_point_size;
  int temp;
  int indx;
  char *srch_c = "-";
  char *srch_str;
  int *available_sizes;
  int num_available_sizes;
  int size_is_available;

  /* It is assumed that fonts are of the standard XLFD naming	*/
  /* convention. This convention  dictates that each font name	*/
  /* is made up of 14 tokens. An example is:			*/
  /* -adobe-helvetica-bold-r-normal--17-120-100-100-p-92-iso8859-1 */
  /* Each token is described below (for the above example):	*/
  /*  1: Foundry	adobe					*/
  /*  2: Family		helvetica				*/
  /*  3: Weight		bold					*/
  /*  4: Slant		r					*/
  /*  5: Set Width	normal					*/
  /*  6: Add Style	missing					*/
  /*  7: Pixels		17					*/
  /*  8: Points		120					*/
  /*  9: Horiz		100					*/
  /* 10: Vert		100					*/
  /* 11: Spacing	p					*/
  /* 12: Avg Width	92					*/
  /* 13: Rgstry		iso8859					*/
  /* 14: Encoding	1					*/
 
  /* Find name of font currently being used. */
 
  switch (font) {

    case LIST :

	strcpy( temp_fontname, List_fontname );
	break;

    case SMALL :

	strcpy( temp_fontname, Small_fontname );
	break;

    case MEDIUM :

	strcpy( temp_fontname, Medium_fontname );
	break;

    case LARGE :

	strcpy( temp_fontname, Large_fontname );
	break;

    case EXTRA_LARGE :

	strcpy( temp_fontname, Extra_large_fontname );
	break;

    case SCALED :

	if( Scaled == SMALL )
	{
	  strcpy( temp_fontname, Small_fontname );
	}
	else if( Scaled == MEDIUM )
	{
	  strcpy( temp_fontname, Medium_fontname );
	}
	else if( Scaled == LARGE )
	{
	  strcpy( temp_fontname, Large_fontname );
	}
	else
	{
	  strcpy( temp_fontname, Extra_large_fontname );
	}
	break;

    default:

	strcpy( temp_fontname, List_fontname );
	break;

  }

  /* Make sure new font is same family as old font. */

  srch_str = strdup( temp_fontname );
  str_tok = strtok(srch_str,"*-");
  if( strcmp( str_tok, "courier" ) == 0 )
  {
    sprintf(new_fontname_head,"-*-courier-bold-r-*-*-*-");
    available_sizes = courier_point_sizes; 
    num_available_sizes = num_courier_sizes; 
  }
  else
  {
    sprintf(new_fontname_head,"-*-helvetica-bold-r-*-*-*-");
    available_sizes = helvetica_point_sizes; 
    num_available_sizes = num_helvetica_sizes; 
  }
  free( srch_str );

  /* Position of point size is always the eighth token,	*/
  /* however, if "Add Style" token is missing, then the	*/
  /* string tokenizing function in C will return the	*/
  /* wrong token. If "Add Style" is missing, adjust	*/
  /* index of token to search for.			*/

  if( strstr( temp_fontname, "--" ) != NULL )
  {
    indx = 6;
  }
  else
  {
    indx = 7;
  }

  srch_str = strdup( temp_fontname );
  str_tok = strtok(srch_str, srch_c );
  for( temp = 0; temp < indx; temp++ )
  {
    str_tok = strtok( NULL, srch_c );
  }

  /* Convert current point size string to an int. Then	*/
  /* adjust by the adjustment factor passed in.		*/

  sprintf( old_point_size_token, "%s", str_tok);
  free( srch_str );
  old_point_size = atoi( old_point_size_token );
  new_point_size = old_point_size * adj_factor;

  /* Make sure the new point size is available on the	*/
  /* font server. If not, round to the closest		*/
  /* available option.					*/

  size_is_available = HCI_NO_FLAG;
  for( temp = 0; temp < num_available_sizes; temp++ )
  {
    if( new_point_size == available_sizes[ temp ] )
    {
      size_is_available = HCI_YES_FLAG;
      break;
    }
  }

  if( size_is_available == HCI_NO_FLAG )
  {
    if( new_point_size < available_sizes[ 0 ] )
    {
      new_point_size = available_sizes[ 0 ];
    }
    else if( new_point_size > available_sizes[ num_available_sizes - 1 ] )
    {
      new_point_size = available_sizes[ num_available_sizes - 1 ];
    }
    else
    {
      temp = 0;
      while( new_point_size > available_sizes[ temp ] )
      {
        temp++;
      }

      if( ( available_sizes[ temp ] - new_point_size ) <
          ( new_point_size - available_sizes[ temp - 1 ] ) )
      {
        new_point_size = available_sizes[ temp ];
      }
      else
      {
        new_point_size = available_sizes[ temp - 1 ];
      }
    }
  }

  /* Convert new point size to string. */

  sprintf( new_point_size_token, "%d",new_point_size);

  /* With new valid point size, assemble new fontname. */

  sprintf(new_fontname,"%s%s%s",new_fontname_head,new_point_size_token,new_fontname_tail);
}

/************************************************************************
 *	Description: This function queries the font server and finds	*
 *		     the number (and values) of point sizes available	*
 *		     for fonts used on the hci.				*
 *									*
 *	Input:	NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void
get_point_size_info ()
{
  char **flist;		/* List of fonts returned from font server. */
  int max_num = 2000;	/* Maximum number of fonts for font server to return. */
  int ret_num;		/* Actual number of fonts font server returned. */
  int temp1, temp2;	/* Looping variables. */
  int temp3;		/* Looping variables. */
  int pt_size;		/* Point size of font. */
  char *srch_str;	/* String be tokenized. */
  char *srch_c = "-";	/* Char to use as tokenizer. */
  char *str_tok;	/* String token returned from strtok(). */
  char buf[ 4 ];	/* Temporary variable. */
  int already_present;	/* Is point size already accounted for? */
  int indx;		/* token index. */

  /* Get list of fonts on this font server that	*/
  /* are similar to the passed in value.	*/

  flist = XListFonts( display, "*helvetica-bold-r*" , max_num, &ret_num );

  num_helvetica_sizes = 0;

  for( temp1 = 0; temp1 < ret_num; temp1++ )
  {
    /* Position of point size is always the eighth token,	*/
    /* however, if "Add Style" token is missing, then the	*/
    /* string tokenizing function in C will return the	*/
    /* wrong token. If "Add Style" is missing, adjust	*/
    /* index of token to search for.			*/

    if( strstr( flist[ temp1 ], "--" ) != NULL )
    {
      indx = 6;
    }
    else
    {
      indx = 7;
    }

    srch_str = strdup( flist[ temp1 ] );
    str_tok = strtok( srch_str, srch_c );
    for( temp2 = 0; temp2 < indx; temp2++ )
    {
      str_tok = strtok( NULL, srch_c );
    }

    /* Convert point size to int. */

    sprintf( buf, "%s", str_tok );
    free( srch_str );
    pt_size = atoi( buf );

    /* Count number of unique point sizes. Keep	*/
    /* unique sizes in an array.		*/

    if( num_helvetica_sizes == 0 )
    {
      helvetica_point_sizes[ num_helvetica_sizes ] = pt_size;
      num_helvetica_sizes++;
    }
    else
    {
      already_present = HCI_NO_FLAG;
      for( temp3 = 0; temp3 < num_helvetica_sizes; temp3++ )
      {
        if( pt_size == helvetica_point_sizes[ temp3 ] )
        {
          already_present = HCI_YES_FLAG;
          break;
        }
      }
      if( already_present == HCI_NO_FLAG )
      {
        helvetica_point_sizes[ num_helvetica_sizes ] = pt_size;
        num_helvetica_sizes++;
      }
    }
  }

  /* Sort array in ascending order. */

  qsort( ( void * ) helvetica_point_sizes,
         num_helvetica_sizes,
         sizeof( int ),
         sort_point_sizes );

  /* Free list of fonts or there's a memory leak. */

  XFreeFontNames(flist);


  /* Get list of fonts on this font server that	*/
  /* are similar to the passed in value.	*/

  flist = XListFonts( display, "*courier-bold-r*" , max_num, &ret_num );

  num_courier_sizes = 0;

  for( temp1 = 0; temp1 < ret_num; temp1++ )
  {
    /* Position of point size is always the eighth token,	*/
    /* however, if "Add Style" token is missing, then the	*/
    /* string tokenizing function in C will return the	*/
    /* wrong token. If "Add Style" is missing, adjust	*/
    /* index of token to search for.			*/

    if( strstr( flist[ temp1 ], "--" ) != NULL )
    {
      indx = 6;
    }
    else
    {
      indx = 7;
    }

    srch_str = strdup( flist[ temp1 ] );
    str_tok = strtok( srch_str, srch_c );
    for( temp2 = 0; temp2 < indx; temp2++ )
    {
      str_tok = strtok( NULL, srch_c );
    }

    /* Convert point size to int. */

    sprintf( buf, "%s", str_tok );
    free( srch_str );
    pt_size = atoi( buf );

    /* Count number of unique point sizes. Keep	*/
    /* unique sizes in an array.		*/

    if( num_courier_sizes == 0 )
    {
      courier_point_sizes[ num_courier_sizes ] = pt_size;
      num_courier_sizes++;
    }
    else
    {
      already_present = HCI_NO_FLAG;
      for( temp3 = 0; temp3 < num_courier_sizes; temp3++ )
      {
        if( pt_size == courier_point_sizes[ temp3 ] )
        {
          already_present = HCI_YES_FLAG;
          break;
        }
      }
      if( already_present == HCI_NO_FLAG )
      {
        courier_point_sizes[ num_courier_sizes ] = pt_size;
        num_courier_sizes++;
      }
    }
  }

  /* Sort array in ascending order. */

  qsort( ( void * ) courier_point_sizes,
         num_courier_sizes,
         sizeof( int ),
         sort_point_sizes );

  /* Free list of fonts or there's a memory leak. */

  XFreeFontNames(flist);
}

/************************************************************************
 *      Description: This function is the comparison function passed    *
 *                   to the qsort algorithm. Sorting is in ascending    *
 *                   order.                                             *
 *                                                                      *
 *      Input:  pt1 - input value #1                                    *
 *              pt2 - input value #2                                    *
 *      Output: NONE                                                    *
 *      Return:  1 if pt1 > pt2                                         *
 *               0 if pt1 = pt2                                         *
 *              -1 if pt1 < pt2                                         *
 ************************************************************************/

static int
sort_point_sizes ( const void *p1, const void *p2 )
{
  const int *pt1 = p1;
  const int *pt2 = p2;
  int diff = *pt1 - *pt2;

  if( diff < 0 ){ return -1; }
  else if( diff == 0 ){ return 0; }
  else{ return 1; }

  return -1; /* Satisfy compiler warning. This will never happen. */
}

