/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/09/05 20:57:12 $
 * $Id: prod_cmpr.cpp,v 1.29 2014/09/05 20:57:12 steves Exp $
 * $Revision: 1.29 $
 * $State: Exp $
 */  

/* System include files */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Local include files */
#include <list.h> /* for link list class */
#include <prod_cmpr.h> /* definition of graphic product */

extern "C" 
{
#include <mnttsk_pat_def.h>
}

#define        DIR_NAME_SIZE 100 /* max num of chars for a dir name */
#define        FILE_NAME_SIZE 150 /* max num of chars for a dir name */
#define        WMO_AWIPS_HDR       (sizeof(WMO_AWIPS_hdr_t))
#define        MSG_HDR_SZ          (sizeof(Prod_msg_header_icd))

#define REDTEXT	"\x1B[1m\x1B[38;5;9m"
#define GRNTEXT	"\x1B[1m\x1B[38;5;28m"
#define BLUTEXT	"\x1B[1m\x1B[38;5;21m"
#define PURTEXT "\x1B[1m\x1B[38;5;53m"
#define BLKTEXT	"\x1B[1m\x1B[38;5;16m"
#define RESET	"\033[0m"


static int     Mismatch_limit = 100; /* default to 100, program will quit aftr 100
                                      mismatches*/
static int     Prod_code_to_cmpr = -1; /* -1 means compare all products, a value between 
                                          16 and 1999 means only compare prducts with 
                                          that code */
static int     Identical_products = 0;
static int     Number_of_mismatches = 0;
static int     Print_no_match_flag = 0;
static int     Verbose = 1; /* default to 1, 0 and 2 are other possible values */
static int     Sort_key = 1; /* default to 1, 2 and 3 are other possible values */
static char    Dir1[DIR_NAME_SIZE] = ""; /* first product directory */
static char    Dir2[DIR_NAME_SIZE] = ""; /* second product directory */
static int     Prod_desc_blk_sz = sizeof(Prod_desc_blk_st);
static int     Graph_prod_hdr_sz = sizeof(Graphic_product);
static char    Prod_table_file[128] = "";
static char    Prod_excl_file[128] = "";
static int     Prod_table_flag = 0;
static int     Exclusion_flag = 0; /* Check exclusion list? (0=n,1=y) */
static int     Decompress_flag = 1; /* 0=Don't decompress, 1=Decompress */
static int     Prod_excl_arr[MAX_PROD_ID] = {0}; /* 0=Don't decompress, 1=Decompress */
static int     Num_exclusions = 0; /* number of prods to exclude from comparison */
static int     Strip_AWIPS_header = 0;
static int     Add_color = 0;

static List <Prod_File_st> ProdDir1; /* Directory name for first set of products */
static List <Prod_File_st> ProdDir2; /* Directory name for second set of products */

static Prod_summary_st Summary_data[MAX_PROD_ID];

static int ReadOptions(int argc, char** argv); 
int ReadFileInfo(char* dirname, List <Prod_File_st>& ProdDir);
int CompareTwoLists(List <Prod_File_st>& ProdDir1, List <Prod_File_st>& ProdDir2);
int FindAMatch(Prod_File_st prod1, Prod_File_st prod2);
int IsElevationBased(short prod_code);
int CompareTwoProducts(Prod_File_st prod1, Prod_File_st prod2);
void PrintNoMatchProds(List <Prod_File_st>& ProdDir);
static void PrintUsage (char **argv);
void ShortToInt( int* out, unsigned short* ms, unsigned short* ls);
void UshortToUint( unsigned int* out, unsigned short* ms, unsigned short* ls);
void InitSummaryData();
void PrintSummaryData();
void PrintVolumeHeader(int volnum, int voldate, int voltime);
static int ProdOnExclusionList(int pr_code);
static int ReadExclusionList();
static int Get_next_file_name( char *dir_name, char *basename,
                               char *buf, int buf_size );

/******************************************************************************
 * Description: This is the main procedure for prod_cmpr. Very simple in
 *              implementation, it just reads the options from command line and 
 *              then reads the product files from the two input directories and 
 *              then generates a comparison report.
 *       Input: 
 *      Output: 
 *      Return: 
 *       Notes: 
 *****************************************************************************/
int main(int argc, char* argv[])
{
  int retval;
  char* cfg_dir;
  char* prod_table_filename = "product_attr_table";

  retval = ReadOptions(argc, argv);
  if (retval < 0)
  {
     PrintUsage(argv);
  }
  
  /* We need the product attribute table in order to do decompression on
     compressed products.  If no product_tables file is given on the command
     line, we try to use the one located in (RPG_LEVEL | HOME)/cfg.  If that's not
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
        strcpy(Prod_table_file, cfg_dir);
        strcat(Prod_table_file, prod_table_filename);
     }
     else
     {
        Decompress_flag = 0;
     }

  }
  else
  {
     /* use the user provided prod tables file */
  }

  if (Init_attr_tbl (Prod_table_file, 1) <= 0)
  {
     fprintf(stderr, "Failure initializing product attrib table.\n");
     Decompress_flag = 0;
  }
  else if( Prod_table_flag == 0 ){

     /* Process any PAT extensions. */
     char ext_name[CFG_NAME_SIZE], cfg_extensions_name[CFG_NAME_SIZE] = "extensions";
     char tmpbuf[CFG_NAME_SIZE], *call_name = NULL;
  
     strcpy( tmpbuf, cfg_dir );
     strcat( tmpbuf, "extensions" );
     strcpy( cfg_extensions_name, tmpbuf );

     call_name = cfg_extensions_name;
     while( Get_next_file_name( call_name, prod_table_filename,
                                ext_name, CFG_NAME_SIZE ) == 0 ){
        if( Init_attr_tbl( ext_name, 0 ) < 0 )
           fprintf( stderr, "Unable to process PAT extension: %s\n", ext_name );

           call_name = NULL;
     }

  }

  if ( Exclusion_flag == 1 )
  {
     /* Initialize the product exclusion list */
     Num_exclusions = ReadExclusionList();
     if ( Num_exclusions < 0 )
     {
        fprintf(stderr,
           "main: ReadExclusionList() failed. No exclusions will be used.\n");
     }
  }

  /* Initialize product summary structure array */
  InitSummaryData();

  retval = ReadFileInfo(Dir1, ProdDir1);
  if (retval < 0)
  {
    fprintf(stderr, "Problem reading files in %s\n", Dir1);
    return (-1);
  }
  retval = ReadFileInfo(Dir2, ProdDir2);
  if (retval < 0)
  {
    fprintf(stderr, "Problem reading files in %s\n", Dir2);
    return (-1);
  }

  /* For very high verbosity levels, print out the ordered lists */
  
  if (Verbose > 2)
  {
    fprintf(stderr, "ProdDir1 list contents:\n");
    ProdDir1.reset();
    while (!ProdDir1.at_end())
    {
       fprintf(stderr, "%s\n", ProdDir1.current().filename);  
       ProdDir1.next();
    }
    fprintf(stderr, "ProdDir2 list contents:\n");
    ProdDir2.reset();
    while (!ProdDir2.at_end())
    {
       fprintf(stderr, "%s\n", ProdDir2.current().filename);  
       ProdDir2.next();
    }
  }

  ProdDir1.reset();
  ProdDir2.reset();

  CompareTwoLists(ProdDir1, ProdDir2);

  PrintSummaryData();

  if (Print_no_match_flag > 0)
  {
    PrintNoMatchProds(ProdDir1);
    PrintNoMatchProds(ProdDir2);
  }

  return (0);

} /* end main() */

/******************************************************************************
 * Description:  This function reads the command line options for prod_cmpr.
 *       Input:  argc and argv 
 *      Output:  none
 *      Return:  a boolean indicating success or failure.
 *       Notes: 
 *****************************************************************************/
