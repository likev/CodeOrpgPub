/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:35 $
 * $Id: cvg_map.c,v 1.8 2009/05/15 17:52:35 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */
/* map.c */
/* functions for displaying a geographical map file on the "digital canvas" */

#include "map.h"





/*  used in display_map() and draw_map_line() */
int   first_point = TRUE;
int     feature_type=-1;
int     prev_feature=-1; 
screen_data *mapsd=NULL;
/*  rank is used by decode.c to limit data file size and */
/*               by cvg_map.c to control amount displayed  */
int feature_rank=-1;
int   pixel1, scanl1; 
int   pixel2=0;
int   scanl2=0;


static int use_cvg_colors = FALSE;   
   
   
   
   
/* plot map overlay file on previously selected image. 
 */
/* 
 * the OLD input file is in USGS DLG (?) format, and is composed of vectors 
 * on a geographic grid with special encoded lat/lon. The first point of
 * a vector is the complet Lat/Lon, the following points are deltas
 *
 * the NEW input file includes a flag Lat/Lon value between vectors and the
 * points are Lat/Longs represented in character strings.  This MAY be changed
 * to using floating point numbers or scaled integers for Lat/Long to
 * reduce the file size
 */
/*
 * we load these vectors, and taking the geographic location of the radar 
 * antenna or the selected alternate location to be the center of the screen, 
 * project the vectors on to the digital canvas
 */
