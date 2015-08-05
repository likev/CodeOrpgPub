/********************************************************************************

       File: nbtcp_process_rpg_msgs.c
             This file contains all the routines used to process the RPG
             messages and to write the product messages as files to disk.

 ********************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/26 21:08:09 $
 * $Id: nbtcp_wmo_awips_hdr.c,v 1.6 2014/03/26 21:08:09 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#include <errno.h>

#include <time.h>        /* time (...) */
#include <ctype.h>	 /* toupper() */
#include <stdio.h>       
#include <product.h>
#include <misc.h>
#include <netinet/in.h>  /* ntohl(...)
                            ntohs(...)  */
#include "nbtcp.h"

static char  CRCRLF[4] = { 0x0d, 0x0d, 0x0a, 0x00 };
static int   Add_AWIPS_WMO_hdr = FALSE;
static WMO_header_t WMO_hdr;
static AWIPS_header_t AWIPS_hdr;
static char *Uc_coo = NULL;

#define MAX_PRODS	200
static Node_t Nodes[MAX_PRODS];

static void Init_awips_hdr();
static Node_t* Add_node( int code, int elev_min, int elev_max, char *cat );
static void Test_param( int elev_param, Node_t *node, WMO_AWIPS_hdr_t *header );


/********************************************************************************

     Description: This routine initializes the AWIPS/WMO headers.

           Input: icao - ICAO to add to WMO and AWIPS headers.

          Output:

          Return: 0 on success; -1 on error


 ********************************************************************************/

int WAH_add_awips_wmo_hdr ( char *icao )
{
   char uc_icao[5];
   int i;
   
   /* Initialize the ICAO. */
   memcpy( &uc_icao[0], "NOP4", 4 );
   uc_icao[4] = '\0';
    
   if( icao != NULL ){

      /* Make sure the ICAO is all caps. */
      i = 0;
      while( (icao[i] != '\0') && (i < 4)  ) {

         uc_icao[i] = toupper( icao[i] );
         i++;

      }

   }

   /* Set the flag indicating WMO/AWIPS header is being added. */
   Add_AWIPS_WMO_hdr = TRUE;
     
   /* Initialize fields of WMO header. */
   memset( &WMO_hdr.form_type[0], 0, sizeof(WMO_header_t) );

   /* Do individual fields of WMO header. */
   memcpy( &WMO_hdr.form_type[0], "SD", 2 );
   if( Uc_coo == NULL )
      memcpy( &WMO_hdr.geo_area[0], "US", 2 );
   else
      memcpy( &WMO_hdr.geo_area[0], Uc_coo, 2 );
   memcpy( &WMO_hdr.distribution[0], "00", 2 );
   WMO_hdr.space1 = ' ';
   memcpy( &WMO_hdr.originator[0], uc_icao, 4 );
   WMO_hdr.space2 = ' ';
   memcpy( &WMO_hdr.crcrlf, &CRCRLF[0], 3 );

   /* Initialize the AWIPS header. */
   memset( &AWIPS_hdr.category[0], 0, sizeof( AWIPS_header_t) );

   /* Do individual fields of AWIPS header. */
   memcpy( &AWIPS_hdr.category[0], "???", 3 );
   memcpy( &AWIPS_hdr.product[0], &uc_icao[1], 3 );
   memcpy( &AWIPS_hdr.crcrlf, &CRCRLF[0], 3 );

   /* Complete the awips header. */ 
   Init_awips_hdr();

   return Add_AWIPS_WMO_hdr;
}


/********************************************************************************

     Description: This routine adds Country of Origin to the WMO header if 
                  other than the default.

           Input: cntry_origin - Country of Origin to add to WMO header.

          Output:

          Return: 0 on success; -1 on error


 ********************************************************************************/

int WAH_add_country_of_origin ( char *cntry_origin )
{
   int i;
   
   /* Allocate space. */
   if( Uc_coo == NULL ){

      Uc_coo = calloc( 4, 1 );
      if( Uc_coo == NULL ){

         fprintf( stderr, "calloc failed for 4 bytes\n" );
         exit(1);

      }

   }

   /* Initialize the County of Origin. */
   memcpy( Uc_coo, "US", 2 );
   Uc_coo[2] = '\0';
    
   /* Make sure the Country of Origin is all caps. */
   if( cntry_origin != NULL ){

      i = 0;
      while( (cntry_origin[i] != '\0') && (i < 2)  ) {

         Uc_coo[i] = toupper( cntry_origin[i] );
         i++;

      }

   }

   return 0;
}


