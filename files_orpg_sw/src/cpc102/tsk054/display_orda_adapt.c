/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2005/06/02 19:31:46 $
 * $Id: display_orda_adapt.c,v 1.2 2005/06/02 19:31:46 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
*/


#include <stdio.h>
#include <orpg.h>
#include <orda_adpt.h>

int main( int argc, char *argv[] ){

   int option, ret, i;
   float value = -1.0;
   ORDA_adpt_data_msg_t *data = NULL;

   ret = ORPGDA_read( ORPGDAT_RDA_ADAPT_DATA, (char *) &data, LB_ALLOC_BUF, 
                      ORPGDAT_RDA_ADAPT_MSG_ID );
   if( ret <= 0 ){

     fprintf( stderr, "ORPGDA_read() Failed (%d)\n", ret );
     return  (-1);

   }

   while(1){


      fprintf( stdout, "\nSelect from the following options:\n" );
      fprintf( stdout, "1 - Bypass Map Generation Parameters\n" );
      fprintf( stdout, "2 - Clutter Map Parameters\n" ); 
      fprintf( stdout, "3 - PRF Parameters\n" );
      fprintf( stdout, "99 - exit\n" );

      scanf( "%d", &option );

      fprintf( stdout, "\n" );

      switch (option ){

         case 1:

            fprintf( stdout, "---> Bypass Map Generation Parameters <---\n" );
            fprintf( stdout, "   max_el_index: %d\n", data->rda_adapt.max_el_index );

            for( i = 0; i < data->rda_adapt.max_el_index; i++ )
               fprintf( stdout, "   --->el_indexr[%02d]: %7.4f\n", i, data->rda_adapt.el_index[i] );

            fprintf( stdout, "   threshold1: %7.4f\n", data->rda_adapt.threshold1 ); 
            fprintf( stdout, "   threshold2: %7.4f\n", data->rda_adapt.threshold2 );
 
            break;
         
         case 2:

            fprintf( stdout, "---> Clutter Map Parameters <---\n" );
            fprintf( stdout, "   nbr_el_segments: %d\n", data->rda_adapt.nbr_el_segments );
            for( i = 1; i <= data->rda_adapt.nbr_el_segments; i++ ){

               if( i == 1 )
                  value = data->rda_adapt.seg1lim;

               else if( i == 2 )
                  value = data->rda_adapt.seg2lim;

               else if( i == 3 )
                  value = data->rda_adapt.seg3lim;

               else if( i == 4 )
                  value = data->rda_adapt.seg4lim;

        
               fprintf( stdout, "   seg%1dlim: %7.4f\n", i, value );

            }

            break; 

         case 3:

            fprintf( stdout, "---> PRF Parameters <---\n" );
            fprintf( stdout, "   Delta PRF: %d\n", data->rda_adapt.deltaprf );

            break;
         default:
            exit(0);

      }

   }
   
   return 0;

}
