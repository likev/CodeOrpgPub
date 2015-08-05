

/* DEBUG T2-1 */
if( (r<3) || (r>362) )
fprintf(stderr,"\nDEBUG T2-1 translating radial %d (0-based) with %d bins\n", 
        r, rad_data->num_range_bins);
if( (r<3) || (r>362) )
fprintf(stderr,"DEBUG bin 1: %d, bin 2: %d, bin 3: %d, bin 4: %d, bin 5: %d\n"
               "      bin 6: %d, bin 7: %d, bin 8: %d, bin 9: %d, bin10: %d\n",
               in_data_ptr[0],in_data_ptr[1],in_data_ptr[2],
               in_data_ptr[3],in_data_ptr[4],in_data_ptr[5],
               in_data_ptr[6],in_data_ptr[7],in_data_ptr[8],
               in_data_ptr[9]);
               

/* DEBUG T2-2 */
if( (r<3) || (r>362) )
fprintf(stderr,"DEBUG bin 1: %d, bin 2: %d, bin 3: %d, bin 4: %d, bin 5: %d\n"
               "      bin 6: %d, bin 7: %d, bin 8: %d, bin 9: %d, bin10: %d\n",
               rad_data->radial_data[r][0],rad_data->radial_data[r][1],
               rad_data->radial_data[r][2],rad_data->radial_data[r][3],
               rad_data->radial_data[r][4],rad_data->radial_data[r][5],
               rad_data->radial_data[r][6],rad_data->radial_data[r][7],
               rad_data->radial_data[r][8],rad_data->radial_data[r][9]);
               
/* DEBUG T2-3 */
if( (r<3) || (r>363) )
fprintf(stderr,"DEBUG T2-3reading internal radial %d (0-based) with %d bins\n", 
        r, rad.num_bytes);
        
/* DEBUG T2-4 */
if( (r<3) || (r>363) )
fprintf(stderr,"DEBUG bin 1: %d, bin 2: %d, bin 3: %d, bin 4: %d, bin 5: %d\n"
               "      bin 6: %d, bin 7: %d, bin 8: %d, bin 9: %d, bin10: %d\n",
               rad_data[0],rad_data[1],rad_data[2],rad_data[3],rad_data[4],
               rad_data[5],rad_data[6],rad_data[7],rad_data[8],rad_data[9]);