/********************************************************************************

     Description: This routine initializes AWIPS header information based on 
                  product code.   

          Input:

          Output:

          Return:

    Note: A future enhancement might be to move this information to config file.

 ********************************************************************************/

static void Init_awips_hdr()
{

   int i;
   Node_t *node = NULL, *node1 = NULL;

   /* Initialize all array elements. */
   for( i = 0; i < MAX_PRODS; i++ ){

      Nodes[i].cat_prod.prod_code = i;
      Nodes[i].cat_prod.elev_based = 0;
      Nodes[i].next = NULL;

      switch (i) {

         case 2:	/* General Status Message. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "GSM", 3 );
            break;
         }

         case 19:	/* Reflectivity. */
         {
            /* 0.4-0.8 deg. */
            memcpy( &Nodes[i].cat_prod.category[0], "N0R", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;

            node = Add_node( i, 12, 16, "N1R" );
            Nodes[i].next = node;
            
            node1 = Add_node( i, 21, 26, "N2R" );
            node->next = node1;

            node  = Add_node( i, 27, 35, "N3R" );
            node1->next = node;
            
            node1 = Add_node( i, 36, 195, "N?R" );
            node->next = node1;
            break;
         }

         case 20:	/* Reflectivity. */
         {
            /* 0.4-0.8 deg. */
            memcpy( &Nodes[i].cat_prod.category[0], "N0Z", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;

            node = Add_node( i, 9, 195, "N?Z" );
            Nodes[i].next = node;
            break;
         }

         case 25:	/* Velocity. */
         {
            /* 0.4-0.8 deg. */
            memcpy( &Nodes[i].cat_prod.category[0], "N0W", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;

            node = Add_node( i, 9, 195, "N?W" );
            Nodes[i].next = node;
            break;
         }

         case 27:	/* Velocity. */
         {
            /* 0.4-0.8 deg. */
            memcpy( &Nodes[i].cat_prod.category[0], "N0V", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;

            node = Add_node( i, 12, 16, "N1V" );
            Nodes[i].next = node;
            
            node1 = Add_node( i, 21, 26, "N2V" );
            node->next = node1;

            node = Add_node( i, 27, 35, "N3V" );
            node1->next = node;
            
            node1 = Add_node( i, 36, 195, "N?V" );
            node->next = node1;
            break;
         }

         case 28:	/* Spectrum Width. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NSP", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;
            break;
         }

         case 30:	/* Spectrum Width. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NSW", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;
            break;
         }

         case 31:	/* User Selectable Accumulation. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "N6P", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 6;
            Nodes[i].cat_prod.elev_max = 6;

            node = Add_node( i, 1, 24, "NUP" );
            Nodes[i].next = node;
            break;
         }

         case 32:	/* Digitial Hybrid Reflectivity. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "DHR", 3 );
            break;
         }

         case 34:	/* Clutter Filter Control. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NC1", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 1;
            Nodes[i].cat_prod.elev_max = 1;

            node = Add_node( i, 2, 2, "NC2" );
            Nodes[i].next = node;
            
            node1 = Add_node( i, 4, 4, "NC3" );
            node->next = node1;

            node = Add_node( i, 8, 8, "NC4" );
            node1->next = node;

            node1 = Add_node( i, 16, 16, "NC5" );
            node->next = node1;
            break;
         }

         case 36:	/* Composite Reflectivity. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NC0", 3 );
            break;
         }

         case 37:	/* Composite Reflectivity. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NCR", 3 );
            break;
         }

         case 38:	/* Composite Reflectivity. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NCZ", 3 );
            break;
         }

         case 41:	/* Echo Tops. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NET", 3 );
            break;
         }

         case 48:	/* VWP. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NVW", 3 );
            break;
         }

         case 56:	/* Storm Relative Velocity. */
         {
            /* 0.4-0.8 deg. */
            memcpy( &Nodes[i].cat_prod.category[0], "N0S", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;

            node = Add_node( i, 12, 16, "N1S" );
            Nodes[i].next = node;
            
            node1 = Add_node( i, 21, 26, "N2S" );
            node->next = node1;

            node = Add_node( i, 27, 35, "N3S" );
            node1->next = node;
            
            node1 = Add_node( i, 36, 195, "N?S" );
            node->next = node1;
            break;
         }

         case 57:	/* VIL. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NVL", 3 );
            break;
         }

         case 58:	/* STI. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NST", 3 );
            break;
         }

         case 59:	/* Hail Index. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NHI", 3 );
            break;
         }

         case 61:	/* TVS. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NTV", 3 );
            break;
         }

         case 62:	/* Storm Structure. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NSS", 3 );
            break;
         }

         case 65:	/* LCR Max Low. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NLL", 3 );
            break;
         }

         case 66:	/* LCR Max Middle. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NML", 3 );
            break;
         }

         case 67:	/* LCR Max AP Removed. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NLA", 3 );
            break;
         }

         case 74:	/* RCM. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "RCM", 3 );
            break;
         }

         case 78:	/* One hour Precipitation Accumulation. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "N1P", 3 );
            break;
         }

         case 79:	/* Three hour Precipitation Accumulation. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "N3P", 3 );
            break;
         }

         case 80:	/* Storm Total Precipitation. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NTP", 3 );
            break;
         }

         case 81:	/* Storm Total Precipitation. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "DPA", 3 );
            break;
         }

         case 90:	/* LCR Max High. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NHL", 3 );
            break;
         }

         case 94:	/* Reflectivity. */
         {
            /* 0.4-0.8 deg. */
            memcpy( &Nodes[i].cat_prod.category[0], "N0Q", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;

            node = Add_node( i, 12, 16, "N1Q" );
            Nodes[i].next = node;
            
            node1 = Add_node( i, 21, 26, "N2Q" );
            node->next = node1;
            
            node = Add_node( i, 27, 36, "N3Q" );
            node1->next = node;

            node1 = Add_node( i, 9, 11, "NAQ" );
            node->next = node1;

            node = Add_node( i, 17, 20, "NBQ" );
            node1->next = node;

            node1 = Add_node( i, 37, 195, "N?Q" );
            node->next = node1;
            break;
         }

         case 99:	/* Velocity. */
         {
            /* 0.4-0.8 deg. */
            memcpy( &Nodes[i].cat_prod.category[0], "N0U", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;

            node = Add_node( i, 12, 16, "N1U" );
            Nodes[i].next = node;
            
            node1 = Add_node( i, 21, 26, "N2U" );
            node->next = node1;

            node = Add_node( i, 27, 36, "N3U" );
            node1->next = node;

            node1 = Add_node( i, 9, 11, "NAU" );
            node->next = node1;

            node = Add_node( i, 17, 20, "NBU" );
            node1->next = node;

            node1 = Add_node( i, 37, 195, "N?U" );
            node->next = node1;
            break;
         }

         case 134:	/* Digital VIL. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "DVL", 3 );
            break;
         }

         case 135:	/* Enhanced Echo Tops. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "EET", 3 );
            break;
         }

         case 138:	/* Digital Accummulation Array. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "DSP", 3 );
            break;
         }

         case 141:	/* Mesocyclone. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "NMD", 3 );
            break;
         }

         case 152:	/* Radar Status Log. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "RSL", 3 );
            break;
         }

         case 159:	/* Differential Reflectivity. */
         {
            /* 0.4-0.8 deg. */
            memcpy( &Nodes[i].cat_prod.category[0], "N0X", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;

            node = Add_node( i, 12, 16, "N1X" );
            Nodes[i].next = node;
            
            node1 = Add_node( i, 21, 26, "N2X" );
            node->next = node1;

            node = Add_node( i, 27, 36, "N3X" );
            node1->next = node;

            node1 = Add_node( i, 9, 11, "NAX" );
            node->next = node1;

            node = Add_node( i, 17, 20, "NBX" );
            node1->next = node;

            node1 = Add_node( i, 37, 195, "N?X" );
            node->next = node1;
            break;
         }

         case 161:	/* Correlation Coefficient. */
         {
            /* 0.4-0.8 deg. */
            memcpy( &Nodes[i].cat_prod.category[0], "N0C", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;

            node = Add_node( i, 12, 16, "N1C" );
            Nodes[i].next = node;
            
            node1 = Add_node( i, 21, 26, "N2C" );
            node->next = node1;

            node = Add_node( i, 27, 36, "N3C" );
            node1->next = node;

            node1 = Add_node( i, 9, 11, "NAC" );
            node->next = node1;

            node = Add_node( i, 17, 20, "NBC" );
            node1->next = node;

            node1 = Add_node( i, 37, 195, "N?C" );
            node->next = node1;
            break;
         }

         case 163:	/* Specific Differential Phase. */
         {
            /* 0.4-0.8 deg. */
            memcpy( &Nodes[i].cat_prod.category[0], "N0K", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;

            node = Add_node( i, 12, 16, "N1K" );
            Nodes[i].next = node;
            
            node1 = Add_node( i, 21, 26, "N2K" );
            node->next = node1;

            node = Add_node( i, 27, 36, "N3K" );
            node1->next = node;

            node1 = Add_node( i, 9, 11, "NAK" );
            node->next = node1;

            node = Add_node( i, 17, 20, "NBK" );
            node1->next = node;

            node1 = Add_node( i, 37, 195, "N?K" );
            node->next = node1;
            break;
         }

         case 165:	/* Hydrometeor Class. */
         {
            /* 0.4-0.8 deg. */
            memcpy( &Nodes[i].cat_prod.category[0], "N0H", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;

            node = Add_node( i, 12, 16, "N1H" );
            Nodes[i].next = node;
            
            node1 = Add_node( i, 21, 26, "N2H" );
            node->next = node1;

            node = Add_node( i, 27, 36, "N3H" );
            node1->next = node;

            node1 = Add_node( i, 9, 11, "NAH" );
            node->next = node1;

            node = Add_node( i, 17, 20, "NBH" );
            node1->next = node;

            node1 = Add_node( i, 37, 195, "N?H" );
            node->next = node1;
            break;
         }

         case 166:	/* Melting Layer. */
         {
            /* 0.4-0.8 deg. */
            memcpy( &Nodes[i].cat_prod.category[0], "N0M", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 4;
            Nodes[i].cat_prod.elev_max = 8;

            node = Add_node( i, 12, 16, "N1M" );
            Nodes[i].next = node;
            
            node1 = Add_node( i, 21, 26, "N2M" );
            node->next = node1;

            node = Add_node( i, 27, 36, "N3M" );
            node1->next = node;

            node1 = Add_node( i, 9, 11, "NAM" );
            node->next = node1;

            node = Add_node( i, 17, 20, "NBM" );
            node1->next = node;

            node1 = Add_node( i, 37, 195, "N?M" );
            node->next = node1;
            break;
         }

         case 169:	/* QPE One HR Accummulation. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "OHA", 3 );
            break;
         }

         case 170:	/* Digital Accummulation Array. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "DAA", 3 );
            break;
         }

         case 171:	/* QPE Storm Total Accummulation. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "PTA", 3 );
            break;
         }

         case 172:	/* Digital Storm Total Accummulation. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "DTA", 3 );
            break;
         }

         case 173:	/* Digital 3HR  and 24HR Accummulations. */
         {
            /* 3HR accumulation. */
            memcpy( &Nodes[i].cat_prod.category[0], "DU3", 3 );
            Nodes[i].cat_prod.elev_based = 1;
            Nodes[i].cat_prod.elev_min = 180;
            Nodes[i].cat_prod.elev_max = 180;

            node = Add_node( i, 1440, 1440, "DU6" );
            Nodes[i].next = node;
            break;
         }

         case 174:	/* One Hour Accummulation Difference. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "DOD", 3 );
            break;
         }

         case 175:	/* Total Accummulation Difference. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "DSD", 3 );
            break;
         }

         case 176:	/* Digital Precipitation Rate. */
         {
            memcpy( &Nodes[i].cat_prod.category[0], "DPR", 3 );
            break;
         }

         default:
         {
            memcpy( &Nodes[i].cat_prod.category[0], "???", 3 );
            break;
         }

      }

   }

}

/********************************************************************************

     Description: This routine initializes AWIPS header information.   

          Input: msg_code         - the ICD message code
                 elev_min         - used to distinguish products having same
                                    message code
                 elev_max         - used to distinguish products having same
                                    message code
                 cat              - category nmemonic

          Output:

          Return:

********************************************************************************/

static Node_t* Add_node( int code, int elev_min, int elev_max, char *cat )
{

   Node_t *node;

   /* calloc space for additional node. */
   node = (Node_t *) calloc( sizeof(Node_t), 1 );
   if( node == NULL ){
          
      char text[128];
      sprintf( &text[0], "calloc Failed for %d bytes\n", sizeof(Node_t) );
      MA_abort(text);

   }

   node->cat_prod.prod_code = code;
   node->cat_prod.elev_based = 1;
   node->cat_prod.elev_min = elev_min;
   node->cat_prod.elev_max = elev_max;
   memcpy( &node->cat_prod.category[0], cat, 3 );
   node->next = NULL;

   return node;

}


/********************************************************************************

	Description: This routine adds the WMO/AWIPS header.   

	Input:	buf		- buffer holding product
		msg_len		- length of "buf", in bytes
		code		- product code associated with buf

	Output:

	Return:
		buf 		- modified buffer with WMO/AWIPS header if 
				  needed, or NULL.

	Note: A future enhancement might be to move this information to config file.

 ********************************************************************************/

char* WAH_add_header( char *buf, int msg_len, int code )
{

   time_t ctime = 0;
   int year, mon, day, hr, min, sec;
   WMO_AWIPS_hdr_t header;
   Node_t *node = &Nodes[code];
   short elev_param;
   char text[8];
   char *temp = NULL;

   /* If add header flag is not set, just return. */
   if( Add_AWIPS_WMO_hdr != TRUE )
      return NULL;

   temp = calloc( msg_len+WMO_HEADER_SIZE , 1 );
   if( temp == NULL ){

      char text[128];
      sprintf( &text[0], "calloc Failed for %d bytes\n", msg_len );
      MA_abort(text);

   }

   memcpy( &header.wmo, &WMO_hdr, sizeof(WMO_header_t) );
   memcpy( &header.awips, &AWIPS_hdr, sizeof(AWIPS_header_t) );

   /* Put in the category value ... this may change if product
      has mutliple elevations. */
   memcpy( &header.awips.category[0], &node->cat_prod.category[0], 3 );

   /* Put in date/time stamp. */
   ctime = time(NULL);
   unix_time( &ctime, &year, &mon, &day, &hr, &min, &sec );
   sprintf( &text[0], "%02d%02d%02d", day, hr, min );
   memcpy( &header.wmo.date_time[0], text, 6 );

   /* Add product specific information. */
   if( node->cat_prod.elev_based ){

      Graphic_product *phd = (Graphic_product *) buf;

      if( code == 31 /* User Selectable Accumulation */ ){

         /* Check node(s). */
         elev_param = ntohs( phd->param_2 );
         Test_param( elev_param, node, &header );

      }
      else if( code == 34 /* Clutter Filter Control */ ){

         /* Check node(s). */
         elev_param = ntohs( phd->param_1 );
         Test_param( elev_param, node, &header );

      }
      else if( code == 173 /* Digital Accumulation */ ){

         /* Check node(s). */
         elev_param = ntohs( phd->param_2 );
         Test_param( elev_param, node, &header );

      }
      else{  

         /* Check node(s). */
         elev_param = ntohs( phd->param_3 );
         Test_param( elev_param, node, &header );

      }

   }

   /* Copy all data to temp. */
   memcpy( temp, &header, WMO_HEADER_SIZE );
   memcpy( temp + WMO_HEADER_SIZE, buf, msg_len );
   buf = temp; 

   return buf;
}


/********************************************************************************

	Description: This routine test product dependent parameter against 
                     min/max values in node.   

	Input:	elev_param	- product dependent parameter
		node		- nodes containing product information 
		header		- WMO/AWIPS header structure

	Output:

	Return:
		buf 		- modified buffer with WMO/AWIPS header if 
				  needed, or NULL.

	Note: A future enhancement might be to move this information to config file.

********************************************************************************/

static void Test_param( int elev_param, Node_t *node, WMO_AWIPS_hdr_t *header )
{

   int match = 0;

   if( (elev_param >= node->cat_prod.elev_min)
                      &&
       (elev_param <= node->cat_prod.elev_max) )
      memcpy( &header->awips.category[0], &node->cat_prod.category[0], 3 );

   /* Check all additional nodes. */
   else{

      /* Find node that meets elev_min, elev_max criteria. */
      while( node->next != NULL ){

         node = node->next;
         if( (elev_param >= node->cat_prod.elev_min)
                            &&
             (elev_param <= node->cat_prod.elev_max) ){

            memcpy( &header->awips.category[0], &node->cat_prod.category[0], 3 );
            match = 1;
            break;

         }

      }

      /* If no match found, change category value to "???" */
      if( !match )
         memcpy( &header->awips.category[0], "???", 3 );

   }

}
