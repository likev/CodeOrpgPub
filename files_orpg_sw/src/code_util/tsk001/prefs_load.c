/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/25 13:51:40 $
 * $Id: prefs_load.c,v 1.9 2014/03/25 13:51:40 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */
/* prefs_load.c */
/* loading preference data */

#include "prefs_load.h"

/* loads various run time preference information that is needed     */

/* checks existance of global configuration directory, if it does   */
/*    does not exist, exits with message to reinstall CVG           */
/*                                                                  */
/* loads from the local configuration directory, if it exists.      */
/*    otherwise, creates the local directory and copies the         */
/*    the configuration files from the global directory             */

/* 12/29/05 TJG - separated loading and editing preferences into a  */
/*                separate module                                   */



void init_prefs() {

  char *home_dir;
  struct stat confdirstat;
  struct stat mapfilestat;
  char global_config_cmd[511];
  
  char mapfilename[256];
  int create_samples;
  
  int ret;


/********************************************************************************/
  
  /* if the HOME directory is not defined, EXIT */
  if((home_dir = getenv("HOME")) == NULL) {
      fprintf(stderr, "     ERROR*****************ERROR************ERROR*\n");
      fprintf(stderr, "     Environmental variable HOME not set.\n");
      fprintf(stderr, "          CVG launch aborted \n");
      fprintf(stderr, "     *********************************************\n");
      exit(0);
  }

      /* set standard location */
      sprintf(global_config_dir,"/usr/local/share/%s",
              CVG_PREF_DIR_NAME);  
      sprintf(map_dir,"/usr/local/share/cvg_map");
              
  /* if the CVG_DEF_PREF_DIR variable is NOT defined, use the standard location */
  if((getenv("CVG_DEF_PREF_DIR")) == NULL) {  

      fprintf(stderr, "\n   * The standard location for the default preference\n");
      fprintf(stderr, "   * files is %s      \n", global_config_dir);
      fprintf(stderr, "\n");


  }
  else  {  /* print message and set according to value of variable */  

      fprintf(stderr,"     NOTE**********************************************\n"); 
      fprintf(stderr,"     * The variable CVG_DEF_PREF_DIR is being used to  \n");
      fprintf(stderr,"     * to specify the location for the default         \n");
      fprintf(stderr,"     * preference files.                               \n");
      fprintf(stderr,"     **************************************************\n");
      sprintf( global_config_dir,"%s/%s",(getenv("CVG_DEF_PREF_DIR")),
                                                              CVG_PREF_DIR_NAME );
      sprintf(map_dir, "%s/cvg_map" , (getenv("CVG_DEF_PREF_DIR")) );
  }

  if(verbose_flag)
      fprintf(stderr, "The global config directory is: %s\n", global_config_dir);

  /* If directory for the global default prefs does not exist, EXIT with message */
  if(stat(global_config_dir, &confdirstat) < 0) {
  
      fprintf(stderr,"\n");
      fprintf(stderr,"    *ERROR*****************ERROR******************ERROR*\n");
      fprintf(stderr,"    * Global configuration directory does not exist    *\n");
      fprintf(stderr,"    *                                                  *\n");
      fprintf(stderr,"    * 1.The value of CVG_DEF_PREF_DIR may be incorrect.*\n");
      fprintf(stderr,"    *                                                  *\n");
      fprintf(stderr,"    * 2.CVG may not have been correctly installed .    *\n");
      fprintf(stderr,"    *                                                  *\n");
      fprintf(stderr,"    * Have the system administrator or other           *\n");
      fprintf(stderr,"    *     knowledgeable individual reinstall CVG       *\n");
      fprintf(stderr,"    *            if needed.                            *\n");
      fprintf(stderr,"    ****************************************************\n");
      fprintf(stderr,"\n"); 
      exit(0);           
  }

  if(verbose_flag)
      fprintf(stderr, "Home directory: %s\n", home_dir);
      
  sprintf(config_dir, "%s/.%s", home_dir,CVG_PREF_DIR_NAME);

  if(verbose_flag)
      fprintf(stderr, "Config directory: %s\n", config_dir);

  /* make sure the local directory exists                        */
  /*    if the local configuration directory exists, do nothing  */
  /*                                                             */
  /*    if the directory does not exist, create it and copy the  */
  /*       default configuration files from the global directory */
  
  if(stat(config_dir, &confdirstat) < 0) {
     
      /* if it doesn't, try to make it */
      if(mkdir(config_dir, 00755) < 0) { 
      /* if that fails, exit */
      fprintf(stderr, "Error in creating local configuration directory\n");  
      return; 
      }
       
      /* copy global configuration files */

      fprintf(stderr, "\n");
      fprintf(stderr, "     NOTE*****************************************\n"); 
      fprintf(stderr, "     * CVG local config directory does not exist  \n"); 
      fprintf(stderr, "     *                                            \n"); 
      fprintf(stderr, "     *   CVG is copying the default configuration \n");
      fprintf(stderr, "     *   files into the  $HOME/.%s  directory     \n", 
                                                           CVG_PREF_DIR_NAME);
      fprintf(stderr, "     *********************************************\n");
      fprintf(stderr, "\n");    
      

      sprintf(global_config_cmd,"cp -R %s/.%s/* %s",
              global_config_dir, CVG_PREF_DIR_NAME, config_dir);
              
      ret = system(global_config_cmd);

      if(ret<0) {
        /* if that fails, exit */
          fprintf(stderr,"\n");
          fprintf(stderr,"    **********************************************\n");
          fprintf(stderr,"    * Error in copying global configuration files \n");
          fprintf(stderr,"    *                                             \n");
          fprintf(stderr,"    *    Check contents of %s    \n",global_config_dir);
          fprintf(stderr,"    *    Or,  check the contents of               \n");
          fprintf(stderr,"    *    $CVG_DEF_PREF_DIR/%s     if defined      \n", 
                                                               CVG_PREF_DIR_NAME);
          fprintf(stderr,"    *                                             \n");
          fprintf(stderr,"    *    Reinstall CVG if required                \n");
          fprintf(stderr,"    **********************************************\n");
          fprintf(stderr,"\n");
          exit(0); 
      }


      /*  ensures all top level config files can be modified */
      sprintf(global_config_cmd,"chmod a+w  %s/*", config_dir);
              
      ret = system(global_config_cmd);
      
      
  } /*  end if local config directory (config_dir) does not exist */



  /*  Always test for the presence of un-zipped map data files   */
  /*  This permits updating the map data without eliminating the */
  /*  local configuration directory.                             */

  /*  if this directory does not exist, create it!  */
  /*   this is normally created during a local installation */
  if(stat(map_dir, &mapfilestat) < 0) {
      sprintf(global_config_cmd,"mkdir -p %s", map_dir);
      
      ret = system(global_config_cmd);
      if(ret<0) {
          fprintf(stderr,"\n");
          fprintf(stderr,"    *******************************************\n");
          fprintf(stderr,"    * Error creating map directory '%s' \n",map_dir);
          fprintf(stderr,"    *******************************************\n");
      }

  } else { /*  directory exists so create sample maps if any of the */
           /*  bzipped map data files exist in the map directory */
      
      create_samples = FALSE;
      sprintf(mapfilename, "%s/us_map.dat.bz2", map_dir);
      if( (stat(mapfilename, &mapfilestat) == 0) ) 
          create_samples = TRUE;
      sprintf(mapfilename, "%s/ak_map.dat.bz2", map_dir);
      if( (stat(mapfilename, &mapfilestat) == 0) ) 
          create_samples = TRUE;
      sprintf(mapfilename, "%s/hi_map.dat.bz2", map_dir);
      if( (stat(mapfilename, &mapfilestat) == 0) ) 
          create_samples = TRUE;
          
      if(create_samples == TRUE) { 
          sprintf(global_config_cmd,"cd %s; create_sample_maps", map_dir);
          ret = system(global_config_cmd);
          if(ret<0) {
              fprintf(stderr,"\n");
              fprintf(stderr,"    ****************************************\n");
              fprintf(stderr,"    * Unable to create sample map files. \n");
              fprintf(stderr,"    ****************************************\n");
          }
      } /*  end if create_samples TRUE */
     
     
  } /*  end else map directory exists */


      
/* REsource file no longer needed, fallback resources defined in cvg.c */
/* and cvg_color_edit.c  */
/*   */ /* now check presence of X resource files */
/*   sprintf(Xresource_file, "%s/CVG", home_dir); */
/*   if(stat(Xresource_file, &confdirstat) < 0) { */
/*                                               */
/*     fprintf(stderr, "    **********************************************\n"); */
/*     fprintf(stderr, "    * NOTE:  X resource file was missing         *\n"); */
/*     fprintf(stderr, "    *                                            *\n"); */
/*     fprintf(stderr, "    *  Copying the X resourse files into         *\n"); */
/*     fprintf(stderr, "    *  the home directory.                       *\n"); */
/*     fprintf(stderr, "    **********************************************\n"); */
/*     sprintf(copy_Xresource_file, "cp %s/CVG %s/CVG", global_config_dir,  */
/*                                                                    home_dir); */
/*     ret = system(copy_Xresource_file);        */
/*     if(ret<0) {                               */
/*         fprintf(stderr,"\n");                */
/*         fprintf(stderr,"    ERROR****************ERROR*****************ERROR\n"); */
/*         fprintf(stderr,"    * Error in copying X resource file             *\n"); */
/*         fprintf(stderr,"    *                                              *\n");  */
/*         fprintf(stderr,"    * X resource file 'CVG' not present in home    *\n"); */
/*         fprintf(stderr,"    * directory.  This may result in font size     *\n"); */
/*         fprintf(stderr,"    * problems.                                    *\n"); */
/*         fprintf(stderr,"    *                                              *\n"); */
/*         fprintf(stderr,"    * Reinstall CVG if needed.                     *\n"); */
/*         fprintf(stderr,"    ******************************************* ****\n"); */
/*     }                                                                */
/*   }                                                                    */
/*     sprintf(Xresource_file, "%s/Cvg_color_edit", home_dir); */
/*   if(stat(Xresource_file, &confdirstat) < 0) { */
/*     fprintf(stderr, "\n     Copying X resource file 'Cvg_color_edit' \n"); */
/*     sprintf(copy_Xresource_file, "cp %s/Cvg_color_edit %s/Cvg_color_edit",  */
/*             global_config_dir, home_dir); */
/*     ret = system(copy_Xresource_file); */
/*     }                      */
                          
  


                          
} /* end init_prefs() */  
                          




                          
                          
