/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 21:45:40 $
 * $Id: prod_size.c,v 1.16 2014/11/07 21:45:40 steves Exp $
 * $Revision: 1.16 $
 * $State: Exp $
*/


#include <stdlib.h>
#include <orpg.h>
#include <rpg_globals.h>
#include <dirent.h>
#include <legacy_prod.h>
#include <orpgdbm.h>

/* Macros */
#define PERMITTED_ELEV_DIFFERENCE	3
#define SMALL_ELEV_DIFFERENCE		1
#define MAX_FIELDS			20
#define MAX_VOLUME_TIMES		100
#define MAX_PRODUCTS      		6000

typedef struct volume_totals {

   int num_prods;
   
   int num_bytes;

} Volume_totals_t;

/* Global variables. */
int All_products;
int Data_base_query;
int Read_stats_lb;
int Read_directory;
int By_volume_time;
int Vol_cov_patt;
int Terse_mode = 0;
int Num_ele = 0;
int Num_tbl_items = 0;
int Products_found = 0;
int Unformatted = 0;
float Elev_angles[MAX_ELEVATION_CUTS];
DIR *Product_directory = NULL;
char Directory_path[256];

/* Note: Db_info should be sized to the larger of MAX_VOLUME_TIMES
         and MAX_PRODUCTS. */ 
RPG_prod_rec_t Db_info[MAX_PRODUCTS];
int Dir_info[MAX_VOLUME_TIMES];
int Num_volumes = 0;
Volume_totals_t *Volume_totals = NULL;

/* Function prototypes. */
static void Process_data_base();
static void Process_data_base_by_volume();
static void Process_directory();
static void Process_directory_by_volume();
static void Process_statistics();
static int Read_options (int argc, char **argv);
static DIR* Open_directory( char *dir_name );
static int Read_prod_directory( int prod_id, int prod_code, int vol_index );
static struct dirent* Read_next_file( int rewind );
static int Read_statistics_lb( int prod_id, int prod_code );
static int Read_prod_data_base( int prod_id, int prod_code, int vol_index );
static int Query_elevation_product( int prod_id, int prod_code, int vol_index );
static int Elevation_product_from_directory( int prod_id, int prod_code, int vol_index );
int Comp_func( const void *data1, const void *data2 );
static int Find_median_and_avg_len( int data[], int num_prods, 
                                    int *min, int *max, 
                                    int *avg, int *med );
static int Get_volumes_from_database();
static int Get_volumes_from_directory();

/***************************************************************

   Description:
      A tool to provide size size statistics.   There are 
      two different inputs:

      1) Product data base (query)
      2) Read from LB where information is provided by 
         third party

      The statistics include min, max, avg, and possibly median.

   Inputs:
      argc - number of command line arguments
      argv - the command line arguments

   Outputs:
      A listing of product statistics

   Returns:

   Notes:

***************************************************************/
int main( int argc, char *argv[] ){

   /* This call is to prevent the annoying message about empty task name. */
   ORPGMISC_init( argc, argv, 100, 0, -1, 0 );

   /* Read any command line arguments. */
   if( Read_options( argc, argv ) < 0 ){

      fprintf( stderr, "Command Line Read Failed\n" );
      exit(-1);

   }

   /* First determine how many products are defined. */
   if( (Num_tbl_items = ORPGPAT_num_tbl_items()) < 0 ){

      fprintf( stderr, "ORPGPAT_num_tbl_items Failed (%d)\n", 
               Num_tbl_items );
      exit(0);

   }

   Volume_totals = calloc( MAX_VOLUME_TIMES, sizeof(Volume_totals_t) );
   if( Volume_totals == NULL ){

      fprintf( stderr, "calloc Failed for %d Bytes\n", 
               sizeof(Volume_totals_t)*MAX_VOLUME_TIMES );
      exit(-2);

   }

   /* Get the list of volume scans. */
   if( By_volume_time ){

      if( Data_base_query )
         Num_volumes = Get_volumes_from_database();

      else if( Read_directory )
         Num_volumes = Get_volumes_from_directory();

   }

   /* Generate statistics. */
   if( Data_base_query ){

      /* Product data base query .... by volume scan time. */
      if( By_volume_time )
         Process_data_base_by_volume();

      else{

         /* Not by volume scan time. */
         Process_data_base();

      }

   }
   else if( Read_directory ){

      /* Read directory by volume scan time. */
      if( By_volume_time )
         Process_directory_by_volume();

      else{

         /* Not by volume scan time. */
         Process_directory();

      }

   }
   else if( Read_stats_lb )
      Process_statistics();

   return 0;

/* End of main() */
}

/***************************************************************

   Description:
      This function queries the product data for match on
      product ID and outputs size information on that product.

  
***************************************************************/
static void Process_data_base(){

   int prod_id, prod_code, ind;

   /* Do For All Product Tables Items ..... */
   for( ind = 0; ind < Num_tbl_items; ind++ ){

      if( (prod_id = ORPGPAT_get_prod_id( ind )) < 0 ){

         if( prod_id == ORPGPAT_ERROR ){

            fprintf( stderr, "ORPGPAT_get_prod_id Return Error for Index %d\n",
                     ind );
            exit(0);

         }
         continue;

      }

      /* Decide whether or not this product is processed. */
      prod_code = ORPGPAT_get_code( prod_id );
      if( (prod_code < 0) || (!All_products && (prod_code < 16)) )
         continue;

      Read_prod_data_base( prod_id, prod_code, -1 );

   }

   /* Tell the operator if no products matching Search Criteria Found in 
      Product Data Base . */
   if( !Products_found )
         fprintf( stderr, "Products Not Found In Product Data Base\n" );


   return;

/* End of Process_data_base(). */
}

