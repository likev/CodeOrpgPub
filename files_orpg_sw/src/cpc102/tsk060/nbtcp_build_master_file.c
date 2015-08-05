/********************************************************************************

    File: build_master_file.c
          This file contains the routines that builds the master RPS file from
          the product_attr_table file

 ********************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 22:25:26 $
 * $Id: nbtcp_build_master_file.c,v 1.14 2014/11/07 22:25:26 steves Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */


#include <strings.h>
#include <stdlib.h>
#include <errno.h>

#include <infr.h>
#include <orpgpat.h>
#include <nbtcp.h>
#include <dirent.h>

#define NUMBER_ELEV_ANGLES         7
#define ELEV_INDEX                 2
#define PRODUCT_MASTER_FILE        "master_rps_list.dat" 
#define CFG_NAME_SIZE              128


static int Prod_dependent_params [MAX_FINAL_PROD_CODE][NUMBER_PROD_PARMS];
static char Cfg_dir[CFG_NAME_SIZE];     /* config directory pathname */
static char Product_tables_name[CFG_NAME_SIZE];
                                        /* file name for prod tables */
static char Product_tables_basename[CFG_NAME_SIZE];
                                        /* base file name for prod tables extensions */
static char Cfg_extensions_name[CFG_NAME_SIZE];
                                        /* directory name for prod tables extensions */


   /* local functions */

static int Set_config_file_names( );
static void Init_product_parameters ();
static int Get_table_file_name (char *name, char *dir, char *basename,
                                                char *tmp, int size);
static int Parse_pat( char *prod_table, FILE *master_file, short *prod_cnt );
static void Write_product (FILE *file, product_msg_info_t product);
static int Get_next_file_name( char *dir_name, char *basename,
                               char *buf, int buf_size );


/**************************************************************************

    Description: This function initializes the master RPS file using
                 the product_attr_table file.

    Input:  file directory where the product_attr_table and master RPS file
            are located

    Output:

    Return: The number of entries of the table on success
            or -1 on failure.

**************************************************************************/
int BMF_init_prod_file ( char *file_dir){

   short prod_cnt = 1;
   FILE *master_file;
   char rps_file [256];
   int err = 0;

   Init_product_parameters ();

   strcpy (rps_file, file_dir);
   strcat (rps_file, PRODUCT_MASTER_FILE);

   /* printf ("master rps file: %s\n", rps_file); */

   /* Open the master RPS file */
   if ((master_file = fopen (rps_file, "w")) == NULL){

      fprintf (stderr, "Error opening the master rps file (file: %s;  err: %d)\n", 
               PRODUCT_MASTER_FILE, errno);
      return (-1);
   }

   /* Initialize the configuration file names. */
   Set_config_file_names();

   /* Do the product_attr_table first. */
   err = Parse_pat( Product_tables_name, master_file, &prod_cnt );

   /* If no error, process all the product attribute table extensions. */
   if( err >= 0 ){

      char ext_name[ CFG_NAME_SIZE ], *call_name;

      call_name = Cfg_extensions_name;
      while( Get_next_file_name( call_name, Product_tables_basename,
                                 ext_name, CFG_NAME_SIZE ) == 0 ){

         if( (err = Parse_pat( ext_name, master_file, &prod_cnt )) < 0 ){

            fprintf( stderr, "Initialize Product Attribute Table Failed\n" );
            break;

         }
         call_name = NULL;

      }

   }

   /* Close RPS list. */
   fclose( master_file );
   
   return (prod_cnt);
}

/**************************************************************************
   Description: 
      This function initializes the configuration file names.

    Input:  
       NONE
           
    Output: 
       NONE

    Return: 
       It returns 0 on success or -1 on failure.

**************************************************************************/
static int Set_config_file_names( ){

    int err = 0;                /* error flag */
    int len;
    char tmpbuf [CFG_NAME_SIZE];

    /* Check to see if the configuration source (CFG_SRC) environmental
       variable is defined.  If so, make this the directory path to the
       various ASCII configuration files. */
    len = MISC_get_cfg_dir (Cfg_dir, CFG_NAME_SIZE);
    if (len > 0)
       strcat (Cfg_dir, "/");

    else
       sprintf( Cfg_dir, "./" );

    strcpy (Product_tables_name, "product_attr_table");
    strcpy (Product_tables_basename, "product_attr_table");
    strcpy (Cfg_extensions_name, "extensions");

    /* Process the Product Tables file. */
    strcpy (tmpbuf, Cfg_dir);
    strcat (tmpbuf, Product_tables_name);
    strcpy (Product_tables_name, tmpbuf);
    if (Get_table_file_name (Product_tables_name, Cfg_dir,
                             "product_attr_table", tmpbuf, CFG_NAME_SIZE) < 0)
       return (-1);

    /* Process the configuration extensions. */
    strcpy (tmpbuf, Cfg_dir);
    strcat (tmpbuf, Cfg_extensions_name);
    strcpy (Cfg_extensions_name, tmpbuf);

    return (err);

}

