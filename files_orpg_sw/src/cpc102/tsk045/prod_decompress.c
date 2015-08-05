/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/07 16:50:56 $
 * $Id: prod_decompress.c,v 1.6 2008/01/07 16:50:56 aamirn Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/*******************************************************************************
* File Name:	
*	prod_decompress.c
*
* Description:
*       This tool takes a product file, determines if it is compressed, and 
*       decompresses it if necessary.  It outputs a new file with a ".uncomp"
*       extension.
*
* Usage:
*
* Examples:
*
* History:	
*	07-14-2005, R. Solomon.  Created.
*******************************************************************************/

#include "prod_decompress.h"

static char  Prod_file[128] = "";
static char  Prod_table[128] = "";
static void* Prod_data;     /* Buffer to hold product data read from file */
static int   Prod_table_flag = 0;
static short Prod_code = 0;        /* Product code of prod to be decompressed */
static int   Prod_size = 0;        /* Product size */


int main( int argc, char** argv)
{
   int retval;
   char* cfg_dir = (char *)malloc(FILENAME_MAX);
   char* prod_table_filename = "product_attr_table";

   retval = Read_options(argc, argv);
   if (retval < 0)
   {
      Print_usage(argv);
   }
  
   /* We need the product attribute table in order to do decompression on
      compressed products.  If no product_tables file is given on the command
      line, we try to use the one located in RPG_LEVEL/cfg.  If that's not
      available, we will not attempt to decompress any products. */
  
   if ( Prod_table_flag == 0 )
   {
     /* attempt to use default prod table file */
     cfg_dir = getenv("RPG_LEVEL");
     if ( cfg_dir == NULL )
     {
        cfg_dir = getenv("HOME");
     }
     if ( cfg_dir != NULL )
     {
        strcat(cfg_dir, "/cfg/");
        strcpy(Prod_table, cfg_dir);
        strcat(Prod_table, prod_table_filename);
     }
   }

   if (Init_attr_tbl (Prod_table, 1) <= 0)
   {
      fprintf(stderr, "Failure initializing product attrib table %s.\n", Prod_table);
   }
   else
   {
      /* print product table being used */
      fprintf(stdout, "Using product attribute table: %s\n", Prod_table);
   }

  retval = ReadFileInfo();
  if (retval < 0)
  {
    fprintf(stderr, "Problem reading file\n");
    return (-1);
  }

  retval = DecompressProd();
  if (retval < 0)
  {
    fprintf(stderr, "Error decompressing product\n");
    return (-1);
  }

  retval = WriteDecompProd();
  if (retval < 0)
  {
    fprintf(stderr, "Error writing product\n");
    return (-1);
  }

  return (0);

} /* end main() */


/******************************************************************************
 * Description:  This function reads the command line options for
 *               prod_decompress.
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
  
  while ((c = getopt(argc, argv, "ht:?")) != EOF)
  {
    switch(c)
    {
      case 't': /* read in the product table file path and name */
        if (sscanf (optarg, "%s", Prod_table) != 1)
        {
           err = -1;
        }
        else
        {
           Prod_table_flag = 1;
        }
        break;

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
 * Description:  This function reads in the product to be decompressed.
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
   Prod_size = fstat_buf.st_size;

   Prod_data = (void *)malloc(Prod_size);
   if (Prod_data == 0)
   {
      fprintf(stderr, "memory allocation error for Prod_data\n");
   }

   len_read = read(fd_prod_file, (char *)Prod_data, Prod_size);
   if (len_read < 0)
   {
      fprintf(stderr, "read error reading in the product file %s\n", Prod_file);
      exit (0);
   }

   /* Determine product code */
   Prod_code = ((Graphic_product *)Prod_data)->prod_code;

   /* If Little Endian machine, byte swap */
   #ifdef LITTLE_ENDIAN_MACHINE
      MISC_swap_shorts (1, &Prod_code);
   #endif

   close(fd_prod_file);
   return (0);

} /* end ReadFileInfo */


/******************************************************************************
 * Description:  This function checks to see if the product is compressed.
 *               If so, it decompresses the product and stores the result in
 *               Prod_data.
 *       Input:  None
 *      Output:  Prod_data is populated with the decompressed product.
 *      Return:  A boolean indicating success (0) or failure (-1).
 *****************************************************************************/