void display_map(int screen)
{
   
   char inbuf[128000];
   unsigned int map_ver;

   double   angle;
   float    bin_res;  /*  pixel to pixel resolution in NM */
   float    azimuth, range;
   float    scale_x, scale_y;

   float    first_latitude;
   float    first_longitude;

   float    second_latitude;
   float    second_longitude;
   float    radar_latitude=0.0;
   float    radar_longitude=0.0;
 
   
   float    flag_latitude=90.4444;
   float    flag_longitude=900.4444;

   int retval;
   
   int      number_of_points;  /*  used in original map logic */
   
   int      center_pixel;
   int      center_scanl;
   int    i, word, offset;
   int    i1, i2, i3, i4;
    

int   x, val;  
int   first_pass;
int   num_pts, num_vectors;  /*  debug only */
int   length, max_length, min_length; /*  debug only */
/* float max_lat, min_lat, max_long, min_long; */


   FILE *map_file;
   
    
    if(screen == SCREEN_1) {
        mapsd = sd1;
        
    } else if(screen == SCREEN_2) {
        mapsd = sd2;
        
    } else if(screen == SCREEN_3) {
        mapsd = sd3;
    }



/* /   if(verbose_flag) */
       printf("MAP file plotting begun\n");
       
       
    center_pixel = pwidth/2;  /* initialized to center of canvas */
    center_scanl = pheight/2; /* initialized to center of canvas */
    /*  note: pwidth and pheight is the size of the drawing area in pixels */

   /* correct center pixel for the off_center value (if any) */
   if(mapsd->icd_product != NULL) { 
       center_pixel = center_pixel - 
                             (int)(mapsd->x_center_offset * mapsd->scale_factor);  
       center_scanl = center_scanl - 
                             (int)(mapsd->y_center_offset * mapsd->scale_factor);
   }


   /* if no product displayed, use inherent resolution otherwise */
   /* if resolution is zero, it is either unknown or N/A         */
   /*       we do not display the map                            */
   
   /* if resolution is 999, it is an overlay product with no underlying     */
   /*       base product and we assume the overlay product has the standard */
   /*       standard inherent resolution                                    */
   
   /* get resolution from gen_radial */
   /*  TO DO - SHOULD SIG DIGITS BE LIMITED??? */
   if(sd->last_image == GENERIC_RADIAL) {
       bin_res = sd->gen_rad->range_interval / 1000 * CVG_KM_TO_NM;
   } else  {
       bin_res = res_index_to_res(mapsd->resolution);
   }

   
   if(mapsd->icd_product == NULL) 
        bin_res = 0.54;
   else if(bin_res == 0.0) /* cannot display range without resolution information */
        return;

   if(bin_res == 999) {/* if overlay product displayed at root level, 
                        * use inherent resolution */
        bin_res = 0.54;
        fprintf(stderr, "DISPLAY MAP - Overlay Product Displayed at Base \n");
        fprintf(stderr, "              Standard 0.54NM resolution assumed.\n");
   } 

   /*  note: mapsd->scale_factor is actually a zoom factor (e.g. 0.5. 1.0 2.0 etc.) */

   /* set up the correct scaling for the screen */

       scale_x =  mapsd->scale_factor;
       scale_y = -mapsd->scale_factor;


   /* we assume that the maps start out correctly scaled for .54nm products */
   scale_x *= 0.54/bin_res;  /*  note: scale_N = zoom_factor * (0.54/product_res) */
   scale_y *= 0.54/bin_res;

   /* get the position of the radar */
   if(mapsd->icd_product == NULL) { 
    /*  NEED TO LOOK UP RADAR LAT-LONG HERE! */
     radar_latitude = mapsd->prev_lat;
     radar_longitude = mapsd->prev_lon;
/*  debug */
/* fprintf(stderr,"DEBUG MAP - PREV LAT is %f, PREV LON is %f\n", */
/*                           mapsd->prev_lat, mapsd->prev_lon);    */

   } else {
      offset = RADAR_POS_OFFSET;
      word = read_word(mapsd->icd_product, &offset);
      radar_latitude = (float)word / 1000.0;
      mapsd->prev_lat = radar_latitude;
      word = read_word(mapsd->icd_product, &offset);
      radar_longitude = (float)word / 1000.0;
      mapsd->prev_lon = radar_longitude;
   }

    /* the clipping area set by plot_image() so that 
       we don't write over any of the sidebar */    

   /* open up the map file */
   map_file = fopen(map_filename, "r");
   if(map_file == NULL) {
       fprintf(stderr,"ERROR - Could not find map file: %s\n", map_filename);
       return;  
   }

   XSetForeground(display, gc, white_color);




   /*  begin new map logic --------------------------------------------------- */
   /*  Enhancements:  */
   /*     (1) used either floats for lat long */
   /*         in order to reduce map size by 50% */
   /*         NOTE: if scaled integers are used, the map Lat and Long */
   /*               must be positive to fit in an unsigned int */

   /* assumptions about new mapfile format:
    *    (1) begins with "1111" in the first 4 bytes
    *    (2) next for bytes an unsigned int to detect Endian type
    *    (3) that at least two points are between vector flags
    *        though we have logic to correctly handle a single point
    */
    
   fseek(map_file, 0, SEEK_SET);
   
   fread (inbuf, 1, 4, map_file);
   
   
   inbuf[4] = '\0';
   
   if ((x = strcmp(inbuf, "1111")) == 0) {  /*  NEW MAP FORMAT DETECTED */

      /*  DEBUG     */
      /* fprintf(stderr, "DEBUG x= %d, inbuf = '%s', new mapfile\n", x, inbuf); */
      
      if(verbose_flag)
         fprintf(stderr, "New mapfile format being used.\n");
 
      /*  detect map version or a map produced on the 'other' Endian platform */
      val = fread((void *)&map_ver, (size_t)1, sizeof(unsigned int), map_file);
      if(val != 4) {
         fprintf(stderr,"\nERROR IN MAPFILE - No vectors present (map_ver read), val is %d\n",val);
         if(map_file != NULL) {
            fclose(map_file);
            map_file = NULL;
         }
         return;
      }
      
      /*  see if map data in floats can be read */
      if(map_ver > 100) {
        
#ifdef LITTLE_ENDIAN_MACHINE
         fprintf(stderr, "***** MAP ERROR ***************************************\n");
         fprintf(stderr, "* Background Map produced on a Big Endian Platform    *\n");
         fprintf(stderr, "* Use the 'cvg_map' utility to create maps for the    *\n");
         fprintf(stderr, "*     Linux Platform                                  *\n");
         fprintf(stderr, "*******************************************************\n");
#endif
#ifndef LITTLE_ENDIAN_MACHINE
         fprintf(stderr, "***** MAP ERROR ***************************************\n");
         fprintf(stderr, "* Background Map produced on a Little Endian Platform *\n");
         fprintf(stderr, "* Use the 'cvg_map' utility to create maps for the    *\n");
         fprintf(stderr, "*     Solaris Platform                                *\n");
         fprintf(stderr, "*******************************************************\n");
#endif

         if(map_file != NULL) {
            fclose(map_file);
            map_file = NULL;
         }
         return;
      
      } /* end map_ver > 100 */

    /*  setup map palette file here - colors in cvg_map.plt are: */
    /* 0-black 1-red 2-white 3-light grey 4-yellow 5-magenta 6-green 7-orange    */
    /* 8-snow(mod) 9-brown 10-cyan 11-indian red(mod) 12-ghost white(mod)        */
    /* 13-dim grey(1/3) 14-inverse-for-snow(1/8) 15-inverse-for-ghost-white(1/12)*/
    
    /* CVG 9.0 - used new function open_default_palette() */
    retval = open_default_palette(BACKGROUND_MAP_DATA);
    
    if(retval==FALSE)
        use_cvg_colors = TRUE;
   
    /* NOTE: If in the future CVG is modified to have a configured color        */
    /*       palette file for maps, call open_config_or_default_palette()       */ 
    
      first_pass = TRUE;
      
 
      /*  GET FIRST FLAG VALUE --------  */
      
      while ( !feof(map_file) )  {         
         /* read first point */
/*  input */
         /*  float version         */
         val = fread((void *)&first_latitude, (size_t)1, sizeof(float), map_file);
         
         if(val != 4) {
            if(first_pass == TRUE)
               fprintf(stderr,"\nERROR IN MAPFILE - No vectors present (Lat read). val is %d\n",val);
            else
               fprintf(stderr,"\nERROR IN MAPFILE - Did not find first flag (Lat read). val is %d\n",val);
            if(map_file != NULL) {
              fclose(map_file);
              map_file = NULL;
            }
            return;
         }

         /*  float version          */
         val = fread((void *)&first_longitude, (size_t)1, sizeof(float), map_file);
         
         if(val != 4) {
            if(first_pass == TRUE)
               fprintf(stderr,"\nERROR IN MAPFILE - No vectors present (Lon read). val is %d\n",val);
            else
               fprintf(stderr,"\nERROR IN MAPFILE - Did not find first flag (Lon read). val is %d\n",val);
            if(map_file != NULL) {
              fclose(map_file);
              map_file = NULL;
            }
            return;
         }
         
         /*  NOTE: all Longitudes are negative though maps use positive number */
         /*        which is a carry over from the USGS 'graphic' data */
         /*  FUTURE ENHANCEMENT: use actual negative Lat Long values */
         first_longitude = first_longitude * -1.0;

/*  DEBUG  */
/* fprintf(stderr,"DEBUG float - very first point lat is %f, lon is %f\n", */
/*                                             first_latitude, first_longitude); */
                                            
/*  DEBUG */
/* fprintf(stderr, "DEBUG vector flag detected Lat/Lon, %f,  %f\n", */
/*     first_latitude ,first_longitude ); */
       


         first_pass = FALSE;
   
         if ((first_latitude > 90.0)) { /*  FLAG FOUND */
             /*  RECORD THE FLAG VALUE */
             flag_latitude = first_latitude;

             /*  float version */
             flag_longitude = -1.0 * first_longitude;
             feature_type = (int)flag_latitude - 90;
             feature_rank = (int)flag_longitude - 900;
/*  DEBUG              */
/* fprintf(stderr,"DEBUG - feature type is %d, rank is %d\n", feature_type, feature_rank); */
                          
             break;         

         }          
                   
      } /*  end while */
                   
      /*  end GET FIRST FLAG VALUE      */
      
 
      /*  debug, to find longest and shortest vectors        */
      num_pts = 0;
      num_vectors = 0;
      length = max_length = 0;
      min_length = 999;                   
                   
      /*  MAIN LOOP NEW MAP FORMAT - READ AND PLOT VECTORS IN MAP */

      while (!feof(map_file)) {

/*  debug */
/* if(num_vectors > 295) { */
/* fprintf(stderr, "DEBUG MAP - Start of outer loop, num_vectors %d\n",num_vectors); */
/* } */
    
         /*  read the first point */
/*  input */
         /*  float version */
         val = fread((void *)&first_latitude, (size_t)1, sizeof(float), map_file);
         if(val != 4) {
            if(verbose_flag)
               fprintf(stderr,"\nENCOUNTERED END OF MAPFILE\n");

            break;
         }

         /*  float version */
         val = fread((void *)&first_longitude, (size_t)1, sizeof(float), map_file);
         if(val != 4) {
            if(verbose_flag)
               fprintf(stderr,"\nENCOUNTERED END OF MAPFILE\n");

            break;
         }

         /*  float version */
         /*  NOTE: all Longitudes are negative though maps use positive number */
         /*        which is a carry over from the USGS 'graphic' data */
         /*  FUTURE ENHANCEMENT: use actual negative Lat Long values */
         first_longitude = first_longitude * -1.0;

/*  DEBUG */
/* if(num_vectors > 295) { */
/* fprintf(stderr,"DEBUG scaled int - first point lat is %d, lon is %d\n", */
/*                                             first_lat_ui, first_lon_ui); */
/* fprintf(stderr,"DEBUG float - first point lat is %f, lon is %f\n", */
/*                                             first_latitude, first_longitude); */
/* } */


                  
         if ((first_latitude > 90.0)) { /*  flag rather than first point */
/* debug */
/* fprintf(stderr, "\nDEBUG ERROR in MAPFILE FORMAT (adjacent flags)\n"); */
            /*  RECORD THE FLAG VALUE */
            flag_latitude = first_latitude;

            /*  float version */
            flag_longitude = -1.0 * first_longitude;
            feature_type = (int)flag_latitude - 90;
            feature_rank = (int)flag_longitude - 900;
/*  DEBUG */
/* fprintf(stderr,"DEBUG - feature type is %d, rank is %d\n", feature_type, feature_rank); */
            continue;
            
         } else {  /*  got the first point after flag value */
            earth_to_radar_coord(first_latitude, first_longitude, 
                     radar_latitude, radar_longitude, 
                     &azimuth, &range);
            /* set up the first point in pixel offsets in drawing area */
            angle  = (azimuth - 90.0) * DEGRAD;
            pixel1 = (range * cos(angle) ) * scale_x + center_pixel;
            angle  = (azimuth + 90.0) * DEGRAD;
            scanl1 = (range * sin(angle) ) * scale_y + center_scanl;
      
            /*  debug, to find longest and shortest vectors */
            num_pts++;
            
            
         } /*  end read first point */
      
         /* read in the second point */
/*  input */
         /*  float version */
         val = fread((void *)&second_latitude, (size_t)1, sizeof(float), map_file);
         if(val != 4) {
            if(verbose_flag)
               fprintf(stderr,"\nENCOUNTERED END OF MAPFILE\n");

            break;
         }

         /*  float version */
         val = fread((void *)&second_longitude, (size_t)1, sizeof(float), map_file);
         if(val != 4) {
            if(verbose_flag)
               fprintf(stderr,"\nENCOUNTERED END OF MAPFILE\n");

            break;
         }

         /*  NOTE: all Longitudes are negative though maps use positive number */
         /*        which is a carry over from the USGS 'graphic' data */
         /*  FUTURE ENHANCEMENT: use actual negative Lat Long values */
         second_longitude = second_longitude * -1.0;

/*  DEBUG */
/* if(num_vectors > 295) { */
/* fprintf(stderr,"DEBUG scaled int - second point lat is %d, lon is %d\n", */
/*                                             second_lat_ui, second_lon_ui); */
/* fprintf(stderr,"DEBUG float - second point lat is %f, lon is %f\n", */
/*                                             second_latitude, second_longitude); */
/* } */

         
         if ((second_latitude > 90.0)) { /*  flag rather than second point */
/* debug */
/*             fprintf(stderr, "\nDEBUG - MAPFILE SKIPPING SINGLE POINT VECTOR\n"); */
            /*  RECORD THE FLAG VALUE */
            flag_latitude = second_latitude;

/*  float version */
            flag_longitude = -1.0 * second_longitude;           
            feature_type = (int)flag_latitude - 90;
            feature_rank = (int)flag_longitude - 900;
/*  DEBUG */
/* fprintf(stderr,"DEBUG - feature type is %d, rank is %d\n", feature_type, feature_rank); */
            /*  debug, to find longest and shortest vectors */
            num_vectors = num_vectors-1;
            continue;
            
            
         } else {  /*  got the second point after flag value */
            earth_to_radar_coord(second_latitude, second_longitude, 
                     radar_latitude, radar_longitude, 
                     &azimuth, &range);
            /* set up the first point in pixel offsets in drawing area */
            angle  = (azimuth - 90.0) * DEGRAD;
            pixel2 = (range * cos(angle) ) * scale_x + center_pixel;
            angle  = (azimuth + 90.0) * DEGRAD;
            scanl2 = (range * sin(angle) ) * scale_y + center_scanl;
      
            /*  debug, to find longest and shortest vectors */
            num_pts++;
            num_vectors = num_vectors+1;
            length = 1;

            
            /*  DRAW LINE */
            draw_map_line();
            
         } /*  end read first point */


         /*  INNER LOOP NEW MAPS - READ NEXT POINT AND PLOT WITH PREVIOUS UNTIL FLAG READ */
         while (!feof(map_file)) {
   
            /*  read the next point in the vector */
/*  input */
            /*  float version */
            val = fread((void *)&second_latitude, (size_t)1, sizeof(float), map_file);
            if(val != 4) {
               if(verbose_flag)
                  fprintf(stderr,"\nENCOUNTERED END OF MAPFILE\n");

               break;
            }

            /*  float version    */
            val = fread((void *)&second_longitude, (size_t)1, sizeof(float), map_file);
            if(val != 4) {
               if(verbose_flag)
                  fprintf(stderr,"\nENCOUNTERED END OF MAPFILE\n");

               break;
            }


            /*  NOTE: all Longitudes are negative though maps use positive number */
            /*        which is a carry over from the USGS 'graphic' data */
            /*  FUTURE ENHANCEMENT: use actual negative Lat Long values */
            second_longitude = second_longitude * -1.0;

/*  DEBUG */
/* if(num_vectors > 296) { */
/* fprintf(stderr,"DEBUG scaled int - next point lat is %d, lon is %d\n", */
/*                                             second_lat_ui, second_lon_ui); */
/* fprintf(stderr,"DEBUG float - next point lat is %f, lon is %f\n", */
/*                                             second_latitude, second_longitude); */
/* } */
   
            if ((second_latitude > 90.0)) { /*  got flag, beginning new vector */
               /*  throw away first point - by rereading it and the second point */
               /*  RECORD THE FLAG VALUE */
               flag_latitude = second_latitude;
               /*  float version */
               flag_longitude = -1.0 * second_longitude;
               feature_type = (int)flag_latitude - 90;
               feature_rank = (int)flag_longitude - 900;    
/*  DEBUG */
/* fprintf(stderr,"DEBUG - feature type is %d, rank is %d\n", feature_type, feature_rank);    */
               /*  debug only, to find longest and shortest vectors */
               if(length > max_length)
                  max_length = length;
               if(length < min_length)
                  min_length = length;   
               length = 0;   
/*  DEBUG */
/* fprintf(stderr, "DEBUG vector end detected Lat/Lon\n"); */
               
               break;
   
            } else {  /*  got the next point in the vector */
               /*  set first point as previous point */
               pixel1 = pixel2;
               scanl1 = scanl2;
                   
               earth_to_radar_coord(second_latitude, second_longitude, 
                         radar_latitude, radar_longitude, 
                         &azimuth, &range);
               /* set up the first point in pixel offsets in drawing area */
               angle  = (azimuth - 90.0) * DEGRAD;
               pixel2 = (range * cos(angle) ) * scale_x + center_pixel;
               angle  = (azimuth + 90.0) * DEGRAD;
               scanl2 = (range * sin(angle) ) * scale_y + center_scanl;
   
               
               /*  DRAW LINE */
               draw_map_line();
               
               /*  debug only, to find longest and shortest vectors */
               num_pts++;
               length++;
              
            } /*  end read next point */
            
/*  debug */
/* if(num_vectors > 296) { */
/* fprintf(stderr, "DEBUG MAP - End inner loop, num_vectors %d\n",num_vectors); */
/* }             */
      
         } /*  end while      */
           /*  END INNER LOOP - NEW MAPS */
         
/*  debug */
/* if(num_vectors > 296) { */
/* fprintf(stderr, "DEBUG MAP - Out of inner loop, num_vectors %d\n",num_vectors); */
/* } */
      
/*  debug */
/* if(num_vectors > 296) { */
/* fprintf(stderr, "DEBUG MAP - End of outer loop, num_vectors %d\n",num_vectors); */
/* } */

      } /*  end while */
        /*  end MAIN LOOP NEW MAP FORMAT   */


      if(map_file != NULL) {
         fclose(map_file);
         map_file = NULL;
      }   
   
/*  DEBUG, to find longest and shortest vectors */
/*       fprintf(stderr, "\nDEBUG - num points = %d, num vectors = %d\n", */
/*             num_pts, num_vectors); */
/*       fprintf(stderr, "DEBUG - max vector length is %d line segments.\n", */
/*             max_length); */
/*       fprintf(stderr, "DEBUG - min vector length is %d line segments.\n", */
/*             min_length);       */
   



   } else {  /*  NEW MAP FORMAT NOT DETECTED - DRAW ORIGINAL MAP FORMAT */
    
      if(verbose_flag)
         fprintf(stderr, "Original mapfile format being used.\n");
/* DEBUG */
/*          fprintf(stderr, "     DEBUG - inbuf = '%s'\n", inbuf); */

      fseek(map_file, 0, SEEK_SET);


      /* read in a vector header block */
      fread(inbuf, 1, 24, map_file);

      while (feof (map_file) == 0) {
         /* get the size of this vector packet */
         i1 = (int)inbuf[1];
         i2 = (int)inbuf[2];
  
         if(i1 < 0)
            i1 = i1+256;
         if(i2 < 0)
            i2 = i2+256;
      
         number_of_points = 100*i1 + i2;
  
         /* extract the starting position */
         i1 = (int)inbuf[17];
         if(i1 > 127)
            i1 = i1 - 256;
         i2 = (int)inbuf[18];
         if(i2 > 127)
            i2 = i2 - 256;
         i3 = (int)inbuf[19];
         if(i3 > 127)
            i3 = i3 - 256;
         i4 = (int)inbuf[20];
         if(i4 > 127)
            i4 = i4 - 256;
  
         first_longitude = 2.0*i1 + i2 + i3/60.0 + i4/3600.0;
  
         i1 = (int)inbuf[21];
         if(i1 > 127)
            i1 = i1 - 256;
         i2 = (int)inbuf[22];
         if(i2 > 127)
            i2 = i2 - 256;
         i3 = (int)inbuf[23];
         if(i3 > 127)
            i3 = i3 - 256;
  
         first_latitude = i1 + i2/60.0 + i3/3600.0;
  
         /* convert it to screen coordinates */
         earth_to_radar_coord(first_latitude, first_longitude,
                   radar_latitude, radar_longitude,
                   &azimuth, &range);
  
         /* set up the inital points */
         angle  = (azimuth - 90.0) * DEGRAD;
         pixel1 = (range * cos(angle) ) * scale_x + center_pixel;
         angle  = (azimuth + 90.0) * DEGRAD;
         scanl1 = (range * sin(angle) ) * scale_y + center_scanl;
  
         /* read in the first group of points */
         fread (inbuf, 1, 2*(number_of_points-1), map_file);
  
         second_latitude  = first_latitude;
         second_longitude = first_longitude;
  
         for (i=1; i<number_of_points; i++) {
            /* get new point */
            i1 = (int)inbuf[i*2];
            if(i1 > 127)
               i1 = i1 - 256;
            i2 = (int)inbuf[i*2+1];
            if(i2 > 127)
               i2 = i2 - 256;
   
            second_latitude  = second_latitude  + i2/3600.0;
            second_longitude = second_longitude + i1/3600.0;
  
            /* change it to screen coordinates */
            earth_to_radar_coord(second_latitude, second_longitude,
                     radar_latitude, radar_longitude,
                     &azimuth, &range);
  
            /* rescale it to fit our screen */
            angle  = (azimuth - 90.0) * DEGRAD;
            pixel2 = (range * cos (angle) ) * scale_x + center_pixel;
            angle  = (azimuth + 90.0) * DEGRAD;
            scanl2 = (range * sin (angle) ) * scale_y + center_scanl;
      
            XDrawLine(display, mapsd->pixmap, gc, pixel1, scanl1, pixel2, scanl2);
  
            /* rotate new points back */
            pixel1 = pixel2;
            scanl1 = scanl2;
         } /*  end for */
  
         /* get next header */
         fread (inbuf, 1, 24, map_file);
      
      } /*  end while */
      
   }  /*  end NEW MAP FORMAT NOT DETECTED - new map logic */

   
   if(map_file != NULL)
       fclose(map_file);

   XSetForeground(display, gc, white_color);
   XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);


} /*  end display_map() */








