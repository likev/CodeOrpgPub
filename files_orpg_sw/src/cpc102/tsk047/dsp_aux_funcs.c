/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/09/27 18:00:08 $
 * $Id: dsp_aux_funcs.c,v 1.2 2011/09/27 18:00:08 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include <infr.h> 
#include <dsp_def.h> 

#define MAX_SEARCH_DEPTH 3	/* Maximum directory search depth */
static int Search_depth;	/* Current directory search depth */

static Ap_vol_file_t *Vol_files;	/* volume file list */
static int N_vol_files = 0;		/* size of array Vol_files */
static void *Vol_file_tblid = NULL;	/* Vol_files table ID */


/* local functions */
static void Search_files (char *d_name);
static void Sort_file_names (int n, Ap_vol_file_t *vf);
static int Is_less_than (Ap_vol_file_t *left, Ap_vol_file_t *right);
static void Complete_dir_path (char *dir_name);
static void Add_slash (char *path);
static void Add_pwd_path (char *path);
static void Add_default_path (char *path);
static void Add_a_new_file( char *dir, char *name, int file_size );
static char *Get_full_path (char *dir, char *name) ;


/******************************************************************

    Searches the volume file directory "dir_name" to get all 
    volume file names. The list of volume file found are returned
    with "vol_files". The return value is the number of volume files
    found.

******************************************************************/
int DSPAUX_search_files( char *d_name, Ap_vol_file_t **vol_files ){

   int i;

   for (i = 0; i < N_vol_files; i++)
      free (Vol_files[i].path);

   if (Vol_file_tblid != NULL)
      MISC_free_table (Vol_file_tblid);

   N_vol_files = 0;
   Vol_file_tblid = NULL;

   Complete_dir_path( d_name );
   Search_depth = 0;
   Search_files( d_name );
   Sort_file_names( N_vol_files, Vol_files );
   *vol_files = Vol_files;

   return (N_vol_files);

}

/******************************************************************

    Searches the file directory "dir_name" to get all product
    file names.

******************************************************************/
static void Search_files ( char *d_name ){

   DIR *dir;
   struct dirent *dp;

   Search_depth++;
   if( Search_depth > MAX_SEARCH_DEPTH )
      return;

   dir = opendir( d_name );
   if( dir == NULL ){

      fprintf (stderr, "opendir (%s) failed, errno %d\n", d_name, errno);
      return;

   }

   while( (dp = readdir (dir)) != NULL ){

      int ret, file_size;
      struct stat st;

      if (strcmp (dp->d_name, ".") == 0 || strcmp (dp->d_name, "..") == 0)
         continue;

      ret = stat( Get_full_path( d_name, dp->d_name ), &st );
      if (ret < 0) 
         continue;

      file_size = st.st_size;

      if( S_ISDIR (st.st_mode) ){	/* a directory */

         char full_name[LOCAL_NAME_SIZE + 4];
	 strcpy( full_name, Get_full_path( d_name, dp->d_name ) );
	 strcat( full_name, "/" );
	 Search_files( full_name );

      }
      else if( S_ISREG (st.st_mode) )
         Add_a_new_file( d_name, dp->d_name, file_size );

   }

   closedir (dir);
   Search_depth--;
   return;

}


/************************************************************************

    Adds a new entry to the volume file list.

************************************************************************/
static void Add_a_new_file( char *dir, char *name, int file_size ){

   char *cpt, *tok, datetime[32], delims[] = ".";
   Ap_vol_file_t *new_vf;
   int new_ind, i;

   if( Vol_file_tblid == NULL ){

      Vol_file_tblid = MISC_open_table( sizeof (Ap_vol_file_t), 32, 
	                                0, &N_vol_files, (char **)&Vol_files );
      if( Vol_file_tblid == NULL ){	/* malloc failed */

         fprintf (stderr, "MISC_open_table failed\n");
	 exit (1);

      }

   }

   cpt = malloc( strlen (Get_full_path (dir, name)) + 2 );
   new_vf = (Ap_vol_file_t *)MISC_table_new_entry (Vol_file_tblid, &new_ind);
   if( (new_vf == NULL) || (cpt == NULL) ){

      /* malloc failed */
      fprintf (stderr, "malloc failed\n");
      exit (1);

   }

   new_vf->path = cpt;
   strcpy (new_vf->path, Get_full_path (dir, name));
   new_vf->name = new_vf->path + strlen (dir);
   cpt += strlen (new_vf->path) + 1;
   new_vf->size = file_size;

   /* Get the file date/time from file name. */
   strcpy( datetime, new_vf->name );

   /* The first token is the ICAO. */
   tok = strtok( datetime, delims );

   /* Process the date/time tokens. */
   i = 0;
   strcpy( delims, "_" );
   while( tok != NULL ){

      tok = strtok( NULL, delims );
      if( tok != NULL ){

         sscanf( tok, "%d", &new_vf->datetime[i] );
         i++;

      }

   }

}

