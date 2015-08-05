/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/07 16:53:05 $
 * $Id: prod_deserialize.c,v 1.2 2008/01/07 16:53:05 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*******************************************************************************
* File Name:	
*	prod_deserialize.c
*
* Description:
*       This tool deserializes a serialized product and prints the data to a
*       text file. It outputs a new file with a ".deser" file name extension.
*
* Usage:
*
* Examples:
*
* History:	
*	01-30-2006, R. Solomon.  Created.
*******************************************************************************/

#include "prod_deserialize.h"

static char  Prod_file[128]   = "";
static void* Prod_data;               /* Bfr to hold prod data read from file */
static int   Msg_code         = 0;    /* Msg code of prod to be deserialized */
static int   File_size        = 0;    /* Product size */
static int   Prod_size        = 0;    /* Product size */
static char* Serial_data      = NULL; /* pointer to serialized product data */
static void* Deserial_data    = NULL; /* pointer to deserialized product data */
static char* Add_space        = "";   /* for product print out */
static int   Prod_desc_flag   = 0;    /* Flag to indicate a prod desc blk exists */


int main( int argc, char** argv)
{
   int retval;
   char ans[1];

   retval = Read_options(argc, argv);
   if (retval < 0)
   {
      Print_usage(argv);
   }
  
   /* Prompt the user for info about the prod description block */
   fprintf(stderr, "Does this product have a Product Description Block? (y/n)\n");
   scanf("%s", ans);
   if (strcmp(ans, "y") == 0)
   {
      Prod_desc_flag = 1;
   }
   else if ( strcmp(ans, "n") == 0)
   {
      Prod_desc_flag = 0;
   }
   else
   {
      fprintf(stderr, "Invalid entry.\n");
      return -1;
   }

   retval = ReadFileInfo();
   if (retval < 0)
   {
      fprintf(stderr, "Problem reading file\n");
      return (-1);
   }

   retval = DeserializeProd();
   if (retval < 0)
   {
      fprintf(stderr, "Error deserializing product\n");
      return (-1);
   }

   if ( Deserial_data != NULL )
   {
      Print_RPGP_product(Deserial_data);
     
      retval = RPGP_product_free( Deserial_data );
      if (retval < 0)
      {
         fprintf(stderr, "Error freeing data\n");
         return (-1);
      }
   }
   else
   {
      fprintf(stderr, "Deserial_data is NULL\n");
      return (-1);
   }

   return (0);

} /* end main() */


/******************************************************************************
 * Description:  This function reads the command line options for
 *               prod_deserialize.
 *       Input:  argc and argv 
 *      Output:  none
 *      Return:  a boolean indicating success or failure.
 *       Notes: 
 *****************************************************************************/
static int Read_options(int argc, char**argv)
{
  extern char      *optarg; /* used by getopt */
  extern int       optind; /* total number of arguments */
  int              c; /* to read the program options */
  int              err=0; /* for error condition */
  
  while ((c = getopt(argc, argv, "h?")) != EOF)
  {
    switch(c)
    {
      case 'h':

      case '?':
        err = -1;
        break;
    }
  }
  if (optind != argc-1)
    err = -1;
  else
  {
    strncpy(Prod_file, argv[argc-1], 100);
  }

  return (err);

} /* end Read_options() */


/******************************************************************************
 * Description:  This function reads in the product to be deserialized.
 *       Input:  None
 *      Output:  
 *      Return:  A boolean indicating success (0) or failure (-1).
 *****************************************************************************/
static int ReadFileInfo()
{
   int fd_prod_file;
   int len_read;
   int fstat_ret = 0;
   struct stat fstat_buf;

 
   /* Open the file and read product data  */
   fd_prod_file = open(Prod_file, O_RDONLY, 0);
   if (fd_prod_file < 0)
   {
      fprintf(stderr, "open failed for %s, errno is %d\n", Prod_file, errno);
   }

   /* Retrieve and store prod size in bytes */
   fstat_ret = fstat( fd_prod_file, &fstat_buf );
   File_size = fstat_buf.st_size;

   Prod_data = (void *)malloc(File_size);
   if (Prod_data == 0)
   {
      fprintf(stderr, "memory allocation error for Prod_data\n");
   }

   len_read = read(fd_prod_file, (char *)Prod_data, File_size);
   if (len_read < 0)
   {
      fprintf(stderr, "read error reading in the product file %s\n", Prod_file);
      exit (0);
   }

   /* Determine message code from the product message header */
   Msg_code = SHORT_BSWAP_L( *(short *)(Prod_data + 0) );

   close(fd_prod_file);
   return (0);

} /* end ReadFileInfo */


