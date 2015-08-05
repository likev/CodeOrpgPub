/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:46 $
 * $Id: packet_28area.c,v 1.5 2009/05/15 17:52:46 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* packet_28area.c */

#include <math.h>
#include <Xm/Xm.h>
#include "packet_28_cvg.h"


/* //#define TEST DO */


/* GLOBALS */
Pixmap area_pix;
static  char dotted[2] = {2,5};
static  char dashed[2] = {8,8};
static  char default_d[2] = {4,4};


/* /////////////////////////////////////////////////////////////// */
void display_area(int index, int location_flag)
{
/* Currently we do not read from the prod headers / symbology block */
/*    int num_halfwords, i; */
/*    short *product_data = (short *)(sd->icd_product); */
    float xin, yin;
    float xpos=0.0, ypos=0.0;
    int pixel;
    int scanl;
    int     center_pixel;
    int     center_scanl;
    float   x_scale=1.0, y_scale=1.0;
    XPoint  s_point[1];       /*  a single point */
    XPoint  *m_points=NULL;  /*  point array */
    XPoint  *Xloc_ptr=NULL;
    int p, num_points=1, ret;
    
    int *prod_res_ind;    
    float base_res, prod_res;
    Prod_header *hdr=NULL;  
    Graphic_product *gp;

    char *mnemonic=NULL;
    char prod_mnemon[4]; /*  3 chararacter mnemonic, initially padded w spaces */
    char *token;
    int product_code=0;
    
    RPGP_product_t *generic_product;
    RPGP_area_t   *area_comp=NULL;
    int loc_type=RPGP_LATLON_LOCATION, area_type=RPGP_AT_POINT;
    int symbol_type, symbol_color, line_color;
    char label_txt[10];

/* CVG 9.0 */
/*    char filename[256],  *def_palette;*/
/*    FILE *the_palette_file=NULL;      */

    RPGP_location_t       *latlon_loc;
    RPGP_xy_location_t    *xy_loc;
    RPGP_azran_location_t *azran_loc;

    
#ifdef TEST
char sub_type_str[30], sub_type_str2[10];
float range, azm;
#endif

    /* used with preview frame */
    XPoint prev_point[2];
    XPoint prev_line[5];
    char prev_mnemonic[4] = "MNE";



      
    /**** SECTION 1 ***********************************************************/
    /* Reading Component Header, Setting Resolution, Creat Point Array & Label*/
    /**************************************************************************/

    /*** Section 1 A *******************************************/
    /* Reading Component header                                */
    

    /* fprintf(stderr,"\n----- Display Generic Area Data ------- index is %d\n\n", index); */
    
    if(location_flag == PRODUCT_FRAME) {

        generic_product = (RPGP_product_t *)sd->generic_prod_data;
        area_comp = (RPGP_area_t *)generic_product->components[index];
                    
        loc_type  = RPGP_LOCATION_TYPE(area_comp->area_type);
        area_type = RPGP_AREA_TYPE(area_comp->area_type);
       
#ifdef TEST
        { /*  TEST CODE 1 ///////////////// */
        if(loc_type==RPGP_LATLON_LOCATION) {
            strcpy(sub_type_str2, "(LAT LON)");    
        } else if(loc_type==RPGP_XY_LOCATION) {
            strcpy(sub_type_str2, "(XY)");    
        } else if(loc_type==RPGP_AZRAN_LOCATION) {
            strcpy(sub_type_str2, "(AZRAN)");
        }
        if(area_type==RPGP_AT_POINT) {
            strcpy(sub_type_str, "Geographical Point");
        } else if(area_type==RPGP_AT_AREA) {
            strcpy(sub_type_str, "Geographical Area");
        } else if(area_type==RPGP_AT_POLYLINE) {
            strcpy(sub_type_str, "Geographical Polyline");
        } 
        fprintf(stderr, "TEST - DRAW AREA COMPONENT, type %s \n",sub_type_str);
        fprintf(stderr, "TEST - location type %s \n",sub_type_str2);  
        fprintf(stderr, "TEST - number of points: %d \n", area_comp->numof_points);
        } /*  end TEST CODE 1 ///////////// */
#endif


    } else if(location_flag == PREFS_FRAME) {
        
        area_type = RPGP_AT_AREA;
        
    } /* end if PREFS_FRAME     */
    

    /*** Section 1 B *******************************************/
    /* Determine resolution, number of points, and point array */

    if(location_flag == PRODUCT_FRAME) {
        /* This looks kludgy, however the Legacy rescale factor assumed 0.54 NM*/
        /*   product displayed on a PUP                                        */
        /* In order to be able to display over other product resolutions : */
        
        /* resolution of the base image displayed */
        base_res = res_index_to_res(sd->resolution);   
            
        /* configured resolution of this product */
        hdr = (Prod_header *)(sd->icd_product);
        prod_res_ind = assoc_access_i(product_res, hdr->g.prod_id);
        prod_res = res_index_to_res(*prod_res_ind);
        
        
        /*  WE CONVERT ALL POSTITIONS TO 1/4 KM COORDINATES SO THAT THIS */
        /*  PRODUCT CAN EASILY BE DISPLAYED WITH TRADITIONAL PRODUCTS */

        /* CVG 9.0 - use new function for scale and center calculations */
        geo_scale_and_center(pwidth, pheight, sd->scale_factor, 
                     sd->x_center_offset, sd->y_center_offset, 
                     base_res, prod_res, &x_scale, &y_scale,
                                  &center_pixel, &center_scanl);


        /* get the data length (in this case number of points) */
        num_points = area_comp->numof_points;
    
        /*  test for errors in product */
        if( num_points==0 ) {
            fprintf(stderr,
              "POSSIBLE ERROR - Area component (index %d) contains no points.\n",
                                                       index);
            return;
        } /*  end no points */
    
        if( (num_points>1) && (area_type==RPGP_AT_POINT) ) {    
            fprintf(stderr,
              "ERROR - Improper Use of Area Component (index %d).\n", index);
            fprintf(stderr,    
              "   Area type Geographical Point contains multiple (%d) points.\n",
                                                                   num_points);
        } /*  end more than one single point */
        
        
        if( num_points==1 ) {
            Xloc_ptr = s_point;
        } else {
            m_points = (XPoint *)malloc(num_points * sizeof(XPoint));
            Xloc_ptr = m_points;
        }


    } else if(location_flag == PREFS_FRAME) {
        
        Xloc_ptr = prev_line;
        num_points = 5;
               
    } /*  end if PREFS_FRAME */



    /*** Section 1 C *******************************************/
    /* Construct symbol label                                  */

    
    /* create and select label type ---------------------------*/
    if(location_flag == PRODUCT_FRAME) {
        gp = (Graphic_product *)(sd->icd_product+96);
        /* get the short product name and create the label*/
        mnemonic = assoc_access_s(product_mnemonics, hdr->g.prod_id);  
        product_code =  gp->prod_code;

    } else if(location_flag == PREFS_FRAME) {
        mnemonic = prev_mnemonic;
        product_code = 999; /*  mot used */
    }

    
    if(mnemonic == NULL) {
        sprintf(label_txt, "%d-%d", product_code, index+1);
        
    } else {
        strcpy(prod_mnemon, mnemonic); 
        token = strtok(prod_mnemon, " ");
        if(token == NULL) 
            sprintf(label_txt, "%s-%d", " ", index+1);
        else  
            sprintf(label_txt, "%s-%d", token, index+1);
    }

    if(area_label==AREA_LBL_NONE) {
        strcpy(label_txt, "");
         
    } else if(area_label==AREA_LBL_MNEMONIC) {
        if(mnemonic == NULL)
            sprintf(label_txt, "%d", product_code);
        else {
            strcpy(prod_mnemon, mnemonic); 
            token = strtok(prod_mnemon, " ");
            if(token == NULL)
                sprintf(label_txt, "%s", " ");
            else 
                sprintf(label_txt, "%s", token);
        }
        
    } else if(area_label==AREA_LBL_COMP_NUM) {
        sprintf(label_txt, "%d", index+1);
        
    } else if(area_label==AREA_LBL_BOTH) {
        if(mnemonic == NULL)
            sprintf(label_txt, "%d-%d", product_code, index+1);
        else {
            strcpy(prod_mnemon, mnemonic); 
            token = strtok(prod_mnemon, " ");
            if(token == NULL)
                sprintf(label_txt, "%s-%d", " ", index+1);
            else 
                sprintf(label_txt, "%s-%d", token, index+1);
        }
    }

    /* end create and select label type -----------------------*/ 
       
    /*  DESIRED ENHANCEMENTS: */
    /*       1. A method for specifying symbol_type. */
    /*       2. A method for selecting the color. */
    /*       3. A method for specifying the label (9 char max).     */
    /*  Do we use parameters or CVG configuration to accomplish this? */


    /**** SECTION 2 ***********************************************************/
    /* Reading Area Component Parameters   (not accomplished in pref preview) */
    /**************************************************************************/
    
    /*  FUTURE ENHANCEMENT */
    

    
    /**** SECTION 3 ***********************************************************/
    /* Recording All Points in the Component (points preset in pref preview)  */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {
    
        /* record all points in component, */
        for(p=0; p<num_points; p++) {
    
            /*  read the point and convert to xy */
            if(loc_type==RPGP_LATLON_LOCATION) {  
                latlon_loc = (RPGP_location_t *) area_comp->points;
                ret = _88D_latlon_to_xy( latlon_loc[p].lat, latlon_loc[p].lon, 
                                                          &xin, &yin ); 
#ifdef TEST
                /* this output is only valid for a centered image since */
                /* xin and yin are not corrected for the center offset  */
                { /* // TEST 2 //////////////////// */
                fprintf(stderr,"TEST 2 OUTPUT ONLY VALID FOR CENTERED IMAGE\n");
                fprintf(stderr, 
                    "TEST2 - Latitude input is %f, Longitude input is %f\n",
                                          latlon_loc[p].lat, latlon_loc[p].lon); 
                fprintf(stderr, 
                    "TEST2 - Derived x is %f KM, derived y is %f KM\n",
                                                                      xin, yin);
                ret = _88D_xy_to_azran( xin, yin, &range, &azm );
                fprintf(stderr, 
                    "TEST2 - Derived azimuth is %f deg, Derived range is %f KM\n",
                                                                     azm, range);
                } /* // END TEST 2 //////////////// */
#endif
                
            } else if(loc_type==RPGP_XY_LOCATION) { 
                xy_loc = (RPGP_xy_location_t *) area_comp->points;
                xin = xy_loc[p].x;
                yin = xy_loc[p].y;
#ifdef TEST
                /* this output is only valid for a centered image since */
                /* xin and yin are not corrected for the center offset  */
                { /* // TEST 2 //////////////////// */
                fprintf(stderr,"TEST 2 OUTPUT ONLY VALID FOR CENTERED IMAGE\n");
                fprintf(stderr, 
                    "TEST2 - x input is %f KM, y input is %f KM\n",
                                                                  xin, yin);
                ret = _88D_xy_to_azran( xin, yin, &range, &azm );
                fprintf(stderr, 
                    "TEST2 - Derived azimuth is %f deg, Derived range is %f KM\n",
                                                                     azm, range);
                } /* // END TEST 2 //////////////// */
#endif
    
            } else if(loc_type==RPGP_AZRAN_LOCATION) {  
                azran_loc = (RPGP_azran_location_t *) area_comp->points;
                ret = _88D_azran_to_xy( azran_loc[p].range, azran_loc[p].azi, 
                                                            &xin, &yin );
#ifdef TEST
                /* this output is only valid for a centered image since */
                /* xin and yin are not corrected for the center offset  */
                { /* // TEST 2 //////////////////// */
                fprintf(stderr,"TEST 2 OUTPUT ONLY VALID FOR CENTERED IMAGE\n");
                fprintf(stderr, 
                    "TEST2 - Axumuth input is %f deg, Range input is %f KM\n",
                                          azran_loc[p].azi, azran_loc[p].range); 
                fprintf(stderr, 
                    "TEST2 - Derived x is %f KM, derived y is %f KM\n",
                                                                  xin, yin);
                } /* // END TEST 2 //////////////// */
#endif
    
            }
            
            /*  convert to 1/4 km xy (xpos & ypos) */
            xpos = 4.0*xin;
            ypos = 4.0*yin;
                    
            /*  This logic is based upon 1/4 KM coordinates with the center */
            /*  being the radar location.  Traditional data packets for  */
            /*  overlay packets (symbols, text, and vectors) used 1/4 KM */
            /*  screen coordinates from the upper left corner as input.    */
            pixel = _88D_Round(xpos*x_scale);
            scanl = _88D_Round(ypos*y_scale);         
            /*  add the point to Xpoint array as screen coordinates for display */
            Xloc_ptr[p].x = pixel+center_pixel;
            Xloc_ptr[p].y = scanl+center_scanl;
            
    
#ifdef TEST
            { /*  TEST 3 CODE////////////////// */
            fprintf(stderr, 
                "TEST3 - DRAW AREA COMPONENT, Symbol\n");
            fprintf(stderr, 
                "TEST3 - inx is %f, in y is %f \n", xin, yin);
            fprintf(stderr, 
                "TEST3 - 1/4KM X is %f, 1/4KM Y is %f, scale factor is %f\n", 
                                                            xpos, ypos, x_scale);
            fprintf(stderr, 
                "TEST3 - X from center is %d pixels, Y from center is %d pixels\n", 
                                                                     pixel, scanl);
            fprintf(stderr, 
                "TEST3 - X screen is %d pixels, Y screen is %d pixels\n", 
                                                   Xloc_ptr[p].x, Xloc_ptr[p].y);
            fprintf(stderr, "TEST3 - symbol label is %s\n", label_txt);
            } /*  END TEST 3 CODE ///////////// */
#endif
            
        } /*  end for each point, record point */


    } else if(location_flag == PREFS_FRAME) {

        prev_point[0].x = 40;
        prev_point[0].y = 65;
        prev_point[1].x = 90;
        prev_point[1].y = 20;
    
        prev_line[0].x = 140;
        prev_line[0].y = 70;
        prev_line[1].x = 190;
        prev_line[1].y = 45;
        prev_line[2].x = 240;
        prev_line[2].y = 20;
        prev_line[3].x = 340;
        prev_line[3].y = 35;
        prev_line[4].x = 290;
        prev_line[4].y = 55;
        
    } /*  end location PREFS_FRAME */
    


    /**** SECTION 4 ***********************************************************/
    /* Displaying All Points (with lines as appropriate) in the Area Component*/
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME)
        area_pix = sd->pixmap;
    else if(location_flag == PREFS_FRAME)
        area_pix = area_prev_pix;


    /* currently set by prefs */
    symbol_type = area_symbol;

    /*  DEFAULT VALUES SET FOR COLOR ATTRIBUTES */
    /* initially the colors in symbol_pkt28.plt are :
         * 0 - P28_BLACK,  1 - P28_WHITE, 2 - P28_YELLOW, 3 - P28_RED, 
         * 4 - P28_GREEN, 5 - P28_BLUE, 6 - P28_ORANGE, 7 - P28_GRAY  */
    /* PERHAPS WE SHOULD JUST USE FOREGROUND & BACKGROUND COLORS FOR SYMBOLS */
    symbol_color = P28_BLACK;  /*  OUTLINED IN WHITE */
    line_color = P28_WHITE;
   
   
   
    /* CVG 9.0 - REMOVED opening the default palette file and calling */
    /*             setup_palette() because this is redundant with       */
    /*             dispatch_packet_type()                               */     
    
    /*  First, draw connecting lines if not a Geographic Point */

    if(location_flag == PRODUCT_FRAME) {

        draw_area_line(Xloc_ptr, num_points, area_type, line_color, 
                                                         area_line_type);

    } else if(location_flag == PREFS_FRAME) {

        draw_area_line(Xloc_ptr, num_points, RPGP_AT_AREA, line_color, 
                                                         area_line_type);
        
        
    } /*  end if PREFS_FRAME */



    /*  Second, Draw the appropriate symbols after the lines */
        
        /*  DESIRED ENHANCEMENTS: */
        /*       1. A method for specifying symbol_type. */
        /*       2. A method for selecting the color. */
        /*       3. A method for specifying the label. */
        /*  Do we use parameters or CVG configuration to accomplish this? */


    if(location_flag == PRODUCT_FRAME) {

        if( (area_type==RPGP_AT_POINT)  || 
            (area_type!=RPGP_AT_POINT && include_points_flag==TRUE) ) {
    
            for(p=0; p<num_points; p++) {                    
                draw_area_symbol(symbol_type, symbol_color, 
                                 Xloc_ptr[p].x, Xloc_ptr[p].y);           
                draw_area_label(label_txt, Xloc_ptr[p].x, Xloc_ptr[p].y);    
            }
            
        }


    } else if(location_flag == PREFS_FRAME) {
        
        draw_area_symbol(symbol_type, symbol_color, 
                         prev_point[0].x, prev_point[0].y);
        if(area_label==AREA_LBL_COMP_NUM) /* artificial component number */
            sprintf(label_txt, "1");
        if(area_label==AREA_LBL_BOTH) 
            sprintf(label_txt, "MNE-1");
        draw_area_label(label_txt, prev_point[0].x, prev_point[0].y); 
        draw_area_symbol(symbol_type, symbol_color, 
                         prev_point[1].x, prev_point[1].y);
        if(area_label==AREA_LBL_COMP_NUM) /* artificial component number */
            sprintf(label_txt, "2");
        if(area_label==AREA_LBL_BOTH) 
            sprintf(label_txt, "MNE-2");
        draw_area_label(label_txt, prev_point[1].x, prev_point[1].y); 

        if(include_points_flag==TRUE) {
            
            for(p=0; p<num_points; p++) {                    
                draw_area_symbol(symbol_type, symbol_color, 
                                 Xloc_ptr[p].x, Xloc_ptr[p].y);
                if(area_label==AREA_LBL_COMP_NUM) /* artificial component number */
                    sprintf(label_txt, "3");
                if(area_label==AREA_LBL_BOTH) 
                    sprintf(label_txt, "MNE-3");
                draw_area_label(label_txt, Xloc_ptr[p].x, Xloc_ptr[p].y);    
            }
            
        }
        
    } /*  end if PREFS_FRAME */
    


    /**** SECTION 5 ***********************************************************/
    /* Cleanup                                                                */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {    
        if( num_points>1 && Xloc_ptr != NULL) {
            free(Xloc_ptr); 
            Xloc_ptr = NULL;
        }
        
    } else  if(location_flag == PRODUCT_FRAME) {    
        if(mnemonic != NULL)
            free(mnemonic);           
    }

    /* reset line width to 1 pixel */
    XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);
    
    
} /*  end display_area */