/***************************************************************

   Description:
      This function queries the product data for match on
      product ID and volume time and outputs size information 
      on that product for each volume.

  
***************************************************************/
static void Process_data_base_by_volume(){

   int vol, prod_id, prod_code, ind;
   int year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0;

   /* Do For All volume times ..... */
   for( vol = 0; vol < Num_volumes; vol++ ){

      /* Convert the volume time to mm/dd/yy hh:mm:ss format. */
      unix_time( (time_t *) &Db_info[vol].vol_t, &year, &mon, 
                 &day, &hour, &min, &sec );

      fprintf( stdout, "\n\n>>>>>>>>>>>>>>>>>>>>>>>>>>> Volume %d Size Statistics <<<<<<<<<<<<<<<<<<<<<<<<<<", vol );
      fprintf( stdout, "\nVolume Date/Time: %02d/%02d/%02d %02d:%02d:%02d\n",
               mon, day, year, hour, min, sec );

      /* Do For All Product Tables Items ..... */
      for( ind = 0; ind < Num_tbl_items; ind++ ){

         if( (prod_id = ORPGPAT_get_prod_id( ind )) < 0 ){

            if( prod_id == ORPGPAT_ERROR ){

               fprintf( stderr, "ORPGPAT_get_prod_id Return Error for Index %d\n",
                        ind );
               exit(0);

            }
            continue;

         }

         /* Decide whether or not this product is processed. */
         prod_code = ORPGPAT_get_code( prod_id );
         if( (prod_code < 0) || (!All_products && (prod_code == 0)) )
            continue;

         Read_prod_data_base( prod_id, prod_code, vol );

      }

      /* List the total number of bytes for this volume scan. */
      fprintf( stdout, "\n\nTotal Products: %d, Total Bytes this Volume: %d\n", 
               Volume_totals[vol].num_prods, Volume_totals[vol].num_bytes );
      fprintf( stdout, "\n******************************************************************************\n" );

   }

   /* Tell the operator if no products matching Search Criteria Found in 
      Product Data Base . */
   if( !Products_found )
         fprintf( stderr, "Products Not Found In Product Data Base\n" );


   return;

/* End of Process_data_base_by_volume(). */
}

/***************************************************************************

   Description:
      Processes directory of product files.

***************************************************************************/
static void Process_directory(){

   int ind, prod_id, prod_code;

   /* Do For all Product Tables Items ..... */
   for( ind = 0; ind < Num_tbl_items; ind++ ){

      if( (prod_id = ORPGPAT_get_prod_id( ind )) < 0 ){

         if( prod_id == ORPGPAT_ERROR ){

            fprintf( stderr, "ORPGPAT_get_prod_id Return Error for Index %d\n",
                     ind );
            exit(0); 

         }
         continue;

      }

      /* Decide whether or not this product is processed. */
      prod_code = ORPGPAT_get_code( prod_id );
      if( (prod_code < 0) || (!All_products && (prod_code == 0)) )
         continue;

    
      Read_prod_directory( prod_id, prod_code, -1 );

   }

   return;

/* End of Process_directory(). */
}

/***************************************************************************

   Description:
      Processes directory of product files by volume time.

***************************************************************************/
static void Process_directory_by_volume(){

   int vol, ind, prod_code, prod_id;
   int year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0;

   /* Do For All volume times ..... */
   for( vol = 0; vol < Num_volumes; vol++ ){

      /* Convert the volume time to mm/dd/yy hh:mm:ss format. */
      unix_time( (time_t *) &Dir_info[vol], &year, &mon, 
                 &day, &hour, &min, &sec );

      fprintf( stdout, "\n\n>>>>>>>>>>>>>>>>>>>>>>>>>>> Volume %d Size Statistics <<<<<<<<<<<<<<<<<<<<<<<<<<\n", vol );
      fprintf( stdout, "\nVolume Date/Time: %02d/%02d/%02d %02d:%02d:%02d\n",
               mon, day, year, hour, min, sec );

      /* Do For all Product Tables Items ..... */
      for( ind = 0; ind < Num_tbl_items; ind++ ){

         if( (prod_id = ORPGPAT_get_prod_id( ind )) < 0 ){

            if( prod_id == ORPGPAT_ERROR ){
   
               fprintf( stderr, "ORPGPAT_get_prod_id Return Error for Index %d\n",
                        ind );
               exit(0); 

            }
            continue;

         }

         /* Decide whether or not this product is processed. */
         prod_code = ORPGPAT_get_code( prod_id );
         if( (prod_code < 0) || (!All_products && (prod_code == 0)) )
            continue;

         Read_prod_directory( prod_id, prod_code, vol );

      }

      /* List the total number of bytes for this volume scan. */
      fprintf( stdout, "\n\nTotal Products: %d, Total Bytes this Volume: %d\n", 
               Volume_totals[vol].num_prods, Volume_totals[vol].num_bytes );
      fprintf( stdout, "\n*******************************************************************************\n" );

   }

   return;

/* Process_directory_by_volume(). */
}

/***************************************************************************

   Description:
      Process statistics LB.

***************************************************************************/
static void Process_statistics(){

   int ind, prod_id, prod_code;

   /* Do For all Product Tables Items ..... */
   for( ind = 0; ind < Num_tbl_items; ind++ ){

      if( (prod_id = ORPGPAT_get_prod_id( ind )) < 0 ){

         if( prod_id == ORPGPAT_ERROR ){

            fprintf( stderr, "ORPGPAT_get_prod_id Return Error for Index %d\n",
                     ind );
            exit(0);

         }
         continue;

      }

      /* Decide whether or not this product is processed. */
      prod_code = ORPGPAT_get_code( prod_id );
      if( (prod_code < 0)
                   ||
          ((Data_base_query || !All_products) && (prod_code == 0)) )
         continue;

      Read_statistics_lb( prod_id, prod_code );

   }

   return;

/* End of Process_statistics(). */
}