/******************************************************************************
 * Description:  This function deserializes the serialized product.
 *       Input:  none
 *      Output:  Deserial_data ptr is populated with the deserialized product.
 *      Return:  A boolean indicating success (0) or failure (-1).
 *****************************************************************************/
static int DeserializeProd()
{
   int             ret;
   short*          short_ptr; 
   unsigned short  size_msw, size_lsw;
   short           comp;

   short_ptr = (short *) Prod_data;

   /* increment pointer past message header */
   short_ptr += SIZE_OF_PD_MSG_HEADER/sizeof(short);

   /* Check to make sure data is not compressed.  We should see the -1 block 
      divider here.  If not, it must be compressed. */
   if ( (comp = SHORT_BSWAP_L( *(short_ptr) )) != -1 )
   {
      fprintf(stderr,
         "DeserializeProd: product is compressed (%d)!  Decompress first.\n",
         comp );
      return(-1);
   }

   /* Reset pointer */
   short_ptr = (short *) Prod_data;

   /* we must handle both prods with and without a prod desc block */
   if ( Prod_desc_flag == 1 )
   {
      /* set pointer to DIV2OFF, the blk div after the prod desc blk */
      short_ptr += DIV2OFF;

      /* Check to make sure the data is either packet 28 or 29 */
      if ( ( SHORT_BSWAP_L( short_ptr[OPCODOFF] ) != 28 ) 
                         && 
           ( SHORT_BSWAP_L( short_ptr[OPCODOFF] ) != 29 ) )
      {
         fprintf(stderr,
            "DeserializeProd: Packet code not 28 or 29!\n");
         return(-1);
      }

      /* Increment pointer to length of serialized data and store prod size */
      size_msw = SHORT_BSWAP_L( short_ptr[ODLENOFF] );
      size_lsw = SHORT_BSWAP_L( short_ptr[ODLENOFF + 1] );
      Prod_size = (size_msw << 16) | size_lsw;

      /* Set the serial data pointer */
      Serial_data = (char *)&short_ptr[OSDATOFF];
   }
   else
   {
      /* increment pointer past message header */
      short_ptr += SIZE_OF_PD_MSG_HEADER/sizeof(short);

      /* Check to make sure the data is either packet 28 or 29 */
      if ( ( SHORT_BSWAP_L( short_ptr[IPCODOFF] ) != 28 ) 
                            && 
           ( SHORT_BSWAP_L( short_ptr[IPCODOFF] ) != 29 ) )
      {
         fprintf(stderr,
            "DeserializeProd: Packet code not 28 or 29, or the product is compressed!\n");
         return(-1);
      }

      /* Increment pointer to length of serialized data and store prod size */
      size_msw = SHORT_BSWAP_L( short_ptr[IDLENOFF] );
      size_lsw = SHORT_BSWAP_L( short_ptr[IDLENOFF + 1] );
      Prod_size = (size_msw << 16) | size_lsw;

      /* Set the serial data pointer */
      Serial_data = (char *)&short_ptr[ISDATOFF];
   }

   ret = RPGP_product_deserialize (Serial_data, Prod_size,
      (void **)&Deserial_data);

   return (0);

} /* end DecompressProd() */


/******************************************************************************
 * Description:  This function writes the deserialized file out (binary).
 *       Input:  None
 *      Output:  None
 *      Return:  A boolean indicating success (0) or failure (-1).
 *****************************************************************************/
static int WriteDeserializedProd()
{
   int fd_prod_file = 0;
   int len_write = 0;
   char* output_file;

   output_file = (char *)calloc(100, sizeof(char));
   strcpy(output_file, Prod_file);
   strcat(output_file, ".deser");

   /* Create and open output file */
   fd_prod_file = creat(output_file, 0777);
   if (fd_prod_file < 0)
   {
      fprintf(stderr, "WriteDeserProd: open failed for %s, errno is %d\n",
         Prod_file, errno);
   }

   /* Write deserialized prod data to file */
   len_write = write(fd_prod_file, (char *)Prod_data, Prod_size);
   if (len_write < 0)
   {
      fprintf(stderr, "write error writing the product file %s\n", Prod_file);
      exit (0);
   }

   /* Free memory */
   free (output_file);

   return (0);

} /* end WriteDeserProd() */