/************************************************************************

    Returns the full path of directory "dir" and file name "name".

************************************************************************/
static char *Get_full_path( char *dir, char *name ){

   static char buffer[LOCAL_NAME_SIZE] = "";

   if( strlen (dir) + strlen (name) >= LOCAL_NAME_SIZE ){

      fprintf (stderr, "Full path %s%s too long\n", dir, name);
      exit (1);

   }

   sprintf (buffer, "%s%s", dir, name);
   return (buffer);

}

/************************************************************************

    Completes the data directory path from possibly incomplete 
    "path". The output is in "path".  Dir_name is always terminated 
    with "/". 

************************************************************************/
static void Complete_dir_path( char *path ){

    Add_default_path( path );
    Add_pwd_path( path );
    Add_slash( path );

}

/************************************************************************

    Adds the default path in front of "path" if necessary.

************************************************************************/
static void Add_default_path (char *path) {

   /* If already absolute path, do nothing. */
   if( (path[0] == '/')
             || 
       (strcmp( path, "." ) == 0)
             || 
       (strncmp( path, "./", 2 ) == 0) )  
      return;

   Add_pwd_path( path );
}

/************************************************************************

    Adds the PWD path in front of "path" if necessary.

************************************************************************/
static void Add_pwd_path( char *path ){

   static char pwd[LOCAL_NAME_SIZE] = "";
   char tmp[LOCAL_NAME_SIZE], *cpt;

   if (path[0] == '/')	/* nothing to do */
      return;

   cpt = path;
   if( *cpt == '.' ){
      cpt++;

   if (*cpt == '/')
      cpt++;

   else if (*cpt != '\0')		/* not pwd */
       cpt = path;

   }

   strcpy (tmp, cpt);
   if( pwd[0] == '\0' ){

      if( getcwd( pwd, LOCAL_NAME_SIZE) == NULL ){ /* get current dir */

         fprintf (stderr, "getcwd failed (errno %d)\n", errno);
	 exit (1);

      }

      Add_slash (pwd);

   }

   if( strlen (pwd) + strlen (tmp) >= LOCAL_NAME_SIZE ){

      fprintf (stderr, "Path too long\n");
      exit (1);

   }

   strcpy (path, pwd);
   strcat (path, tmp);

}

/********************************************************************
			
    Appends "/" to "path" if the last character is not "/".

********************************************************************/
static void Add_slash (char *path) {

   if( path[strlen (path) - 1] != '/' ){

      if( strlen (path) + 1 >= LOCAL_NAME_SIZE ){

         fprintf (stderr, "Path too long\n");
	 exit (1);

      }

      strcat( path, "/" );

   }

}

/********************************************************************
			
    The Heapsort algorithm from "Numerical recipes in C". Refer to 
    the book. It sorts array "vf" of size "n" into ascending order.

********************************************************************/
static void Sort_file_names (int n, Ap_vol_file_t *vf){

    int l, j, ir, i;
    Ap_vol_file_t tvf;				/* type dependent */

    if (n <= 1)
	return;
    vf--;
    l = (n >> 1) + 1;
    ir = n;
    for (;;) {
	if (l > 1)
	    tvf = vf[--l];
	else {
	    tvf = vf[ir];
	    vf[ir] = vf[1];
	    if (--ir == 1) {
		vf[1] = tvf;
		return;
	    }
	}
	i = l;
	j = l << 1;
	while (j <= ir) {
	    if (j < ir && Is_less_than (vf + j, vf + j + 1))
						/* type dependent */
		++j;
	    if (Is_less_than (&tvf, vf + j)) {	/* type dependent */
		vf[i] = vf[j];
		j += (i = j);
	    }
	    else
		j = ir + 1;
	}
	vf[i] = tvf;
    }
}

/********************************************************************
			
    File name comparison function sorting the file names. "left"
    and "right" are the two name to compare. Returns 1 if "left" should
    be in front of "right" or 0 otherwise.

********************************************************************/
static int Is_less_than (Ap_vol_file_t *left, Ap_vol_file_t *right) {

   /* Do the comparisons. */
   if( left->datetime[0] < right->datetime[0] )
      return 1;

   else if( right->datetime[0] < left->datetime[0] )
      return 0;

   if( left->datetime[1] < right->datetime[1] )
      return 1;

   else if( right->datetime[1] < left->datetime[1] )
      return 0;

   if( left->datetime[2] < right->datetime[2] )
      return 1;

   else if( right->datetime[2] < left->datetime[2] )
      return 0;
     
   if( left->datetime[3] < right->datetime[3] )
      return 1;

   else if( right->datetime[3] < left->datetime[3] )
      return 0;

   return (0);
}