/***************************************************************************

   Description:
      Reads product size statistics from repository.   Displays the 
      statistics.

   Inputs:
      prod_id - Product ID or buffer number
      prod_code - Product Code

   Outputs:
      Displays product statistics

   Returns:
      Always returns 0.

   Notes:

***************************************************************************/
static int Read_statistics_lb( int prod_id, int prod_code ){

#ifdef PROD_STATS_LB_DEFINED

   int ret;
   Prod_stats_t *p_stat = NULL;
   char *buf = NULL;

   ret = ORPGDA_read( ORPGDAT_PROD_STATISTICS, &buf, LB_ALLOC_BUF, prod_id );
   if( ret < 0 ){

      if( ret != LB_NOT_FOUND ){

         fprintf( stderr, "ORPGDA_read Returned Error For Prod ID %d (%d)\n",
                  prod_id, ret );
         exit(0);

      }

      return 0;
   }

   p_stat = (Prod_stats_t *) buf;

   if( Unformatted ){

      fprintf( stdout, "\nCode ID Num Min Max Avg, %d, %d, %7d, %7d, %7d, %7d\n", 
               prod_code, prod_id, p_stat->num_prods, p_stat->min_size, 
               p_stat->max_size, p_stat->avg_size );                

   }
   else{

      fprintf( stdout, "\nProduct Code: %d (ID %d)\n", prod_code, prod_id );
      fprintf( stdout, "--> Num: %7d   Min Size: %7d   Max Size: %7d   Avg Size: %7d\n",
               p_stat->num_prods, p_stat->min_size, p_stat->max_size, p_stat->avg_size );                

   }

   free( buf );
 
#endif

   return 0 ;

/* Read_statistics_lb(). */
}

/*****************************************************************************

   Description:
      Queries the product data base for product "prod_code".  Using the 
      query results, computes min, max, avg, and median product sizes.

   Inputs:
      prod_id - Product ID or buffer number
      prod_code - Product code
      vol_index - volume scan time index

   Outputs:
      Display the product statistics

   Returns:
      Always returns 0.  On product data base read failures, the too
      exits.

   Notes:

*****************************************************************************/
static int Read_prod_data_base( int prod_id, int prod_code, int vol_index ){

   RPG_prod_rec_t db_info[MAX_PRODUCTS];
   ORPGDBM_query_data_t query_data[2];

   Prod_header phd;

   char *desc = NULL;
   char *mnem = NULL;

   int i, ret, num, len_prod[MAX_PRODUCTS]; 
   int p_code, query_fields = 0, volind = 0;

   int num_prods = 0, min_size = 0, max_size = 0, avg_size = 0, med_size = 0;

   if( Vol_cov_patt && (ORPGPAT_elevation_based( prod_id ) >= 0) )
      return( Query_elevation_product( prod_id, prod_code, vol_index ) );
      
   /* Set the product code query field. */
   query_data[query_fields].field = RPGP_PCODE;
   query_data[query_fields].value = prod_code;
   query_fields++;

   /* Check on volume scan time .... A value of 0 means the value is 
      undefined. */
   if( vol_index >= 0 ){

      volind = vol_index;
      query_data[query_fields].field = RPGP_VOLT;
      query_data[query_fields].value = Db_info[vol_index].vol_t;
      query_fields++;

   }

   /* Query the product database for the product. */
   num = ORPGDBM_query( db_info, query_data, query_fields, MAX_PRODUCTS );
   if( num <= 0 )
      return 0;

   /* Get the product description for display purposes. */
   desc = ORPGPAT_get_description( prod_id, STRIP_NOTHING );
   mnem = ORPGPAT_get_mnemonic( prod_id );

   /* Get product mnemonic. */
   if( mnem == NULL )
      mnem = "???";

   num_prods = 0;

   for( i = 0; i < num; i++ ){

      /* Read the product header to get the product size. */
      ret = ORPGDA_read( ORPGDAT_PRODUCTS, (char *) &phd, sizeof(Prod_header),
                         db_info[i].msg_id );

      if( (ret < 0) && (ret != LB_BUF_TOO_SMALL) ){
         
         fprintf( stderr, "ORPGDA_read Failed (%d)\n", ret );
         exit(-1);

      }

      /* Validate the product code in the product header. */
      p_code = ORPGPAT_get_code( phd.g.prod_id );
      if( p_code != prod_code )
         continue;

      /* Check the volume coverage pattern */
      if( (Vol_cov_patt > 0) && (phd.vcp_num != Vol_cov_patt))
         continue;

      /* Store the product size. */
      len_prod[num_prods] = phd.g.len - sizeof(Prod_header);
      Volume_totals[volind].num_bytes += len_prod[num_prods];
      Volume_totals[volind].num_prods++;
      num_prods++;
         
   }  

   if( num_prods > 0 ){

      /* Set the "Products_found" flag. */
      Products_found = 1;

      /* Sort the lengths into ascending order. */
      qsort( len_prod, num_prods, sizeof(int), Comp_func );

      /* Find the avg product length and the median product length. */
      Find_median_and_avg_len( len_prod, num_prods, 
                               &min_size, &max_size, 
                               &avg_size, &med_size );

      /* Output the data. */
      if( Unformatted ){

         if( desc != NULL )
            fprintf( stdout, "\n Desc Code ID Num Min Max Avg Med, %s, %d, %d ", 
                     desc, prod_code, prod_id );

         else 
            fprintf( stdout, "\n Desc Code ID Num Min Max Avg Med, , %d, %d", 
                     prod_code, prod_id );

         fprintf( stdout, " %7d, %7d, %7d, %7d, %7d\n", 
                  num_prods, min_size, max_size, avg_size, med_size );                

      }
      else{

         if( !Terse_mode ){

            if( desc != NULL )
               fprintf( stdout, "\n%s  --  Product Code: %d (ID %d)\n", desc, prod_code, prod_id );

            else 
               fprintf( stdout, "\nProduct Code: %d (ID %d)\n", prod_code, prod_id );

               fprintf( stdout, 
                "--> # Prods: %7d   Min Size: %7d   Max Size: %7d   Avg Size: %7d   Med Size: %7d\n",
                num_prods, min_size, max_size, avg_size, med_size );                

            }
            else
               fprintf( stdout, 
               "Code: %4d   Mne: %3s   Elev:   0.0   Min Size: %7d   Max Size: %7d   Avg Size: %7d   Med Size: %7d\n",
               prod_code, mnem, min_size, max_size, avg_size, med_size );                
      }
      
   }

   return 0;

/* End of Read_prod_data_base(). */
}