static int DecompressProd()
{
   int stat = 0;  /* function call return status */
   char ans[1];
   short *uncompress = NULL; /* pointer to uncompressed product data */

   /* check compression type for product */
   stat=ORPGPAT_get_compression_type(ORPGPAT_get_prod_id_from_code(Prod_code));
   if (stat == 0) 
   {
      fprintf(stderr,
         "According to the prod_attr_table, this prod is not compressed.\n");
      fprintf(stderr, "Do you want to continue anyway? (y/n)\n");
      scanf("%s", ans);
      if ( strcmp(ans, "y") != 0)
         return(-1);
   }
   else if (stat < 0) 
   {
      fprintf(stderr,
         "DecompressProd: Error (%d) determining compression type.\n",
         stat);
      return(-1);
   }

   /* Ask the user if this msg has a product description block */
   fprintf(stderr, "Does this message have a prod description block? (y/n)\n");
   scanf("%s", ans);

   if ( strcmp(ans, "y") == 0 )
   {
      uncompress = (short*) Decompress_product_w_PDB ( Prod_data, &Prod_size );
   }
   else if ( strcmp(ans, "n") == 0 )
   {
      uncompress = (short*) Decompress_product_wo_PDB ( Prod_data, &Prod_size );
   }
   else
   {
      fprintf(stderr, "DecompressProd: invalid entry.\n");
      return(-1);
   }
        
   /* Reallocate more space for prod data buffer */
   free (Prod_data);
   Prod_data = (void *)malloc(Prod_size);
        
   /* Copy uncompressed data to the prod data buffer */
   memcpy((void*) Prod_data, (void *) uncompress, (size_t) Prod_size);
           
   free (uncompress);
   
   return (0);

} /* end DecompressProd() */


/******************************************************************************
 * Description:  This function writes the decompressed file out (binary).
 *       Input:  None
 *      Output:  None
 *      Return:  A boolean indicating success (0) or failure (-1).
 *****************************************************************************/
static int WriteDecompProd()
{
   int fd_prod_file = 0;
   int len_write = 0;
   char* output_file;

   output_file = (char *)calloc(100, sizeof(char));
   strcpy(output_file, Prod_file);
   strcat(output_file, ".uncomp");

   /* Create and open output file */
   fd_prod_file = creat(output_file, 0777);
   if (fd_prod_file < 0)
   {
      fprintf(stderr, "WriteDecompProd: open failed for %s, errno is %d\n",
         Prod_file, errno);
   }

   /* Write decompressed prod data to file */
   len_write = write(fd_prod_file, (char *)Prod_data, Prod_size);
   if (len_write < 0)
   {
      fprintf(stderr, "write error writing the product file %s\n", Prod_file);
      exit (0);
   }

   /* Free memory */
   free (output_file);

   return (0);

} /* end WriteDecompProd() */

/**************************************************************************
   Description:
      This function initializes the product attribute table.

   Input:
      clear_table - flag, if set, causes existing table to be cleared.

   Output:
      NONE

   Return:
      It returns the number of entries of the table on success
      or -1 on failure.

**************************************************************************/
int Init_attr_tbl( char *file_name, int clear_table ){

    int p_cnt = 0;

    LE_send_msg (GL_INFO, "Initializing product attributes table\n");

/*  If the attributes table already exists, clear it and get ready      *
 *  to create new data.                                                 */

    if( clear_table )
       ORPGPAT_clear_tbl ();

/*  Create a new PAT.                                                   */
    p_cnt = ORPGPAT_read_ASCII_PAT( file_name );

    if( p_cnt < 0 )
        return (-1);

    else{

       return (p_cnt);

    }

/* End of Init_attr_tbl() */
}


/******************************************************************************
 * Description: This function just writes out the usage of prod_decompress tool 
 *       Input: argv 
 *      Output: Usage of the prod_decompress tool 
 *      Return: None 
 *       Notes: 
 *****************************************************************************/
static void Print_usage (char **argv) 
{
    printf ("Usage: %s (options) <file> \n", argv[0]);
    printf ("       options:\n");
    printf ("       -h (print this message)\n");
    printf ("       -t <Product table path/filename>\n");
    exit (0);
} 