/* /////////////////////////////////////////////////////////////////////////////// */
/*  helper functions */
/* /////////////////////////////////////////////////////////////////////////////// */

void draw_area_symbol(s_type, s_color, x_pix, y_pix) {

    int size=6;
    unsigned int width;

    XPoint  s_X[5];

    
    /* we use 3 pixels and 2 pixels basic line width */
    if((s_type == 1) || (s_type == 2))
         width = 3;
    else
         width = 2;

    
    XSetForeground(display, gc, display_colors[s_color].pixel);
    XSetLineAttributes (display, gc, width, LineSolid, CapButt, JoinMiter);

#ifdef TEST
{ /*  TEST 5 ////////////////////// */
fprintf(stderr, "\nTEST5 - draw_area_symbol() \n");
fprintf(stderr, "TEST5 - type = %d, width = %d, x pix = %d, y pix = %d\n",
                                                s_type, width, x_pix, y_pix);
} /*  END TEST 5 ////////////////// */
#endif

    if( (s_type == 0) ) { /* (types 0) are points */

        /*  white point */
        XSetLineAttributes (display, gc, 3, LineSolid, CapButt, JoinMiter);
        XSetForeground(display, gc, white_color);

        XDrawArc(display, area_pix, gc, x_pix-(2), y_pix-(2), 
                                          (2)*2, (2)*2, 0, -(360*64));

    } else if( (s_type == 1) ) { /* (types 1) are complete circles */

        /*  colored complete circle */
        XDrawArc(display, area_pix, gc, x_pix-(size), y_pix-(size), 
                                          (size)*2, (size)*2, 0, -(360*64));

        /*  white outline for complete circle */
        XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);
        XSetForeground(display, gc, white_color);
        XDrawArc(display, area_pix, gc, x_pix-(size+width), y_pix-(size+width), 
                                   (size+width)*2, (size+width)*2, 0, -(360*64));
                   
    } else if( (s_type == 2) ){ /* (types 2) are segmented circles */
        /*  colored segmented circle */
        XDrawArc(display, area_pix, gc, x_pix-(size), y_pix-(size), 
                                          (size)*2, (size)*2, 55*64, 70*64);
        XDrawArc(display, area_pix, gc, x_pix-(size), y_pix-(size), 
                                          (size)*2, (size)*2, 145*64, 70*64);
        XDrawArc(display, area_pix, gc, x_pix-(size), y_pix-(size), 
                                          (size)*2, (size)*2, 235*64, 70*64);
        XDrawArc(display, area_pix, gc, x_pix-(size), y_pix-(size), 
                                          (size)*2, (size)*2, 325*64, 70*64);

        /*  white outline for segmented circle */
        XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);
        XSetForeground(display, gc, white_color);
        XDrawArc(display, area_pix, gc, x_pix-(size+width), y_pix-(size+width), 
                                   (size+width)*2, (size+width)*2, 55*64, 70*64);
        XDrawArc(display, area_pix, gc, x_pix-(size+width), y_pix-(size+width), 
                                  (size+width)*2, (size+width)*2, 145*64, 70*64);
        XDrawArc(display, area_pix, gc, x_pix-(size+width), y_pix-(size+width), 
                                  (size+width)*2, (size+width)*2, 235*64, 70*64);
        XDrawArc(display, area_pix, gc, x_pix-(size+width), y_pix-(size+width), 
                                  (size+width)*2, (size+width)*2, 325*64, 70*64);        
                   
    } else if( (s_type == 3) ) { /* (types 3) are diamonds  */
        /*  colored diamond */
        s_X[0].x = x_pix;
        s_X[0].y = y_pix+(size+2);
        s_X[1].x = x_pix+(size+2);
        s_X[1].y = y_pix;
        s_X[2].x = x_pix;
        s_X[2].y = y_pix-(size+2);
        s_X[3].x = x_pix-(size+2);
        s_X[3].y = y_pix;
        s_X[4].x = x_pix;
        s_X[4].y = y_pix+(size+2);
        XDrawLines(display, area_pix, gc, s_X, 5, CoordModeOrigin);  
        

        /*  a white outline for the diamond */
        XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);
        XSetForeground(display, gc, white_color);
        s_X[0].x = x_pix;
        s_X[0].y = y_pix+(size+2+width);
        s_X[1].x = x_pix+(size+2+width);
        s_X[1].y = y_pix;
                   s_X[2].x = x_pix;
        s_X[2].y = y_pix-(size+2+width);
        s_X[3].x = x_pix-(size+2+width);
        s_X[3].y = y_pix;
        s_X[4].x = x_pix;
        s_X[4].y = y_pix+(size+2+width);            
        XDrawLines(display, area_pix, gc, s_X, 5, CoordModeOrigin);

        
    } else if( (s_type == 4) ) { /* (types 4) are squares   */
        /*  colored square */
        s_X[0].x = x_pix+(size);
        s_X[0].y = y_pix+(size);
        s_X[1].x = x_pix+(size);
        s_X[1].y = y_pix-(size);
        s_X[2].x = x_pix-(size);
        s_X[2].y = y_pix-(size);
        s_X[3].x = x_pix-(size);
        s_X[3].y = y_pix+(size);
        s_X[4].x = x_pix+(size);
        s_X[4].y = y_pix+(size);
        XDrawLines(display, area_pix, gc, s_X, 5, CoordModeOrigin);
        

        /*  a white outline for the square */
        XSetLineAttributes (display, gc, 2, LineSolid, CapButt, JoinMiter);
        XSetForeground(display, gc, white_color);
        s_X[0].x = x_pix+(size+width);
        s_X[0].y = y_pix+(size+width);
        s_X[1].x = x_pix+(size+width);
        s_X[1].y = y_pix-(size+width);
        s_X[2].x = x_pix-(size+width);
        s_X[2].y = y_pix-(size+width);
        s_X[3].x = x_pix-(size+width);
        s_X[3].y = y_pix+(size+width);
        s_X[4].x = x_pix+(size+width);
        s_X[4].y = y_pix+(size+width); 
        XDrawLines(display, area_pix, gc, s_X, 5, CoordModeOrigin);                 
        
    }

    
    /* reset line width to 1 pixel */
    XSetLineAttributes (display, gc, 1, LineSolid, CapButt, JoinMiter);
    XSetForeground(display, gc, white_color);


} /*  end draw_area_symbol() */




