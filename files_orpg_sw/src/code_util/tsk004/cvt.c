/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/19 18:02:32 $
 * $Id: cvt.c,v 1.8 2014/03/19 18:02:32 jeffs Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/* cvt.c - graphic product ICD validation tool */


#include "cvt.h"

/* #include "cvt_dispatcher.h" */


/* GLOBALS */
/*  char *home_dir=NULL;    CVT 4.4 moved to cvt.h */
  



int main (int argc,char *argv[]) {


/* the subdirectory in the home account for exported files */
#define BASE_GRAPHIC_DIR "graphic"

int standalone_flag = FALSE;


  char *orpg_build_str; 
  struct stat confdirstat;
  /* CVT 4.4 */
  char config_dir[128];
  


#ifdef LITTLE_ENDIAN_MACHINE
    fprintf(stderr, "\nCVT binary was compiled for the PC Linux Platform.\n");
# else
    fprintf(stderr, "\nCVT binary was compiled for the Solaris Sparc Platform.\n");
#endif




  /* first, we get the home directory */
  if((home_dir = getenv("HOME")) == NULL) {
      fprintf(stderr, "Environmental variable HOME not set.\n");
      exit(0);
  }

  /* CVT 4.4 */
  /* make directory for placement of user prod configuratrion files */
  sprintf(config_dir, "%s/.cvt", home_dir);
  
  /* make sure the directory exists */
  if(stat(config_dir, &confdirstat) < 0) {
      /* if it doesn't, try to make it */
      if(mkdir(config_dir, 00755) < 0) { 
      /* if that fails, exit */
      fprintf(stderr, "Error in creating local configuration directory\n");  
      return 1; 
      }
  }




#ifdef LIB_LINK_STATIC
standalone_flag = TRUE;
#endif

#ifdef LIB_LINK_DYNAMIC
standalone_flag = FALSE;
#endif



  /* check presence of required libraries    */
  /* function has some dependency on cvt.mak */
  check_libraries(standalone_flag);



  /* define the complete path to the graphic directory */
  sprintf(graphic_dir, "%s/%s", home_dir, BASE_GRAPHIC_DIR);

  /* make sure the directory exists */
  if(stat(graphic_dir, &confdirstat) < 0) { 
      /* if it doesn't, try to make it */
      if(mkdir(graphic_dir, 00775) < 0) { 
	  /* if that fails, exit */
	  fprintf(stderr, "Error in creating default export directory\n");  
	  return 1; 
      }
  }

  fprintf(stderr,"%s\n\n",CV);

  if((orpg_build_str = getenv("CV_ORPG_BUILD")) == NULL) {
      orpg_build_i = 999;
      printf("Variable CV_ORPG_BUILD is not set.  CVT assumes Build 6 or later\n");
  } else {
      orpg_build_i = atoi(orpg_build_str);  
      printf("CVT using variable CV_ORPG_BUILD to set ORPG Build to %d\n", orpg_build_i); 
  }
  
  
  dispatch_tasks(argc,argv);


  printf("program complete\n");

  return 1;

}



/* //////////////////////////////////////////////////////// */

  /* check presence of required libraries    */
  /* function has some dependency on cvg.mak */
void check_libraries(int install_type)
{


  
struct stat st;
/* int result; */

char infr_filename[256];
char bzip2_filename[256];


int all_available = TRUE;    
 
  
    if(install_type == TRUE) { /*  STANDALONE   */
      
      fprintf(stderr,"CVT Compiled with the Standalone Installation.  This assumes\n");
      fprintf(stderr,"     all libraries are statically linked.\n");
       
    }  else { /*  NORMAL LOCAL INSTALLATION */

      /*  the needed filenames are dependent upon the cvg.mak file */
#ifdef LITTLE_ENDIAN_MACHINE
      sprintf(infr_filename, "%s/lib/lnux_x86/libinfr.so", home_dir);
      sprintf(bzip2_filename, "/usr/lib/libbz2.so"); /*  no orpg lib on Linux */
#else
      sprintf(infr_filename, "%s/lib/slrs_spk/libinfr.so", home_dir);
      sprintf(bzip2_filename,"%s/lib/slrs_spk/libbzip2.so", home_dir); 
#endif

    
      if(stat(infr_filename, &st) < 0) {
          fprintf(stderr, " ERROR*****************ERROR************ERROR\n");
          fprintf(stderr, " Required library %s is not present.\n", infr_filename);
          all_available = FALSE;
      }
      
      if(stat(bzip2_filename, &st) < 0) {
          fprintf(stderr, " ERROR*****************ERROR************ERROR*\n");
          fprintf(stderr, " Required library %s is not present.\n", bzip2_filename);
          all_available = FALSE;
      }
      
      if(all_available == TRUE) {

          fprintf(stderr, "This version of CVT must be run within an ORPG account.\n");
          
      } else {
          fprintf(stderr, "\nThis version of CVT must be run within an ORPG account.\n");
          exit(0);
      }
     
    } /*  end NORMAL LOCAL INSTALLATION */
    
    
    
}