/******************************************************************************/
/* after ensuring local preference files exist, load them */
void load_prefs()
{

    /* load whatever info we need right now */
    load_program_prefs(TRUE); /*  during initial load is TRUE */
    load_palette_list();

    load_resolution_info();   
  
    load_db_size(); 
    load_sort_method();


    /* get the name arrays ready */
    assoc_init_s(&product_names);
    assoc_init_s(&short_prod_names);
    assoc_init_s(&product_mnemonics);

    load_product_names(FALSE,TRUE); /*  during initial load is TRUE */
                                    /*  from child process is FALSE */


    /* initialize the list of product configuration data */
    assoc_init_i(&pid_list);
    /* the type message, e.g., geographic, non-geographic, unknown, etc. */
    assoc_init_i(&msg_type_list);
    /* CVG 9.1 - init new array for the packet 1 coord override feature */
    /* the packet 1 flag, i.e., Default 1/4 km coord, Override to sceen pixel coord */
    assoc_init_i(&packet_1_geo_coord_flag);
    /* CVG 9.1`- init new array of non-2d array packets with colors overriden */
    assoc_init_s(&override_palette);
    assoc_init_i(&override_packet);
    
    /* the horizontal resolution */
    assoc_init_i(&product_res);
    /* and the list of whether the the thresholds are stored in digital format */
    assoc_init_i(&digital_legend_flag);
    /* AND the legend information (for if it's a digital product) */
    assoc_init_s(&digital_legend_file);
    assoc_init_s(&dig_legend_file_2);
    /* AND the configured palette file(s) for this product */
    assoc_init_s(&configured_palette);
    assoc_init_s(&config_palette_2);
    /* AND the packet to apply this palette to */
    assoc_init_i(&associated_packet);
    /* AND the list of unit strings */
    assoc_init_s(&legend_units);
    /* AND the elevation flag */
    assoc_init_i(&elev_flag);

    load_product_info();


    /* set up list of RDA site IDs  (ICAOs) */
    assoc_init_s(&icao_list);
    /* as well as the list of the type of radar at that place */
    assoc_init_i(&radar_type_list);
   /* initialize list of radar names */
    assoc_init_s(&radar_names);

    load_site_info();
    load_radar_info();    

  
} /*  end load_prefs */