/******************************************************************************
 * Description: This function just writes out the usage of prod_deserialize tool 
 *       Input: argv 
 *      Output: Usage of the prod_deserialize tool 
 *      Return: None 
 *       Notes: 
 *****************************************************************************/
static void Print_usage (char **argv) 
{
    printf ("Usage: %s (options) <file> \n", argv[0]);
    printf ("       options:\n");
    printf ("       -h (print this message)\n");
    exit (0);
} 


/******************************************************************
 * Description: Prints the RPGP product struct
 *       Input: prodp - deserialized product data pointer 
 *      Output: Prints RPGP_product_t structure data to screen
 *      Return: None
 *       Notes: Based off of Z. Jing's function located in the 
 *              xdr_test.c file in cpc101/lib003.
******************************************************************/
static void Print_RPGP_product (void *prodp)
{
   RPGP_product_t *prod;

   prod = (RPGP_product_t *)prodp;
   printf ("\n");
   printf ("product id %d, name: %s, desc: %s.\n",
      prod->product_id, prod->name, prod->description);
   printf ("    type %d, time %d\n", prod->type, prod->gen_time);
   if (prod->type == RPGP_EXTERNAL)
   {
      RPGP_ext_data_t *eprod = (RPGP_ext_data_t *)prodp;
      printf ("    compress type %d, size %d\n",
         eprod->compress_type, eprod->size_decompressed);
      Print_params (eprod->numof_prod_params, eprod->prod_params);
      Print_components (eprod->numof_components, (char **)eprod->components);
   }
   else
   {
      printf ("    Radar name: %s, lat %8.4f, lon %8.4f, height %8.4f\n",
         prod->radar_name, prod->radar_lat, prod->radar_lon, prod->radar_height);
      printf ("    Vol time %d, elev time %d, vol number %d, elev number %d\n",
         prod->volume_time, prod->elevation_time, prod->volume_number,
         prod->elevation_number);
      printf ("    VCP %d, Op mode %d, elev %8.4f\n", prod->vcp,
         prod->operation_mode, prod->elevation_angle);
      printf ("    compress type %d, size %d\n",
         prod->compress_type, prod->size_decompressed);
      Print_params (prod->numof_prod_params, prod->prod_params);
      Print_components (prod->numof_components, (char **)prod->components);
   }
   printf ("\n");
}


/******************************************************************
    (Copied from xdr_test.c written by Z. Jing).
    Prints components array in "comps" of size "n_comps" for 
    verification.

******************************************************************/

static void Print_components (int n_comps, char **comps) {
    RPGP_area_t **gencomps;
    int i;

    printf ("%s    number of components %d\n", Add_space, n_comps);
    gencomps = (RPGP_area_t **)comps;
    for (i = 0; i < n_comps; i++) {

	switch (gencomps[i]->comp_type) {

	    case RPGP_AREA_COMP:
		Print_area ((RPGP_area_t *)comps[i]);
		break;

	    case RPGP_TEXT_COMP:
		Print_text ((RPGP_text_t *)comps[i]);
		break;

	    case RPGP_EVENT_COMP:
		Print_event ((RPGP_event_t *)comps[i]);
		break;

	    case RPGP_GRID_COMP:
		Print_grid ((RPGP_grid_t *)comps[i]);
		break;

	    case RPGP_TABLE_COMP:
		Print_table ((RPGP_table_t *)comps[i]);
		break;

	    default:
		break;
	}
    }
}

/******************************************************************

    (Copied from xdr_test.c written by Z. Jing).
    Prints area "area" for verification.

******************************************************************/

static void Print_area (RPGP_area_t *area) {

    printf ("%s    AREA: (%d), area type %d, location type %x\n", 
			Add_space, area->comp_type, 
			RPGP_AREA_TYPE (area->area_type),
			RPGP_LOCATION_TYPE (area->area_type));
    Print_params (area->numof_comp_params, area->comp_params);
    switch (RPGP_LOCATION_TYPE (area->area_type)) {
	case RPGP_LATLON_LOCATION:
	    Print_points (area->numof_points, area->points);
	    break;
	case RPGP_XY_LOCATION:
	    Print_xy_points (area->numof_points, area->points);
	    break;
	case RPGP_AZRAN_LOCATION:
	    Print_azran_points (area->numof_points, area->points);
	    break;
	default:
	    printf ("Unexpected location type (%d)\n", 
			RPGP_LOCATION_TYPE (area->area_type));
    }
}

