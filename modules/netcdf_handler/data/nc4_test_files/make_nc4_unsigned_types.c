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
main() {/* create nc4_unsigned_types.nc */

    int  stat;  /* return status */
    int  ncid;  /* netCDF id */

    /* group ids */
    int root_grp;

    /* dimension ids */
    int lat_dim;
    int lon_dim;
    int time_dim;

    /* dimension lengths */
    size_t lat_len = 6;
    size_t lon_len = 5;
    size_t time_len = NC_UNLIMITED;

    /* variable ids */
    int lat_id;
    int lon_id;
    int time_id;
    int temp_id;
    int rh_id;

    /* rank (number of dimensions) for each variable */
#   define RANK_lat 1
#   define RANK_lon 1
#   define RANK_time 1
#   define RANK_temp 3
#   define RANK_rh 3

    /* variable shapes */
    int lat_dims[RANK_lat];
    int lon_dims[RANK_lon];
    int time_dims[RANK_time];
    int temp_dims[RANK_temp];
    int rh_dims[RANK_rh];

    /* enter define mode */
    stat = nc_create("nc4_unsigned_types.nc", NC_CLOBBER|NC_NETCDF4, &ncid);
    check_err(stat,__LINE__,__FILE__);
    root_grp = ncid;

    /* define dimensions */
    stat = nc_def_dim(root_grp, "lat", lat_len, &lat_dim);
    check_err(stat,__LINE__,__FILE__);
    stat = nc_def_dim(root_grp, "lon", lon_len, &lon_dim);
    check_err(stat,__LINE__,__FILE__);
    stat = nc_def_dim(root_grp, "time", time_len, &time_dim);
    check_err(stat,__LINE__,__FILE__);

    /* define variables */

    lat_dims[0] = lat_dim;
    stat = nc_def_var(root_grp, "lat", NC_INT, RANK_lat, lat_dims, &lat_id);
    check_err(stat,__LINE__,__FILE__);

    lon_dims[0] = lon_dim;
    stat = nc_def_var(root_grp, "lon", NC_INT, RANK_lon, lon_dims, &lon_id);
    check_err(stat,__LINE__,__FILE__);

    time_dims[0] = time_dim;
    stat = nc_def_var(root_grp, "time", NC_INT, RANK_time, time_dims, &time_id);
    check_err(stat,__LINE__,__FILE__);

    temp_dims[0] = time_dim;
    temp_dims[1] = lat_dim;
    temp_dims[2] = lon_dim;
    stat = nc_def_var(root_grp, "temp", NC_UINT, RANK_temp, temp_dims, &temp_id);
    check_err(stat,__LINE__,__FILE__);

    rh_dims[0] = time_dim;
    rh_dims[1] = lat_dim;
    rh_dims[2] = lon_dim;
    stat = nc_def_var(root_grp, "rh", NC_USHORT, RANK_rh, rh_dims, &rh_id);
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
    { /* units */
    stat = nc_put_att_text(root_grp, lat_id, "units", 13, "degrees_north");
    check_err(stat,__LINE__,__FILE__);
    }
    { /* units */
    stat = nc_put_att_text(root_grp, lon_id, "units", 12, "degrees_east");
    check_err(stat,__LINE__,__FILE__);
    }
    { /* units */
    stat = nc_put_att_text(root_grp, time_id, "units", 7, "seconds");
    check_err(stat,__LINE__,__FILE__);
    }
    { /* _FillValue */
    static const unsigned short rh_FillValue_att[1] = {9999} ;
    stat = nc_put_att_ushort(root_grp, rh_id, "_FillValue", NC_USHORT, 1, rh_FillValue_att);    check_err(stat,__LINE__,__FILE__);
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
    int time_data[2] = {1, 2} ;
    size_t time_startset[1] = {0} ;
    size_t time_countset[1] = {2} ;
    stat = nc_put_vara(root_grp, time_id, time_startset, time_countset, time_data);
    check_err(stat,__LINE__,__FILE__);
    }

    {
    unsigned int temp_data[60] = {7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U} ;
    size_t temp_startset[3] = {0, 0, 0} ;
    size_t temp_countset[3] = {2, 6, 5} ;
    stat = nc_put_vara(root_grp, temp_id, temp_startset, temp_countset, temp_data);
    check_err(stat,__LINE__,__FILE__);
    }

    {
    unsigned short rh_data[60] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2} ;
    size_t rh_startset[3] = {0, 0, 0} ;
    size_t rh_countset[3] = {2, 6, 5} ;
    stat = nc_put_vara(root_grp, rh_id, rh_startset, rh_countset, rh_data);
    check_err(stat,__LINE__,__FILE__);
    }


    stat = nc_close(root_grp);
    check_err(stat,__LINE__,__FILE__);
    return 0;
}