/* reads in information indexed by product ID for later reference */
void load_product_info()
{
    char  filename[150], buf[300]; /* CVG 9.1 - increased buf from 200 to 300 */
    char leg_file[60], leg_file2[60], pal_file[60], pal_file2[60];
    FILE *list_file;
    int   pid, pres, flag, packetid, msg_type, num_read, el, position;
    int pkt1; /* CVG 9.1 - added packet 1 coord override for geographic products */
    int i=0, j;
    
    /* CVG 9.1 - added override of colors for non-2d array packets */
    int override_pkt;
    char override_pal[60];

    /* CVG 9.1 - added the packet 1 coord override flag */
    /*           and the override palette and packete for */
    /*           non 2d array packets                     */
    /* the format of this file is on each line:
       <product_id> 
       <msg_type>   
       <pkt1_coord_flag>
       <resolution_index> 
       <override_palette>
       <override_packet>
       <digital_legend_flag> 
       <digital_legend_file1> 
       <digital_legend_file2>
       <configured_palette_file1> 
       <configured_palette_file2>
       <assoc_packet_type> 
       <legend_units_string> 
     */

/* DEBUG */
/*fprintf(stderr,"DEBUG - enterning load_product_info() \n"); */
       
    /* open the product configuration list */
    sprintf(filename, "%s/prod_config", config_dir);
    if((list_file=fopen(filename, "r"))==NULL) {
        fprintf(stderr, "***********************************\n");
        fprintf(stderr, "*  Could not open product info    *\n");
        fprintf(stderr, "*                                 *\n");
        fprintf(stderr, "*  The file 'prod_config' in the  *\n");
        fprintf(stderr, "*     ~/.cvgN.N directory is      *\n");
        fprintf(stderr, "*     either missing or corrupt   *\n");
        fprintf(stderr, "***********************************\n");
    exit(0);
    }

/* DEBUG */
/*fprintf(stderr,"DEBUG load_product_info - filename is %s \n", filename); */

    /* read in as much as we can */

/* DEBUG */
/* fprintf(stderr,"DEBUG Load Prod Info - entering loop\n"); */


                     
    while(feof(list_file) == 0) {  
        /* continue reading until we get to eof */

        /* read in one whole line */
        
        i++;
       
        read_to_eol(list_file, buf);  
        
/* CVG 9.1 - modified to permit comment lines - DOES NOT WORK */
/*        if(buf[0] == '#') {
            i--;
            continue;
        }
 */
         
/* DEBUG */
/*fprintf(stderr,"DEBUG - load_product_info(), line %d \n", i); */

        /* FUTURE ENHANCEMENT - ignore comment lines */
     
        /* all but final string */
        /* CVG 9.1 - added packet 1 coord override flag for geographic products */
        num_read = sscanf(buf, "%d%d%d%d%s%d%d%s%s%s%s%d%d%n", 
                          &pid, &msg_type, &pkt1, &pres, 
                          override_pal, &override_pkt,
                          &flag, leg_file, leg_file2, pal_file, pal_file2, 
                                            &packetid, &el, &position);

        /* get units string, the last parameter can include white space */
        /* skip initial white space */
        j=0;
        while(buf[position+j] == ' ') 
            j++;
            
        if(strlen(&buf[position+j])>=1)
            assoc_insert_s(legend_units, pid, &buf[position+j]);
        else 
            assoc_insert_s(legend_units, pid, " ");
                  


        if(num_read != EOF && num_read >=1) { /*  read all fields last line and */
                                              /*  at least the product id */
            /* CVG 9.1 - reading 10 parameters separately rather than 9 */
            /* if(num_read < 9) {  *//*  parameter 10 read separately */
            /* if(num_read < 10) { *//*  parameter 11 read separately */
            if(num_read < 13) {  /*  parameter 14 read separately */
                fprintf(stderr,"ERROR LOADING PREFERENCES FOR PRODUCT ID %d\n",
                                                                           pid);
                fprintf(stderr,"  At line %d in the product config file\n", i);
            }
 
/* DEBUG */
/* fprintf(stderr, "%d %d %d %d %s %d %d '%s' '%s' '%s' '%s' %d '%s' \n",         */
/*                                pid, msg_type, pkt1, pres,                      */
/*                                override_pal, override_pkt                      */
/*                                flag, leg_file, leg_file2, pal_file, pal_file2, */
/*                                                packetid, &buf[position+j]);    */
           
            
            /* the following tests for valid values must at least accept the
             * the option buttons defined in product_info_edit_window_callback()
             * in prefs.c.  It is not a problem if a few additional associated
             * packets are permitted without a warning message.  They will be
             * caught via the product_edit_fill_fields() function in prefs_edit.c
             *
             * Both the original default (not configured) legend/palette filenames
             * "." and the new defaults ".lgd" and ".plt" are accepted for 
             * backwards compatibility.  The new defaults are placed into the array.
             */
            
            
            if(num_read >= 1) {
                /*  test for valid integer  0-1999, 0 for potential intermed prods */
                if( !((pid >=0) && (pid <= 1999)) ) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,"ERROR in product ID (%d) at line %d "
                                   "of product preferences file\n", pid, i);
                }           
                /*  always insert pid, the list is used opening the edit window */
                assoc_insert_i(pid_list, pid, i); 
                
            }
            
            if(num_read >= 2) {
                /*  test for valid integer -1, 0-4, 999 */
                if( (msg_type != -1) && (msg_type != 999) &&
                    !((msg_type >=0) && (msg_type <= 4)) ) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,"ERROR in message type (%d) at line %d "
                                   "of product preferences file\n", msg_type, i);
                } else {
                    assoc_insert_i(msg_type_list, pid, msg_type); 
                }
            } else {
                assoc_insert_i(msg_type_list, pid, -1);
            }
                 
            /* CVG 9.1 - added reading the packet 1 coord override flag */
            if(num_read >= 3) {
                /*  test for valid integer 0 or 1 */
                if( (pkt1 != 0) && (pkt1 != 1) ) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,
                        "ERROR in packet 1 coord override flag (%d) at line %d "
                        "of product preferences file\n", pkt1, i);
                } else {
                    assoc_insert_i(packet_1_geo_coord_flag, pid, pkt1); 
                }
            } else {
                assoc_insert_i(packet_1_geo_coord_flag, pid, 0);
            }
            
            
            /* CVG 9.1 - changed 3 to 4 for adding the packet 1 coord flag */
            if(num_read >= 4) { 
                /*  test for valid integer -1, 0-13 */
                if( (pres != -1) &&
                    !( (pres >=0) && (pres <= 13) ) ) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,"ERROR in product resolution (%d) at line %d "
                                   "of product preferences file\n", pres, i);
                } else {
                    assoc_insert_i(product_res, pid, pres);
                }
            } else {
                assoc_insert_i(product_res, pid, -1);
            }
            
            
            
            
            
            /* CVG 9.1 - added non-2d array packet being overriden */
            if(num_read >= 5) {
                /*  test for .plt filename */
                if(check_filename("plt", override_pal)==FALSE) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,"ERROR in override palette filename (%s) at line "
                                   "%d of product preferences file\n", override_pal, i);
                } else {
                 /*  we convert the original default "." to the new default ".plt" */
                    if((strcmp(pal_file, ".")==0)) 
                        assoc_insert_s(override_palette, pid, ".plt");
                    else 
                        assoc_insert_s(override_palette, pid, override_pal);
                }
            } else {
                assoc_insert_s(override_palette, pid, ".plt");
            }            
            
            /* CVG 9.1 - added non-2d array packet being overriden */
            if(num_read >= 6) {
                /*  test for integer 0-15, 19-27, 42-46, 50-52 */
                if( !((override_pkt >=0) && (override_pkt <= 15)) &&
                    !((override_pkt >=19) && (override_pkt <= 27)) &&
                    !((override_pkt >=43) && (override_pkt <= 46)) &&
                    !((override_pkt >=50) && (override_pkt <= 52)) ) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,"ERROR in overriden packet (%d) at line %d "
                                   "of product preferences file\n", override_pkt, i);
                } else {
                    assoc_insert_i(override_packet, pid, override_pkt);
                }
            } else {
                assoc_insert_i(override_packet, pid, 0);
            }
            