/******************************************************************

    (Copied from xdr_test.c written by Z. Jing).
    Prints text "text" for verification.

******************************************************************/

static void Print_text (RPGP_text_t *text) {

    printf ("%s    TEXT: (%d)\n", Add_space, text->comp_type);
    Print_params (text->numof_comp_params, text->comp_params);
    printf ("%s    text: %s\n", Add_space, text->text);
}

/******************************************************************

    (Copied from xdr_test.c written by Z. Jing).
    Prints grid "grid" for verification.

******************************************************************/

static void Print_grid (RPGP_grid_t *grid) {
    int i, total_data;
    char type[128];

    printf ("%s    GRID: (%d), gtype %d, ndim %d (", 
	Add_space, grid->comp_type, grid->grid_type, grid->n_dimensions);
    total_data = 1;
    for (i = 0; i < grid->n_dimensions; i++) {
	printf (" %d", grid->dimensions[i]);
	total_data *= grid->dimensions[i];
    }
    printf (")\n");
    Print_params (grid->numof_comp_params, grid->comp_params);
    if (grid->n_dimensions <= 0) {
	printf ("Print_grid: n_dimensions is %d\n", grid->n_dimensions);
	return;
    }
    if (total_data <= 0) {
	printf ("Print_grid: No data\n");
	return;
    }
    printf ("data attributes: %s\n", grid->data.attrs);
    if (Get_data_type (grid->data.attrs, type, 128) > 0) {
	if (strcmp (type, "short") == 0) {
	    short *spt;
	    printf ("Data:");
	    spt = (short *)grid->data.data;
	    for (i = 0; i < total_data; i++)
		printf (" %d", spt[i]);
	    printf ("\n");
	}
	else if (strcmp (type, "ubyte") == 0) {
	    unsigned char *cpt;
	    printf ("Data:");
	    cpt = (unsigned char *)grid->data.data;
	    for (i = 0; i < total_data; i++)
		printf (" %d", cpt[i]);
	    printf ("\n");
	}
	else if (strcmp (type, "float") == 0) {
	    float *fpt;
	    printf ("Data:");
	    fpt = (float *)grid->data.data;
	    for (i = 0; i < total_data; i++)
		printf (" %f", fpt[i]);
	    printf ("\n");
	}
	else
	    printf ("Print_grid: type (%s) not implemented\n", type);
    }
    else
	printf ("Print_grid: type not found\n");
}

/******************************************************************

    (Copied from xdr_test.c written by Z. Jing).
    Prints grid "grid" for verification.

******************************************************************/

static void Print_table (RPGP_table_t *table) {
    int i, j;

    printf ("%s    TABLE: (%d), n_columns %d, n_rows %d\n", 
	Add_space, table->comp_type, table->n_columns, table->n_rows);

    Print_params (table->numof_comp_params, table->comp_params);

    printf ("    title: %s\n", table->title.text);
    printf ("    column lables:");
    for (i = 0; i < table->n_columns; i++)
	printf (" %s", table->column_labels[i].text);
    printf ("\n");
    printf ("    row lables:");
    for (i = 0; i < table->n_rows; i++)
	printf (" %s", table->row_labels[i].text);
    printf ("\n");
    printf ("    table:\n");
    for (i = 0; i < table->n_rows; i++) {
	printf ("    ");
	for (j = 0; j < table->n_columns; j++)
	    printf (" %s", table->entries[i * table->n_columns + j].text);
	printf ("\n");
    }
}

/******************************************************************

    (Copied from xdr_test.c written by Z. Jing).
    Prints event "event" for verification.

******************************************************************/

static void Print_event (RPGP_event_t *event) {

    printf ("    EVENT: (%d)\n", event->comp_type);
    Add_space = "    ";
    Print_params (event->numof_event_params, event->event_params);
    Print_components (event->numof_components, (char **)event->components);
    Add_space = "";
}

/******************************************************************

    (Copied from xdr_test.c written by Z. Jing).
    Prints "n_params" parameters pointed to by "params" for 
    verification.

******************************************************************/

static void Print_params (int n_params, RPGP_parameter_t *params) {
    int i;

    printf ("%s        # %d params:\n", Add_space, n_params);
    for (i = 0; i < n_params; i++) {
	printf ("%s            param[%d]: id: %s, attrs: %s\n", 
		Add_space, i, params[i].id, params[i].attrs);
    }
}