static int ReadOptions(int argc, char**argv)
{
  extern char      *optarg; /* used by getopt */
  extern int       optind; /* total number of arguments */
  int              c; /* to read the program options */
  int              err=0; /* for error condition */
  
  while ((c = getopt(argc, argv, "cahl:mt:p:v:e:?")) != EOF)
  {
    switch(c)
    {

      case 'a':
         Strip_AWIPS_header = 1;
         break;

      case 'c':
         Add_color = 1;
         break;

      case 'l': /* read in the mismatch limit */
        if (sscanf (optarg, "%d", &Mismatch_limit) != 1)
          err = -1;
        break;

      case 'm': /* set the no match flag */
       Print_no_match_flag = 1;
        break;

      case 't': /* read in the product table file path and name */
        if (sscanf (optarg, "%s", Prod_table_file) != 1)
        {
           err = -1;
        }
        else
        {
           Prod_table_flag = 1;
        }
        break;

      case 'p': /* set the code to compare */
        if (sscanf (optarg, "%d", &Prod_code_to_cmpr) != 1)
          err = -1;
        else
        {
          /* check to see if the product code to compare is valid */
          if ( (Prod_code_to_cmpr < 16) || (Prod_code_to_cmpr >= MAX_PROD_ID))
          {
            err = -1;
          }
        }
        break;

      case 'v': /* set the verbosity level */
        if (sscanf (optarg, "%d", &Verbose) != 1)
          err = -1;
        break;
      case 's': /* set the sort key value */
        if (sscanf (optarg, "%d", &Sort_key) != 1)
          err = -1;
        else
        {
          if ( (Sort_key < 1) || (Sort_key > 2))
          {
            err = -1;
          }
        }
        break;
        
      case 'e': /* read in the exclusion file path and name */
        if (sscanf (optarg, "%s", Prod_excl_file) != 1)
        {
           err = -1;
        }
        else
        {
           Exclusion_flag = 1;
        }
        break;

      case 'h':
      case '?':
        err = -1;
        break;
    }
  }
  if (optind != argc-2)
    err = -1;
  else
  {
    strncpy(Dir1, argv[argc-2], DIR_NAME_SIZE);
    strncpy(Dir2, argv[argc-1], DIR_NAME_SIZE);
  }

  return (err);

} /* end ReadOptions() */


/******************************************************************************
 * Description:  This function reads in the products that need to be compared
 *       Input:  A string that has directory name from where the products need
 *               to be read and the list of products that needs to be populated.
 *      Output:  ProdDir is populated with the products that are read from the
 *               dirname.
 *      Return:  a boolean indicating success (0) or failure (-1).
 *       Notes: 
 *****************************************************************************/
int ReadFileInfo(char* dirname, List <Prod_File_st>& ProdDir)
{
  DIR               *dp; /* read the entries in product directory */
  struct dirent     *dirp;
  Prod_File_st      prod_entry; /* to read in the product record */
  struct stat       fstat_buf;
  int               fd_prod_file; /* file handle for product file */
  int               fstat_ret;
  short*            msg_hdr_data = NULL;
  short*            prod_desc_data = NULL;
  int               len_read;
  int               pcode;
  int               size = 0;
  int               offset = 0;

  static char       buf[MSG_HDR_SZ+WMO_AWIPS_HDR];

  if ( (dp = opendir(dirname)) == NULL)
  {
    fprintf(stderr, "cannot open directory %s", dirname);
    return (-1);
  }

  /* point to the first memeber of the list */
  ProdDir.reset();
  while ( (dirp = readdir(dp)) != NULL)
  {
    /* ignore all files that start with a '.' including "." and ".." */
    if (dirp->d_name[0] != '.')
    {
      /* assign filename along with relative directory path */
      prod_entry.filename =
         (char *)malloc(strlen(dirname)+strlen(dirp->d_name)+3);
      if (prod_entry.filename == 0)
      {
        fprintf(stderr, "memory allocation error for prod_entry.filename\n");
      }

      /* copy the directory name */
      strcpy(prod_entry.filename, dirname);

      /* make sure that there is a '/' between full directory path
         and filename */
      if (dirname[strlen(dirname)-1] != '/')
      {
        strcat(prod_entry.filename, "/");
      }

      /* copy the filename */
      strcat(prod_entry.filename, dirp->d_name);

      /* now open this file and read the info */
      errno = 0;
      fd_prod_file = open(prod_entry.filename, O_RDONLY, 0);

      /* some error checking */
      if (fd_prod_file < 0)
      {
        fprintf(stderr,
           "call to open failed for %s, errno is %d\n", prod_entry.filename,
           errno);
      }
      else
      {
        errno = 0;
        /* get the stat information to prepare to read in the product */
        fstat_ret = fstat(fd_prod_file, &fstat_buf);
        if (fstat_ret < 0)
        {
          fprintf(stderr, "call to fstate failed for file %s, errno is %d\n",
             prod_entry.filename, errno);
          continue; /* go on to next product file in dir */
        }
        else
        {
          prod_entry.file_sz = fstat_buf.st_size;
          if (prod_entry.file_sz == 0)
          {
            fprintf(stderr, "product file %s: size is 0\n", prod_entry.filename);
            continue; /* go on to next product file in dir */
          }
          else if (prod_entry.file_sz < MSG_HDR_SZ)
          {
            fprintf(stderr, "product file %s: size less than %d\n",
               prod_entry.filename, MSG_HDR_SZ);
            continue; /* go on to next product file in dir */
          }
          else
          {
            errno = 0;
            /* allocate memory to read in the message header */
            msg_hdr_data = (short *)malloc(MSG_HDR_SZ);
            if (msg_hdr_data == 0)
            {
              fprintf(stderr, "product file %s: malloc error for prod_data\n",
                 prod_entry.filename);
              continue; /* go on to next product file in dir */
            }
            /* read in the product message header */
            size = MSG_HDR_SZ;
            offset = 0;
            if( Strip_AWIPS_header ){

               size += WMO_AWIPS_HDR;
               offset = WMO_AWIPS_HDR;

            }

            len_read = read(fd_prod_file, (char *)buf, size);
            if (len_read < 0)
            {
              fprintf(stderr, "product file %s: error reading msg hdr\n",
                 prod_entry.filename);
              continue; /* go on to next product file in dir */
            }
            else
            {

              /* Strip off the AWIPS/WMO header if needed. */
              memcpy( msg_hdr_data, buf+offset, MSG_HDR_SZ );
                 
              /* If necessary byte swap the msg hdr block (in place) */
              #ifdef LITTLE_ENDIAN_MACHINE
                MISC_swap_shorts (MSG_HDR_SZ/sizeof(short), msg_hdr_data);
              #endif 

              /* assign the message header pointer */
              prod_entry.msg_hdr_ptr = (Prod_msg_header_icd *)msg_hdr_data;

              /* store the message/product code */
              pcode = prod_entry.msg_hdr_ptr->msg_code;

              /* Allocate space and read in the product description block */
              prod_desc_data = (short *)malloc(Prod_desc_blk_sz);
              if (prod_desc_data == 0)
              {
                 fprintf(stderr,
                   "product file %s: malloc error for prod desc blk\n",
                   prod_entry.filename);
                 continue; /* go on to next product file */
              }

              /* insert the product in the list if it is a valid code */
              if (!ProdOnExclusionList(pcode) && 
                 ((Prod_code_to_cmpr < 0) && (pcode >= 16 && pcode < MAX_PROD_ID)) || 
                 ((Prod_code_to_cmpr > 0) && (Prod_code_to_cmpr == pcode)))
              {
                int inserted = 0; /* 0 = not inserted, 1 = inserted into list */
                unsigned short new_time_ms; /* new file vol time MSW */
                unsigned short new_time_ls; /* new file vol time LSW */
                int new_vol_tm; /* vol time of new prod file */

                len_read = read(fd_prod_file, (char *)prod_desc_data, Prod_desc_blk_sz);
                if (len_read < 0)
                {
                  fprintf(stderr, "product file %s: error reading prod desc blk\n",
                     prod_entry.filename);
                  continue; /* go on to next product file in dir */
                }
                
                /* read was OK, proceed */

                /* assign pointer to prod description block */
                prod_entry.prod_desc_blk_ptr = (Prod_desc_blk_st *)prod_desc_data; 

                /* byte swap the prod desc blk if necessary */
                #ifdef LITTLE_ENDIAN_MACHINE
                  MISC_swap_shorts (Prod_desc_blk_sz/sizeof(short),
                    (short*)(prod_desc_data));
                #endif 
 
                /* separate the most significant and least significant HWs */
                new_time_ms = (unsigned short)
                  (prod_entry.prod_desc_blk_ptr->vol_time_ms);
                new_time_ls = (unsigned short)
                  (prod_entry.prod_desc_blk_ptr->vol_time_ls);
 
                /* put them together to get the correct volume time */
                ShortToInt(&new_vol_tm, &new_time_ms, &new_time_ls);  
     
                /* Reset the list pointer to the beginning of the list */
                ProdDir.reset();

                /* if list length is zero, insert file as the first record */
                if (ProdDir.length() == 0)
                {
                  ProdDir.insert(prod_entry);
                  inserted = 1;
                  continue; /* go process next prod file in directory */
                }

                /* insert according to the volume date, then time then prod id */
                while(!ProdDir.at_end() && (inserted == 0))
                {
                   short volume_date; /* new file volume date */
                   short curr_volume_date; /* curr list file volume date */
                   unsigned short curr_time_ms; /* curr list file vol time MSW */
                   unsigned short curr_time_ls; /* curr list file vol time LSW */
                   int curr_vol_tm; /* current list file vol time */
                   short curr_prod_code; /* current list file prod code */

                   volume_date = 
                      (short) prod_entry.prod_desc_blk_ptr->vol_date;
                   curr_volume_date =
                      (short) ProdDir.current().prod_desc_blk_ptr->vol_date;

                   if ( volume_date < curr_volume_date )
                   {
                      /* insert prod file before list pointer position */
                      ProdDir.insert(prod_entry);
                      inserted = 1;
                   }
                   else if ( volume_date > curr_volume_date )
                   {
                      /* continue to next prod file in list */
                      ProdDir.next();
                   }
                   else if ( volume_date == curr_volume_date )
                   {
                      curr_time_ms = 
                         (unsigned short)
                         (ProdDir.current().prod_desc_blk_ptr->vol_time_ms);
                      curr_time_ls = 
                         (unsigned short)
                         (ProdDir.current().prod_desc_blk_ptr->vol_time_ls);

                      /* join the two HWs to get the curr list file vol time */
                      ShortToInt(&curr_vol_tm, &curr_time_ms, &curr_time_ls);  

                      if ( new_vol_tm < curr_vol_tm )
                      {
                         /* insert prod file before list pointer position */
                         ProdDir.insert(prod_entry);
                         inserted = 1;
                      }
                      else if ( new_vol_tm > curr_vol_tm )
                      {
                         /* continue to next prod file in list */
                         ProdDir.next();
                      }
                      else /* new_vol_tm is equal to curr_vol_tm */
                      {
                         /* store the curr list file product code */
                         curr_prod_code = 
                            ProdDir.current().prod_desc_blk_ptr->prod_code;

                         if ( pcode < curr_prod_code )
                         {
                            /* insert prod file before list pointer position */
                            if (inserted != 1)
                            {
                               ProdDir.insert(prod_entry);
                               inserted = 1;
                            }
                         }
                         else if ( pcode > curr_prod_code )
                         {
                            /* continue to next prod file in list */
                            ProdDir.next();
                         }
                         else /* pcode is equal to curr_prod_code */
                         {
                            /* insert after current list pointer position */
                            if (inserted != 1)
                            {
                               ProdDir.next();
                               ProdDir.insert(prod_entry);
                               inserted = 1;
                            }
                         }
                      }
                   }
                }

                /* If still not inserted then insert this product at the very
                   end of the list */
                if (inserted != 1)
                {
                   ProdDir.insert_tail(prod_entry);
                }
              }
              else
              {
                /* free the memory */
                if ( prod_entry.filename != NULL )
                {
                   free(prod_entry.filename);
                }
                if ( msg_hdr_data != NULL )
                {
                   free(msg_hdr_data);
                }
                if ( prod_desc_data != NULL )
                {
                   free(prod_desc_data);
                }
              }
            }
          }
          close(fd_prod_file);
        }
      }
    }
  }
  closedir(dp);
  return (0);

} /* end ReadFileInfo() */


