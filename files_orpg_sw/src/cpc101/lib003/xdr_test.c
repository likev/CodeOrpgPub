
/******************************************************************

    Routines for testing serialization/deserialization of the generic
    products. The routines here can be used as coding examples for
    creating and using the generic product format.
	
******************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "orpg_product.h"

#define MAX_ARRAY_SIZE 0x7fffffff	/* there is no limit on array size in C
					*/
static char *Add_space = "";		/* for product print out */
static int Seq_number = 0;		/* for initializing test product */

static void Initialize_prod (RPGP_product_t *prod);
static void Print_prod (void *prodp);
static RPGP_area_t *Initialize_area ();
static void Print_components (int n_comps, char **comps);
static void Print_area (RPGP_area_t *area);
static void Print_points (int n_points, RPGP_location_t *points);
static void Print_params (int n_params, RPGP_parameter_t *params);
static RPGP_event_t *Initialize_event ();
static void Print_event (RPGP_event_t *event);
static void Set_param (RPGP_parameter_t *param);
static void Save_product (char *data, int size);
static int Read_product (char *fname, char **data, int has_header);
static void Read_bytes (int fd, char *data, int n_bytes);
static void Write_bytes (int fd, char *data, int n_bytes);
static RPGP_text_t *Initialize_text ();
static void Print_text (RPGP_text_t *text);
static void Initialize_ext_prod (RPGP_ext_data_t *prod);
static RPGP_grid_t *Initialize_grid ();
static RPGP_table_t *Initialize_table ();
static void Print_grid (RPGP_grid_t *grid);
static char *Get_token (char *text, char *buf, int buf_size);
static int Get_data_type (char *attrs, char *buf, int buf_size);
static char *Get_string (char *text);
static void Print_table (RPGP_table_t *table);
static void Print_azran_points (int n_points, RPGP_azran_location_t *points);
static void Print_xy_points (int n_points, RPGP_xy_location_t *points);
static RPGP_radial_t *Initialize_radial ();
static void Initialize_a_radial (RPGP_radial_data_t *rad);
static void Print_binaty_data (RPGP_data_t *data, int cnt);
static void Print_radial (RPGP_radial_t *radial);
static void Print_a_radial (RPGP_radial_data_t *rad);


/* #include "dbg_malloc.c" */

/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {
    void *prod, *deserial_data;
    char *serial_data;
    int sel_size, ret;

    if (argc > 1 &&
	strcmp (argv[1], "-r") == 0) {		/* reads serialized product */
	sel_size = Read_product (argv[2], &serial_data, 1);
	prod = NULL;
    }
    else if (argc > 1 &&
	strcmp (argv[1], "-x") == 0) {		/* reads serialized product */
	sel_size = Read_product (argv[2], &serial_data, 0);
	prod = NULL;
    }
    else {			/* creates a product and serializes it */
	if (1) {
	    prod = malloc (sizeof (RPGP_product_t));
	    Initialize_prod (prod);	/* test RPGP_product_t */
	}
	else {
	    prod = malloc (sizeof (RPGP_ext_data_t));
	    Initialize_ext_prod (prod);	/* test RPGP_ext_data_t */
	}
	Print_prod (prod);

	sel_size = RPGP_product_serialize (prod, &serial_data);
	printf ("RPGP_product_serialize ret %d\n", sel_size);
	if (sel_size < 0)
	    exit (1);
    }

    if (argc > 1 &&
	strcmp (argv[1], "-s") == 0) {	/* save serialized product to file */
	Save_product (serial_data, sel_size);
	exit (0);
    }

    ret = RPGP_product_deserialize (serial_data, sel_size, 
						(void **)&deserial_data);
    printf ("RPGP_product_deserialize ret %d\n", ret);
    if (ret < 0)
	exit (1);

    Print_prod (deserial_data);

    ret = RPGP_product_free (deserial_data);
    printf ("RPGP_product_free deserial_data ret %d\n", ret);
    if (prod != NULL) {
	ret = RPGP_product_free (prod);
	printf ("RPGP_product_free prod ret %d\n", ret);
    }
    free (serial_data);
  
    exit (0);
}

/******************************************************************

    Initializes the RPGP product struct for testing.

******************************************************************/

