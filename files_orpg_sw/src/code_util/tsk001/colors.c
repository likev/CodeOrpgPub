/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/08/19 15:11:39 $
 * $Id: colors.c,v 1.10 2009/08/19 15:11:39 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */
/* colors.c */

#include "colors.h"




/* called by setup_palette before display of each packet or display
 * of the legend
 * called by output_image_to_gif and output_image_to_png 
 */
int global_pixel_to_color(int the_pixel, int *found)
{

    int i;
    int max=0, max_pix=0, c;
    if(sd==sd1) {
        for(i=0; i<global_palette_size_1; i++)
            if(the_pixel==global_display_colors_1[i].pixel) {
                *found = 1;
                return i;       
            } else {
            c = global_display_colors_1[i].red + 
                global_display_colors_1[i].green +
                global_display_colors_1[i].blue;
            if(c > max) {
                max = c;
                max_pix = i;
            }
        }
    }
    
    else if(sd==sd2) {
        for(i=0; i<global_palette_size_2; i++)
            if(the_pixel==global_display_colors_2[i].pixel) {
                *found = 1;
                return i;       
            } else {
            c = global_display_colors_2[i].red + 
                global_display_colors_2[i].green +
                global_display_colors_2[i].blue;
            if(c > max) {
                max = c;
                max_pix = i;
            }
        }
    }
    
    else if(sd==sd3) {
        for(i=0; i<global_palette_size_3; i++)
            if(the_pixel==global_display_colors_3[i].pixel) {
                *found = 1;
                return i;       
            } else {
            c = global_display_colors_3[i].red + 
                global_display_colors_3[i].green +
                global_display_colors_3[i].blue;
            if(c > max) {
                max = c;
                max_pix = i;
            }
        }
    }
        

    /* didn't find 'nuthin - return the closest we have to white */
   
    *found = 0;
    return max_pix;

} /* end global_pixel_to_color */




/* Load the global default palette list.  The file is organized as a simple
 * mapping from packet type to palette list.  Called when loading 
 * preferences during CVG launch
 */
void load_palette_list()
{
    FILE *list_file;
    char filename[100], buf[100];
    int  packet_index, i;

    /* open the palette list */
    sprintf(filename, "%s/colors/palette_list", config_dir);
    if((list_file=fopen(filename, "r"))==NULL) {
        fprintf(stderr, "**************************************\n");
        fprintf(stderr, "*  Could not open palette list       *\n");
        fprintf(stderr, "*                                    *\n");
        fprintf(stderr, "*  The file 'palette_list' in the    *\n");
        fprintf(stderr, "*     ~/.cvgN.n/colors  directory is *\n");
        fprintf(stderr, "*     either missing or corrupt      *\n");
        fprintf(stderr, "**************************************\n");
    exit(0);
    }

    assoc_init_s(&palettes);

    /* first read in the index, then the file name */
    while(feof(list_file) == 0) {
        fscanf(list_file, "%d", &packet_index);
    read_to_eol(list_file, buf);

    /* skip over spaces */
    i=0;
    while((buf[i] == ' ') && (i < strlen(buf))) i++;

    assoc_insert_s(palettes, packet_index, buf+i);
    }

    fclose(list_file);

} /* end load_palette_list() */





/* loads the palette for a given packet type for the current product */
/* called by dispatch_packet with a FALSE flag during product display 
 * and by display_legend_blocks with a TRUE flag
 */