/*********************************************************************************
   Description:
      Queries the product data base for "prod_code" for elevations in 
      "Vol_cov_patt".  Outputs statistics on a elevation basis.

   Inputs:
      prod_id - Product ID (buffer number)
      prod_code - Product Code
      vol_index - Volume scan time index.

   Outputs:
      Statistics for elevation-bases products as a function of elevation angle.

   Returns:
      Always returns 0.

   Notes:

*********************************************************************************/
static int Query_elevation_product( int prod_id, int prod_code, int vol_index ){

   ORPGDBM_query_data_t query_data[10]; 

   static Prod_header phd;

   char *desc = NULL;
   char *mnem = NULL;

   int i, ele, ret, num, len_prod[MAX_PRODUCTS]; 
   int p_code, write_desc = 1, query_fields = 0, volind = 0;
   int elev_difference = 0;

   int num_prods = 0, min_size = 0, max_size = 0, avg_size = 0, med_size = 0;

   /* Use to index Vol_totals. */
   if( vol_index >= 0 )
      volind = vol_index;

   /* If the Vol_cov_patt flag is set, use SMALL_ELEV_DIFFERENCE. */
   elev_difference = PERMITTED_ELEV_DIFFERENCE;
   if( Vol_cov_patt > 0 )
      elev_difference = SMALL_ELEV_DIFFERENCE;

   /* Do For All elevations. */
   for( ele = 0; ele < Num_ele; ele++ ){

      query_fields = 0;

      /* Set the product code query field. */
      query_data[query_fields].field = RPGP_PCODE;
      query_data[query_fields].value = prod_code;
      query_fields++;

      query_data[query_fields].field = ORPGDBM_ELEV_RANGE;
      query_data[query_fields].value =  (short) (Elev_angles[ele]*10.0) -
                                                elev_difference;
      if( query_data[query_fields].value < ORPGDBM_MIN_ELEV )
         query_data[query_fields].value = (short) ORPGDBM_MIN_ELEV;

      query_data[query_fields].value2 =  (short) (Elev_angles[ele]*10.0) +
                                                  elev_difference;
      if( query_data[query_fields].value2 > ORPGDBM_MAX_ELEV )
         query_data [query_fields].value2 = (short) ORPGDBM_MAX_ELEV;

      query_fields++;

      /* Check on volume scan time .... A value of 0 means the value is 
         undefined. */
      if( vol_index >= 0 ){

         query_data[query_fields].field = RPGP_VOLT;
         query_data[query_fields].value = Db_info[vol_index].vol_t;
         query_fields++;

      }

      /* Query the product database for the product. */
      num = ORPGDBM_query( Db_info, query_data, query_fields, MAX_PRODUCTS );
      if( num <= 0 )
         return 0;

      num_prods = 0;
      for( i = 0; i < num; i++ ){

         /* Read the product header to get the product size. */
         ret = ORPGDA_read( ORPGDAT_PRODUCTS, (char *) &phd, sizeof(Prod_header),
                            Db_info[i].msg_id );

         if( (ret < 0) && (ret != LB_BUF_TOO_SMALL) ){

            fprintf( stderr, "ORPGDA_read Failed (%d)\n", ret );
            exit(-1);

         }

         /* Validate the product code in the product header. */
         p_code = ORPGPAT_get_code( phd.g.prod_id );
         if( p_code != prod_code )
            continue;

         /* Validate the VCP number matches. */
         if( (Vol_cov_patt > 0) && (phd.vcp_num != Vol_cov_patt) )
            continue;

         /* Store the product size. */
         len_prod[num_prods] = phd.g.len - sizeof(Prod_header);

         Volume_totals[volind].num_bytes += len_prod[num_prods];
         Volume_totals[volind].num_prods++;
         num_prods++;

      } 

      if( num_prods > 0 ){

         /* Set the "Products_found" flag. */
         Products_found = 1;

         /* Get the product description for display purposes. */
         desc = ORPGPAT_get_description( prod_id, STRIP_NOTHING );
         mnem = ORPGPAT_get_mnemonic( prod_id );

         /* Get product mnemonic. */
         if( mnem == NULL )
            mnem = "???";

         if( !Terse_mode && write_desc ){

            if( Unformatted ){

               if( desc != NULL )
                  fprintf( stdout, "\nDesc Code ID Elev Num Min Max Avg Med, %s, %d, %d\n", 
                           desc, prod_code, prod_id );

               else
                  fprintf( stdout, "\nDesc Code ID Elev Num Min Max Avg Med, , %d, %d\n", 
                           prod_code, prod_id );
            }
            else{

               if( desc != NULL )
                  fprintf( stdout, "\n%s  --  Product Code: %d (ID %d)\n", desc, prod_code, prod_id );

               else
                  fprintf( stdout, "\nProduct Code: %d (ID %d)\n", prod_code, prod_id );

            }

            write_desc = 0;

         }

         /* Sort the lengths into ascending order. */
         qsort( len_prod, num_prods, sizeof(int), Comp_func );

         /* Find the avg product length and the median product length. */
         Find_median_and_avg_len( len_prod, num_prods,
                                  &min_size, &max_size,
                                  &avg_size, &med_size );
         
         if( Unformatted ){

            fprintf( stdout, "%5.1f, %5d, %7d, %7d, %7d, %7d\n",
                     Elev_angles[ele], num_prods, min_size, max_size, avg_size, med_size );                
         }
         else{

            if( Terse_mode )
               fprintf( stdout, 
               "Code: %4d   Mne: %3s   Elev: %5.1f   Min Size: %7d   Max Size: %7d   Avg Size: %7d   Med Size: %7d\n",
               prod_code, mnem, Elev_angles[ele], min_size, max_size, avg_size, med_size );                

            else 
               fprintf( stdout, 
               "--> Elev: %5.1f   # Prods: %5d   Min Size: %7d   Max Size: %7d   Avg Size: %7d   Med Size: %7d\n",
               Elev_angles[ele], num_prods, min_size, max_size, avg_size, med_size );                

         }

      }

   }

   return 0;

/* Query_elevation_product(). */
}