static void Initialize_prod (RPGP_product_t *prod) {

    prod->name = Get_string ("test product");
    prod->description = Get_string ("for test only");
    prod->product_id = 99;
    prod->type = RPGP_VOLUME;
    prod->gen_time = time (NULL);
    prod->radar_name = Get_string ("KTLX");
    prod->radar_lat = 10.1;
    prod->radar_lon = 20.2;
    prod->radar_height = 30.3;
    prod->volume_time = 12345;
    prod->elevation_time = 54321;
    prod->elevation_angle = .5;
    prod->volume_number = 9;
    prod->operation_mode = RPGP_OP_WEATHER;
    prod->vcp = 13;
    prod->elevation_number = 2;
    prod->compress_type = 0;
    prod->size_decompressed = 0;

    prod->numof_prod_params = 2;
    prod->prod_params = malloc (sizeof (RPGP_parameter_t) * 2);
    Set_param (prod->prod_params + 0);
    Set_param (prod->prod_params + 1);

    /* initialize a component */
    prod->numof_components = 2;
    prod->components = (void **)malloc (sizeof (char *) * 
						prod->numof_components);
    prod->components[0] = (char *)Initialize_grid ();
		/* to test different component, replace the following by
       		   Initialize_grid, Initialize_text, Initialize_area, 
		   Initialize_event, Initialize_radial, or Initialize_table */

    /* If we use two components, the following sets the second one */
    if (prod->numof_components > 1)
	prod->components[1] = (char *)Initialize_radial ();
}

/******************************************************************

    Initializes the RPGP external product struct for testing.

******************************************************************/

static void Initialize_ext_prod (RPGP_ext_data_t *prod) {

    prod->name = Get_string ("test external product");
    prod->description = Get_string ("for test only");
    prod->product_id = 99;
    prod->type = RPGP_EXTERNAL;
    prod->spare[0] = 0;
    prod->spare[1] = 0;
    prod->spare[2] = 0;
    prod->spare[3] = 0;
    prod->spare[4] = 0;
    prod->gen_time = time (NULL);
    prod->compress_type = 0;
    prod->size_decompressed = 0;

    prod->numof_prod_params = 2;
    prod->prod_params = malloc (sizeof (RPGP_parameter_t) * 2);
    Set_param (prod->prod_params + 0);
    Set_param (prod->prod_params + 1);

    /* initialize a component */
    prod->numof_components = 1;
    prod->components = (void **)malloc (sizeof (char *) * 
						prod->numof_components);
    prod->components[0] = (char *)Initialize_grid ();
		/* to test different component, replace the following by
       		   Initialize_grid, Initialize_text, Initialize_area, 
		   Initialize_event, Initialize_radial or Initialize_table */

}

/******************************************************************

    Initializes parameter "param" for testing.

******************************************************************/

static void Set_param (RPGP_parameter_t *param) {
    char buf[128];

    sprintf (buf, "ID-%d", Seq_number++);
    param->id = Get_string (buf);
    sprintf (buf, "name = (%d)", Seq_number++);
    param->attrs = Get_string (buf);
}

/******************************************************************

    Initializes RPGP_event_t "event" for testing.

******************************************************************/

static RPGP_event_t *Initialize_event () {
    char **comps;
    RPGP_event_t *event;

    event = (RPGP_event_t *)malloc (sizeof (RPGP_event_t));
    event->comp_type = RPGP_EVENT_COMP;
    event->numof_event_params = 2;
    event->event_params = malloc (sizeof (RPGP_parameter_t) * 2);
    Set_param (event->event_params + 0);
    Set_param (event->event_params + 1);

    event->numof_components = 2;
    comps = malloc (sizeof (char *) * 2);
    event->components = (void **)comps;
    comps[0] = (void *)Initialize_area ();
    comps[1] = (void *)Initialize_area ();
    return (event);
}

/******************************************************************

    Initializes a radial component for testing.

******************************************************************/