/* DEBUG */
/*fprintf(stderr,"DEBUG load_product_info - override_pal '%s', override_pkt %d\n",*/
/*               override_pal, override_pkt);                                     */
            
            
            
            
            /* CVG 9.1 - changed 4 to 5 for adding the packet 1 coord flag */
            /* CVG 9.1 - changed 5 to 7 for adding plt override for non-2d pkt */
            if(num_read >= 7) { 
                /*  test for valid integer -1, 0-3 */
                if( (flag != -1) &&
                    !( (flag >=0) && (flag <= 6) ) ) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,"ERROR in legend flag (%d) at line %d "
                                   "of product preferences file\n", flag, i);
                } else {
                    assoc_insert_i(digital_legend_flag, pid, flag);
                }
            } else {
                assoc_insert_i(digital_legend_flag, pid, -1);
            }
            
            /* CVG 9.1 - changed 5 to 6 for adding the packet 1 coord flag */
            /* CVG 9.1 - changed 6 to 8 for adding plt override for non-2d pkt */
            if(num_read >= 8) {
                /*  test for .lgd filename */
                if(check_filename("lgd", leg_file)==FALSE) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,"ERROR in legend filename (%s) at line %d "
                                   "of product preferences file\n", leg_file, i);
                } else {
                 /*  we convert the original default "." to the new default ".lgd" */
                    if((strcmp(leg_file, ".")==0))
                        assoc_insert_s(digital_legend_file, pid, ".lgd");
                    else 
                        assoc_insert_s(digital_legend_file, pid, leg_file);
                }
                
            } else {
                assoc_insert_s(digital_legend_file, pid, ".lgd");
            }
            
            /* CVG 9.1 - changed 6 to 7 for adding the packet 1 coord flag */
            /* CVG 9.1 - changed 7 to 9 for adding plt override for non-2d pkt */
            if(num_read >= 9) { 
                /*  test for .lgd filename */
                if(check_filename("lgd", leg_file2)==FALSE) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,"ERROR in legend filename (%s) at line %d "
                                   "of product preferences file\n", leg_file2, i);
                } else {
                 /*  we convert the original default "." to the new default ".lgd" */
                    if((strcmp(leg_file2, ".")==0)) 
                        assoc_insert_s(dig_legend_file_2, pid, ".lgd");
                    else 
                        assoc_insert_s(dig_legend_file_2, pid, leg_file2);
                }
            } else {
                
                assoc_insert_s(dig_legend_file_2, pid, ".lgd");
            }
            
            /* CVG 9.1 - changed 7 to 8 for adding the packet 1 coord flag */
            /* CVG 9.1 - changed 8 to 10 for adding plt override for non-2d pkt */
            if(num_read >= 10) {
                /*  test for .plt filename */
                if(check_filename("plt", pal_file)==FALSE) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,"ERROR in palette filename (%s) at line %d "
                                   "of product preferences file\n", pal_file, i);
                } else {
                 /*  we convert the original default "." to the new default ".plt" */
                    if((strcmp(pal_file, ".")==0)) 
                        assoc_insert_s(configured_palette, pid, ".plt");
                    else 
                        assoc_insert_s(configured_palette, pid, pal_file);
                }
            } else {
                assoc_insert_s(configured_palette, pid, ".plt");
            }
            
            /* CVG 9.1 - changed 8 to 9 for adding the packet 1 coord flag */
            /* CVG 9.1 - changed 9 to 11 for adding plt override for non-2d pkt */
            if(num_read >= 11) {
                /*  test for .plt filename */
                if(check_filename("plt", pal_file2)==FALSE) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,"ERROR in palette filename (%s) at line %d "
                                   "of product preferences file\n", pal_file2, i);
                } else {
                 /*  we convert the original default "." to the new default ".plt" */
                    if((strcmp(pal_file2, ".")==0)) 
                        assoc_insert_s(config_palette_2, pid, ".plt");
                    else 
                        assoc_insert_s(config_palette_2, pid, pal_file2);
                }
                
            } else {
                assoc_insert_s(config_palette_2, pid, ".plt");
            }
            
            /* CVG 9.1 - changed 9 to 10 for adding the packet 1 coord flag */
            /* CVG 9.1 - changed 10 to 12 for adding plt override for non-2d pkt */
            if(num_read >= 12) {
                /*  test for integer 0-27, 41-46, 50-55 */
                if( !((packetid >=0) && (packetid <= 27)) &&
                    !((packetid >=41) && (packetid <= 46)) &&
                    !((packetid >=50) && (packetid <= 55)) ) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,"ERROR in associated packet (%d) at line %d "
                                   "of product preferences file\n", packetid, i);
                } else {
                    assoc_insert_i(associated_packet, pid, packetid);
                }
            } else {
                assoc_insert_i(associated_packet, pid, 0);
            }
                            
            /* CVG 9.3 - Added 13 for the elevation flag */
            if(num_read >= 13) {
                /*  test for 0 (not elevation-based) or 1 (elevation-based)*/
                if( !((el >=0) && (el <= 1))) {
                    fprintf(stderr,"\nERROR IN PRODUCT CONFIGURATION -----\n");
                    fprintf(stderr,"ERROR in elevation flag (%d) at line %d "
                                   "of product preferences file\n", el, i);
                } else {
                    assoc_insert_i(elev_flag, pid, el);
                }
            } else {
                assoc_insert_i(elev_flag, pid, 0);
            }                   
        } else {
            if(num_read >= 1)
              fprintf(stderr,"\nERROR LOADING PREFERENCES FOR PRODUCT ID %d\n\n",
                                                                             pid);
                         
        }

    
    } /*  end while */

    fclose(list_file);

} /*  end load_product_info */