/*****************************************************************************

   Description:
      Queries the product data base for product "prod_code".  Using the 
      query results, computes min, max, avg, and median product sizes.

   Inputs:
      prod_id - Product ID or buffer number
      prod_code - Product code
      vol_index - volume time index

   Outputs:
      Display the product statistics

   Returns:
      Always returns 0.  On product data base read failures, the too
      exits.

   Notes:

*****************************************************************************/
static int Read_prod_directory( int prod_id, int prod_code, int vol_index ){

   char *desc = NULL;
   char *mnem = NULL;

   int size, p_code, p_vol_time, fd, len_prod[MAX_PRODUCTS], rewind = 1;
   int min_size = 0, max_size = 0, avg_size = 0, med_size = 0;
   int num_prods = 0, volind = 0, length;

   struct dirent *dir_entry = NULL;

   static struct stat status;
   static char path_name[256];
   static Graphic_product phd;

   Prod_msg_header_icd *hd = NULL;
   

   /* Get the product description for display purposes. */
   desc = ORPGPAT_get_description( prod_id, STRIP_NOTHING );
   mnem = ORPGPAT_get_mnemonic( prod_id );

   /* Get product mnemonic. */
   if( mnem == NULL )
      mnem = "???";

   /* If elevation product and volume scan reporting, call routine
      to check from matching volume coverage pattern. */
   if( Vol_cov_patt && (ORPGPAT_elevation_based( prod_id ) >= 0) )
      return( Elevation_product_from_directory( prod_id, prod_code, vol_index ) );

   while( (dir_entry = Read_next_file( rewind )) != NULL ){

      rewind = 0;

      /* Ignore '..', '.' and directory entries. */
      if( (strcmp( dir_entry->d_name, "." ) == 0)
                        ||
          (strcmp( dir_entry->d_name, ".." ) == 0) )
         continue;
                  
      /* Construct the path name of the file. */
      memset( path_name, 0, 256 );
      strcat( path_name, Directory_path );
      strcat( path_name, dir_entry->d_name );

      if( stat( path_name, &status ) != 0 ){

         fprintf( stderr, "stat Failed (%d)\n", errno );
         exit(1);

      }

      /* Open the file for reading.  Ignore directories. */
      if( S_ISDIR( status.st_mode ) )
         continue;

      if( (fd = open( path_name, O_RDONLY )) < 0 ){

         fprintf( stderr, "Open Failed For File: %s (%d)\n", 
                  path_name, errno );
         exit(1);

      }
       
      /* Read the product header. */
      size = read( fd, &phd, sizeof( Graphic_product ));
      if( size < sizeof(Graphic_product) ){

         close(fd);
         continue;

      }
         
      /* Validate the product code and volume time in the product header. */
      hd = (Prod_msg_header_icd *) &phd;
      p_code = SHORT_BSWAP_L( hd->msg_code );

      p_vol_time = 0;
      if( vol_index >= 0 ){

         unsigned int vol_ms = SHORT_BSWAP_L( phd.vol_time_ms );
         unsigned int vol_ls = SHORT_BSWAP_L( phd.vol_time_ls );
         unsigned int vol_date = SHORT_BSWAP_L( phd.vol_date );

         volind = vol_index;
         p_vol_time = ((vol_date-1)*86400) + ((vol_ms << 16) | vol_ls);

      }

      if( (p_code == prod_code) && (p_vol_time == Dir_info[vol_index]) ){

         /* Store the product size. */
         length = (SHORT_BSWAP_L(hd->lengthm << 16)) | SHORT_BSWAP_L(hd->lengthl);
         len_prod[num_prods] = length;
         Volume_totals[volind].num_bytes += len_prod[num_prods];
         Volume_totals[volind].num_prods++;
         num_prods++;

      }

      /* Close the file. */
      close( fd );
         
   }  

   /* Did we find any matching products? */
   if( num_prods > 0 ){

      /* Set the "Products_found" flag. */
      Products_found = 1;

      /* Sort the lengths into ascending order. */
      qsort( len_prod, num_prods, sizeof(int), Comp_func );

      /* Find the avg product length and the median product length. */
      Find_median_and_avg_len( len_prod, num_prods, 
                               &min_size, &max_size, 
                               &avg_size, &med_size );

      if( !Terse_mode && Unformatted ){

         if( desc != NULL )
            fprintf( stdout, "\n Desc Code ID Num Min Max Avg Med, %s, %d, %d ", 
                     desc, prod_code, prod_id );

         else 
            fprintf( stdout, "\n Desc Code ID Num Min Max Avg Med, , %d, %d", 
                     prod_code, prod_id );

         fprintf( stdout, " %7d, %7d, %7d, %7d, %7d\n", 
                  num_prods, min_size, max_size, avg_size, med_size );                

      }
      else{

         if( !Terse_mode ){

            if( desc != NULL )
               fprintf( stdout, "\n%s  --  Product Code: %d (ID %d)\n", desc, prod_code, prod_id );

            else 
               fprintf( stdout, "\nProduct Code: %d (ID %d)\n", prod_code, prod_id );

            fprintf( stdout, 
             "--> # Prods: %7d   Min Size: %7d   Max Size: %7d   Avg Size: %7d   Med Size: %7d\n",
             num_prods, min_size, max_size, avg_size, med_size );                

         }
         else
            fprintf( stdout, 
            "Code: %4d   Mne: %3s   Elev:   0.0   Min Size: %7d   Max Size: %7d   Avg Size: %7d   Med Size: %7d\n",
            prod_code, mnem, min_size, max_size, avg_size, med_size );                



      }
      
   }

   return 0;

/* Read_prod_directory(). */
}