/******************************************************************************
 * Description:  This function compares the two lists to find matching
 *               products and then call other function to compare the matching 
 *               products.
 *       Input:  Two product lists
 *      Output:  None
 *      Return:  a boolean indicating success or failure.
 *       Notes: 
 *****************************************************************************/
int CompareTwoLists(List <Prod_File_st>& ProdDir1, List <Prod_File_st>& ProdDir2)
{
  int ret_val;
  int fd_prod_file1, fd_prod_file2; /* file descriptors for product files */
  void *prod1_data;
  void *prod2_data;
  int len_read1, len_read2;
  short found_flag = 0;
  int matches_found = 0;
  short *uncompress1 = NULL; /* pointer to uncompressed product 1 data */
  short *uncompress2 = NULL; /* pointer to uncompressed product 2 data */
  int free_uncompress1;
  int free_uncompress2;
  int prod1_code, prod2_code;
  int prod1_size, prod2_size;
  int prod1_id;
  unsigned short voltime_ms, voltime_ls;
  int volnum=0, voldate=0, voltime=0, prev_voldate=0, prev_voltime=0;
  int volume_header_print = 1; /* 0=don't need to print, 1=need to print */
  int ret;

  ProdDir1.reset();

  /* go through the first list */
  while (!ProdDir1.at_end())
  {
    ProdDir2.reset();
    /* try to find the matching entry for every product in the first list */
    while (!ProdDir2.at_end())
    {
      /* only need to read the product data if actual match is found */
      ret_val = FindAMatch(ProdDir1.current(), ProdDir2.current());
      if (ret_val == PROD_MATCH_FOUND)
      {
        ++matches_found;

        /* store the product codes */
        prod1_code = (ProdDir1.current()).prod_desc_blk_ptr->prod_code;
        prod2_code = (ProdDir2.current()).prod_desc_blk_ptr->prod_code;

        /* store previous vol date/time info */
        prev_voldate = voldate;
        prev_voltime = voltime;

        /* extract the volume information for the vol header print */
        volnum = (int)(ProdDir1.current()).prod_desc_blk_ptr->vol_num;
        voldate = (int)(ProdDir1.current()).prod_desc_blk_ptr->vol_date;
        voltime_ms = (unsigned short)
          (ProdDir1.current().prod_desc_blk_ptr->vol_time_ms);
        voltime_ls = (unsigned short)
          (ProdDir1.current().prod_desc_blk_ptr->vol_time_ls);
        ShortToInt(&voltime, &voltime_ms, &voltime_ls);

        /* if vol date/time are different, set flag to print vol header */
        if ( (voldate != prev_voldate ) || (voltime != prev_voltime))
        {
           volume_header_print = 1;
        }

        /* Increment the summary product count for this product */
        prod1_id = ORPGPAT_get_prod_id_from_code(prod1_code);
        Summary_data[prod1_id].total_num++;

        /* Open the file for product 1 */
        fd_prod_file1 = open(ProdDir1.current().filename, O_RDONLY, 0);
        if (fd_prod_file1 < 0)
        {
          fprintf(stderr, "call to open failed for %s, errno is %d\n", 
                  ProdDir1.current().filename, errno);
        }

        /* store prod 1 size (bytes) */
        prod1_size = ProdDir1.current().file_sz;

        /* allocate enough space to store product 1 */
        prod1_data = (void *)malloc(prod1_size);
        if (prod1_data == 0)
        {
          fprintf(stderr, "memory allocation error for prod1_data\n");
          exit (0);
        }

        /* read product 1 */
        len_read1 = read(fd_prod_file1, (char *)prod1_data, prod1_size);
        if (len_read1 < 0)
        {
          fprintf(stderr, "read error reading in the product file %s\n",
                  ProdDir1.current().filename);
          exit (0);
        }


        /* Store prod 2 size in bytes */
        prod2_size = ProdDir2.current().file_sz;

        /* open file for product 2 */
        fd_prod_file2 = open(ProdDir2.current().filename, O_RDONLY, 0);
        if (fd_prod_file2 < 0)
        {
          fprintf(stderr, "call to open failed for %s, errno is %d\n", 
                  ProdDir2.current().filename, errno);
        }

        /* allocate enough space to store product 2 */
        prod2_data = (void *)malloc(ProdDir2.current().file_sz);
        if (prod2_data == 0)
        {
          fprintf(stderr, "memory allocation error for prod2_data\n");
        }

        /* read and store product 2 */
        len_read2 = read(fd_prod_file2, (char *)prod2_data,
          ProdDir2.current().file_sz);
        if (len_read2 < 0)
        {
          fprintf(stderr, "read error reading in the product file %s\n", 
                  ProdDir2.current().filename);
        }

        /* If necessary byte swap the prod 1 graphic header blk (in place) */
        #ifdef LITTLE_ENDIAN_MACHINE
          MISC_swap_shorts (Graph_prod_hdr_sz/sizeof(short),
            (short*) prod1_data);
        #endif 

        /* If necessary byte swap the prod 2 graphic header blk (in place) */
        #ifdef LITTLE_ENDIAN_MACHINE
          MISC_swap_shorts (Graph_prod_hdr_sz/sizeof(short),
            (short*) prod2_data);
        #endif 

        /* Check to see if product 1 needs to be decompressed.  If so,          *
         * decompress it.                                                       */
        
        if ( Decompress_flag == 1 )
        {
           free_uncompress1 = 0;
           ret = ORPGPAT_get_compression_type( ORPGPAT_get_prod_id_from_code (prod1_code));
           if ( ret > 0) 
           {
              if (Verbose > 1)
              {
                 fprintf(stderr, "Decompressing %s\n", ProdDir1.current().filename);
              }
              uncompress1 = (short*)
                Decompress_product ( prod1_data, &prod1_size );

              /* We must reset the file size with the new decompressed size */
              ProdDir1.current().file_sz = prod1_size;
       
              /* Reallocate more space for prod 1 data buffer */
              free (prod1_data);
              prod1_data = (void *)malloc(prod1_size);
              if (prod1_data == 0)
              {
                 fprintf(stderr, "memory allocation error for prod1_data\n");
              }
        
              /* Copy uncompressed data to the prod 1 data buffer */
              memcpy((void*)prod1_data, (void*)uncompress1, (size_t) prod1_size);
              free_uncompress1 = 1;
           }
  
           /* Check to see if product 2 needs to be decompressed.  If so,          *
            * decompress it.                                                       */
           
           free_uncompress2 = 0;
           ret = ORPGPAT_get_compression_type( ORPGPAT_get_prod_id_from_code (prod2_code));
           if ( ret > 0) 
           {
              if (Verbose > 1)
              {
                 fprintf(stderr, "Decompressing %s", ProdDir2.current().filename);
              }
              uncompress2 = (short*)
                Decompress_product ( prod2_data, &prod2_size );
           
              /* We must reset the file size to the decompressed size */
              ProdDir2.current().file_sz = prod2_size;

              /* Reallocate more space for prod 2 data buffer */
              free (prod2_data);
              prod2_data = (void *)malloc(prod2_size);
           
              /* Copy uncompressed data to the prod 2 data buffer */
              memcpy((void*)prod2_data, (void*)uncompress2, (size_t) prod2_size);
              free_uncompress2 = 1;
           }
        }

        /* if necessary byte swap the decompressed prod data */
        #ifdef LITTLE_ENDIAN_MACHINE
          MISC_swap_shorts (prod1_size/sizeof(short), (short*) prod1_data);
          MISC_swap_shorts (prod2_size/sizeof(short), (short*) prod2_data);
        #endif 

        /* assign the actual product data to the correct element in each list */
        ProdDir1.current().rest_of_data_ptr =
          (short *) ((size_t)prod1_data + Graph_prod_hdr_sz);
        ProdDir2.current().rest_of_data_ptr =
          (short *) ((size_t)prod2_data + Graph_prod_hdr_sz);

        /* close files */
        close(fd_prod_file1);
        close(fd_prod_file2);

        /* If we need to, print the volume scan header information */
        if ( volume_header_print == 1 )
        {
           PrintVolumeHeader(volnum, voldate, voltime);
           volume_header_print = 0;
        }

        /* compare the two products to see if they match */
        ret_val = CompareTwoProducts(ProdDir1.current(), ProdDir2.current());
        if (ret_val == PROD_MATCH_FOUND)
        {
          Identical_products++;
          if (Verbose > 1)
          {
            fprintf(stderr, "Product %s and Product %s are identical\n",
                    (ProdDir1.current()).filename, (ProdDir2.current()).filename);
          }
        }
        else if (ret_val == FILESIZE_MISMATCH)
        {
          Number_of_mismatches++;
        }
        else
        {
          Number_of_mismatches++;
        }

        if ( free_uncompress1 )
          free (uncompress1);

        if ( free_uncompress2 )
          free (uncompress2);

        if ( prod1_data != NULL )
          free (prod1_data);

        if ( prod2_data != NULL)
          free (prod2_data);

        ProdDir1.remove();
        ProdDir2.remove();
        found_flag = 1;
        break; /* break  out of the loop since the match has been found */
      }
      ProdDir2.next();
    }
    if(found_flag)
      found_flag = 0;
    else
      ProdDir1.next();
  }

  if (Verbose > 0)
  {
     if( Add_color ){

        fprintf(stderr, BLKTEXT "\nNumber of Identical products in two data sets is %d\n\n" RESET,
                Identical_products);
        fprintf(stderr, BLKTEXT
                "Total number of products to compare in two data sets are %d\n\n" RESET,
                matches_found);

     }
     else{

        fprintf(stderr, "\nNumber of Identical products in two data sets is %d\n\n",
                Identical_products);
        fprintf(stderr, 
                "Total number of products to compare in two data sets are %d\n\n",
                matches_found);

     }
  }
 
  return (0);

} /* CompareTwoLists */