static RPGP_radial_t *Initialize_radial () {
    RPGP_radial_t *radial;
    int i;

    radial = malloc (sizeof (RPGP_radial_t));
    radial->comp_type = RPGP_RADIAL_COMP;
    radial->description = Get_string ("reflectivity");
    radial->bin_size = 250.;
    radial->first_range = 10.2;
    radial->numof_comp_params = 2;
    radial->comp_params = malloc (sizeof (RPGP_parameter_t) * 2);
    Set_param (radial->comp_params + 0);
    Set_param (radial->comp_params + 1);

    radial->numof_radials = 2;
    radial->radials = malloc (radial->numof_radials * 
					sizeof (RPGP_radial_data_t));
    for (i = 0; i < radial->numof_radials; i++)
	Initialize_a_radial (radial->radials + i);
	
    return (radial);
}

/******************************************************************

    Initializes a radial for testing.

******************************************************************/

static void Initialize_a_radial (RPGP_radial_data_t *rad) {
    static int v = 1;
    unsigned char *pt;
    int i;

    rad->azimuth = (float)(v++) + .1;
    rad->width = (float)(v++) + .2;
    rad->elevation = (float)(v++) + .3;
    rad->n_bins = 3;

    rad->bins.attrs = Get_string ("type = ubyte");
    rad->bins.data = malloc (rad->n_bins * sizeof (char));
    pt = (unsigned char *)rad->bins.data;
    for (i = 0; i < rad->n_bins; i++)
	pt[i] = v;
}

/******************************************************************

    Initializes RPGP_area_t "area" for testing.

******************************************************************/

static RPGP_area_t *Initialize_area () {
    RPGP_location_t *loc;
    RPGP_area_t *area;

    area = malloc (sizeof (RPGP_area_t));
    area->comp_type = RPGP_AREA_COMP;
    area->numof_comp_params = 2;
    area->comp_params = malloc (sizeof (RPGP_parameter_t) * 2);
    Set_param (area->comp_params + 0);
    Set_param (area->comp_params + 1);

    area->area_type = RPGP_XY_LOCATION | RPGP_AT_POLYLINE;
    area->numof_points = 2;
    loc = malloc (sizeof (RPGP_location_t) * 2);
    area->points = loc;
    loc[0].lat = (float)Seq_number++;
    loc[0].lon = (float)Seq_number++;
    loc[1].lat = (float)Seq_number++;
    loc[1].lon = (float)Seq_number++;
    return (area);
}

/******************************************************************

    Initializes RPGP_text_t "text" for testing.

******************************************************************/

static RPGP_text_t *Initialize_text () {
    RPGP_text_t *text;

    text = malloc (sizeof (RPGP_text_t));
    text->comp_type = RPGP_TEXT_COMP;
    text->numof_comp_params = 2;
    text->comp_params = malloc (sizeof (RPGP_parameter_t) * 2);
    Set_param (text->comp_params + 0);
    Set_param (text->comp_params + 1);

    text->text = malloc (256);
    strcpy (text->text, "This is line 1\nThis is line 2\n");
    return (text);
}

/******************************************************************

    Initializes a grid component for testing.

******************************************************************/

static RPGP_grid_t *Initialize_grid () {
    RPGP_grid_t *grid;
    int n_dim, dim0, dim1, i;
/*    unsigned char *pt; */
    float *pt;

    n_dim = 2;
    dim0 = 3;
    dim1 = 4;
    grid = malloc (sizeof (RPGP_grid_t));
    grid->comp_type = RPGP_GRID_COMP;
    grid->n_dimensions = n_dim;
    grid->dimensions = malloc (2 * sizeof (int));
    grid->dimensions[0] = dim0;
    grid->dimensions[1] = dim1;
    grid->grid_type = RPGP_GT_EQUALLY_SPACED;
    grid->numof_comp_params = 2;
    grid->comp_params = malloc (sizeof (RPGP_parameter_t) * 2);
    Set_param (grid->comp_params + 0);
    Set_param (grid->comp_params + 1);

    grid->data.attrs = Get_string ("type = float");
    grid->data.data = malloc (dim0 * dim1 * sizeof (float));
    pt = (float *)grid->data.data;
    for (i = 0; i < dim0 * dim1; i++)
	pt[i] = i;
	
    return (grid);
}

/******************************************************************

    Initializes a table component for testing.

******************************************************************/