/*********************************************************************************
   Description:
      Reads products from directory with "prod_code" for elevations in 
      "Vol_cov_patt".  Outputs statistics on a elevation basis.

   Inputs:
      prod_id - Product ID (buffer number)
      prod_code - Product Code
      vol_index - Volume Time index

   Outputs:
      Statistics for elevation-bases products as a function of elevation angle.

   Returns:
      Always returns 0.

   Notes:

*********************************************************************************/
static int Elevation_product_from_directory( int prod_id, int prod_code, int vol_index ){

   struct dirent *dir_entry = NULL;

   char *desc = NULL;
   char *mnem = NULL;

   int fd, ele, len_prod[MAX_PRODUCTS]; 
   int size, p_code, p_vol_time, rewind, volind = 0, write_desc = 1;
   int num_prods = 0, min_size = 0, max_size = 0, avg_size = 0, med_size = 0;

   static struct stat status;
   static Graphic_product phd;
   static char path_name[256];

   /* Get the product description for display purposes. */
   desc = ORPGPAT_get_description( prod_id, STRIP_NOTHING );
   mnem = ORPGPAT_get_mnemonic( prod_id );

   for( ele = 0; ele < Num_ele; ele++ ){

      rewind = 1;
      num_prods = 0;

      while( (dir_entry = Read_next_file( rewind )) != NULL ){

         rewind = 0;

         /* Ignore '..', '.' and directory entries. */
         if( (strcmp( dir_entry->d_name, "." ) == 0)
                          ||
             (strcmp( dir_entry->d_name, ".." ) == 0) )
            continue;

         /* Construct the path name of the file. */
         memset( path_name, 0, 256 );
         strcat( path_name, Directory_path );
         strcat( path_name, dir_entry->d_name );

         if( stat( path_name, &status ) != 0 ){

            fprintf( stderr, "stat Failed (%d)\n", errno );
            exit(1);

         }

         /* Open the file for reading.  Ignore directories. */
         if( S_ISDIR( status.st_mode ) )
            continue;

         if( (fd = open( path_name, O_RDONLY )) < 0 ){

            fprintf( stderr, "Open Failed For File: %s (%d)\n",
                     path_name, errno );
            exit(1);

         }

         /* Read the product header. */
         size = read( fd, &phd, sizeof( Graphic_product ));
         if( size < sizeof(Graphic_product) ){

            close(fd);
            continue;

         }

         /* Validate the VCP number and elevation index matches. */
         if( Vol_cov_patt > 0 ){

            if( (SHORT_BSWAP_L(phd.elev_ind) != (ele+1) )  
                            || 
                (SHORT_BSWAP_L(phd.vcp_num) != Vol_cov_patt) ){

               close(fd);
               continue;

            } 

         }

         /* Validate the product code in the product header. */
         p_code = SHORT_BSWAP_L( phd.msg_code );

         p_vol_time = 0;
         if( vol_index >= 0 ){

            unsigned int vol_ms = SHORT_BSWAP_L( phd.vol_time_ms );
            unsigned int vol_ls = SHORT_BSWAP_L( phd.vol_time_ls );
            unsigned int vol_date = SHORT_BSWAP_L( phd.vol_date );

            volind = vol_index;
            p_vol_time = ((vol_date-1)*86400) + ((vol_ms << 16) | vol_ls);

         }

         if( (p_code == prod_code) && (p_vol_time == Dir_info[vol_index]) ){

            /* Store the product size. */
            len_prod[num_prods] = INT_BSWAP_L(phd.msg_len);
            Volume_totals[volind].num_bytes += len_prod[num_prods];
            Volume_totals[volind].num_prods++;
            num_prods++;

         }

         /* Close the file. */
         close( fd );

      }

      if( num_prods > 0 ){

         /* Set the "Products_found" flag. */
         Products_found = 1;

         /* Get the product description for display purposes. */
         desc = ORPGPAT_get_description( prod_id, STRIP_NOTHING );
         mnem = ORPGPAT_get_mnemonic( prod_id );

         /* Extract product mnemonic. */
         if( mnem == NULL )
            mnem = "???";

         if( write_desc && !Terse_mode ){

            if( Unformatted ){

               if( desc != NULL )
                  fprintf( stdout, "\nDesc Code ID Elev Num Min Max Avg Med, %s, %d, %d\n", 
                           desc, prod_code, prod_id );

               else
                  fprintf( stdout, "\nDesc Code ID Elev Num Min Max Avg Med, , %d, %d\n", 
                           prod_code, prod_id );
            }
            else{

               if( desc != NULL )
                  fprintf( stdout, "\n%s  --  Product Code: %d (ID %d)\n", desc, prod_code, prod_id );

               else
                  fprintf( stdout, "\nProduct Code: %d (ID %d)\n", prod_code, prod_id );

            }

            write_desc = 0;

         }

         /* Sort the lengths into ascending order. */
         qsort( len_prod, num_prods, sizeof(int), Comp_func );

         /* Find the avg product length and the median product length. */
         Find_median_and_avg_len( len_prod, num_prods,
                                   &min_size, &max_size,
                                   &avg_size, &med_size );
         
         if( Unformatted )
            fprintf( stdout, "%5.1f, %5d, %7d, %7d, %7d, %7d\n",
                     Elev_angles[ele], num_prods, min_size, max_size, avg_size, med_size );                
         else {

            if( Terse_mode )
               fprintf( stdout, 
               "Code: %4d   Mne: %3s   Elev: %5.1f   Min Size: %7d   Max Size: %7d   Avg Size: %7d   Med Size: %7d\n",
               prod_code, mnem, Elev_angles[ele], min_size, max_size, avg_size, med_size );                

            else
               fprintf( stdout, 
               "--> Elev: %5.1f   # Prods: %5d   Min Size: %7d   Max Size: %7d   Avg Size: %7d   Med Size: %7d\n",
               Elev_angles[ele], num_prods, min_size, max_size, avg_size, med_size );                

         }

      }

   }

   return 0;

/* End of Elevation_product_from_directory(). */
}