/******************************************************************************
 * Description: This function returns a value indicating if the two products
 *              need to be compared. Definition of a two matching products 
 *              is that they must have the same product code, volume data and 
 *              time, and if they are elevation based then the same elevation.
 *       Input: Two product records to be compared 
 *      Output: None 
 *      Return:  
 *       Notes: 
 *****************************************************************************/
int FindAMatch(Prod_File_st prod1, Prod_File_st prod2)
{
  short prod1_code, prod2_code;
  short prod1_seq_num, prod2_seq_num;

  prod1_code = prod1.prod_desc_blk_ptr->prod_code;
  prod2_code = prod2.prod_desc_blk_ptr->prod_code;
  prod1_seq_num = prod1.prod_desc_blk_ptr->seq_num;
  prod2_seq_num = prod2.prod_desc_blk_ptr->seq_num;

  /* Compare product codes */
  if (prod1_code != prod2_code)
    return PRODCODE_MISMATCH;

  /* Compare vol dates and times */
  if ((prod1.prod_desc_blk_ptr->vol_date != prod2.prod_desc_blk_ptr->vol_date) ||
      (prod1.prod_desc_blk_ptr->vol_time_ms != prod2.prod_desc_blk_ptr->vol_time_ms) ||
      (prod1.prod_desc_blk_ptr->vol_time_ls != prod2.prod_desc_blk_ptr->vol_time_ls))
    return VOLTIME_MISMATCH;

  /* Check elevation angle (on elevation based products) */
  if (IsElevationBased(prod1_code) && 
     (prod1.prod_desc_blk_ptr->param_3 != prod2.prod_desc_blk_ptr->param_3))
    return ELANGLE_MISMATCH;

  /* If a product is generated because of an alert condition, the seq num is
     set to -13.  We need to make sure we're comparing alert-generated prods
     with other alert-generated prods. */
  prod1_seq_num = prod1.prod_desc_blk_ptr->seq_num;
  prod2_seq_num = prod2.prod_desc_blk_ptr->seq_num;
  if (((prod1_seq_num == -13) && (prod2_seq_num != -13)) ||
      ((prod2_seq_num == -13) && (prod1_seq_num != -13)))
     return PROD_NOT_MATCH;

  return PROD_MATCH_FOUND;

} /* end FindAMatch() */