/* CVG 9.0 - changed function name from setup_colors */
void open_config_or_default_palette(int packet_type, int legend_flag)
{

    int *assoc_packet;
    FILE *the_palette_file;
    char filename[256], *config_name, *def_palette;
    /* CVG 9.1 */
    char *override_name;
    int *override_pkt, packet_was_override;
    
    Prod_header *hdr;
    Graphic_product *gp;

    int *flag_ptr, digital_flag;

    /* if there is a  palette configured for this product, use it */
    /* "." is the original symbol for no configured palette */
    /* ".plt" is the new symbol for no configured palette */
    if(sd->icd_product == NULL) { /*  product not loaded */
        
        return;
    
    } else  { /* product loaded - open and setup colors */
        /* if it's called by the legend we need to setup the base product colors */
        if(legend_flag==TRUE && overlay_flag==TRUE) {
           hdr = (Prod_header *)(sd->history[0].icd_product);
           gp  = (Graphic_product *)(sd->history[0].icd_product + 96);
           
        } else {         
           hdr = (Prod_header *)(sd->icd_product);
           gp  = (Graphic_product *)(sd->icd_product + 96);
           
        }

    /* CVG 9.1 */
    /***** PART A - process override palette and packet *************************/
        packet_was_override = FALSE;
        
        override_name = assoc_access_s(override_palette, hdr->g.prod_id);
    
        
        if( (override_name != NULL) && 
            (strcmp(override_name, ".") != 0) && /* original non-configured entry */
            (strcmp(override_name, ".plt") != 0) ) {
            /*  WE HAVE AN OVERRIDE PALETTE            */
            override_pkt = assoc_access_i(override_packet, hdr->g.prod_id);
            /*  check if there is an associated packet configured for the product */
            /*  if so see if this packet matches */
            if( (override_pkt != NULL) && (*override_pkt == packet_type) ) {
                /*  using the configured palette */
                sprintf(filename, "%s/colors/%s", config_dir, override_name);
                packet_was_override = TRUE;  
                          
               if(verbose_flag)
                   fprintf(stderr,"Override Palette file: %s\n", filename);
               
               if((the_palette_file = fopen(filename, "r")) == NULL) {
                   fprintf(stderr, "Could not open override palette file: %s\n", 
                           filename);
                   packet_was_override = FALSE;
               } else {
                  setup_palette(the_palette_file, packet_type, legend_flag);
                  fclose(the_palette_file);
               } 
               
            } 
            /* the default palette is used in the configured palette section PART B*/


/* The following would be added here if the confiured palette was limited to */
/* just the 2d data array packets which are used in the legend block display */
/*            } else {                                                                   */
/*                                                                                       */
/*                def_palette = assoc_access_s(palettes, packet_type);                   */
/*                                                                                       */
/*                if(def_palette == NULL) {                                              */
/*                    fprintf(stderr,                                                    */
/*                         "ERROR - default palette file not defined for packet %d\n",   */
/*                         packet_type);                                                 */
/*                    sprintf(filename, "");                                             */
/*                } else {                                                               */
/*                    sprintf(filename, "%s/colors/%s", config_dir, def_palette);        */
/*                }                                                                      */
/*                                                                                       */
/*            }                                                                          */
/*                                                                                       */
/*            if(verbose_flag)                                                           */
/*                if(packet_was_override==TRUE)                                          */
/*                   fprintf(stderr,"Override Palette file: %s\n", filename);            */
/*                else                                                                   */
/*                   fprintf(stderr,"Default Palette file: %s\n", filename);             */
/*                                                                                       */
/*            if((the_palette_file = fopen(filename, "r")) == NULL) {                    */
/*               fprintf(stderr, "Could not open override palette file: %s\n", filename);*/
/*                                                                                       */
/*            } else {                                                                   */
/*               setup_palette(the_palette_file, packet_type, legend_flag);              */
/*               fclose(the_palette_file);                                               */
/*                                                                                       */
/*            }                                                                          */

        } /* end open and set override palette */        


        /* don't permit configuration of a palette twice */
        if(packet_was_override==TRUE)
            return;


    /***** PART B - process associated palette and packet **************************/
       
        flag_ptr = assoc_access_i(digital_legend_flag, hdr->g.prod_id);
        digital_flag = *flag_ptr;
        

        /*  TEST DIGITAL FLAG 3 HERE */
        if(digital_flag == 3) {       
      
            if(gp->level_1 == -635 ) {
              config_name = assoc_access_s(configured_palette, hdr->g.prod_id);     
            } else if(gp->level_1 == -1270 ) {
              config_name = assoc_access_s(config_palette_2, hdr->g.prod_id);        
            } else {  /*  unexpected value - use first legend file */
              config_name = assoc_access_s(configured_palette, hdr->g.prod_id);        
            }
      
        } else {
           
            config_name = assoc_access_s(configured_palette, hdr->g.prod_id);
            
        }



        /* CVG 9.0 - LOGIC SIMPLIFIED */
        if( (config_name != NULL) && 
            (strcmp(config_name, ".") != 0) &&  /* the orig non-configured entry */
            (strcmp(config_name, ".plt") != 0) ) {
            /*  WE HAVE A CONFIGURED PALETTE            */
            assoc_packet = assoc_access_i(associated_packet, hdr->g.prod_id);
            /*  check if there is an associated packet configured for the product */
            /*  if so see if this packet matches */
            /* CVG 9.1 removed (*assoc_packet == 0), 0 now means no pkt configured */
            if( (assoc_packet != NULL) && (*assoc_packet == packet_type) ) {
                /*  using the configured palette */
                sprintf(filename, "%s/colors/%s", config_dir, config_name);
                
                /* CVG 9.1 - if a 2-d array, set flag to display legend blocks */
                /*           initialized to FALSE by plot_image()              */
                if(packet_type == DIGITAL_RADIAL_DATA_ARRAY  || 
                   packet_type == DIGITAL_PRECIP_DATA_ARRAY  || 
                   packet_type == GENERIC_RADIAL_DATA        || 
                   packet_type == GENERIC_GRID_DATA          || 
                   packet_type == RADIAL_DATA_16_LEVELS      || 
                   packet_type == RASTER_DATA_7 || packet_type == RASTER_DATA_F)
                    display_color_bars = TRUE;
                    
            /*  the configured palette is not being used for this packet */
            /* CVG 9.1 - only non 2-d arrays have a default palette */
            } else if(packet_type != DIGITAL_RADIAL_DATA_ARRAY  && 
                      packet_type != DIGITAL_PRECIP_DATA_ARRAY  &&  
                      packet_type != GENERIC_RADIAL_DATA        &&  
                      packet_type != GENERIC_GRID_DATA          &&  
                      packet_type != RADIAL_DATA_16_LEVELS      &&  
                      packet_type != RASTER_DATA_7 && packet_type != RASTER_DATA_F) { 
                
                /* make sure that the palette entry exists */
                def_palette = assoc_access_s(palettes, packet_type);
                
                if(def_palette == NULL) {
                    fprintf(stderr, 
                         "ERROR - default palette file not defined for packet %d\n",
                         packet_type);
                    return;
                }
              
                sprintf(filename, "%s/colors/%s", config_dir, def_palette);
            }
            
        /*  WE DO NOT HAVE A CONFIGURED PALETTE             */
        /* CVG 9.1 - only non 2-d arrays have a default palette */
        } else if(packet_type != DIGITAL_RADIAL_DATA_ARRAY  && 
                  packet_type != DIGITAL_PRECIP_DATA_ARRAY  &&  
                  packet_type != GENERIC_RADIAL_DATA        &&  
                  packet_type != GENERIC_GRID_DATA          &&  
                  packet_type != RADIAL_DATA_16_LEVELS      &&  
                  packet_type != RASTER_DATA_7 && packet_type != RASTER_DATA_F) { 
            def_palette = assoc_access_s(palettes, packet_type);
            /* make sure that the palette entry exists */
            if(def_palette == NULL) {
                fprintf(stderr, 
                         "ERROR - default palette file not defined for packet %d\n",
                         packet_type);
                return;
            }
              
            sprintf(filename, "%s/colors/%s", config_dir, def_palette);
        }  /* end we do not have a configured palette */
        
        if(verbose_flag)
            fprintf(stderr,"Palette file: %s\n", filename);
    
        if((the_palette_file = fopen(filename, "r")) == NULL) {
           fprintf(stderr, "Could not open configured palette file: %s\n", filename);
           /* CVG 9.1 - do not attempt to display legend bars for 2-d arrays */
           if(packet_type == DIGITAL_RADIAL_DATA_ARRAY  || 
              packet_type == DIGITAL_PRECIP_DATA_ARRAY  || 
              packet_type == GENERIC_RADIAL_DATA        || 
              packet_type == GENERIC_GRID_DATA          || 
              packet_type == RADIAL_DATA_16_LEVELS      || 
              packet_type == RASTER_DATA_7 || packet_type == RASTER_DATA_F)
                    display_color_bars = FALSE;
           return;
        }
    
        /* CVG 9.0 - changed legend block param from always FALSE to 'legend_flag' */
        /* if it's called with FALSE, the colors are added to the global palette */
        setup_palette(the_palette_file, packet_type, legend_flag);
    
        fclose(the_palette_file);

        
 
 
    } /* end product loaded - open and setup colors */
 


    
} /* end open_config_or_default_palette */





