/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/08/19 15:11:40 $
 * $Id: cvg.c,v 1.16 2009/08/19 15:11:40 ccalvert Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */
/* cvg.c  CODEview Graphics */

#include "cvg.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*  FOR SIGNAL HANDLING */
#include <signal.h>

/* CVG 9.1 */
char list_fname[256];
struct stat db_list_stat;
int res;

  /* adding fallback resources */
  /* eliminates need for resource file CVG */
  String fallbacks [] = 
  { 
  "*fontList: -*-lucida-medium-r-normal-*-11-80-*-*-p-*-iso8859-1,"
  "-*-lucidatypewriter-medium-r-normal-*-11-80-*-*-m-*-iso8859-1=tabfont,"
  "-*-lucida-medium-r-normal-*-14-100-*-*-p-*-iso8859-1=largefont",
      NULL
  };






int main(int argc, char* argv[]) 
{

  char *orpg_build_str; 

  Atom WM_DELETE_WINDOW;

/*   char *charval=NULL; */


int standalone_flag = FALSE;


/*    if((charval = getenv("STANDALONE_CVG")) != NULL)  */
/*        standalone_flag = TRUE; */
/*    else  */
/*        standalone_flag = FALSE; */



#ifdef LITTLE_ENDIAN_MACHINE
    fprintf(stderr, "\nThis CVG binary was compiled for the PC Linux Platform.\n");
#else
    fprintf(stderr, "\nThis CVG binary was compiled for the Solaris Sparc Platform.\n");
#endif



#ifdef LIB_LINK_STATIC
standalone_flag = TRUE;
#endif

#ifdef LIB_LINK_DYNAMIC
standalone_flag = FALSE;
#endif


  /* check presence of required libraries    */
  /* function has some dependency on cvg.mak */
  check_libraries(standalone_flag);

  init_prefs();
                                                  
  fprintf(stderr,"CVG START - FINISHED INITIALIZING PREFERENCES\n");


  if(standalone_flag == TRUE) {
      use_cvg_list_flag = prev_cvg_list_flag = TRUE;                
  } else {
      use_cvg_list_flag = prev_cvg_list_flag = FALSE;
  } 
  write_descript_source(use_cvg_list_flag);




  /*  should be moved to preferences!! */
  if((orpg_build_str = getenv("CV_ORPG_BUILD")) == NULL) {
        orpg_build_i = CVG_DEFAULT_BUILD;
        printf("Variable CV_ORPG_BUILD is not set.  CVG assumes default build %d\n", orpg_build_i);
  } else {
        orpg_build_i = atoi(orpg_build_str);  
        printf("CVG using variable CV_ORPG_BUILD to set ORPG Build to %d\n", orpg_build_i); 
  }
  write_orpg_build(orpg_build_i);



  shell = XtAppInitialize(&app,"CVG.RESOURCE",NULL,0,&argc,argv,fallbacks,
                             NULL,0);

  /* intercept the normal window menu 'Close' function */
  XtVaSetValues(shell, XmNdeleteResponse, XmDO_NOTHING, NULL);
  WM_DELETE_WINDOW = XInternAtom ( XtDisplay(shell), "WM_DELETE_WINDOW", FALSE );
  XmAddWMProtocolCallback ( shell, WM_DELETE_WINDOW, exit_callback, NULL );


/* set attrribute so radio buttons are always round */
/*  REQUIRED ON LINUX */
XtVaSetValues(XmGetXmDisplay(XtDisplay(shell)),
     XmNenableToggleVisual,     TRUE,
     NULL);


  
  XtVaSetValues(shell, 
		XmNtitle,  CVG_VERSION_STRING, 


#ifdef SUNOS
                XmNwidth,  615,
                XmNheight, 560,
#endif
#ifdef LINUX
                XmNwidth,  610,
                XmNheight, 570,
#endif


        XmNmwmDecorations,    MWM_DECOR_BORDER^MWM_DECOR_MINIMIZE^MWM_DECOR_TITLE^MWM_DECOR_MENU,
/*  limit available window functions */
        XmNmwmFunctions,      MWM_FUNC_CLOSE^MWM_FUNC_MINIMIZE^MWM_FUNC_MOVE,

        XmNx,         10,
        XmNy,         5, 

/* LINUX KDE ISSUE -  it wouold be convenient to allow the
   applicaion shell to be sized by the widgets by
       a. not specifying an initial size for the shell
       b. setting XmNallowShellResize to TRUE BEFORE widgets are managed
   however when using a SelectionBox with the application shell
   on Linux, KDE crashes when no ititial size is specified for the shell
*/
/*Also, when specifying an initial size for the application shell
    a warning that DialogStype should be XmDIALOG_MODELESS is provided
*/
/* Note: a combination of
       a. Initial size specified for shell
       b. setting XmNallowShellResize to TRUE AFTER managing all widgets
   results in the initial movement of the window, three row selection
   list, and a problem displaying all wigets
*/

/* XmNallowShellResize, TRUE, */
		NULL);
 

  XtRealizeWidget(shell);


  init_globals();
                                                
  fprintf(stderr,"CVG START - FINISHED INITIALIZING GLOBAL DATA\n");  
  
  load_prefs();


  /* CVG 9.1 - erase existing cvg product list to avoid potential problems */
  sprintf(list_fname, "%s/cvg_db_list.lb", config_dir); 
  if(stat(list_fname, &db_list_stat) == 0) {
    res = remove(list_fname);
    fprintf(stderr,"Removed existing cvg product list\n");
  }


  fprintf(stderr,"CVG START - FINISHED LOADING PREFERENCES\n");


/* /// CODE FOR BACKGROUND TASK /////// */

  new_pid = fork();

  switch(new_pid) {
    
      case  -1:  /* Error */
           perror("fork failed");
           exit(1);    
      case  0:  /* child */
         { /*  begin child block */
        
/*  future exec'd executable */
           execlp("cvg_read_db", "cvg_read_db", (char *)0);
/*             read_db(); */
            
         } /*  end child block */
           break;
      default:  /* parent */ 

      fprintf(stderr,"CVG START - READ_DATABASE PROCESS STARTED, PID=%d\n",
                        (int) new_pid);


                        
      init_db_prod_list();
/* DEBUG */
/* fprintf(stderr,"DEBUG CVG START - finished init_db_prod_list\n"); */
          
      setup_gui_display(); 
/* DEBUG */
/* fprintf(stderr,"DEBUG CVG START - finished setup_gui_display\n"); */

      XtAppAddTimeOut(app, 5000, build_list, NULL);          
          
      XtAppMainLoop(app);
    
  } /*  end switch */



/* ///////////////////////////////////////// */

/*   XtAppMainLoop(app); */

  return 0;
}