/******************************************************************************
 * Description: This function is used to determine if a certain product is
 *              elevation based or not 
 *       Input: Product code 
 *      Output: None 
 *      Return: 1 indictating that prod_code is elevation based, 0 otherwise
 *       Notes: 
 *****************************************************************************/
int IsElevationBased(short prod_code)
{

  int prod_id = ORPGPAT_get_prod_id_from_code( prod_code );
  int elev_based = ORPGPAT_elevation_based( prod_id );

  if( elev_based >= 0 )
      return 1;
  else
    return 0;

} /* end IsElevationBased() */

/******************************************************************************
 * Description: This function takes two products and does a word by word
 *              comparison to find out if they match. There are certain bits 
 *              that are excluded from this comparison because thay can be 
 *              different even if the product data is the same. 
 *       Input: Two product records to be compared 
 *      Output: None 
 *      Return: Return value indicates if the products match 
 *       Notes: 
 *****************************************************************************/
int CompareTwoProducts(Prod_File_st prod1, Prod_File_st prod2)
{
  short *prod1_hdr = (short *)prod1.msg_hdr_ptr;
  short *prod2_hdr = (short *)prod2.msg_hdr_ptr;
  short *prod1_desc_blk = (short *)prod1.prod_desc_blk_ptr;
  short *prod2_desc_blk = (short *)prod2.prod_desc_blk_ptr;
  unsigned int    diff_hdr[Mismatch_limit]; /* tracks hw diffs in hdr */
  unsigned int    diff_desc_blk[Mismatch_limit];/* tracks hw diffs in desc blk */
  unsigned int    diff_data[Mismatch_limit];/* tracks hw diffs in data */
  int    index_hdr = -1;
  int    index_desc_blk = -1;
  int    index_data = -1;
  int    diff_flag = 0;
  int    sym_flag = 0;
  int    gra_flag = 0;
  int    tab_flag = 0;
  int    sym_hdr_print_flag = 0;
  int    gra_hdr_print_flag = 0;
  int    tab_hdr_print_flag = 0;
  int    larger_prod_sz = 0;
  int    hdr_mismatch_limit_reached = 0;
  int    desc_blk_mismatch_limit_reached = 0;
  int    data_mismatch_limit_reached = 0;
  unsigned int    sym_blk_offset_prod1 = 0;
  unsigned int    sym_blk_offset_prod2 = 0;
  unsigned int    gra_blk_offset_prod1 = 0;
  unsigned int    gra_blk_offset_prod2 = 0;
  unsigned int    tab_blk_offset_prod1 = 0;
  unsigned int    tab_blk_offset_prod2 = 0;
  int    prod_id = 0;

  if (prod1.file_sz < (unsigned int) Graph_prod_hdr_sz)
      return FILESIZE_MISMATCH;

  /* Handle the case where the prod file sizes differ */
  if (prod1.file_sz != prod2.file_sz)
  {
     /* size mismatch found, print message */
     if (Verbose > 0)
     {
        if( Add_color )
           fprintf(stderr, REDTEXT "Product %s (%d) and \nProduct %s (%d) have different sizes.\n" RESET,
                   (ProdDir1.current()).filename, prod1.file_sz,
                   (ProdDir2.current()).filename, prod2.file_sz);
        else
           fprintf(stderr, "Product %s (%d) and \nProduct %s (%d) have different sizes.\n",
                   (ProdDir1.current()).filename, prod1.file_sz,
                   (ProdDir2.current()).filename, prod2.file_sz);
     }

     /* We need to know which prod is larger */
     if ( prod1.file_sz >= prod2.file_sz)
        larger_prod_sz = prod1.file_sz;
     else
        larger_prod_sz = prod2.file_sz;
  }
  else
  {
     /* The prods are the same size.  Set the larger prod sz to prod 1 sz.*/
     larger_prod_sz = prod1.file_sz;
  }

  /* initialize the block offset values */
  UshortToUint(&sym_blk_offset_prod1, &prod1.prod_desc_blk_ptr->sym_off_msw, 
     &prod1.prod_desc_blk_ptr->sym_off_lsw);  

  UshortToUint(&sym_blk_offset_prod2, &prod2.prod_desc_blk_ptr->sym_off_msw,
     &prod2.prod_desc_blk_ptr->sym_off_lsw);  

  UshortToUint(&gra_blk_offset_prod1, &prod1.prod_desc_blk_ptr->gra_off_msw,
     &prod1.prod_desc_blk_ptr->gra_off_lsw);  

  UshortToUint(&gra_blk_offset_prod2, &prod2.prod_desc_blk_ptr->gra_off_msw,
     &prod2.prod_desc_blk_ptr->gra_off_lsw);  

  UshortToUint(&tab_blk_offset_prod1, &prod1.prod_desc_blk_ptr->tab_off_msw,
     &prod1.prod_desc_blk_ptr->tab_off_lsw);  

  UshortToUint(&tab_blk_offset_prod2, &prod2.prod_desc_blk_ptr->tab_off_msw,
     &prod2.prod_desc_blk_ptr->tab_off_lsw);  

  /* If necessary, byte swap the block offsets. Note that we had already
     byte swapped the header on short boundaries.  Now, since these
     are 4-byte integers, we need to swap them on int boundaries. */
  #ifdef LITTLE_ENDIAN_MACHINE
/*
    sym_blk_offset_prod1 = (prod1.prod_desc_blk_ptr->sym_off << 16) |
                           (prod1.prod_desc_blk_ptr->sym_off >> 16);
    sym_blk_offset_prod2 = (prod2.prod_desc_blk_ptr->sym_off << 16) |
                           (prod2.prod_desc_blk_ptr->sym_off >> 16);
    gra_blk_offset_prod1 = (prod1.prod_desc_blk_ptr->gra_off << 16) |
                           (prod1.prod_desc_blk_ptr->gra_off >> 16);
    gra_blk_offset_prod2 = (prod2.prod_desc_blk_ptr->gra_off << 16) |
                           (prod2.prod_desc_blk_ptr->gra_off >> 16);
    tab_blk_offset_prod1 = (prod1.prod_desc_blk_ptr->tab_off << 16) |
                           (prod1.prod_desc_blk_ptr->tab_off >> 16);
    tab_blk_offset_prod2 = (prod2.prod_desc_blk_ptr->tab_off << 16) |
                           (prod2.prod_desc_blk_ptr->tab_off >> 16);
*/
  #endif

  /* Set block flags */
  if (sym_blk_offset_prod1 > 0)
     sym_flag = 1;
  if (gra_blk_offset_prod1 > 0)
     gra_flag = 1;
  if (tab_blk_offset_prod1 > 0)
     tab_flag = 1;

  /* Compare the message header portion and store the halfwords that differ */
  for (int i=0; i<(MSG_HDR_SZ/2); i++)
  {
    /* don't compare the halfwords that may be different for the same product */
    if ((i!=1) && (i!=2) && (i!=3) && (i!=6) && (i!=7) &&
       (prod1_hdr[i] != prod2_hdr[i]))
    {
      diff_flag++;
      if (diff_flag <= Mismatch_limit)
        diff_hdr[++index_hdr]=i;
      else
        hdr_mismatch_limit_reached = 1;
    }
  }

  /* Compare the prod desc blk portion and store the halfwords that differ */
  for (int i=0; i<(Prod_desc_blk_sz/2); i++)
  {
    /* don't compare the half words that may be different for the same product */
    if ((i!=9) && (i!=10) && (i!=14) && (i!=15) && (i!=16) && 
       (prod1_desc_blk[i] != prod2_desc_blk[i]))
    {
      diff_flag++;
      if (diff_flag <= Mismatch_limit)
        diff_desc_blk[++index_desc_blk]=i;
      else
        desc_blk_mismatch_limit_reached = 1;
    }
  }

  /* Compare the data portion and store the halfwords that differ */
  if ( (tab_blk_offset_prod1 > 0) && 
       (tab_blk_offset_prod1 == tab_blk_offset_prod2 ))
  {
    /* NOTE: Since we separated the hdr portion from the data portion we need to
       adjust the offset. */

    int tab_hdr = tab_blk_offset_prod1 -60 + 4 ;

    for (int i=0; i<(larger_prod_sz-Graph_prod_hdr_sz)/2; i++)
    {
      if ((i!=tab_hdr+1) && (i!=tab_hdr+2) && (i!=tab_hdr+3) && (i!=tab_hdr+6) && 
          (i!=tab_hdr+7) && (i!=tab_hdr+18) && (i!=tab_hdr+19) &&  
          (i!=tab_hdr+23) && (i!=tab_hdr+24) && (i!=tab_hdr+25) && 
          prod1.rest_of_data_ptr[i] != prod2.rest_of_data_ptr[i])
      {
         diff_flag++;
         if (diff_flag <= Mismatch_limit)
           diff_data[++index_data]=i;
         else
           data_mismatch_limit_reached = 1;
      }
    }
  }
  else
  {
    for (int i=0; i<(larger_prod_sz-Graph_prod_hdr_sz)/2; i++)
    { 
      if (prod1.rest_of_data_ptr[i] != prod2.rest_of_data_ptr[i])
      {
         diff_flag++;
         if (diff_flag <= Mismatch_limit)
           diff_data[++index_data]=i;
         else
           data_mismatch_limit_reached = 1;
      }
    }
  }

  if (diff_flag)
  {
    /* Print prod filenames and prod codes */
    if( Add_color ){

       fprintf(stderr, BLKTEXT "\n(Left side) %s code %d\n" RESET,
               prod1.filename, prod1.prod_desc_blk_ptr->prod_code);
       fprintf(stderr, BLKTEXT "(Right side) %s code %d\n" RESET,
               prod2.filename, prod2.prod_desc_blk_ptr->prod_code);
    }
    else {

       fprintf(stderr, "\n(Left side) %s code %d\n",
               prod1.filename, prod1.prod_desc_blk_ptr->prod_code);
       fprintf(stderr, "(Right side) %s code %d\n",
               prod2.filename, prod2.prod_desc_blk_ptr->prod_code);

    }

    /* Print differences in the msg hdr block */
    for (int j=0; j<=index_hdr; j++)
    {
      fprintf(stderr,"Half word %d     %04hx            %04hx\n",
                diff_hdr[j],prod1_hdr[diff_hdr[j]],prod2_hdr[diff_hdr[j]]);
    }

    /* Print mismatch limit warning if necessary */
    if ( hdr_mismatch_limit_reached ){

       if( Add_color )
          fprintf(stderr, REDTEXT "******** MISMATCH LIMIT REACHED ********\n" RESET);
       else 
          fprintf(stderr, "******** MISMATCH LIMIT REACHED ********\n");

    }

    /* Print differences in the prod description block */
    for (int j=0; j<=index_desc_blk; j++)
    {
      fprintf(stderr,"Half word %d     %04hx            %04hx\n",
         diff_desc_blk[j]+DESC_BLK_HW_OFFSET, prod1_desc_blk[diff_desc_blk[j]],
         prod2_desc_blk[diff_desc_blk[j]]);
    }

    /* Print mismatch limit warning if necessary */
    if ( desc_blk_mismatch_limit_reached ){

       if( Add_color )
          fprintf(stderr, REDTEXT "******** MISMATCH LIMIT REACHED ********\n" RESET );

       else
          fprintf(stderr, "******** MISMATCH LIMIT REACHED ********\n" );

    }

    /* Print differences in the data blocks */
    for (int j=0; j<=index_data; j++)
    {
      /* Print necessary block header info */
      if ( ((diff_data[j]+DATA_BLK_HW_OFFSET) > tab_blk_offset_prod1 ) &&
           (tab_flag == 1 ) &&
           (tab_hdr_print_flag == 0 ))
      {
         if( Add_color )
            fprintf(stderr, PURTEXT "\nTabular Alphanumeric Block (Begin HW %d)\n" RESET,
                    tab_blk_offset_prod1);
         else
            fprintf(stderr, "\nTabular Alphanumeric Block (Begin HW %d)\n",
                    tab_blk_offset_prod1);
         tab_hdr_print_flag = 1;
         gra_hdr_print_flag = 1;
         sym_hdr_print_flag = 1;
      }
      else if ( ((diff_data[j]+DATA_BLK_HW_OFFSET) > gra_blk_offset_prod1 ) &&
                (gra_flag == 1 ) &&
                (gra_hdr_print_flag == 0 ))
      {
         if( Add_color )
            fprintf(stderr, PURTEXT "\nGraphic Alphanumeric Block (Begin HW %d)\n" RESET,
                    gra_blk_offset_prod1);
         else
            fprintf(stderr, "\nGraphic Alphanumeric Block (Begin HW %d)\n",
                    gra_blk_offset_prod1);
         gra_hdr_print_flag = 1;
         sym_hdr_print_flag = 1;
      }
      else if ( ((diff_data[j]+DATA_BLK_HW_OFFSET) > sym_blk_offset_prod1 ) &&
                (sym_flag == 1 ) &&
                (sym_hdr_print_flag == 0 ))
      {
         if( Add_color )
            fprintf(stderr, PURTEXT "\nSymbology Block (Begin HW %d)\n" RESET,
                    sym_blk_offset_prod1);
         else
            fprintf(stderr, "\nSymbology Block (Begin HW %d)\n",
                    sym_blk_offset_prod1);
         sym_hdr_print_flag = 1;
      }
      
      if ( (((diff_data[j]+DATA_BLK_HW_OFFSET)*2) <= prod1.file_sz) &&
           (((diff_data[j]+DATA_BLK_HW_OFFSET)*2) <= prod2.file_sz))
      {
         fprintf(stderr,"Half word %d     %04hx            %04hx\n",
                diff_data[j]+DATA_BLK_HW_OFFSET,prod1.rest_of_data_ptr[diff_data[j]],
                prod2.rest_of_data_ptr[diff_data[j]]);
      }
      else if ( (((diff_data[j]+DATA_BLK_HW_OFFSET)*2) <= prod1.file_sz) &&
                (((diff_data[j]+DATA_BLK_HW_OFFSET)*2) > prod2.file_sz))
      {
         fprintf(stderr,"Half word %d     %04hx                 \n",
                diff_data[j]+DATA_BLK_HW_OFFSET,prod1.rest_of_data_ptr[diff_data[j]]);
      }
      else if ( (((diff_data[j]+DATA_BLK_HW_OFFSET)*2) > prod1.file_sz) &&
                (((diff_data[j]+DATA_BLK_HW_OFFSET)*2) <= prod2.file_sz))
      {
         fprintf(stderr,"Half word %d                     %04hx\n",
                diff_data[j]+DATA_BLK_HW_OFFSET,prod2.rest_of_data_ptr[diff_data[j]]);
      }
    }

    /* Print mismatch limit warning if necessary */
    if ( data_mismatch_limit_reached ){

       if( Add_color )
          fprintf(stderr, REDTEXT "******** MISMATCH LIMIT REACHED ********\n" RESET);

       else 
          fprintf(stderr, "******** MISMATCH LIMIT REACHED ********\n");

    }

    /* Print product divider */
    fprintf(stderr,
       "-------------------------------------------------------------------------------\n");

    /* Record miscompare in summary data */
    prod_id = ORPGPAT_get_prod_id_from_code(prod1.prod_desc_blk_ptr->prod_code);
    Summary_data[prod_id].num_miscompared++;

    return PROD_NOT_MATCH;
  }
  return (PROD_MATCH_FOUND);

} /* end CompareTwoProducts() */