/******************************************************************

   Description:
    Searches in directory "dir" for file
    named "dir/basename" or "dir/basenames". If such a file is found,
    the full path of the file name is returned in "name". If it is
    not found, one of these names is returned. The buffer size
    of "name" are assumed to be big enough. "size" is the 
    buffer size of "tmp" which is assumed to be no less than the size
    of "name".

    Returns 0 on success or -1 if both files are found.
        
******************************************************************/
static int Get_table_file_name (char *name, char *dir, char *basename,
                                                char *tmp, int size) {
    int len, cnt;
    char *call_name;

    call_name = dir;
    len = strlen (name);
    cnt = 0;
    while (Get_next_file_name (call_name, basename,
                                                tmp, size) == 0) {
        if (strlen (tmp) == len)
            cnt++;
        else if (strlen (tmp) == len + 1 && tmp[len] == 's') {
            if (cnt == 0)
                strcat (name, "s");
            cnt++;
        }
        call_name = NULL;
    }
    if (cnt >= 2) {
        fprintf (stderr, "dupllicated %s found in %s", basename, dir);
        return (-1);
    }
    return (0);
}

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
            fprintf (stderr,
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
            fprintf (stderr,
                "file name (%s) does not fit in tmp buffer\n", dp->d_name);
            continue;
        }
        strcpy (fullpath, saved_dirname);
        strcat (fullpath, dp->d_name);
        if (stat (fullpath, &st) < 0) {
            fprintf (stderr,
                "stat (%s) failed, errno %d\n", fullpath, errno);
            continue;
        }
        if (!(st.st_mode & S_IFREG))    /* not a regular file */
            continue;

        if (strlen (fullpath) >= buf_size) {
            fprintf (stderr,
                "caller's buffer is too small (for %s)\n", fullpath);
            continue;
        }
        strcpy (buf, fullpath);
        return (0);
    }

    return (-1);

/* End of Get_next_file_name() */
}



