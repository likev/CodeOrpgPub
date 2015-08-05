/* prefs_load.h */

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/25 13:51:40 $
 * $Id: prefs_load.h,v 1.6 2014/03/25 13:51:40 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
 
#ifndef _PREFS_LOAD_H_
#define _PREFS_LOAD_H_



#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "global.h"

/*  beginning with cvg 6.2 we use the variable */
/*  CVG_PREF_DIR_NAME from global2.h this is */
/*  typically defined as 'cvgN' or 'cvgN.n' */


extern Widget screen_1, screen_2, screen_3;

int maxProducts;

int sort_method;
int prev_sort_method;

char config_dir[255];   /* the path in which the configuration files are kept */
char global_config_dir[255]; /* the location of the default configuration files */


/* standard directory for map files */
char map_dir[255]; /*  map_data in map_cvg */


assoc_array_i *pid_list;

assoc_array_i *msg_type_list;
/* CVG 9.1 - packet 1 coordinate override for geographic products */
assoc_array_i *packet_1_geo_coord_flag;
/* CVG 9.1 - added override of colors for non-2d array packets */
assoc_array_s *override_palette;
assoc_array_i *override_packet;

assoc_array_i *digital_legend_flag;
assoc_array_s *digital_legend_file;
assoc_array_s *dig_legend_file_2;
assoc_array_s *configured_palette;
assoc_array_s *config_palette_2;
assoc_array_i *associated_packet;
/* CVG 9.3 - added elevation flag 1=elevation-based product) */
assoc_array_i *elev_flag;
assoc_array_s *legend_units;

assoc_array_s *icao_list;
assoc_array_i *radar_type_list;
assoc_array_s *radar_names;

extern assoc_array_s *product_names;
extern assoc_array_s *short_prod_names;
extern assoc_array_s *product_mnemonics;



float *resolution_number_list;
char **resolution_name_list;
int number_of_resolutions;



/* default setting values for display attributes */
Boolean def_ring_val, def_map_val, def_line_val;

/* extern int overlay_flag; */


extern int  map_flag1, az_line_flag1, range_ring_flag1;
extern int  map_flag2, az_line_flag2, range_ring_flag2;
extern int  map_flag3, az_line_flag3, range_ring_flag3;

/*  following not yet used by preferences */
/* /extern int transp_label_flag1, transp_label_flag2; */
/* extern int linked_flag;  */ /* flag to determine if scrolling and animation are linked */

extern int verbose_flag;
/* CVG 9.0 */
extern int large_screen_flag, large_image_flag;

/* to be used in future */
extern int area_label;
extern int area_symbol;
extern int area_line_type;

extern assoc_array_i *product_res;

char map_filename[255];  /* the full file name of the map overlay file */
char product_database_filename[256];
char prev_db_filename[256];

/* prototypes */
void init_prefs();
void load_prefs();
extern void load_palette_list();

extern void read_to_eol(FILE *list_file, char *buf);

void load_product_info();
void load_site_info();

void load_program_prefs(int initial_read);
/* CVG 9.0 - added scr_sz and img_sz parameters */
void write_prefs_file(FILE *p_file, char *m_file, char *rr, char *az, 
                      char *map, char *vb, char *scr_sz, char *img_sz, char *db_file);
void load_radar_info();
void load_resolution_info();
void load_db_size();
void write_orpg_build(int build);

void write_sort_method(int method);
void load_sort_method();

extern void write_descript_source(int use_cvg_list);


extern void load_product_names(int from_child, int initial_read);
extern int check_for_directory_existence(char *dirname);
extern int check_filename(const char *type_file, char *buffer);


#endif

