
/* I modified this program so that the attributes for 'station' are strings
 * and not chars. jhrg 8/5/11
 */

#include <stdio.h>
#include <stdlib.h>
#include <netcdf.h>




void
check_err(const int stat, const int line, const char *file) {
    if (stat != NC_NOERR) {
        (void)fprintf(stderr,"line %d of %s: %s\n", line, file, nc_strerror(stat));
        fflush(stderr);
        exit(1);
    }
}

int
main() {/* create nc4_strings.nc */

    int  stat;  /* return status */
    int  ncid;  /* netCDF id */

    /* group ids */
    int root_grp;

    /* dimension ids */
    int lat_dim;
    int lon_dim;

    /* dimension lengths */
    size_t lat_len = 6;
    size_t lon_len = 5;

    /* variable ids */
    int lat_id;
    int lon_id;
    int station_id;
    int scan_line_id;
    int codec_name_id;

    /* rank (number of dimensions) for each variable */
#   define RANK_lat 1
#   define RANK_lon 1
#   define RANK_station 2
#   define RANK_scan_line 1
#   define RANK_codec_name 0

    /* variable shapes */
    int lat_dims[RANK_lat];
    int lon_dims[RANK_lon];
    int station_dims[RANK_station];
    int scan_line_dims[RANK_scan_line];

    /* enter define mode */
    stat = nc_create("nc4_strings.nc", NC_CLOBBER|NC_NETCDF4, &ncid);
    check_err(stat,__LINE__,__FILE__);
    root_grp = ncid;

    /* define dimensions */
    stat = nc_def_dim(root_grp, "lat", lat_len, &lat_dim);
    check_err(stat,__LINE__,__FILE__);
    stat = nc_def_dim(root_grp, "lon", lon_len, &lon_dim);
    check_err(stat,__LINE__,__FILE__);

    /* define variables */

    lat_dims[0] = lat_dim;
    stat = nc_def_var(root_grp, "lat", NC_INT, RANK_lat, lat_dims, &lat_id);
    check_err(stat,__LINE__,__FILE__);

    lon_dims[0] = lon_dim;
    stat = nc_def_var(root_grp, "lon", NC_INT, RANK_lon, lon_dims, &lon_id);
    check_err(stat,__LINE__,__FILE__);

    station_dims[0] = lat_dim;
    station_dims[1] = lon_dim;
    stat = nc_def_var(root_grp, "station", NC_STRING, RANK_station, station_dims, &station_id);
    check_err(stat,__LINE__,__FILE__);

    scan_line_dims[0] = lon_dim;
    stat = nc_def_var(root_grp, "scan_line", NC_STRING, RANK_scan_line, scan_line_dims, &scan_line_id);
    check_err(stat,__LINE__,__FILE__);

    stat = nc_def_var(root_grp, "codec_name", NC_STRING, RANK_codec_name, 0, &codec_name_id);
    check_err(stat,__LINE__,__FILE__);

    /* assign global attributes */
    { /* title */
    stat = nc_put_att_text(root_grp, NC_GLOBAL, "title", 32, "Hyrax/netcdf handler test file 2");
    check_err(stat,__LINE__,__FILE__);
    }
    { /* version */
    static const double version_att[1] = {1} ;
    stat = nc_put_att_double(root_grp, NC_GLOBAL, "version", NC_DOUBLE, 1, version_att);
    check_err(stat,__LINE__,__FILE__);
    }
    { /* description */
    stat = nc_put_att_text(root_grp, NC_GLOBAL, "description", 58, "This file has all of the new netcdf 4 unsigned data types.");
    check_err(stat,__LINE__,__FILE__);
    }

    /* assign per-variable attributes */
    { /* who */
    char *vals_1[1] = {"james"};
    stat = nc_put_att_string(root_grp, station_id, "who", 1, vals_1);
    check_err(stat,__LINE__,__FILE__);
    }
    { /* names */
    char* vals_2[3] = {"site_1", "site_2", "site_3"};
    stat = nc_put_att_string(root_grp, station_id, "names", 3, vals_2);
    check_err(stat,__LINE__,__FILE__);
    }


    /* leave define mode */
    stat = nc_enddef (root_grp);
    check_err(stat,__LINE__,__FILE__);

    /* assign variable data */
    {
    int lat_data[6] = {0, 10, 20, 30, 40, 50} ;
    size_t lat_startset[1] = {0} ;
    size_t lat_countset[1] = {6} ;
    stat = nc_put_vara(root_grp, lat_id, lat_startset, lat_countset, lat_data);
    check_err(stat,__LINE__,__FILE__);
    }

    {
    int lon_data[5] = {-140, -118, -96, -84, -52} ;
    size_t lon_startset[1] = {0} ;
    size_t lon_countset[1] = {5} ;
    stat = nc_put_vara(root_grp, lon_id, lon_startset, lon_countset, lon_data);
    check_err(stat,__LINE__,__FILE__);
    }

    {
    char* station_data[30] = {"one", "two", "three", "four", "five", "one_b", "two_b", "three_b", "four_b", "five_b", "one_c", "two_c", "three_c", "four_c", "five_c", "one", "two", "three", "four", "five", "one", "two", "three", "four", "five", "one_f", "two_f", "three_f", "four_f", "five_f"} ;
    size_t station_startset[2] = {0, 0} ;
    size_t station_countset[2] = {6, 5} ;
    stat = nc_put_vara(root_grp, station_id, station_startset, station_countset, station_data);
    check_err(stat,__LINE__,__FILE__);
    }

    {
    char* scan_line_data[5] = {"r", "r1", "r2", "r3", "r4"} ;
    size_t scan_line_startset[1] = {0} ;
    size_t scan_line_countset[1] = {5} ;
    stat = nc_put_vara(root_grp, scan_line_id, scan_line_startset, scan_line_countset, scan_line_data);
    check_err(stat,__LINE__,__FILE__);
    }

    {
    size_t zero = 0;
    static char* codec_name_data[1] = {"mp3"};
    stat = nc_put_var1(root_grp, codec_name_id, &zero, codec_name_data);    check_err(stat,__LINE__,__FILE__);
    }

    stat = nc_close(root_grp);
    check_err(stat,__LINE__,__FILE__);
    return 0;
}