/******************************************************************************
 * Description: This function prints out the list of products that do not have
 *              a valid match
 *       Input: List of all products that don't have a valid match 
 *      Output: A message indicating that the product contained in a specific
 *              filename doesn't have a valid match
 *      Return: None 
 *       Notes: 
 *****************************************************************************/
void PrintNoMatchProds(List <Prod_File_st>& ProdDir)
{
  ProdDir.reset();
  while (!ProdDir.at_end())
  {
    if (Verbose > 0)
    {
       fprintf(stderr,"Product %s does not have any matching product\n", 
          (ProdDir.current()).filename);
    }
    ProdDir.next();
  }
} /* end PrintNoMatchProds() */

/******************************************************************************
 * Description: This function just writes out the usage of prod_cmpr tool 
 *       Input: argv 
 *      Output: Usage of the prod_cmpr tool 
 *      Return: None 
 *       Notes: 
 *****************************************************************************/
static void PrintUsage (char **argv) 
{
    printf ("Usage: %s (options) <dir1> <dir2>\n", argv[0]);
    printf ("       options:\n");
    printf ("       -a product files contain WMO/AWIPS header\n" );
    printf ("       -c Add color to output\n" );
    printf ("       -h (print this message)\n");
    printf ("       -l <limit_on_mismatches> (default is 100)\n");
    printf ("       -m (print summary of products with no corresponding product\n");
    printf ("           to compare to)\n");
    printf ("       -p <prod_code> (must be between 16 and 1999, ");
    printf ("default is to compare all products)\n");
    printf ("       -s <sort_key> (possible values are 1, 2 and 3 (default is 1)\n");
    printf ("                       where 1 is sort by product id\n");
    printf ("                             2 is sort by product date and time\n");
    printf ("       -v <verbosity_level> (0=low, 1=default, 2=high, 3=debug)\n");
    printf ("       -t <file_name> Product attribute table file path and name\n");
    printf ("       -e <file_name> Exclusion file path and name\n");
    exit (0);
} /* end PrintUsage() */