/********************************************************************

   Description:
      This function finds the median value and average value from
      a list of lengths.   Also returns the min and max length.

   Inputs:
      data - sorted list of lengths
      num_prods - number in sorted list

   Outputs:
      min - minimum length found in list
      max - maximum length found in list
      avg - average of all lengths found in list
      med - median value of lengths in list

   Returns:
      Always returns 0.

   Notes:

********************************************************************/
static int Find_median_and_avg_len( int data[], int num_prods,
                                    int *min, int *max, int *avg,
                                    int *med ){

   int i, sum = 0;

   if( num_prods <= 0 ){

      *min = *max = *avg = *med = 0;
      return 0;

   }

   *min = data[0];
   *max = data[num_prods - 1];

   /* Compute the average value. */
   for( i = 0; i < num_prods; i++ )
      sum += data[i];

   *avg = sum / num_prods;


   /* Compute the median value. */
   if( num_prods == 1 )
      *med = data[0];

   else if( (num_prods % 2) == 1 )
      *med = data[num_prods/2];
    
   else
      *med = (data[num_prods/2 - 1] + data[num_prods/2])/2;

   return 0;

/* End of Find_median_and_avg_len(). */
}

/***********************************************************************

       Description:  This function reads command line arguments.

       Input:        argc - Number of command line arguments.
                     argv - Command line arguments.

       Output:       Usage message

       Returns:      0 on success or -1 on failure

       Notes:        

*********************************************************************/
static int Read_options (int argc, char **argv){

   extern char *optarg;    /* used by getopt */
   extern int optind;
   int c;                  /* used by getopt */
   int err;                /* error flag */
   int index;

   Terse_mode = 0;
   All_products = 0;
   Data_base_query = 1;
   Vol_cov_patt = 0;
   Unformatted = 0;
   Read_stats_lb = 0;
   Read_directory = 0;
   By_volume_time = 0;
   Directory_path[0] = '\0';

   err = 0;
#ifdef PROD_STATS_LB_DEFINED
   while ((c = getopt (argc, argv, "astud:v:V:h:")) != EOF) {
#else
   while ((c = getopt (argc, argv, "atud:v:V:h:")) != EOF) {
#endif
      switch (c) {

         /* Display all products (only valid when not data base query) */
         case 'a':
            All_products = 1;
            break;

#ifdef PROD_STATS_LB_DEFINED
         case 's':
            Data_base_query = 0;
            Read_stats_lb = 1;
            break;
#endif

         case 'd':
            Data_base_query = 0;
            Read_directory = 1;
            if( (Product_directory = Open_directory( optarg )) == NULL ){
        
               Read_directory = 0;
               fprintf( stderr, "Could Not Open Directory %s\n", optarg );
               err = 1;

            }
            break;

         case 'u':
            Unformatted = 1;
            break;

         case 'v':
            Vol_cov_patt = atoi(optarg);
            break;

         case 'V':
            Vol_cov_patt = atoi(optarg);
            Terse_mode = 1;
            break;

         case 't':
            By_volume_time = 1;
            break;

         /* Print out the usage information. */
         case 'h':
         case '?':
            err = 1;
            break;
      }

   }

   if (err == 1) {                     /* Print usage message */

      printf ("Usage: %s [options]\n", argv[0]);
      printf ("       Options:\n");
      printf ("       -a All Products (default: Products With Codes Only)\n");
#ifdef PROD_STATS_LB_DEFINED
      printf ("       -s Product Size Statistics LB (default: Data Base Query)\n");
#endif
      printf ("       -v <Volume Coverage Pattern>\n");
      printf ("       -V <Volume Coverage Pattern> (same as -v except terse mode)\n");
      printf ("       -d <Product Directory>\n");
      printf ("       -u Unformatted Output (default: Formatted Output)\n");
      printf ("       -t By Volume Time (default: Not by Volume Time)\n");
      exit(-1);

   }

   /* If using data base query or reading from a directory, then 
      "All_products" is cleared. */
   if( Data_base_query || Read_directory )
      All_products = 0;

   /* "By_volume_time" not support for Product Size Statistics LB. */
   if( Read_stats_lb && By_volume_time ){

      fprintf( stderr, "By Volume Time Disabled for Product Size Statistics LB\n" );
      By_volume_time = 0;

   }

   /* Get the elevation angles corresponding to "Vol_cov_patt". */
   if( (Vol_cov_patt > 0) && (Data_base_query || Read_directory) ){

      if( (index = ORPGVCP_index( Vol_cov_patt )) < 0 )
         index = ORPGVCP_index( 21 ); 

      if( index >= 0 )
         Num_ele = ORPGVCP_get_all_elevation_angles( Vol_cov_patt, MAX_ELEVATION_CUTS, 
                                                     Elev_angles );

   }
   else{

      Num_ele = 0;
      Vol_cov_patt = 0;

   }

   /* If unformatted output, then disable Terse_mode.  Terse_mode is only supported 
      with formatted output. */
   if( Unformatted )
      Terse_mode = 0;

   return (0);

/* End of Read_options() */
}


/*********************************************************************************

   Description:
      Open product directory (that was populated by nbtcp, for example).

   Inputs:
      dir_name - directory name.

   Returns:
      Pointer to directory stream or NULL on error. 

*********************************************************************************/
static DIR* Open_directory( char *dir_name ){

   DIR *dir = NULL;

   if( (dir = opendir( dir_name )) != NULL ){

      sprintf( Directory_path, "%s", dir_name );
      if( Directory_path[ strlen(Directory_path) - 1 ] != '/' )
         strcat( Directory_path, "/" );

   }

   return( dir );

/* End of Open_directory(). */
}

/*********************************************************************************

   Description:
      Reads product directory (that was populated by nbtcp, for example).

   Returns:
      Next directory entry.

*********************************************************************************/
static struct dirent* Read_next_file( int rewind ){

   /* Rewind the directory. */
   if( rewind )
      rewinddir( Product_directory );

   return( readdir( Product_directory ) );

/* End of Read_next_file(). */
}

/*********************************************************************************

   Description:
      Performs a product oata base query for volume scan times.   

   Returns:
      The number of distinct volume scans in the data base.

*********************************************************************************/
static int Get_volumes_from_database(){

   ORPGDBM_query_data_t *query_data = NULL;
   int vols;

   /* Allocate storage for the query data. */
   query_data = (ORPGDBM_query_data_t *)
                        calloc( 1, sizeof(ORPGDBM_query_data_t)*MAX_FIELDS );

   if( query_data == NULL ){

      LE_send_msg( GL_ERROR, "calloc Failed for %d Bytes.\n",
                   sizeof(ORPGDBM_query_data_t)*MAX_VOLUME_TIMES );
      return 0;

   }

   /* Establish the query data */
   query_data[0].field = ORPGDBM_MODE;
   query_data[0].value = ORPGDBM_FULL_SEARCH | ORPGDBM_HIGHEND_SEARCH | 
                         ORPGDBM_DISTINCT_FIELD_VALUES;

   query_data[1].field = RPGP_VOLT;
   query_data[1].value = 0;
   
   vols = ORPGDBM_query( Db_info, query_data, 2, MAX_VOLUME_TIMES );

   /* Initialize the total byte count. */
   memset( Volume_totals, 0, MAX_VOLUME_TIMES*sizeof(Volume_totals_t) );

   return vols;

/* End of Get_volumes(). */
}

/*********************************************************************************

   Description:
      Reads the products from the product directory and builds a list of
      volume scan times.

*********************************************************************************/
static int Get_volumes_from_directory(){

   struct dirent *dir_entry = NULL;

   int p_code, fd, size, p_vol_time, num_volumes, not_found, i, rewind = 1;
   unsigned int vol_ms, vol_ls, vol_date;

   static struct stat status;
   static Graphic_product phd;
   static char path_name[256];

   num_volumes = 0;
   while( (dir_entry = Read_next_file( rewind )) != NULL ){

      rewind = 0;

      /* Ignore '..', '.' and directory entries. */
      if( (strcmp( dir_entry->d_name, "." ) == 0)
                        ||
          (strcmp( dir_entry->d_name, ".." ) == 0) )
         continue;

      /* Construct the path name of the file. */
      memset( path_name, 0, 256 );
      strcat( path_name, Directory_path );
      strcat( path_name, dir_entry->d_name );

      if( stat( path_name, &status ) != 0 ){

         fprintf( stderr, "stat Failed (%d)\n", errno );
         exit(1);

      }

      /* Open the file for reading.  Ignore directories. */
      if( S_ISDIR( status.st_mode ) )
         continue;

      if( (fd = open( path_name, O_RDONLY )) < 0 ){

         fprintf( stderr, "Open Failed For File: %s (%d)\n",
                  path_name, errno );
         exit(1);

      }

      /* Read the product header. */
      size = read( fd, &phd, sizeof( Graphic_product ));
      if( size < sizeof(Graphic_product) ){

         close(fd);
         continue;

      }

      vol_ms = SHORT_BSWAP_L( phd.vol_time_ms );
      vol_ls = SHORT_BSWAP_L( phd.vol_time_ls );
      vol_date = SHORT_BSWAP_L( phd.vol_date );
      p_vol_time = ((vol_date-1)*86400) + ((vol_ms << 16) | vol_ls);

      p_code = SHORT_BSWAP_L( phd.msg_code );

      /* Close the file descriptor since the file is no longer needed. */
      close(fd);


      /* Product code must be 16 or greater. */
      if( p_code < 16 )
         continue;

      /* Check if this volume scan has already been encountered. */
      not_found = 1;
      for( i = 0; i < num_volumes; i++ ){

         if( Dir_info[i] == p_vol_time ){

            not_found = 0;
            break;

         }
        
      }

      /* If volume time not in list, add it to the list. */
      if( not_found ){

         Dir_info[num_volumes] = p_vol_time;
         num_volumes++;

      }

   }

   /* Sort the volume scan times. */
   qsort( Dir_info, num_volumes, sizeof(int), Comp_func );

   return( num_volumes );

/* End of Get_volumes_from_directory(). */
}

/*******************************************************************
   Description:
      This function is the comparison function used by qsort.
  
   Inputs:
      data1 - Data value 1 to be compared.
      data2 - Data value 2 to be compared.
  
   Returns:
      Returns -1 if data1 < data2, 0 if data1 == data2 and 
      1 if data1 > data2.

   Notes:

******************************************************************/
int Comp_func( const void *data1, const void *data2 ){


   int value1 = (int) *((int *) data1);
   int value2 = (int) *((int *) data2);

   if( value1 < value2 )
      return -1;

   else if( value1 == value2 )
      return 0;

   else
      return 1;

/* End of Comp_func( ). */
}