/* //////////////////////////////////////////////////////// */
/* //////////////////////////////////////////////////////// */

  /* check presence of required libraries    */
  /* function has some dependency on cvg.mak */
void check_libraries(int install_type)
{

char *home_dir;
  
struct stat st;
/* int result; */

char infr_filename[256];
char bzip2_filename[256];
char gd_filename[256];


/* // the needed filenames are dependent upon the cvg.mak file */
/* #ifdef LITTLE_ENDIAN_MACHINE */
/* sprintf(infr_filename, ""); */
/* sprintf(bzip2_filename, ""); */
/* sprintf(gd_filename, ""); */
/* #endif */


int all_available = TRUE;    


    /* if the HOME directory is not defined, EXIT */
    /* the HOME directory is also required for preferences */
    if((home_dir = getenv("HOME")) == NULL) {
        fprintf(stderr, "     ERROR*****************ERROR************ERROR*\n");
        fprintf(stderr, "     Environmental variable HOME not set.\n");
        fprintf(stderr, "          CVG launch aborted \n");
        fprintf(stderr, "     *********************************************\n");
        exit(0);
    }
  
  
  
    if(install_type == TRUE) { /*  STANDALONE   */
      
      fprintf(stderr,"\nCVG Compiled with the Standalone Installation.  This assumes\n");
      fprintf(stderr,"     all libraries except libz and libpng are statically linked.\n");
       /*  DO NOTHING, ALL SHOULD BE WELL though a run-time check for libpng and libz  */
       /*  should be accomplished if the export to png function is used */
       
    }  else { /*  NORMAL LOCAL INSTALLATION */

      /*  the needed filenames are dependent upon the cvg.mak file */
#ifdef LITTLE_ENDIAN_MACHINE
      sprintf(infr_filename, "%s/lib/lnux_x86/libinfr.so", home_dir);
      sprintf(bzip2_filename, "/usr/lib/libbz2.so"); 
      sprintf(gd_filename, "/usr/lib/libgd.so");
      
#else
      sprintf(infr_filename, "%s/lib/slrs_spk/libinfr.so", home_dir);
      /* CVG 9.1 - removed home directory */
      sprintf(bzip2_filename, /usr/local/lib/libbzip2.so"); 
      sprintf(gd_filename, /usr/local/lib/libgd.so");
      
#endif

      fprintf(stderr,"\n");
    
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
      
      if(stat(gd_filename, &st) < 0) {
          fprintf(stderr, " ERROR*****************ERROR************ERROR*\n");
          fprintf(stderr, " Library %s is not present. \n"
                          " Image cannot be exported to graphic file\n", gd_filename);
          
      }
      
      
      if(all_available == TRUE) {
          fprintf(stderr, " This version of CVG must be run within an ORPG account.\n");
          
      } else {
          fprintf(stderr, " \nThis version of CVG must be run within an ORPG account.\n");
          exit(0);
      }
     
    } /*  end NORMAL LOCAL INSTALLATION */
    
    
    
}



/* ///////////////////////////////////////////////////////// */
/* ///////////////////////////////////////////////////////// */

/* initializes key global variables to desirable values */
/* MUST BE CALLED BEFORE load_prefs                     */
void init_globals()
{
/*  for DEBUG */
/* anim_data *test_ad; */
  
  disp_width = DisplayWidth(XtDisplay(shell),
            XScreenNumberOfScreen(XtScreen(shell)));
  disp_height = DisplayHeight(XtDisplay(shell),
            XScreenNumberOfScreen(XtScreen(shell)));
  fprintf (stderr,"display screen width=%d, display screen height=%d\n", 
            disp_width, disp_height);    
            
/*  calculate initial position of product display screens and TAB screens */
/*  width & height are the size of the product display portal  */
/*                 in each product display screen */
/*  115 is the total width of the sidebar and window margins */
/*                 in each product display screen */
  spacexy = 10;
 
  screen1x = 20;
  screen1y = screen2y = 45;
  screen2x = disp_width - spacexy - width - 115;
  
  screen3x = screen3y = 200;
 
  tab1x = 50;
  tab1y = tab2y = disp_height - tabheight - spacexy;
  tab2x = disp_width - tabwidth - spacexy;

  tab3x = tab3y = 400;
    
  main_screen = screen_1 = screen_2 = NULL;
  screen_3 = NULL;
  
  scroll1_xpos = scroll1_ypos = scroll2_xpos = scroll2_ypos = 0;
  scroll3_xpos = scroll3_ypos = 0;
  
  selected_screen = SCREEN_1;

  map_flag1 = az_line_flag1 = range_ring_flag1 = FALSE;
  map_flag2 = az_line_flag2 = range_ring_flag2 = FALSE;  
  map_flag3 = az_line_flag3 = range_ring_flag3 = FALSE;


/*  defined in prefs.c */
/* #define ROAD_NONE 0 */
/* #define ROAD_MAJOR 12 */
/* #define ROAD_MORE 31  // NOTE: only upto 15 in CONUS maps */
  road_d = 31;    /*   */
/*  defined in prefs.c */
/* #define RAIL_NONE 70 */
/* #define RAIL_MAJOR 75 // NOTE: only upto 72 in CONUS maps                 */
  rail_d = 70;
/*  settings not yet defined   */
  admin_d = 0;   /*  eliminates all ( */

  co_d = TRUE;


  anim1.loop_size = anim2.loop_size = anim3.loop_size = 0;
  anim1.anim_type = anim2.anim_type = anim3.anim_type = 1;
  anim1.reset_ts_flag = anim2.reset_ts_flag = anim3.reset_ts_flag = ANIM_NO_INIT;

  anim1.reset_es_flag = anim2.reset_es_flag = anim3.reset_es_flag = ANIM_NO_INIT;
  anim1.reset_au_flag = anim2.reset_au_flag = anim3.reset_au_flag = ANIM_NO_INIT;

  anim1.lower_vol_num = anim2.lower_vol_num = anim3.lower_vol_num = -1;
  anim1.first_index_num = anim2.first_index_num = anim3.first_index_num = -1;
  anim1.last_index_num = anim2.last_index_num = anim3.last_index_num = -1;

  anim1.lower_vol_time = anim2.lower_vol_time = anim3.lower_vol_time = -1;
  anim1.upper_vol_time = anim2.upper_vol_time = anim3.upper_vol_time = -1;

  anim1.es_first_index = anim2.es_first_index = anim2.es_first_index = -1;


/*  PERSISTENT GLOBAL STRINGS */
    hide_xmstr = XmStringCreateLtoR("Hide Center Location Icon", XmFONTLIST_DEFAULT_TAG);
    show_xmstr = XmStringCreateLtoR("Show Center Location Icon", XmFONTLIST_DEFAULT_TAG);

/* DEBUG */
/* test_ad = &anim1; */
/* fprintf(stderr,"DEBUG init_globals, anim type is %d, first index num is %d\n", */
/*         test_ad->anim_type, test_ad->first_index_num); */

  sd1 = malloc(sizeof(screen_data));
  setup_default_screen_data_values(sd1);
  sd2 = malloc(sizeof(screen_data));
  setup_default_screen_data_values(sd2);
  sd3 = malloc(sizeof(screen_data));
  setup_default_screen_data_values(sd3);

  sd = sd1;
  
  disk_last_filename[0] = '\0';  
  
  
  /* DEFAULT SETTINGS, NOT INITITALIZATIONS */
  linked_flag = FALSE;
  prod_filter = FILTER_PROD_ID;
  area_label = AREA_LBL_NONE;
  area_symbol = AREA_SYM_POINT;
  area_line_type = AREA_LINE_SOLID;
  include_points_flag = FALSE;
  overlay_flag = verbose_flag = FALSE;
  /*CVG 9.0 */
  large_screen_flag = large_image_flag = FALSE;
  
  select_all_flag = TRUE; 
  ctr_loc_icon_visible_flag = FALSE;
 
}