/******************************************************************************
 * Description: This function combines two shorts to get one int 
 *       Input: Two shorts to be combined 
 *      Output: one int that is result of combining two shorts 
 *      Return: None 
 *       Notes: 
 *****************************************************************************/
void ShortToInt( int* out, unsigned short* ms, unsigned short* ls)
{
        *out = *ms & 0x0000ffff;
        *out = (*out)<<16;
        *out = (*out) | (*ls);
} /* end ShortToInt() */


/******************************************************************************
 * Description: This function combines two ushorts to get one uint 
 *       Input: Two unsigned shorts to be combined 
 *      Output: one unsigned int that is result of combining two ushorts 
 *      Return: None 
 *       Notes: 
 *****************************************************************************/
void UshortToUint( unsigned int* out, unsigned short* ms, unsigned short* ls)
{
        *out = *ms & 0x0000ffff;
        *out = (*out)<<16;
        *out = (*out) | (*ls);
} /* end UshortToUint() */


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
    int ret   = 0;

    if (Verbose > 1)
    {
       fprintf (stderr, "Initializing product attributes table\n");
    }

    /*  If the attributes table already exists, clear it and get ready to
        create new data. */ 
    if( clear_table )
    {
       ret = ORPGPAT_clear_tbl();
       if ( ret == 1 )
       {
          if (Verbose > 1)
          {
             fprintf (stderr,
                "ORPGPAT_clear_tbl: PAT previously initialized (ret=%d)\n", ret);
          }
       }
       else if ( ret == 0 )
       {
          if (Verbose > 1)
          {
             fprintf (stderr,
                "ORPGPAT_clear_tbl: PAT not previously initialized (ret=%d)\n",
                ret);
          }
       }
    }

    /*  Create a new PAT. */

    if (Verbose > 1)
    {
       fprintf(stderr, "PAT file name: %s\n", file_name);
    }

    p_cnt = ORPGPAT_read_ASCII_PAT( file_name );
    if( p_cnt < 0 )
    {
        fprintf (stderr, "Error in ORPGPAT_read_ASCII_PAT (ret=%d)\n",
           p_cnt);
        return (-1);
    }
    else
    {
       if (Verbose > 1)
       {
          fprintf(stderr, "Number of products in PAT = %d\n", p_cnt);
       }

       return (p_cnt);
    }

} /* End of Init_attr_tbl() */


/******************************************************************************
 * Description: This function initializes the product comparison summary
 *              data structure.
 *       Input: None
 *      Output: None
 *      Return: None 
 *       Notes: 
 *****************************************************************************/
void InitSummaryData()
{
   int prod_idx = 0;

   for ( prod_idx = 0; prod_idx < MAX_PROD_ID; prod_idx++)
   {
      Summary_data[prod_idx].total_num = 0;
      Summary_data[prod_idx].num_miscompared = 0;
   }
} /* end InitSummaryData() */


/******************************************************************************
 * Description: This function prints the product comparison summary information,
 *              including the product code, alphanumeric ID, total number of
 *              files, and number of files that miscompared.
 *       Input: None
 *      Output: Summary information printed to stdout
 *      Return: None 
 *       Notes: 
 *****************************************************************************/
void PrintSummaryData()
{
   int prod_idx = 0;
   int prod_code = 0;
   char* temp_str = NULL;
   char* prod_mnemonic = NULL;
   char* pwd;

   prod_mnemonic = (char*) malloc(MAX_MNEMONIC_LEN);

   /* First print header */
   if( Add_color ){

      fprintf(stdout, BLUTEXT
         "-------------------------------------------------------------------------------\n" RESET);
      fprintf(stdout, BLUTEXT
         "----------------------------------  SUMMARY  ----------------------------------\n" RESET);
      fprintf(stdout, BLUTEXT
         "-------------------------------------------------------------------------------\n" RESET);

   }
   else {

      fprintf(stdout, 
         "-------------------------------------------------------------------------------\n");
      fprintf(stdout, 
         "----------------------------------  SUMMARY  ----------------------------------\n");
      fprintf(stdout, 
         "-------------------------------------------------------------------------------\n");

   }

   /* Print PWD and directory names */
   pwd = getenv("PWD");  /* get present working directory */
   if ( pwd != NULL )
   {
      if( Add_color )
         fprintf(stdout, BLKTEXT "PWD: %s\n" RESET, pwd);
      else
         fprintf(stdout, "PWD: %s\n", pwd);
   }

   if( Add_color ){

      fprintf(stdout, BLKTEXT "Prod Dir1: %s\n" RESET, Dir1);
      fprintf(stdout, BLKTEXT "Prod Dir2: %s\n" RESET, Dir2);

   }
   else {

      fprintf(stdout, "Prod Dir1: %s\n", Dir1);
      fprintf(stdout, "Prod Dir2: %s\n", Dir2);

   }

   if ( Exclusion_flag == 1 )
   {
      fprintf(stdout, "Products Excluded from Comparison:\n");
      for (int i = 0; i < Num_exclusions; i++)
      {
         prod_idx = ORPGPAT_get_prod_id_from_code(Prod_excl_arr[i]);
         temp_str = ORPGPAT_get_mnemonic( prod_idx );
         prod_mnemonic = strcpy(prod_mnemonic, temp_str);
         fprintf(stdout, "%d (%s) ", Prod_excl_arr[i], prod_mnemonic);
         if ( (i != 0) && (i%4 == 0) )
            fprintf(stdout, "\n");
      }
   }

   fprintf(stdout, "\n");
   if( Add_color ){

      fprintf(stdout, BLKTEXT "----------------    -------------    -----    -----------\n" RESET);
      fprintf(stdout, BLKTEXT "Prod Code ( ID )    Prod Mnemonic    Total    Miscompared\n" RESET);
      fprintf(stdout, BLKTEXT "----------------    -------------    -----    -----------\n" RESET);

   }
   else {

      fprintf(stdout, "----------------    -------------    -----    -----------\n" );
      fprintf(stdout, "Prod Code ( ID )    Prod Mnemonic    Total    Miscompared\n" );
      fprintf(stdout,  "----------------    -------------    -----    -----------\n" );

   }

   for ( prod_idx = 0; prod_idx < MAX_PROD_ID; prod_idx++)
   {
      if ( Summary_data[prod_idx].total_num > 0 )
      {
         prod_code = ORPGPAT_get_code(prod_idx);
         temp_str = ORPGPAT_get_mnemonic( prod_idx );
         prod_mnemonic = strcpy(prod_mnemonic, temp_str);
         if( Summary_data[prod_idx].num_miscompared == 0 ){

            if( Add_color )
               fprintf(stdout, GRNTEXT "%9d (%4d)    %13s    %5d    %11d\n" RESET, prod_code,
                       prod_idx, prod_mnemonic, Summary_data[prod_idx].total_num,
                       Summary_data[prod_idx].num_miscompared);
            else
               fprintf(stdout, "%9d (%4d)    %13s    %5d    %11d\n", prod_code,
                       prod_idx, prod_mnemonic, Summary_data[prod_idx].total_num,
                       Summary_data[prod_idx].num_miscompared);

         }
         else{

            if( Add_color )
               fprintf(stdout, REDTEXT "%9d (%4d)    %13s    %5d    %11d\n" RESET, prod_code,
                       prod_idx, prod_mnemonic, Summary_data[prod_idx].total_num,
                       Summary_data[prod_idx].num_miscompared);
            else
               fprintf(stdout, "%9d (%4d)    %13s    %5d    %11d\n", prod_code,
                       prod_idx, prod_mnemonic, Summary_data[prod_idx].total_num,
                       Summary_data[prod_idx].num_miscompared);

         }
      }
   }

   fprintf(stdout,
      "-------------------------------------------------------------------------------\n");
} /* end PrintSummaryData() */