/**************************************************************************

    Description: This function parses product attribute table files.

    Input:  file directory where the product_attr_table and master RPS file
            are located, and the current product count.

    Output:

    Return: The number of entries of the table on success
            or -1 on failure.

**************************************************************************/
static int Parse_pat( char *prod_table, FILE *master_file, short *prod_cnt ){

   short wx_modes;
   short prod_code;
   short type;
   short alert;
   char  desc [ORPGPAT_CS_DESCRIPTION_BUFSIZE];
   short param_index;
   int   err = 0;
   int   i;
   product_msg_info_t prod_msg;

   /* default elevations for product generation */
   int elevations[NUMBER_ELEV_ANGLES] = {5, 9, 13, 18, 24, 31, 40}; 

   /* printf ("CS_cfg_name: %s\n", CS_cfg_name (prod_table)); */
   CS_cfg_name (prod_table);

   CS_control (CS_COMMENT | '#');

   /*  Repeat for all the product definitions in the product_attributes   
       configuration file. */

   do {
      int len;
      int mne_len;

      if (CS_level (CS_DOWN_LEVEL) < 0)
         continue;

      if( (CS_entry (ORPGPAT_CS_PROD_CODE_KEY, ORPGPAT_CS_PROD_CODE_TOK, 
                     0, (void *)&prod_code) <= 0)
                              ||
          (CS_entry (ORPGPAT_CS_WX_MODES_KEY, ORPGPAT_CS_WX_MODES_TOK, 
                    0, (void *)&wx_modes) <= 0)
                              ||
          (CS_entry (ORPGPAT_CS_ALERT_KEY, ORPGPAT_CS_ALERT_TOK,
                     0, (void *)&alert) <= 0)
                              ||
          (CS_entry (ORPGPAT_CS_TYPE_KEY, ORPGPAT_CS_TYPE_TOK, 
                    0, (void *)&type) <= 0) ) {
         fprintf (stderr, "Error parsing keys\n" );
         err = 1;
         break;
      }

         /* skip the product if it is not a final product or if the 
            product is a demand-type product. */

      if ((prod_code < MIN_FINAL_PROD_CODE) 
                        || 
          (prod_code > MAX_FINAL_PROD_CODE)
                        || 
          (type == TYPE_ON_DEMAND))  {
              CS_level (CS_UP_LEVEL);
              continue;
      }

      memset (&prod_msg, 0, sizeof (product_msg_info_t));
      for (i = 0; i < NUMBER_PROD_PARMS; i++)  
         sprintf (prod_msg.params[i], "%s", PARAM_UNUSED_STRING);

      /* A new definition was read in so create a new table entry. */
      snprintf (prod_msg.prod_id, PROD_ID_LEN, "%d", *prod_cnt);
      snprintf (prod_msg.prod_code, PROD_CODE_LEN, "%d", prod_code); 

      /* snprintf (prod_msg.wx_modes, 2, "%d", wx_modes); */
      if ((len = CS_entry (ORPGPAT_CS_DESCRIPTION_KEY, ORPGPAT_CS_DESCRIPTION_TOK, 
                           ORPGPAT_CS_DESCRIPTION_BUFSIZE, (void *)desc)) < 0) {
         fprintf (stderr, "Error reading the desc key\n");
         err = 1;
         break;
      }

      mne_len = strcspn (desc, " ");

      if (mne_len > MNE_LEN) {
         strncpy (prod_msg.mne, desc, MNE_LEN);
         len -= MNE_LEN;
      }
      else {
         strncpy (prod_msg.mne, desc, mne_len);
         len -= mne_len;
      }

      if (len >= STRNG_LEN) { 
         strncpy (prod_msg.descrp, (char *) &desc[mne_len + 1], STRNG_LEN); 
         strcpy (&prod_msg.descrp[STRNG_LEN-1], "\0");
      }
      else {
         strncpy (prod_msg.descrp, (char *) &desc[mne_len + 1], len); 
         strcpy (&prod_msg.descrp[len-1], "\0");
      }

      /* read the product parameter list */
      CS_control (CS_KEY_OPTIONAL);

      if (CS_entry (ORPGPAT_CS_PARM_KEY, ORPGPAT_CS_PARM_TOK, 0, (void *)NULL) >= 0) {

         while (CS_entry (CS_NEXT_LINE, 0, 0, NULL) >= 0) {
            if (CS_entry (CS_THIS_LINE, ORPGPAT_CS_PARM_INDEX_TOK, 0,
                (void *)&param_index) <= 0)
                   break;

             /* initialize the product dependent parameters */
             snprintf (prod_msg.params[param_index], PROD_PARM_LEN, "%d", 
                       Prod_dependent_params[prod_code][param_index]);

         }
      }

      /* if this product is elevation based, then generate an entry 
         for each elevation angle spcified in the elevations array */
      if (type == 1) {
         
         for (i = 0; i < NUMBER_ELEV_ANGLES; i++)  {
            snprintf (prod_msg.params[ELEV_INDEX], PROD_PARM_LEN, "%d", elevations[i]);

            /* write the product to the master product file */
            Write_product (master_file, prod_msg);
            (*prod_cnt)++;
            snprintf (prod_msg.prod_id, PROD_ID_LEN, "%d", *prod_cnt);
         }
      }
      else {   /* write the product to the master product file */
         Write_product (master_file, prod_msg);
         (*prod_cnt)++;
      }

      CS_control (CS_KEY_REQUIRED);

      CS_level (CS_UP_LEVEL);

   } while (CS_entry (CS_NEXT_LINE, 0, 0, NULL) >= 0);

   if( err )
      return -1;

   return *prod_cnt;
}

/**************************************************************************

    Description: This function initializes the product dependent parameters

    Input:
  
    Output:

    Return: 

**************************************************************************/

