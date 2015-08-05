#include <orpgsails.h>

int main( int argc, char *argv[] ){

   int response, retval, num_cuts;

   /* Needed for registering adaptation data LB. */
   ORPGMISC_init( argc, argv, 100, 0, -1, 0 );

   /* Initialize the SAILS Status Message. */
   ORPGSAILS_init();

   while(1){

      fprintf( stderr, "Enter Option:\n" );
      fprintf( stderr, "\n0) Exit\n" );
      fprintf( stderr, "1) Get SAILS status\n" );
      fprintf( stderr, "2) Get Number SAILS cuts\n" );
      fprintf( stderr, "3) Get maximum Number SAILS cuts\n" );
      fprintf( stderr, "4) Get site maximum Number SAILS cuts\n" );
      fprintf( stderr, "5) Get Number Requested SAILS cuts\n" );
      fprintf( stderr, "6) Set Number Requested SAILS cuts\n" );

      scanf( "%d", &response );
      switch( response ){

         default:
            break;

         case 0:
            exit(0);

         case 1:
         {
            retval = ORPGSAILS_get_status();
            fprintf( stderr, "SAILS Status: %d\n", retval );
            break;
         }

         case 2:
         {
            retval = ORPGSAILS_get_num_cuts();
            fprintf( stderr, "SAILS Num Cuts: %d\n", retval );
            break;
         }

         case 3:
         {
            retval = ORPGSAILS_get_max_cuts();
            fprintf( stderr, "SAILS Max Num Cuts: %d\n", retval );
            break;
         }

         case 4:
         {
            retval = ORPGSAILS_get_site_max_cuts();
            fprintf( stderr, "SAILS Site Max Num Cuts: %d\n", retval );
            break;
         }

         case 5:
         {
            retval = ORPGSAILS_get_req_num_cuts();
            fprintf( stderr, "SAILS Req Num Cuts: %d\n", retval );
            break;
         }

         case 6:
         {

            fprintf( stderr, "\nEnter the Number of Requested SAILS cuts\n" );
            scanf( "%d", &num_cuts );
            fprintf( stderr, "\nRequested Number Cuts: %d\n", num_cuts );
            retval = ORPGSAILS_set_req_num_cuts( num_cuts );
            fprintf( stderr, "SAILS Set Req Num Cuts: %d (retval: %d)\n", 
                     num_cuts, retval );
            break;
         }

      }

      response = -1;
      continue;

   }

   return 0;

}