static RPGP_table_t *Initialize_table () {
    RPGP_table_t *table;
    int i, j;
    char buf[128];

    table = malloc (sizeof (RPGP_table_t));
    table->comp_type = RPGP_TABLE_COMP;
    table->title.text = Get_string ("This is title");
    table->n_columns = 2;
    table->n_rows = 3;

    table->numof_comp_params = 2;
    table->comp_params = malloc (sizeof (RPGP_parameter_t) * 2);
    Set_param (table->comp_params + 0);
    Set_param (table->comp_params + 1);

    table->column_labels = malloc (table->n_columns * sizeof (RPGP_string_t));
    table->row_labels = malloc (table->n_rows * sizeof (RPGP_string_t));
    table->entries = 
	malloc (table->n_rows * table->n_columns * sizeof (RPGP_string_t));

    for (i = 0; i < table->n_columns; i++) {
	sprintf (buf, "C %d", i);
	table->column_labels[i].text = Get_string (buf);
    }
    for (i = 0; i < table->n_rows; i++) {
	sprintf (buf, "R %d", i);
	table->row_labels[i].text = Get_string (buf);
	for (j = 0; j < table->n_columns; j++) {
	    sprintf (buf, "%d-%d", i, j);
	    table->entries[i * table->n_columns + j].text = Get_string (buf);
	}
    }
	
    return (table);
}

/******************************************************************

    Returns string based on "text".

******************************************************************/

static char *Get_string (char *text) {
    char *p = malloc (strlen (text) + 1);
    strcpy (p, text);
    return (p);
}

/******************************************************************

    Prints the RPGP product struct for verification.

******************************************************************/

static void Print_prod (void *prodp) {
    RPGP_product_t *prod;

    prod = (RPGP_product_t *)prodp;
    printf ("\n");
    printf ("product id %d, name: %s, desc: %s.\n", 
			prod->product_id, prod->name, prod->description);
    printf ("    type %d, time %d\n", prod->type, prod->gen_time);
    if (prod->type == RPGP_EXTERNAL) {
	RPGP_ext_data_t *eprod = (RPGP_ext_data_t *)prodp;
	printf ("    compress type %d, size %d\n", 
			    eprod->compress_type, eprod->size_decompressed);
	Print_params (eprod->numof_prod_params, eprod->prod_params);
	Print_components (eprod->numof_components, (char **)eprod->components);
    }
    else {
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
	Print_params (prod->numof_prod_params, prod->prod_params);
	Print_components (prod->numof_components, (char **)prod->components);
    }
    printf ("\n");
}

/******************************************************************

    Prints components array in "comps" of size "n_comps" for 
    verification.

******************************************************************/

static void Print_components (int n_comps, char **comps) {
    RPGP_area_t **gencomps;
    int i;

    printf ("%s    number of components %d\n", Add_space, n_comps);
    gencomps = (RPGP_area_t **)comps;
    for (i = 0; i < n_comps; i++) {

	switch (gencomps[i]->comp_type) {

	    case RPGP_AREA_COMP:
		Print_area ((RPGP_area_t *)comps[i]);
		break;

	    case RPGP_TEXT_COMP:
		Print_text ((RPGP_text_t *)comps[i]);
		break;

	    case RPGP_EVENT_COMP:
		Print_event ((RPGP_event_t *)comps[i]);
		break;

	    case RPGP_GRID_COMP:
		Print_grid ((RPGP_grid_t *)comps[i]);
		break;

	    case RPGP_RADIAL_COMP:
		Print_radial ((RPGP_radial_t *)comps[i]);
		break;

	    case RPGP_TABLE_COMP:
		Print_table ((RPGP_table_t *)comps[i]);
		break;

	    default:
		break;
	}
    }
}

/******************************************************************

    Prints area "area" for verification.

******************************************************************/

static void Print_area (RPGP_area_t *area) {

    printf ("%s    AREA: (%d), area type %d, location type %x\n", 
			Add_space, area->comp_type, 
			RPGP_AREA_TYPE (area->area_type),
			RPGP_LOCATION_TYPE (area->area_type));
    Print_params (area->numof_comp_params, area->comp_params);
    switch (RPGP_LOCATION_TYPE (area->area_type)) {
	case RPGP_LATLON_LOCATION:
	    Print_points (area->numof_points, area->points);
	    break;
	case RPGP_XY_LOCATION:
	    Print_xy_points (area->numof_points, area->points);
	    break;
	case RPGP_AZRAN_LOCATION:
	    Print_azran_points (area->numof_points, area->points);
	    break;
	default:
	    printf ("Unexpected location type (%d)\n", 
			RPGP_LOCATION_TYPE (area->area_type));
    }
}