static void Init_product_parameters ()
{
   Prod_dependent_params [31][0] = -1;    /* end time */
   Prod_dependent_params [31][1] = 24;   /* time span */

   Prod_dependent_params [34][0] = 0;     /* bit map */

   Prod_dependent_params [43][0] = 2700;  /* window azimuth */
   Prod_dependent_params [43][1] = 600;   /* center range of window */
   Prod_dependent_params [43][2] = 20;    /* center elevation angle */

   Prod_dependent_params [44][0] = 2700;  /* window azimuth */
   Prod_dependent_params [44][1] = 600;   /* center range of window */
   Prod_dependent_params [44][2] = 20;    /* center elevation angle */

   Prod_dependent_params [45][0] = 2700;  /* window azimuth */
   Prod_dependent_params [45][1] = 600;   /* center range of window */
   Prod_dependent_params [45][2] = 20;    /* center elevation angle */

   Prod_dependent_params [46][0] = 2700;  /* window azimuth */
   Prod_dependent_params [46][1] = 600;   /* center range of window */
   Prod_dependent_params [46][2] = 20;    /* center elevation angle */

   Prod_dependent_params [50][0] = 900;   /* pt 1 azmuth */
   Prod_dependent_params [50][1] = 300;   /* pt 1 range */
   Prod_dependent_params [50][2] = 1250;  /* pt 2 azimuth */
   Prod_dependent_params [50][3] = 600;   /* pt 2 range */

   Prod_dependent_params [51][0] = 900;   /* pt 1 azmuth */
   Prod_dependent_params [51][1] = 300;   /* pt 1 range */
   Prod_dependent_params [51][2] = 1250;  /* pt 2 azimuth */
   Prod_dependent_params [51][3] = 600;   /* pt 2 range */

   Prod_dependent_params [55][0] = 2250;  /* azimuth of window center */
   Prod_dependent_params [55][1] = 450;   /* range of window center */
   Prod_dependent_params [55][2] = 15;   /* elevation angle */
   Prod_dependent_params [55][3] = -1;    /* storm speed */
   Prod_dependent_params [55][4] = 1800;  /* storm direction */

   Prod_dependent_params [56][2] = 15;     /* elevation angle */
   Prod_dependent_params [56][3] = -1;    /* storm speed */
   Prod_dependent_params [56][4] = 1800;  /* storm direction */

   Prod_dependent_params [84][2] = 15;    /* altitude (k ft) */

   Prod_dependent_params [85][0] = 900;   /* pt 1 azmuth */
   Prod_dependent_params [85][1] = 300;   /* pt 1 range */
   Prod_dependent_params [85][2] = 1250;  /* pt 2 azimuth */
   Prod_dependent_params [85][3] = 600;   /* pt 2 range */

   Prod_dependent_params [86][0] = 900;   /* pt 1 azmuth */
   Prod_dependent_params [86][1] = 300;   /* pt 1 range */
   Prod_dependent_params [86][2] = 1250;  /* pt 2 azimuth */
   Prod_dependent_params [86][3] = 600;   /* pt 2 range */

   return;
}


/********************************************************************************

     Description: This routine writes the product to the master RPS file.

           Input:    file - the name of the master RPS file
                  product - the product info for each prodcut to write 
                            in the RPS file
          Output:

          Return:

 ********************************************************************************/

static void Write_product (FILE *file, product_msg_info_t product)
{
   int i;
   char tmp[10];
   char mne[10];
   char desc[10];
   char parm[10];
   char code[10];
   char id[10];

      /* build the format specifiers for each field for formatted output */

   memset (&tmp, 0, 10);
   sprintf (tmp, "%d", PROD_ID_LEN);
   strcpy (id, "%-\0");
   strcat (id, tmp);
   strcat (id, "s\0");

   memset (&tmp, 0, 10);
   sprintf (tmp, "%d", PROD_CODE_LEN);
   strcpy (code, "%-\0");
   strcat (code, tmp);
   strcat (code, "s\0");

   memset (&tmp, 0 ,10);
   sprintf (tmp, "%d", MNE_LEN);
   strcpy (mne, "%-\0");
   strcat (mne, tmp);
   strcat (mne, "s\0");

   memset (&tmp, 0 , 10);
   sprintf (tmp, "%d", STRNG_LEN);
   strcpy (desc, "%-\0");
   strcat (desc, tmp);
   strcat (desc, "s\0");

   memset (&tmp, 0, 10);
   sprintf (tmp, "%d", PROD_PARM_LEN);
   strcpy (parm, "%-\0");
   strcat (parm, tmp);
   strcat (parm, "s\0");

      /* write each field to the ouput file */

   fprintf (file, id, product.prod_id);
   fprintf (file, "%-s", " ");

   fprintf (file, code, product.prod_code);
   fprintf (file, "%-s", " ");

   fprintf (file, mne, product.mne); 
   fprintf (file, "%-s", " ");

   fprintf (file, desc, product.descrp); 
   fprintf (file, "%-s", " ");
/*
   fprintf (file, "%-2s", product.wx_modes);
   fprintf (file, "%-s", " ");
*/

   for (i = 0; i < NUMBER_PROD_PARMS; i++) {
      fprintf (file, parm, product.params[i]);
      fprintf (file, "%-s", " ");
   }

   fprintf (file, "%s", "\n");  /* terminate the record entry */

   return;
}