/* reads in site dependant info for later use */
void load_site_info()
{
    char buf[150], filename[100], sub[50], icao[5];
    FILE *site_file;
    int i, j, site_id, radar_type;
    

    /* open the site info list */
    sprintf(filename, "%s/site_data", config_dir);
    if((site_file=fopen(filename, "r"))==NULL) {
        fprintf(stderr, "***********************************\n");
        fprintf(stderr, "*  Could not open site info data  *\n");
        fprintf(stderr, "*                                 *\n");
        fprintf(stderr, "*  The file 'site_data' in the    *\n");
        fprintf(stderr, "*     ~/.cvgN.N directory is      *\n");
        fprintf(stderr, "*     either missing or corrupt   *\n");
        fprintf(stderr, "***********************************\n");
    exit(0);
    }

    /* the file is structured such that each line is a seperate record:
     * <site_id> <radar_type> <icao>
     */
     
/*  future improvement, read a line at a time to handle corrupted entries */
/*                      see loading of product preferences */

    while(feof(site_file) == 0) {  /* continue reading until we get to eof */
        /* read in one whole line */
        read_to_eol(site_file, buf);

        /* skip over any initial spaces */
        i=0;
        while(buf[i] == ' ') i++;
    
        /* read in an integer (the site id) */
        j=0;
        while(isdigit((int)(buf[i]))) 
            sub[j++] = buf[i++];
        sub[j] = '\0';
        site_id = atoi(sub);
        
/*  future improvement, give warning on out of range ID's */
    
        /* if we couldn't read in any number, then stop parsing */
        if(j==0)
          break;
    
        /* skip any more spaces */
        while(buf[i] == ' ') i++;
    
        /* read in an integer (the radar type index) */
        j=0;
        while(isdigit((int)(buf[i]))) 
            sub[j++] = buf[i++];
        sub[j] = '\0';
        radar_type = atoi(sub);
        
/*  future improvement, give warning on out of range type indexes */
    
        assoc_insert_i(radar_type_list, site_id, radar_type);
    
        /* skip any more spaces */
        while(buf[i] == ' ') i++;
    
        /* read in the next up to four letters as the ICAO */
        j = 0;
/*         while(isalpha((int)(buf[i])) && j < 4) */
        while(isalnum((int)(buf[i])) && j < 4)
          icao[j++] = buf[i++];
        icao[j] = '\0';
    
        assoc_insert_s(icao_list, site_id, icao);
    
    } /*  end while not EOF */

    fclose(site_file);

} /*  end load_site_info */