void draw_area_label(char *label, int x_pix, int y_pix) {

int length = 2;

Boolean norm_format_flag = False;
Boolean bkgd_format_flag = False;

    if(area_pix == sd->pixmap) {
        if(sd==sd1) {
            norm_format_flag = norm_format1;
            bkgd_format_flag = bkgd_format1;
        } else if(sd==sd2) {
            norm_format_flag = norm_format2;
            bkgd_format_flag = bkgd_format2;
        } else if(sd==sd3) {
            norm_format_flag = norm_format3;
            bkgd_format_flag = bkgd_format3;
        }

    } else if(area_pix == area_prev_pix) { /*  picked black background */
        norm_format_flag = FALSE;
        bkgd_format_flag = TRUE;
    }

    length = strlen(label);
    
    XSetForeground(display, gc, white_color);

    if(norm_format_flag==TRUE)
        XDrawString(display, area_pix, gc, x_pix+4, 
                                 y_pix+14, label, length);
    else if(bkgd_format_flag==TRUE)
        XDrawImageString(display, area_pix, gc, x_pix+4, 
                                 y_pix+14, label, length);
    
} /*  end draw_area_label() */




void draw_area_line(XPoint  *points, int n_points, int a_type, int l_color, int l_type) 
{

    if(l_type==AREA_LINE_SOLID) {   
        XSetLineAttributes (display, gc,2, LineSolid, CapButt, JoinMiter);

    } else if(l_type==AREA_LINE_DASH_CLEAR) { 
        XSetLineAttributes (display, gc,2, LineOnOffDash, CapButt, JoinMiter);
        XSetDashes(display, gc, 0, dashed, 2);
      
    } else if(l_type==AREA_LINE_DASH_BLACK) { 
        XSetLineAttributes (display, gc,2, LineDoubleDash, CapButt, JoinMiter);
        XSetDashes(display, gc, 0, dashed, 2);
   
    } else if(l_type==AREA_LINE_DOTTED) { 
        XSetLineAttributes (display, gc,2, LineOnOffDash, CapButt, JoinMiter);
        XSetDashes(display, gc, 0, dotted, 2);
    
    }

    XSetForeground(display, gc, display_colors[l_color].pixel);
            
    if(a_type==RPGP_AT_AREA || a_type==RPGP_AT_POLYLINE)
        XDrawLines(display, area_pix, gc, points, n_points, CoordModeOrigin);
        
    if(a_type==RPGP_AT_AREA)   
        XDrawLine(display, area_pix, gc, points[0].x, points[0].y, 
                              points[n_points-1].x, points[n_points-1].y);
    

    XSetDashes(display, gc, 0, default_d, 2);

    
} /*  end draw_area_line */