/* This routine draws a single line vector between two points in the new *
 * map logic                                                             *
 */
void draw_map_line()
{
    

    /* ////// SETUP THE COLOR AND LINE ATTRIBUTES FOR FEATURE TYPE //////// */

    if(prev_feature != feature_type) {  /*  change color */
        
       if(feature_type == POLITICAL) {
          /*  some political attributes set for individual feature ranks */
/*            if(use_cvg_colors == TRUE)  */
/*                XSetForeground(display, gc, light_grey_color); */
/*            else */
/*                XSetForeground(display, gc, display_colors[3].pixel);      */

       } else if(feature_type == ADMIN) { 
          if(use_cvg_colors == TRUE) 
               XSetForeground(display, gc, red_color);
          else
               XSetForeground(display, gc, display_colors[1].pixel);
          XSetLineAttributes (display, gc, 1, LineSolid, 
                                       CapButt, JoinMiter); 
       
       } else if(feature_type == WATER) { 
          /*  looks like it is better if wb is white with pb gray */
          if(use_cvg_colors == TRUE) 
               XSetForeground(display, gc, ghost_white_color);
          else
               XSetForeground(display, gc, display_colors[12].pixel);
          XSetLineAttributes (display, gc, 1, LineSolid, 
                                       CapButt, JoinMiter); 
       
       } else if(feature_type == STREAMS) { 
          /*  could use white if only persistent streams shown */
          if(use_cvg_colors == TRUE) 
               XSetForeground(display, gc, light_grey_color);
          else
               XSetForeground(display, gc, display_colors[3].pixel);
          XSetLineAttributes (display, gc, 1, LineSolid, 
                                       CapButt, JoinMiter); 
       
       } else if(feature_type == ROADS) {
          /*  yellow is good */
          if(use_cvg_colors == TRUE) 
               XSetForeground(display, gc, yellow_color);
          else
               XSetForeground(display, gc, display_colors[4].pixel);
          XSetLineAttributes (display, gc, 1, LineSolid, 
                                       CapButt, JoinMiter); 

       } else if(feature_type == RAIL) {
          /*  blue is good */
          if(use_cvg_colors == TRUE) 
               XSetForeground(display, gc, indian_red_color);
          else
               XSetForeground(display, gc, display_colors[11].pixel);
          XSetLineAttributes (display, gc, 1, LineSolid, 
                                       CapButt, JoinMiter); 

       } else if(feature_type == HYPSOG) {
          /*  try white */
          if(use_cvg_colors == TRUE) 
               XSetForeground(display, gc, orange_color);
          else
               XSetForeground(display, gc, display_colors[7].pixel);
          XSetLineAttributes (display, gc, 1, LineSolid, 
                                       CapButt, JoinMiter); 
                                       
       } else if(feature_type == CULTURAL) {
          /*  try white */
          if(use_cvg_colors == TRUE) 
               XSetForeground(display, gc, green_color);
          else
               XSetForeground(display, gc, display_colors[6].pixel);
          XSetLineAttributes (display, gc, 1, LineSolid, 
                                       CapButt, JoinMiter); 

       }
       
       prev_feature = feature_type;

    } /*  end change color / attributes */
    
    
    
    /* ////// CHECK RANK THEN DRAW FEATURE //////////////////// */
    /*  DEVELOPMENT NOTES: The following logic for water feature_rank and */
    /*  preferences logic for selecting the amout of road and rail detail */
    /*  has been simplified because of the limits of water, road, rail data */
    /*  that has been placed into the map data files that map_cvg uses to */
    /*  create the cvg maps */
    
    
    if(feature_type == POLITICAL) {
       /*  Probably do not need CVG display options for this */
       if( 
           (feature_rank == 1) ||  /*  US */

/*                (feature_rank == 2) ||  // US LINES OVER WATER */

           (feature_rank == 5) ||  /*  STATE LINES OVER LAND */

/*                (feature_rank == 6) || // STATE LINES OVER WATER */

           ((feature_rank == 9)
             && (co_d == TRUE)) ||  /*  COUNTY  */

/*                (feature_rank == 10) || // COUNTY OVER WATER */

/* /               (feature_rank == 11) || // removing 11-14 removes cities, */
/* /               (feature_rank == 12) || // DC and Baltimore CO remain */
/* /               (feature_rank == 14) || // ? */
           (feature_rank == 0)     /*  none */
                                       ) {
           /*  future: only change attributes if different from previous rank */
           if(feature_rank == 9) {
               if(use_cvg_colors == TRUE) 
                   XSetForeground(display, gc, snow_color);
               else
                   XSetForeground(display, gc, display_colors[8].pixel);

               XSetLineAttributes (display, gc, 1, LineSolid, 
                                            CapButt, JoinMiter);
           } else {
               if(use_cvg_colors == TRUE) 
                   XSetForeground(display, gc, light_grey_color);
               else
                   XSetForeground(display, gc, display_colors[3].pixel);
            
               XSetLineAttributes (display, gc, 2, LineSolid, 
                                            CapButt, JoinMiter);
           }
           
           XDrawLine(display, mapsd->pixmap, gc, pixel1, scanl1, 
                                                  pixel2, scanl2);
       }


    } else if(feature_type == ADMIN) { 
       /*  no admin boundaries better than all */
       /*  ADMIN: values run from 21 to 97, decode includes 21 to 55 */
       /*  DRAW NONE FOR NOW */
/* /       if(feature_rank < admin_d) */
/* /           XDrawLine(display, mapsd->pixmap, gc, pixel1, scanl1,  */
/* /                                              pixel2, scanl2); */

    } else if(feature_type == WATER) { 
       /*  try to eliminate intermittent wb, up to 15 minimum most locations */
       /*  THESE WATER LIMITS ARE IMPLEMENTED IN DECODE.C  */
       /*  (REMOVED FROM DATA FILES)   */
       /*  would be nice to avoid 5&6 in many locations */
       /*  BUT 5 & 6 needed for FL Keys for some reason */
/* /       if(  (feature_rank <= 1) || */
/* /           ((feature_rank >= 5) && (feature_rank <= 14)) ) */
           XDrawLine(display, mapsd->pixmap, gc, pixel1, scanl1, 
                                              pixel2, scanl2);

    } else if(feature_type == STREAMS) { 
       /*  THESE STREAM LIMITS ARE IMPLEMENTED IN DECODE.C  */
       /*  (REMOVED FROM DATA FILES except AK and HI) */
       /*       FOR HI, STREAMS <=3 gives first streams */
       /*       FOR ALL OTHERS STREAMS <=1 SEEMS OK */
       if(  (feature_rank <= 1)  ) /*  limit needed for AK */
           XDrawLine(display, mapsd->pixmap, gc, pixel1, scanl1, 
                                              pixel2, scanl2);

    } else if(feature_type == ROADS) {
       /*  12 gives major roads */
       /*  15 good most location (up to 14 included by decode.c) */
       /*  21 if zoom 4 is an option */
       /*  32 is OK for AK and HI */
       /*  eliminate ferry routes (Cape May, Anchorage, etc.) */
       if(feature_rank < road_d)
           XDrawLine(display, mapsd->pixmap, gc, pixel1, scanl1, 
                                              pixel2, scanl2);

    } else if(feature_type == RAIL) {
       /*  most are 71-75 with a few 79 & 80 */
       /*  For all but HI AK, 71 & 72 gives major rail so */
       /*       decode.c removes all above 72 */
       /*  AK needs 75 (78 removed by decode.c) */
       /*  HI only 75 */
       if(feature_rank < rail_d)
           XDrawLine(display, mapsd->pixmap, gc, pixel1, scanl1, 
                                              pixel2, scanl2);

    } else if(feature_type == HYPSOG) {
       /*  continental divide */
       /*  DRAW ALL FOR NOW */
           XDrawLine(display, mapsd->pixmap, gc, pixel1, scanl1, 
                                              pixel2, scanl2);


    } else if(feature_type == CULTURAL) {
        /*  AK pipeline */
        /*  DRAW ALL FOR NOW              if(feature_rank < culture_d) */
           XDrawLine(display, mapsd->pixmap, gc, pixel1, scanl1, 
                                              pixel2, scanl2);
    }



    
} /*  end draw_map_line() */




