/* loads a bunch of keyword/value pairs */
void load_program_prefs(int initial_read)
{
    char buf1[100], buf2[100], filename[150], *charval;
    char rr_buf[10], az_buf[10], map_buf[10], vb_buf[10];
    /* CVG 9.0 */
    char scr_buf[10], img_buf[10];

    FILE *pref_file;
    
char line_buf[200];
int num_read;

int db_name_configured = FALSE;
int db_variable_set = FALSE;

/*  FUTURE IMPROVEMENT: AFTER PROVIDING WARNING MESSAGES, RECOVER */
/*  FROM VARIOUS FAILURES BY CREATING DEFAULT PREFERENCE FILE */

/* DEBUG */
/* fprintf(stderr,"DEBUG - enterning load_program_prefs()\n"); */

    /* default the display attributes to false */
    def_ring_val = def_map_val = def_line_val = FALSE;

    /* open the program preferences data file */
    sprintf(filename, "%s/prefs", config_dir);
    if((pref_file=fopen(filename, "r"))==NULL) {
        fprintf(stderr, "**********************************\n");
        fprintf(stderr, "*  Could not open preferences    *\n");
        fprintf(stderr, "*                                *\n");
        fprintf(stderr, "*  The file 'prefs' in the       *\n");
        fprintf(stderr, "*     ~/.cvgN.N directory is     *\n");
        fprintf(stderr, "*     either missing or corrupt  *\n");
        fprintf(stderr, "**********************************\n");
    exit(0);
    }

    /* CREATE A DEFAULT PRODUCT DATABASE FILENAME TO USE IF NOT DEFINED IN PREFS */
    /* get the path for the products database */
    charval = getenv("ORPG_PRODUCTS_DATABASE");

   
    if(charval == NULL) { /* variable not set, default is blank */
        product_database_filename[0] = '\0';          
#ifdef LIB_LINK_DYNAMIC              
        fprintf(stderr,"WARNING: The environmental variable for ");
        fprintf(stderr,"ORPG_PRODUCTS_DATABASE is not set\n");
#endif        

    } else { /* variable is set, see if file exists */
        
        db_variable_set = TRUE;
        printf("(Main Task) ORPG_PRODUCTS_DATABASE (normal ORPG location) is:\n");
        printf("            '%s'\n",charval);
   
        if(check_for_directory_existence(charval) == FALSE) {

            fprintf(stderr,"Note:The ORPG_PRODUCTS_DATABASE file was not found\n");
            fprintf(stderr,"  If in a configured ORPG account, either the file\n");
            fprintf(stderr,"  is missing (start the ORPG to create the linear \n");
            fprintf(stderr,"  buffer) or a configuration error has been made. \n");
        } 
        
        /* make it a default value */
        strcpy(product_database_filename, charval);
        
        
    } /*  end else not NULL */

/* DEBUG */
/* fprintf(stderr,"DEBUG load_prod_prefs - default db filename is %s\n",  */
/*                                             product_database_filename); */



    /* CREATE A DEFAULT BACKGROUND MAP FILENAME TO USE IF NOT DEFINED IN PREFS */
    charval = getenv("HOME");

   if(charval == NULL) { /* variable not set, default is blank */
        map_filename[0] = '\0';
        if(verbose_flag)
            printf("ERROR: (PREFS) environmental variable for HOME is not set\n");

    } else { /* variable is set, create default map path */

        sprintf(map_filename, "%s/kmlb_cvg250_lnux.map", map_dir);
        
        /*  ensure ~/tools/data directory exists */

        if(check_for_directory_existence(map_dir) == FALSE) {
            /* tell user if file does not exist */
            fprintf(stderr,"NOTE: (PREFS) The standard location for mapfiles, the \n");
            fprintf(stderr,"      %s directory, does not exist.\n", map_dir);
        }
        
    } /*  end else not NULL */



  /* CREATE DEFAULTS FOR THE FLAGS IN CASE THEY ARE NOT DEFINED IN THE PREFS */
  strcpy(rr_buf, "false");
  strcpy(az_buf, "false");
  strcpy(map_buf, "false");
  strcpy(vb_buf, "false");
  /* CVG 9.0 */
  strcpy(scr_buf, "small");
  strcpy(img_buf, "small");

  /* now read the basic prefs file, override filename defaults if defined */
    /*  RECOVERS FROM VARIOUS FAILURES BY WRITING DEFAULT VALUES FOR */
    /*           ANY ITEM NOT DEFINED */

    while(feof(pref_file) == 0) {

        read_to_eol(pref_file, line_buf);
    
        num_read = sscanf(line_buf, "%s%s", buf1, buf2);
    
    
        if(verbose_flag)
            printf("Read Pref   %s: %s\n", buf1, buf2);
            
        if(strcmp(buf1, "map_file") == 0) {
            if(num_read == 2)
                strcpy(map_filename, buf2);
            
        } else if(strcmp(buf1, "range_ring_on") == 0) {
            if(num_read == 2) {
                if(strcmp(buf2, "true") == 0) { 
                    if(screen_1 == NULL)
                       range_ring_flag1 = TRUE;
                    if(screen_2 == NULL)
                       range_ring_flag2 = TRUE;
                    if(screen_3 == NULL)
                       range_ring_flag3 = TRUE;
                    def_ring_val = TRUE;
                    strcpy(rr_buf, buf2);
                } else {
                    def_ring_val = FALSE;
                }
            }
            
        } else if(strcmp(buf1, "azimuth_line_on") == 0) {
            if(num_read == 2) {
                if(strcmp(buf2, "true") == 0) { 
                    if(screen_1 == NULL)
                       az_line_flag1 = TRUE;
                    if(screen_2 == NULL)
                       az_line_flag2 = TRUE;
                    if(screen_3 == NULL)
                       az_line_flag3 = TRUE;
                    def_line_val = TRUE;
                    strcpy(az_buf, buf2);
                } else {
                    def_line_val = FALSE;
                }   
            }
            
        } else if(strcmp(buf1, "map_bkgd_on") == 0) {
            if(num_read == 2) {
                if(strcmp(buf2, "true") == 0) { 
                    if(screen_1 == NULL)
                       map_flag1 = TRUE;
                    if(screen_2 == NULL)
                       map_flag2 = TRUE;
                    if(screen_3 == NULL)
                       map_flag3 = TRUE;
                    def_map_val = TRUE;
                    strcpy(map_buf, buf2);
                } else {
                    def_map_val = FALSE;
                    
                }   
            }
            
        } else if(strcmp(buf1, "verbose_output") == 0) {
            if(num_read == 2) {
                if(strcmp(buf2, "true") == 0) { 
                    verbose_flag = TRUE;
                    strcpy(vb_buf, buf2);
                } else {
                    verbose_flag = FALSE;
                }   
            }
            
        /* CVG 9.0 */
        } else if(strcmp(buf1, "screen_size") == 0) {
            if(num_read == 2) {
                if(strcmp(buf2, "large") == 0) { 
                    large_screen_flag = TRUE;
                    strcpy(scr_buf, buf2);
                } else {
                    large_screen_flag = FALSE;
                }   
            }
        
        /* CVG 9.0 */
        } else if(strcmp(buf1, "image_size") == 0) {
            if(num_read == 2) {
                if(strcmp(buf2, "large") == 0) { 
                    large_image_flag = TRUE;
                    strcpy(img_buf, buf2);
                } else {
                    large_image_flag = FALSE;
                }   
            }
        
        } else if(strcmp(buf1, "product_database_lb") == 0) {
            if(num_read == 2) {
                db_name_configured = TRUE;
                strcpy(product_database_filename, buf2);                
            }
/* DEBUG */
/* fprintf(stderr,"DEBUG load_prod_prefs - loaded db filename is %s\n",  */
/*                                               product_database_filename); */
            
        } else {
            if(verbose_flag)
                printf("WARNING: Unknown key in cvg program preferences 'prefs' file.");
        } 

    } /*  end while */

    if(initial_read == TRUE)
        strcpy(prev_db_filename, product_database_filename);


    fclose(pref_file);  /*  was read only */

    if( (db_variable_set==FALSE) && (db_name_configured==FALSE) ) {
        fprintf(stderr,"  ******************************************\n");
        fprintf(stderr,"  *  A database file name must be manually *\n");
        fprintf(stderr,"  *  selected via the Preferences menu.    *\n");
        fprintf(stderr,"  ******************************************\n");
        
    }

    /* rewrite prefs in the case the database filename was blank, */
    /*         this gets the default database filename for an ORPG  */
    /*         account in the file for the background task to read! */
    /*         this also corrects corrupted files with default entries. */

    if((pref_file=fopen(filename, "w"))==NULL) {
        fprintf(stderr, "Could not open program preferences\n");
    exit(0);
    }

    /* CVG 9.0 - added scr_buf and img_buf parameters */
    write_prefs_file(pref_file, map_filename, rr_buf, az_buf, map_buf, 
                     vb_buf, scr_buf, img_buf, product_database_filename);


    fflush(pref_file);
    fclose(pref_file);

} /*  end load_program_prefs */








