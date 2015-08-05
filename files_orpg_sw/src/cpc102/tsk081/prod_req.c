#include <orpg.h>
#include <dpprep_isdp.h>
#include <infr.h>

#define ALL_ELEVATIONS_VCP	0x4000
#define ALL_ELEVATIONS_AT       0x2000
#define ALL_CUTS_AT             0x6000

int main( int argc, char *argv[] ){

   int response;
   unsigned short request;
   int angle, cut, irequest;

   /* Query on bits 13 and 14. */
   fprintf( stderr, "Enter 0, 1, 2, 3 or 4\n" );
   fprintf( stderr, "\n0 - Quit\n" );
   fprintf( stderr, "Encode Request:\n" );
   fprintf( stderr, "1 - All Elevations of the VCP\n" );
   fprintf( stderr, "2 - All Elevations at or below Elevation Angle\n" );
   fprintf( stderr, "3 - All Elevations at or below Elevation Cut Number\n" );
   fprintf( stderr, "Decode Request\n" );
   fprintf( stderr, "4 - Decode Request\n" );

   fscanf( stdin, "%d", &response );

   switch( response ){

      case 0:
      default:
      {
         return 0;
      }

      case 1:
      {
         request = (unsigned short) ALL_ELEVATIONS_VCP;

         fprintf( stderr, "\nSpecify Elevation Angle*10 (Repeat Cuts) or 0\n" );
         fscanf( stdin, "%d", &angle );
         request += angle;
 
         fprintf( stderr, "\nRequest Value: %d\n", request );
         break;
      }
 
      case 2:
      {
         request = (unsigned short) ALL_ELEVATIONS_AT;

         fprintf( stderr, "\nSpecify Elevation Angle*10 (Repeat Cuts) or 0\n" );
         fscanf( stdin, "%d", &angle );
         request += angle;

         fprintf( stderr, "\nRequest Value: %d\n", request );
         break;
      }
 
      case 3:
      {
         request = (unsigned short) ALL_CUTS_AT;

         fprintf( stderr, "\nSpecify Elevation Cut #\n" );
         fscanf( stdin, "%d", &cut );
         request += cut;

         fprintf( stderr, "\nRequest Value: %d\n", request );
         break;
      }
 
      case 4:
      {
         fprintf( stderr, "\nSpecify Request Value (decimal)\n" );
         fscanf( stdin, "%d", &irequest );
         request = (unsigned short) irequest;

         if( (request & 0x6000) == ALL_CUTS_AT ){

            cut = (request & 0x0fff);
            fprintf( stderr, "\nThe lowest %d cuts of the VCP requested\n", cut );

         }
         else if( (request & 0x4000) == ALL_ELEVATIONS_VCP ){

            angle = (request & 0x0fff);
            if( angle == 0 )
               fprintf( stderr, "\nAll elevations of the VCP requested\n" );

            else
               fprintf( stderr, "\nAll %4.1f deg elevations of the VCP requested\n",
                        (float) angle / 10.0 );

         }
         else if( (request & 0x2000) == ALL_ELEVATIONS_AT ){

            angle = (request & 0x0fff);
            fprintf( stderr, "\nAll elevations of the VCP at or below %4.1f deg requested\n",
                     (float) angle / 10.0 );

         }
         else
            fprintf( stderr, "\nRequest Value: %d\n", request );

         break;
      } 

   }

   return 0;

}