/*----------------------------------------------------------------------*
 *  This routine is used by the IRAS package to calculate the distance  *
 *  between two earth coordinates in polar coordinates.  The input      *
 *  data are:                               *
 *                                  *
 *  latitude        -  Target latitude (deg)        *
 *  longitude       -  Target longitude (deg)       *
 *  reference_latitude  -  Reference latitude (deg)     *
 *  reference_longitude -  Reference longitude (deg)        *
 *                                  *
 *  The output from this routine are:                   *
 *                                  *
 *  azimuth         -  Azimuthal angle of target from   *
 *                 reference (deg).         *
 *  range           -  Azimuthal distance of target from    *
 *                 reference (km).          *
 *----------------------------------------------------------------------*
*/
void earth_to_radar_coord(float latitude, float longitude, float reference_latitude,
              float reference_longitude, float *azimuth, float *range)
{
    double  temp1, temp2;
    double  a, b, c, a1, a2, alpha;

/*  Begin calculation now....                       */

    temp1 = (reference_latitude-latitude) * 3.14159265 / 180.0;
    a = fabs (temp1);
    temp1 = latitude * 3.14159265 / 180.0;
    temp2 = (reference_longitude-longitude) * 3.14159265 / 180.0;
    b = cos (temp1) * fabs (temp2);
    temp1 = cos (a) * cos (b);
    c = acos (temp1);

    a1 = sin (c);

    if (a1 <= 0.0) {
        *azimuth = 0.0;
        *range   = 0.0;
    } else {
        a2 = sin (b) / a1;

        if (a2 > 0.9999999) 
            a2 = 0.9999999;

        temp1 = cos (a) * a2;
        alpha = acos (temp1) / (3.14159265/180.0);
        *range = c * 111.12 / (3.14159265/180.0);

/*  Put azimuth angle in proper quadrants.....              */

        if (latitude < reference_latitude) {
            if (longitude > reference_longitude) {
                *azimuth = 90.0 + alpha;
            } else {
                *azimuth = 270.0 - alpha;
            }
        } else {
            if (longitude > reference_longitude) {
                *azimuth = 90.0 - alpha;
            } else {
                *azimuth = 270.0 + alpha;
            }
        }
    }

}







