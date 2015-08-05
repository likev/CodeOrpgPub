/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/03 22:31:58 $
 * $Id: rpgp_gen_prod_debug.c,v 1.1 2005/03/03 22:31:58 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/******************************************************************

   Generic Product Debug Routines .... Not intended for operational
   use.

******************************************************************/

#include <rpgp.h>

static char *Add_space = "";         /* for product print out */


/******************************************************************

   Prints the RPGP product struct for verification.

******************************************************************/
void RPGP_print_prod (RPGP_product_t *prod) {

   printf ("\n");
   printf ("product id %d, name: %s, desc: %s.\n",
                prod->product_id, prod->name, prod->description);
   printf ("    type %d, time %d\n", prod->type, prod->gen_time);
   printf ("    Radar name: %s, lat %8.4f, lon %8.4f, height %8.4f\n",
                prod->radar_name, prod->radar_lat,
                prod->radar_lon, prod->radar_height);
   printf ("    Vol time %d, elev time %d, vol number %d, elev number %d\n",
                prod->volume_time, prod->elevation_time,
                prod->volume_number, prod->elevation_number);
   printf ("    VCP %d, Op mode %d, elev %8.4f\n", prod->vcp,
                prod->operation_mode, prod->elevation_angle);
   printf ("    compress type %d, size %d\n",
                prod->compress_type, prod->size_decompressed);
   RPGP_print_params (prod->numof_prod_params, prod->prod_params);
   RPGP_print_components (prod->numof_components, (char **)prod->components);
   printf ("\n");

}

/******************************************************************

   Prints components array in "comps" of size "n_comps" for
   verification.

******************************************************************/
void RPGP_print_components (int n_comps, char **comps) {

   RPGP_area_t **gencomps;
   int i;

   printf ("%s    number of components %d\n", Add_space, n_comps);
   gencomps = (RPGP_area_t **)comps;
   for (i = 0; i < n_comps; i++) {

      switch (gencomps[i]->comp_type) {

         case RPGP_AREA_COMP:
            RPGP_print_area ((RPGP_area_t *)comps[i]);
            break;

         case RPGP_EVENT_COMP:
            RPGP_print_event ((RPGP_event_t *)comps[i]);
            break;

         default:
            break;

      }
   }
}

/******************************************************************

   Prints area "area" for verification.

******************************************************************/
void RPGP_print_area (RPGP_area_t *area) {

   printf ("%s    AREA: (%d), area type %d\n",
                Add_space, area->comp_type, area->area_type);
   RPGP_print_params (area->numof_comp_params, area->comp_params);
   RPGP_print_points (area->numof_points, area->points);
}

/******************************************************************

   Prints event "event" for verification.

******************************************************************/
void RPGP_print_event (RPGP_event_t *event) {

   printf ("    EVENT: (%d)\n", event->comp_type);
   Add_space = "    ";
   RPGP_print_params (event->numof_event_params, event->event_params);
   RPGP_print_components (event->numof_components, (char **)event->components);
   Add_space = "";
}

/******************************************************************

   Prints "n_params" parameters pointed to by "params" for
   verification.

******************************************************************/
void RPGP_print_params (int n_params, RPGP_parameter_t *params) {

   int i;

   printf ("%s        # %d params:\n", Add_space, n_params);
   for (i = 0; i < n_params; i++) {
        printf ("%s            param[%d]: id: %s, attrs: %s\n",
        Add_space, i, params[i].id, params[i].attrs);
   }
}

/******************************************************************

   Prints array "points" of size "n_points" for verification.

******************************************************************/
void RPGP_print_points (int n_points, RPGP_location_t *points) {
   int i;

   printf ("%s        $ %d points:\n", Add_space, n_points);
   for (i = 0; i < n_points; i++) {
        printf ("%s            point[%d]: lat = %8.2f, lon = %8.2f\n",
        Add_space, i, points[i].lat, points[i].lon);
   }
}