/* updates the contents of the CVG prefs file, called after loading and after edit
 * assumes the prefs file opened for writing.
 */
/* CVG 9.0 - added scr_sz and img_sz parameters */
void write_prefs_file(FILE *p_file, char *m_file, char *rr, char *az, 
                     char *map, char *vb, char *scr_sz, char *img_sz, char *db_file)
{

          fprintf(p_file, "map_file %s\n", m_file);
          fprintf(p_file, "range_ring_on %s\n", rr);
          fprintf(p_file, "azimuth_line_on %s\n", az);
          fprintf(p_file, "map_bkgd_on %s\n", map);
          fprintf(p_file, "verbose_output %s\n", vb);
          fprintf(p_file, "screen_size %s\n", scr_sz);
          fprintf(p_file, "image_size %s\n", img_sz);
          fprintf(p_file, "product_database_lb %s", db_file);

} /*  end write_prefs_file */









/* loads information about the different radar types that
 * can be associated with each site
 */
void load_radar_info()
{
    char buf[200], filename[150];
    FILE *radar_file;
    int index;

    /* open the program preferences data file */
    sprintf(filename, "%s/radar_info", config_dir);
    if((radar_file=fopen(filename, "r"))==NULL) {
        fprintf(stderr, "***********************************\n");
        fprintf(stderr, "*  Could not open radar info data *\n");
        fprintf(stderr, "*                                 *\n");
        fprintf(stderr, "*  The file 'radar_info' in the   *\n");
        fprintf(stderr, "*     ~/.cvgN.N directory is      *\n");
        fprintf(stderr, "*     either missing or corrupt   *\n");
        fprintf(stderr, "***********************************\n");
        exit(0);
    }

    /* initialize list of radar names */

    while(feof(radar_file) == 0) {  /* continue reading until we get to eof */
      /* read in index to radar data, then the name of the radar */
      if(fscanf(radar_file, "%d", &index) != 1)
          continue;
      read_to_eol(radar_file, buf);
  
      assoc_insert_s(radar_names, index, buf);
  
      /*printf("Loaded radar: %d, %s\n", index, buf);*/
    }

    fclose(radar_file);

} /*  end load_radar_info */