/******************************************************************************
 * Description: This function prints a volume scan header
 *       Input: integer - volume scan number
 *              integer - volume date (Modified Julian)
 *              integer - volume time (seconds after midnight GMT)
 *      Output: Volume scan num, date/time information printed to stdout
 *      Return: None 
 *       Notes: 
 *****************************************************************************/
void PrintVolumeHeader(int vol_n, int vol_d, int vol_t)
{
   int err = 0;
   int yr, mo, day;
   int hr, min, sec, msec;
   unsigned int time_ms;  /* vol scan time converted to milliseconds */
   char output_time[9];

   /* Convert the volume date to a human readable format */
   err = RPGCS_julian_to_date( vol_d, &yr, &mo, &day );
   if ( err < 0 )
   {
     yr = 0;
     mo = 0;
     day = 0;
   }

   /* convert vol scan time to milliseconds */
   time_ms = vol_t * 1000;

   /* Convert the volume time to a human readable format */
   err = RPGCS_convert_radial_time( time_ms, &hr, &min, &sec, &msec);
   sprintf(output_time, "%02d:%02d:%02d", hr, min, sec);

   if( Add_color ){
      fprintf(stdout, "\n");
      fprintf(stdout, BLUTEXT
         "*******************************************************************************\n" RESET);
      fprintf(stdout, BLUTEXT
         "*                                                                              \n" RESET );
      fprintf(stdout, BLUTEXT
        "* Volume Scan Number, Date, Time: %d, %d/%d/%d, %s\n" RESET, vol_n, mo, day, yr,
        output_time);
      fprintf(stdout, BLUTEXT
          "*                                                                              \n" RESET);
      fprintf(stdout, BLUTEXT
          "*******************************************************************************\n" RESET);
   }
   else {
      fprintf(stdout, "\n");
      fprintf(stdout, 
         "*******************************************************************************\n");
      fprintf(stdout, 
         "*                                                                              \n" );
      fprintf(stdout, 
        "* Volume Scan Number, Date, Time: %d, %d/%d/%d, %s\n", vol_n, mo, day, yr,
        output_time);
      fprintf(stdout, 
          "*                                                                              \n");
      fprintf(stdout, 
          "*******************************************************************************\n");

   }
}

/******************************************************************************
 * Description: This function reads and stores the product exclusion list.
 *       Input: None
 *      Output: None
 *      Return: int - Number of exclusions upon success, -1 upon failure
 *       Notes: 
 *****************************************************************************/
static int ReadExclusionList()
{
   int ret_val = 0;
   FILE *file;
   int excl_val = 0 ; /* product code read from exclusion file */
   int stat = 0;      /* return status of function calls */
   int excl_cnt = 0;  /* count of exclusions */

   /* if exclusion option not chosen, return */
   if ( Exclusion_flag == 0 )
   {
      ret_val = 0;
   }
   else
   {
      /* open exclusion file */
      file = fopen(Prod_excl_file, "r");
      if ( file == NULL )
      {
         fprintf(stderr, "ReadExclusionList: can't open file %s\n",
            Prod_excl_file);
         ret_val = -1;
      }
      else
      {
         /* read contents and store into global array */
         while ((stat = fscanf(file, "%d", &excl_val)) != EOF)
         {
            /* store value in array */
            Prod_excl_arr[excl_cnt++] = excl_val;
         }

         ret_val = excl_cnt;

         /* close file */
         fclose (file);
      }
   }

   return ret_val;

} /* end ReadExclusionList() */


/******************************************************************************
 * Description: This function checks to see if the input product code is on
 *              the product exclusion list (to determine whether or not it should
 *              be excluded from the product comparison).  If so returns 1,
 *              else returns 0.
 *       Input: int - pr_code - the input product code
 *      Output: None
 *      Return: int - 0 if not to be excluded, 1 if is to be excluded 
 *       Notes: 
 *****************************************************************************/
static int ProdOnExclusionList(int pr_code)
{
   int ret_val = 0;

   /* if exclusion option not chosen, return */
   if ( Exclusion_flag == 0 )
   {
      ret_val = 0;
   }
   else
   {
      /* check to see if prod code is on the list */
      for (int i = 0; i < MAX_PROD_ID; i++ )
      {
         if ( pr_code == Prod_excl_arr[i] )
         {
            ret_val = 1;
            break;
         }
      }
   }

   return ret_val;

} /* end ProdOnExclusionList() */

/*******************************************************************

   Description:
      Returns the name of the first (dir_name != NULL) or the next 
      (dir_name = NULL) file in directory "dir_name" whose name matches 
      "basename".*. The caller provides the buffer "buf" of size 
      "buf_size" for returning the file name. 

   Inputs:  
      dir_name - directory name or NULL
      basename - product table base name
      buf - receiving buffer for next file name
      buf_size - size of receiving buffer

   Outputs:
      buf - contains next file name

   Returns:
      It returns 0 on success or -1 on failure.

*******************************************************************/
static int Get_next_file_name( char *dir_name, char *basename,
                               char *buf, int buf_size ){

    static DIR *dir = NULL;     /* the current open dir */
    static char saved_dirname[CFG_NAME_SIZE] = "";
    struct dirent *dp;

    /* If directory is not NULL, open directory. */
    if( dir_name != NULL ){

        int len;

        len = strlen (dir_name);
        if (len + 1 >= CFG_NAME_SIZE) {
            LE_send_msg (GL_ERROR,
                "dir name (%s) does not fit in tmp buffer\n", dir_name);
            return (-1);
        }
        strcpy (saved_dirname, dir_name);
        if (len == 0 || saved_dirname[len - 1] != '/')
            strcat (saved_dirname, "/");
        if (dir != NULL)
            closedir (dir);
        dir = opendir (dir_name);
        if (dir == NULL)
            return (-1);
    }

    if (dir == NULL)
        return (-1);

    /* Read the directory. */
    while ((dp = readdir (dir)) != NULL) {

        struct stat st;
        char fullpath[2 * CFG_NAME_SIZE];

        if (strncmp (basename, dp->d_name, strlen (basename)) != 0)
            continue;

        if (strlen (dp->d_name) >= CFG_NAME_SIZE) {
            LE_send_msg (GL_ERROR,
                "file name (%s) does not fit in tmp buffer\n", dp->d_name);
            continue;
        }
        strcpy (fullpath, saved_dirname);
        strcat (fullpath, dp->d_name);
        if (stat (fullpath, &st) < 0) {
            LE_send_msg (GL_ERROR,
                "stat (%s) failed, errno %d\n", fullpath, errno);
            continue;
        }
        if (!(st.st_mode & S_IFREG))    /* not a regular file */
            continue;

        if (strlen (fullpath) >= buf_size) {
            LE_send_msg (GL_ERROR,
                "caller's buffer is too small (for %s)\n", fullpath);
            continue;
        }
        strcpy (buf, fullpath);
        return (0);
    }

    return (-1);

/* End of Get_next_file_name() */
}