/* CVG 9.0 - ADDED */
/* there is no legend flag, setup_palette is called with FALSE */
/* to ensure colors are always added to the global palette */
/* returns FALSE on failure to permit using local colors */
int open_default_palette(int packet_type)
{
    
FILE *the_palette_file;
char filename[256], *def_palette;
 
   def_palette = assoc_access_s(palettes, packet_type);
   /* make sure that the palette entry exists */
   if(def_palette == NULL)
       return FALSE;

   sprintf(filename, "%s/colors/%s", config_dir, def_palette);
    
    if(verbose_flag)
       fprintf(stderr,"Palette file(default): %s\n", filename);

    if((the_palette_file = fopen(filename, "r")) == NULL) {
       fprintf(stderr, "Could not open default palette file: %s\n", filename);
       return FALSE;
    }

    /* if it's called with FALSE, the colors are added to the global palette */
    setup_palette(the_palette_file, packet_type, FALSE);

    fclose(the_palette_file);
    
    return TRUE;
    
} /* end open_default_palette */










void setup_palette(FILE *the_palette_file, int packet_type, int legend_block)
{
    int i, in_color, found, flag=0;

    /* get number of colors */
    fscanf(the_palette_file, "%d", &palette_size);
    if(verbose_flag)
        fprintf(stderr, "number of colors in palette: %d\n", palette_size);

    /* read in color table. First all R values, then all G, then all B */
    /* loads display_colors[] with the palette file color table        */
    for(i=0; i<palette_size; i++) {
        display_colors[i].flags = DoRed | DoGreen | DoBlue;
        if( (fscanf(the_palette_file, "%d", &in_color) != 1) || 
            (ferror(the_palette_file) != 0) ) {
            fprintf(stderr, "Error reading palette file.\n");
        }
        display_colors[i].red   = ((unsigned short)in_color)<<8;
    }                            
    for(i=0; i<palette_size; i++) {
        if( (fscanf(the_palette_file, "%d", &in_color) != 1) || 
            (ferror(the_palette_file) != 0) ) {
            fprintf(stderr, "Error reading palette file.\n");
        }                            
        display_colors[i].green = ((unsigned short)in_color)<<8;
    }                            
    for(i=0; i<palette_size; i++) {
        if( (fscanf(the_palette_file, "%d", &in_color) != 1) || 
        (ferror(the_palette_file) != 0) ) {
            fprintf(stderr, "Error reading palette file.\n");
        }
        display_colors[i].blue  = ((unsigned short)in_color)<<8;
    }

    /* global palettes and global packet lists are not modified if
     * setting up colors for legend bocks
     */
   
    /* if not setting up palette for the legend blocks then:          */      
    /* if the packet_type has already been processed for the display on */
    /* this screen (array originally set to 0 with overlay flag FALSE), */
    /* the 'flag' is set to true  */
    if(legend_block==FALSE) {
        if(sd==sd1)
            if(global_packet_list[packet_type]) flag=1;
        if(sd==sd2)
            if(global_packet_list_2[packet_type]) flag=1;
        if(sd==sd3)
            if(global_packet_list_3[packet_type]) flag=1;
    }
    
    /* reads the display_colors[] array and if not setting up palette for  */
    /* the legend_blocks then:         (1) searches the global palette   */
    /* array for the applicable screen (global_display_colors_N) for each  */
    /* color, (2) adds the color to the global palette if not already there */   
    for(i=0; i<palette_size; i++) {
        if(XAllocColor(display, cmap, &display_colors[i]) == 0)
            fprintf(stderr, "XAllocColor %d failed for product colors!\n", i);
/*  DEBUG             */
/* printf("%d\n",display_colors[i].pixel); */

        if(legend_block==FALSE) {
            
            /*  if packet_type is not already processed then search the global  */
            /*  palette for the pixel, if color not found we add the color to the  */
            /*  global palette and increment the global palette size */
            if(!flag) {
/*  TEST */
/* if(i==0) */
/* fprintf(stderr, "\nTEST - PROCESSING PACKET %d FOR GLOBAL PALETTE\n", packet_type); */
                if(sd==sd1) {
                    global_pixel_to_color(display_colors[i].pixel,&found);
                    if(!found) {
                        global_display_colors_1[global_palette_size_1].pixel =
                                                            display_colors[i].pixel;
                        global_display_colors_1[global_palette_size_1].red =
                                                            display_colors[i].red;
                        global_display_colors_1[global_palette_size_1].green =
                                                            display_colors[i].green;
                        global_display_colors_1[global_palette_size_1].blue =
                                                            display_colors[i].blue;
                        global_display_colors_1[global_palette_size_1].flags =
                                                            display_colors[i].flags;
                        global_palette_size_1++; 
                    }
                }
                
                else if (sd==sd2) {
                    global_pixel_to_color(display_colors[i].pixel,&found);
                    if(!found) {
                        global_display_colors_2[global_palette_size_2].pixel =
                                                            display_colors[i].pixel;
                        global_display_colors_2[global_palette_size_2].red =
                                                            display_colors[i].red;
                        global_display_colors_2[global_palette_size_2].green =
                                                            display_colors[i].green;
                        global_display_colors_2[global_palette_size_2].blue =
                                                            display_colors[i].blue;
                        global_display_colors_2[global_palette_size_2].flags =
                                                            display_colors[i].flags;
                        global_palette_size_2++;
                    }
                }
                
                else if (sd==sd3) {
                    global_pixel_to_color(display_colors[i].pixel,&found);
                    if(!found) {
                        global_display_colors_3[global_palette_size_3].pixel =
                                                            display_colors[i].pixel;
                        global_display_colors_3[global_palette_size_3].red =
                                                            display_colors[i].red;
                        global_display_colors_3[global_palette_size_3].green =
                                                            display_colors[i].green;
                        global_display_colors_3[global_palette_size_3].blue =
                                                            display_colors[i].blue;
                        global_display_colors_3[global_palette_size_3].flags =
                                                            display_colors[i].flags;
                        global_palette_size_3++;
                    }
                }
                
           } /*  end if flag */
       
        } /*  end if legend_block FALSE */

    } /*  end for */
    
/*  DEBUG */
/* printf("global palette size %d, last pixel is %d\n", */
/*             global_palette_size_1, */
/*             global_display_colors_1[global_palette_size_1-1].pixel); */
                       

    if(legend_block==FALSE) {
        if(sd==sd1)
            global_packet_list[packet_type]=1;
        if(sd==sd2)
            global_packet_list_2[packet_type]=1;
        if(sd==sd3)
            global_packet_list_3[packet_type]=1;
    }
    
        
} /* end setup_palette() */


