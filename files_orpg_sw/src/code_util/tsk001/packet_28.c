/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:46 $
 * $Id: packet_28.c,v 1.5 2009/05/15 17:52:46 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* packet_28.c */

#include <math.h>
#include <Xm/Xm.h>
#include "packet_28_cvg.h"

/* #define TEST_CONV DO.. */



void packet_28_skip(char *buffer,int *offset) 
{
    
    short align_hw;
    int ser_len;
    char *serial_data;  /* offset to the serialized data in the packet */

  
  /* This function only skips over the packet.  The serialized data is 
   * de-serialized in the calling function parse_packet_numbers()
   */
  
   
  
    align_hw = read_half_flip(buffer,offset);
    ser_len = read_word_flip(buffer,offset);

    if(verbose_flag)
        fprintf(stderr,"entering packet_28_skip(), offset of serial data is %d\n",*offset);

    /* use a local variable rather than offset ?? */
    serial_data = buffer+*offset;
    
    if(verbose_flag) {
        fprintf(stderr,"Contents of Align HW = %hd, ",align_hw);
        fprintf(stderr,"Length of Serialized Data = %d bytes\n",ser_len);
    }
    


    *offset+=ser_len;
    
    if(verbose_flag)
        fprintf(stderr, "Packet 28 ending offset is %d\n",*offset);


}


/* ////////////////////////////////////////////////////////// */
/*  Should we have separate decode and display functions? */
/*      Perhaps for the grid component.   */
/* ///////////////////////////////////////////////////////// */



void display_packet_28(int packet, int index, int replay)
{

#ifdef TEST_CONV
test_coord_conv();
#endif

     /* AN ALTERNATIVE DESIGN WOULD BE TO CALL INDIVIDUAL FUNCTION */
     /* display_generic_radial(), display_generic_area() etc. from */
     /* dispatch_packet_type()                                     */
     
     switch (packet) {
         case GENERIC_RADIAL_DATA: 
             display_radial(index, PRODUCT_FRAME, replay);
             break;
         case GENERIC_GRID_DATA:
             display_grid(index, PRODUCT_FRAME);
             break;
         case GENERIC_AREA_DATA:
             display_area(index, PRODUCT_FRAME);
             break;
         case GENERIC_TEXT_DATA:
             display_text(index, PRODUCT_FRAME);
             break;
         case GENERIC_TABLE_DATA:
             display_table(index, PRODUCT_FRAME);
             break;
         case GENERIC_EVENT_DATA:
             display_event(index, PRODUCT_FRAME);
             break;
         default:
             break;
     } /*  end switch */

} /*  end display_packet_28 */



/* ////////////////////////////////////////////////////////////////////////// */
/*  The following 'empty' functions are placeholders for the */
/*  component types not yet supported for display. */





/* /////////////////////////////////////////////////////////////// */
void display_grid(int index, int location_flag)
{



  RPGP_product_t *generic_product;
  RPGP_grid_t   *grid_comp=NULL;
/*   int sub_type, r; */
/*   char sub_type_str[30], sub_type_str2[10]; */
/*   size_t t_len; */
/*   char title_str[51];       */

    /**** SECTION 1 ***********************************************************/
    /* Reading Grid Component Header Information                              */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {
    
        generic_product = (RPGP_product_t *)sd->generic_prod_data;
        grid_comp = (RPGP_grid_t *)generic_product->components[index];

fprintf(stderr,"\n----- Display Generic Grid Data ------- index is %d\n\n", index);
    
/*  see get_component_subtype() in packetselect.c     */

        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }



    /**** SECTION 2 ***********************************************************/
    /* Reading Grid Component Parameters                                      */
    /**************************************************************************/
    
    /*  FUTURE ENHANCEMENT */
    

    
    /**** SECTION 3 ***********************************************************/
    /* Decoding Grid Component                                                */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {
        
        ;
        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }
    

    /**** SECTION 4 ***********************************************************/
    /* Displaying Grid Component                                              */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {
        
        ;
        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }
    

    /**** SECTION 5 ***********************************************************/
    /* Cleanup                                                                */
    /**************************************************************************/    

    if(location_flag == PRODUCT_FRAME) {
        
        ;
        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }
    
    
} /*  end display_grid */