/******************************************************************

    (Copied from xdr_test.c written by Z. Jing).
    Prints array "points" of type RPGP_location_t of size "n_points" 
    for verification.

******************************************************************/

static void Print_points (int n_points, RPGP_location_t *points) {
    int i;

    printf ("%s        $ %d points:\n", Add_space, n_points);
    for (i = 0; i < n_points; i++) {
	printf ("%s            point[%d]: lat = %8.2f, lon = %8.2f\n", 
		Add_space, i, points[i].lat, points[i].lon);
    }
}

/******************************************************************

    (Copied from xdr_test.c written by Z. Jing).
    Prints array "points" of type RPGP_xy_location_t of size "n_points" 
    for verification.

******************************************************************/

static void Print_xy_points (int n_points, RPGP_xy_location_t *points) {
    int i;

    printf ("%s        $ %d points:\n", Add_space, n_points);
    for (i = 0; i < n_points; i++) {
	printf ("%s            point[%d]: x = %8.2f, y = %8.2f\n", 
		Add_space, i, points[i].x, points[i].y);
    }
}

/******************************************************************

    (Copied from xdr_test.c written by Z. Jing).
    Prints array "points" of type RPGP_azran_location_t of size "n_points" 
    for verification.

******************************************************************/

static void Print_azran_points (int n_points, RPGP_azran_location_t *points) {
    int i;

    printf ("%s        $ %d points:\n", Add_space, n_points);
    for (i = 0; i < n_points; i++) {
	printf ("%s            point[%d]: range = %8.2f, azi = %8.2f\n", 
		Add_space, i, points[i].range, points[i].azi);
    }
}


/******************************************************************
    (Copied from xdr_test.c written by Z. Jing).
    Searches for the type value in attribute string "attrs". If the
    type value is found, it is returned in "buf" of size "buf_size"
    and the function returns the length of the type value. Returns
    -1 if the type is not found. The type value must be non-empty
    and single token.
******************************************************************/
static int Get_data_type (char *attrs, char *buf, int buf_size) {
    char t1[16], t2[16], t3[16], t4[16], *p;
    int new_attr;

    if (attrs == NULL)
	return (-1);
    p = attrs;
    new_attr = 1;
    while (1) {
	p = Get_token (p, t1, 16);
	if (t1[0] == '\0')
	    break;
	if (strcmp (t1, ";") == 0) {
	    new_attr = 1;
	    continue;
	}
	if (!new_attr)
	    continue;
	if (strcmp (t1, "type") == 0) {
	    char *pp = Get_token (p, t2, 16);
	    if (strcmp (t2, "=") == 0) {
		int len;
		pp = Get_token (pp, t3, 16);
		pp = Get_token (pp, t4, 16);
		if (t3[0] != '\0' &&
		    (t4[0] == '\0' || strcmp (t4, ";") == 0)) {
		    len = strlen (t3);
		    if (len >= buf_size)
			len = buf_size - 1;
		    strncpy (buf, t3, len);
		    buf[len] = '\0';
		    return (len);
		}
	    }
	}
	new_attr = 0;
    }
    buf[0] = '\0';
    return (-1);
}

/******************************************************************

    (Copied from xdr_test.c written by Z. Jing).
    Finds the first token of "text" in "buf" of size "buf_size". The
    returned token is always null-terminated and possibly truncated.
    Returns the pointer after the token. A token is a word separated
    by space, tab or line return. "=" and ";" is considered as a
    token even if they are not separated by space. If "text" is an
    empty string, an empty string is returned in "buf" and the return
    value is "text".
	
******************************************************************/

static char *Get_token (char *text, char *buf, int buf_size) {
    char *p, *st, *next;
    int len;

    p = text;
    while (*p == ' ' || *p == '\t' || *p == '\n')
	p++;
    st = p;
    if (*p == '=' || *p == ';')
	len = 1;
    else if (*p == '\0')
	len = 0;
    else {
	while (*p != '\0' && *p != ' ' && *p != '\t' && 
				*p != '\n' && *p != '='  && *p != ';')
	    p++;
	len = p - st;
    }
    next = st + len;
    if (len >= buf_size)
	len = buf_size - 1;
    strncpy (buf, st, len);
    buf[len] = '\0';
    return (next);
}