/******************************************************************

    Prints text "text" for verification.

******************************************************************/

static void Print_text (RPGP_text_t *text) {

    printf ("%s    TEXT: (%d)\n", Add_space, text->comp_type);
    Print_params (text->numof_comp_params, text->comp_params);
    printf ("%s    text: %s\n", Add_space, text->text);
}

/******************************************************************

    Prints grid "grid" for verification.

******************************************************************/

static void Print_grid (RPGP_grid_t *grid) {
    int i, total_data;

    printf ("%s    GRID: (%d), gtype %d, ndim %d (", 
	Add_space, grid->comp_type, grid->grid_type, grid->n_dimensions);
    total_data = 1;
    for (i = 0; i < grid->n_dimensions; i++) {
	printf (" %d", grid->dimensions[i]);
	total_data *= grid->dimensions[i];
    }
    printf (")\n");
    Print_params (grid->numof_comp_params, grid->comp_params);
    if (grid->n_dimensions <= 0) {
	printf ("Print_grid: n_dimensions is %d\n", grid->n_dimensions);
	return;
    }
    if (total_data <= 0) {
	printf ("Print_grid: No data\n");
	return;
    }
    Print_binaty_data (&(grid->data), total_data);
}

/******************************************************************

    Prints binary data struct "data" for verification.

******************************************************************/

static void Print_binaty_data (RPGP_data_t *data, int cnt) {
    char type[128];

    printf ("data attributes: %s\n", data->attrs);
    if (Get_data_type (data->attrs, type, 128) > 0) {
	int i;
	if (strcmp (type, "short") == 0) {
	    short *spt;
	    printf ("Data:");
	    spt = (short *)data->data;
	    for (i = 0; i < cnt; i++)
		printf (" %d", spt[i]);
	    printf ("\n");
	}
	else if (strcmp (type, "ubyte") == 0) {
	    unsigned char *cpt;
	    printf ("Data:");
	    cpt = (unsigned char *)data->data;
	    for (i = 0; i < cnt; i++)
		printf (" %d", cpt[i]);
	    printf ("\n");
	}
	else if (strcmp (type, "float") == 0) {
	    float *fpt;
	    printf ("Data:");
	    fpt = (float *)data->data;
	    for (i = 0; i < cnt; i++)
		printf (" %f", fpt[i]);
	    printf ("\n");
	}
	else if (strcmp (type, "ushort") == 0) {
	    unsigned short *spt;
	    printf ("Data:");
	    spt = (unsigned short *)data->data;
	    for (i = 0; i < cnt; i++)
		printf (" %d", spt[i]);
	    printf ("\n");
	}
	else
	    printf ("Print_grid: type (%s) not implemented\n", type);
    }
    else
	printf ("Print_grid: type not found\n");
}

/******************************************************************

    Prints radial component "radial" for verification.

******************************************************************/

static void Print_radial (RPGP_radial_t *radial) {
    int i;

    printf ("%s    RADIAL: (%d), description %s\n", 
	Add_space, radial->comp_type, radial->description);
    printf ("%s    bin_size %f, first_range %f, %d radials\n", 
	Add_space, radial->bin_size, radial->first_range, 
						radial->numof_radials);
    Print_params (radial->numof_comp_params, radial->comp_params);
    if (radial->numof_radials <= 0)
	return;

    for (i = 0; i < radial->numof_radials; i++) {
	printf ("    RADIAL %d\n", i);
	Print_a_radial (radial->radials + i);
    }
}

/******************************************************************

    Prints radial "rad" for verification.

******************************************************************/

static void Print_a_radial (RPGP_radial_data_t *rad) {

    printf ("%s    azimuth %f, width %f, elevation %f, n_bins %d\n", 
	Add_space, rad->azimuth, rad->width, rad->elevation, rad->n_bins);
    Print_binaty_data (&(rad->bins), rad->n_bins);
}

/******************************************************************

    Prints grid "grid" for verification.

******************************************************************/