/* /////////////////////////////////////////////////////////////// */
void display_text(int index, int location_flag)
{



  RPGP_product_t *generic_product;
  RPGP_text_t   *text_comp=NULL;
/*   int sub_type, r; */
/*   char sub_type_str[30], sub_type_str2[10]; */
/*   size_t t_len; */
/*   char title_str[51];       */

    /**** SECTION 1 ***********************************************************/
    /* Reading Text Component Header Information                              */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {
            
        generic_product = (RPGP_product_t *)sd->generic_prod_data;
        text_comp = (RPGP_text_t *)generic_product->components[index];

fprintf(stderr,"\n----- Display Generic Text Data ------- index is %d\n\n", index);


        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }
    

    /**** SECTION 2 ***********************************************************/
    /* Reading Text Component Parameters                                      */
    /**************************************************************************/
    
    /*  FUTURE ENHANCEMENT */
    

    
    /**** SECTION 3 ***********************************************************/
    /* Decoding Text Component                                                */
    /**************************************************************************/    

    if(location_flag == PRODUCT_FRAME) {
        
        ;
        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }


    /**** SECTION 4 ***********************************************************/
    /* Displaying Text Component                                              */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {
        
        ;
        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }


    /**** SECTION 5 ***********************************************************/
    /* Cleanup                                                                */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {
        
        ;
        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }
            
       
} /*  end display_text */




/* /////////////////////////////////////////////////////////////// */
void display_table(int index, int location_flag)
{



  RPGP_product_t *generic_product;
  RPGP_table_t   *table_comp=NULL;
/*   int sub_type, r; */
/*   char sub_type_str[30], sub_type_str2[10]; */
/*   size_t t_len; */
/*   char title_str[51];       */

    /**** SECTION 1 ***********************************************************/
    /* Reading Table Component Header Information                             */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {
    
        generic_product = (RPGP_product_t *)sd->generic_prod_data;
        table_comp = (RPGP_table_t *)generic_product->components[index];

fprintf(stderr,"\n----- Display Generic Table Data ------ index is %d\n\n", index);
        

        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }



    /**** SECTION 2 ***********************************************************/
    /* Reading Table Component Parameters                                     */
    /**************************************************************************/
    
    /*  FUTURE ENHANCEMENT */
    

    
    /**** SECTION 3 ***********************************************************/
    /* Decoding Table Component                                               */
    /**************************************************************************/    

    if(location_flag == PRODUCT_FRAME) {
        
        ;
        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }


    /**** SECTION 4 ***********************************************************/
    /* Displaying Table Component                                             */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {
        
        ;
        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }


    /**** SECTION 5 ***********************************************************/
    /* Cleanup                                                                */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {
        
        ;
        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }
            
        
} /*  end display_table */




/* /////////////////////////////////////////////////////////////// */
void display_event(int index, int location_flag)
{




  RPGP_product_t *generic_product;
  RPGP_event_t   *event_comp=NULL;
/*   int sub_type, r; */
/*   char sub_type_str[30], sub_type_str2[10]; */
/*   size_t t_len; */
/*   char title_str[51];       */

    /**** SECTION 1 ***********************************************************/
    /* Reading Event Component Header Information                             */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {
    
        generic_product = (RPGP_product_t *)sd->generic_prod_data;
        event_comp = (RPGP_event_t *)generic_product->components[index];

fprintf(stderr,"\n----- Display Generic Event Data ------ index is %d\n\n", index);
        

        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }
    
    
    /**** SECTION 2 ***********************************************************/
    /* Reading Event Component Parameters                                     */
    /**************************************************************************/
    
    /*  FUTURE ENHANCEMENT */
    

    
    /**** SECTION 3 ***********************************************************/
    /* Decoding Event Component                                               */
    /**************************************************************************/    

    if(location_flag == PRODUCT_FRAME) {
        
        ;
        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }


    /**** SECTION 4 ***********************************************************/
    /* Displaying Event Component                                             */
    /**************************************************************************/

    if(location_flag == PRODUCT_FRAME) {
        
        ;
        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }


    /**** SECTION 5 ***********************************************************/
    /* Cleanup                                                                */
    /**************************************************************************/    

    if(location_flag == PRODUCT_FRAME) {
        
        ;
        
    } else if(location_flag == PREFS_FRAME) {
        
        ;
        
    }
     
        
} /*  end display_event */