/* loads information about the different resolutions that
 * can be associated with each site
 */
void load_resolution_info()
{
    char buf[200], filename[150];
    FILE *res_file;
    int i;
    float the_res;

    /* open the program preferences data file */
    sprintf(filename, "%s/resolutions", config_dir);
    if((res_file=fopen(filename, "r"))==NULL) {
        fprintf(stderr, "***********************************\n");
        fprintf(stderr, "*  Could not open resolution data *\n");
        fprintf(stderr, "*                                 *\n");
        fprintf(stderr, "*  The file 'resolutions' in the  *\n");
        fprintf(stderr, "*     ~/.cvgN.N directory is      *\n");
        fprintf(stderr, "*     either missing or corrupt   *\n");
        fprintf(stderr, "***********************************\n");
    exit(0);
    }

    /* get the number of possible resolutions */
    fscanf(res_file, "%d", &number_of_resolutions);

    /* initialize lists of resolution data */
    if( (resolution_number_list=malloc(sizeof(float)*number_of_resolutions)) == NULL) {
      number_of_resolutions = 0;
      return;
    }
    if( (resolution_name_list=malloc(sizeof(float)*number_of_resolutions)) == NULL) {
      number_of_resolutions = 0;
      return;
    }


    i=0;
    while(feof(res_file) == 0) {  /* continue reading until we get to eof */
      /* read in resolution size, then name */
    if(fscanf(res_file, "%f", &the_res) != 1)
        continue;
    read_to_eol(res_file, buf);
    
    if( (resolution_name_list[i]=malloc(sizeof(char)*(strlen(buf)+1))) == NULL) {
        number_of_resolutions = 0;
        return;
    }
    strcpy(resolution_name_list[i], buf);
    resolution_number_list[i] = the_res;
    i++;
    }

    fclose(res_file);

} /*  end load_resolution_info */


/*****************************************************************/





/*  no load function provided, cvg begins with the default based upon */
/*  an environmental variable */
void write_orpg_build(int build)
{
    char filename[150];
    FILE *build_file;
 
    /* open the program preferences data file */
    sprintf(filename, "%s/orpg_build", config_dir);

    if((build_file=fopen(filename, "w"))==NULL) {
        fprintf(stderr, "***********************************\n");
        fprintf(stderr, "*  Could not open orpg build file *\n");
        fprintf(stderr, "*                                 *\n");
        fprintf(stderr, "***********************************\n");
    exit(0);
    }

    fprintf(build_file,"%d", build);

    fflush(build_file);
    fclose(build_file);

}








/*****************************************************************/

void write_sort_method(int method)
{
    char filename[150];
    FILE *sort_meth_file;
 
    /* open the program preferences data file */
    sprintf(filename, "%s/sort_method", config_dir);

    if((sort_meth_file=fopen(filename, "w"))==NULL) {
        fprintf(stderr, "************************************\n");
        fprintf(stderr, "*  Could not open sort method file *\n");
        fprintf(stderr, "*                                  *\n");
        fprintf(stderr, "************************************\n");
    exit(0);
    }

    fprintf(sort_meth_file,"%d", method);

    fflush(sort_meth_file);
    fclose(sort_meth_file);

}





/* ///////////////////////////////////////////////////////////////// */

void load_sort_method()
{
    char filename[150];
    FILE *sort_meth_file;
    
 
    /* open the sort method file */
    sprintf(filename, "%s/sort_method", config_dir);

    if((sort_meth_file=fopen(filename, "r"))==NULL) {
        fprintf(stderr, "************************************\n");
        fprintf(stderr, "*  Could not open sort method file *\n");
        fprintf(stderr, "*  Using default volume date-time. *\n");
        fprintf(stderr, "************************************\n");
    sort_method = 1;
    
    } else {

        fscanf(sort_meth_file, "%d", &sort_method);
        
        prev_sort_method = sort_method;
        
        if(sort_method==1)
            fprintf(stderr,"CVG sorting products using volume date-time\n");
        else
            fprintf(stderr,"CVG sorting products using volume sequence number\n");

        fclose(sort_meth_file);

    }

}








/****************************************************************/

/* checks to see if a file has been added to alter the 
 * product database list data structure size
 */
void load_db_size()
{
    char filename[150];
    FILE *db_size_file;
    int value;

    /* open the database size data file */
    sprintf(filename, "%s/prod_db_size", config_dir);
    
    if((db_size_file=fopen(filename, "r"))==NULL) {
        
        fprintf(stderr, "CVG Product DB List Size 16000 (default)\n");
        maxProducts=DEFAULT_DB_SIZE;
        
    } else {

          fscanf(db_size_file, "%d", &value);
          /* CVG 9.2 - changed minimum db size from 12000 to 18000 */
          if(value < 18000)
              maxProducts=18000; 
          else if(value > 32000)
              maxProducts=32000;
          else
              maxProducts=value;
          fprintf(stderr, "CVG Product DB List Size %d\n", maxProducts);
            
          fclose(db_size_file);
      
    }

    
}

 
 

/* //////////////////////////////////////////////////////////////////////// */
/* //////////////////////////////////////////////////////////////////////// */