static void Print_table (RPGP_table_t *table) {
    int i, j;

    printf ("%s    TABLE: (%d), n_columns %d, n_rows %d\n", 
	Add_space, table->comp_type, table->n_columns, table->n_rows);

    Print_params (table->numof_comp_params, table->comp_params);

    printf ("    title: %s\n", table->title.text);
    printf ("    column lables:");
    for (i = 0; i < table->n_columns; i++)
	printf (" %s", table->column_labels[i].text);
    printf ("\n");
    printf ("    row lables:");
    for (i = 0; i < table->n_rows; i++)
	printf (" %s", table->row_labels[i].text);
    printf ("\n");
    printf ("    table:\n");
    for (i = 0; i < table->n_rows; i++) {
	printf ("    ");
	for (j = 0; j < table->n_columns; j++)
	    printf (" %s", table->entries[i * table->n_columns + j].text);
	printf ("\n");
    }
}

/******************************************************************

    Prints event "event" for verification.

******************************************************************/

static void Print_event (RPGP_event_t *event) {

    printf ("    EVENT: (%d)\n", event->comp_type);
    Add_space = "    ";
    Print_params (event->numof_event_params, event->event_params);
    Print_components (event->numof_components, (char **)event->components);
    Add_space = "";
}

/******************************************************************

    Prints "n_params" parameters pointed to by "params" for 
    verification.

******************************************************************/

static void Print_params (int n_params, RPGP_parameter_t *params) {
    int i;

    printf ("%s        # %d params:\n", Add_space, n_params);
    for (i = 0; i < n_params; i++) {
	printf ("%s            param[%d]: id: %s, attrs: %s\n", 
		Add_space, i, params[i].id, params[i].attrs);
    }
}

/******************************************************************

    Prints array "points" of type RPGP_location_t of size "n_points" 
    for verification.

******************************************************************/

static void Print_points (int n_points, RPGP_location_t *points) {
    int i;

    printf ("%s        $ %d points:\n", Add_space, n_points);
    for (i = 0; i < n_points; i++) {
	printf ("%s            point[%d]: lat = %8.2f, lon = %8.2f\n", 
		Add_space, i, points[i].lat, points[i].lon);
    }
}

/******************************************************************

    Prints array "points" of type RPGP_xy_location_t of size "n_points" 
    for verification.

******************************************************************/

static void Print_xy_points (int n_points, RPGP_xy_location_t *points) {
    int i;

    printf ("%s        $ %d points:\n", Add_space, n_points);
    for (i = 0; i < n_points; i++) {
	printf ("%s            point[%d]: x = %8.2f, y = %8.2f\n", 
		Add_space, i, points[i].x, points[i].y);
    }
}

/******************************************************************

    Prints array "points" of type RPGP_azran_location_t of size "n_points" 
    for verification.

******************************************************************/

static void Print_azran_points (int n_points, RPGP_azran_location_t *points) {
    int i;

    printf ("%s        $ %d points:\n", Add_space, n_points);
    for (i = 0; i < n_points; i++) {
	printf ("%s            point[%d]: range = %8.2f, azi = %8.2f\n", 
		Add_space, i, points[i].range, points[i].azi);
    }
}

/******************************************************************

    Saves data "data" of "size" bytes to a file. A header is written
    before the data.

******************************************************************/

#define HEADER_SIZE 144 + 96

static void Save_product (char *data, int size) {
    int fd;
    char hd[HEADER_SIZE];

    fd = open ("xdr_test_data", O_CREAT | O_RDWR | O_TRUNC, 0660);
    if (fd < 0) {
	fprintf (stderr, "open (for writing) failed (errno %d)\n", errno);
	exit (1);
    }
    Write_bytes (fd, hd, HEADER_SIZE);
    Write_bytes (fd, data, size);
    printf ("%d bytes saved to \"xdr_test_data\"\n", size);
    close (fd);
}

/******************************************************************

    Writes "data" of "n_bytes" bytes to file "fd" and checks error.

******************************************************************/

static void Write_bytes (int fd, char *data, int n_bytes) {
    int ret;

    ret = write (fd, data, n_bytes);
    if (ret < 0) {
	fprintf (stderr, "write failed (errno %d)\n", errno);
	exit (1);
    }
    if (ret != n_bytes) {
	fprintf (stderr, "write (%d bytes) failed (returned %d)\n", 
						n_bytes, ret);
	exit (1);
    }
}

/******************************************************************

    Reads all data in a file and returns the data with "data". The 
    return values is the number of bytes in the data. The hd before
    the data is read first and discarded.

******************************************************************/

static int Read_product (char *fname, char **data, int has_header) {
    int fd, size;
    char *buf, hd[HEADER_SIZE];

    fd = open (fname, O_RDONLY);
    if (fd < 0) {
	fprintf (stderr, "open (%s, for reading) failed (errno %d)\n", 
						fname, errno);
	exit (1);
    }
    size = lseek (fd, 0, SEEK_END);
    if (size < 0) {
	fprintf (stderr, "lseek failed (errno %d)\n", errno);
	exit (1);
    }
    lseek (fd, 0, SEEK_SET);
    if (has_header) {
	Read_bytes (fd, hd, HEADER_SIZE);
	size -= HEADER_SIZE;
    }
    if (size <= 0) {
	fprintf (stderr, "No data to read (%d)\n", size);
	exit (1);
    }
    buf = malloc (size);
    if (buf == NULL) {
	fprintf (stderr, "malloc failed\n");
	exit (1);
    }
    Read_bytes (fd, buf, size);
    *data = buf;
    printf ("%d bytes read from %s\n", size, fname);
    close (fd);
    return (size);
}

/******************************************************************

    Reads "n_bytes" bytes from file "fd", checks error and returns
    the data with "data".

******************************************************************/

static void Read_bytes (int fd, char *data, int n_bytes) {
    int ret;

    ret = read (fd, data, n_bytes);
    if (ret < 0) {
	fprintf (stderr, "read failed (errno %d)\n", errno);
	exit (1);
    }
    if (ret != n_bytes) {
	fprintf (stderr, "read (%d bytes) failed (returned %d)\n", 
						n_bytes, ret);
	exit (1);
    }
}

/******************************************************************

    Searches for the type value in attribute string "attrs". If the
    type value is found, it is returned in "buf" of size "buf_size" 
    and the function returns the length of the type value. Returns 
    -1 if the type is not found. The type value must be non-empty 
    and single token.  

******************************************************************/

static int Get_data_type (char *attrs, char *buf, int buf_size) {
    char t1[16], t2[16], t3[16], t4[16], *p;
    int new_attr;

    if (attrs == NULL)
	return (-1);
    p = attrs;
    new_attr = 1;
    while (1) {
	p = Get_token (p, t1, 16);
	if (t1[0] == '\0')
	    break;
	if (strcmp (t1, ";") == 0) {
	    new_attr = 1;
	    continue;
	}
	if (!new_attr)
	    continue;
	if (strcasecmp (t1, "type") == 0) {
	    char *pp = Get_token (p, t2, 16);
	    if (strcmp (t2, "=") == 0) {
		int len;
		pp = Get_token (pp, t3, 16);
		pp = Get_token (pp, t4, 16);
		if (t3[0] != '\0' &&
		    (t4[0] == '\0' || strcmp (t4, ";") == 0)) {
		    len = strlen (t3);
		    if (len >= buf_size)
			len = buf_size - 1;
		    strncpy (buf, t3, len);
		    buf[len] = '\0';
		    return (len);
		}
	    }
	}
	new_attr = 0;
    }
    buf[0] = '\0';
    return (-1);
}

/******************************************************************

    Finds the first token of "text" in "buf" of size "buf_size". The
    returned token is always null-terminated and possibly truncated.
    Returns the pointer after the token. A token is a word separated
    by space, tab or line return. "=" and ";" is considered as a
    token even if they are not separated by space. If "text" is an
    empty string, an empty string is returned in "buf" and the return
    value is "text".
	
******************************************************************/

static char *Get_token (char *text, char *buf, int buf_size) {
    char *p, *st, *next;
    int len;

    p = text;
    while (*p == ' ' || *p == '\t' || *p == '\n')
	p++;
    st = p;
    if (*p == '=' || *p == ';')
	len = 1;
    else if (*p == '\0')
	len = 0;
    else {
	while (*p != '\0' && *p != ' ' && *p != '\t' && 
				*p != '\n' && *p != '='  && *p != ';')
	    p++;
	len = p - st;
    }
    next = st + len;
    if (len >= buf_size)
	len = buf_size - 1;
    strncpy (buf, st, len);
    buf[len] = '\0';
    return (next);
}





