/// @file

/// This class contains the main functionality for reading a file that
/// may be given a STARE sidecar file.

/// Ed Hartnett 4/4/20

#ifndef GEO_FILE_H_ /**< Protect file from double include. */
#define GEO_FILE_H_

#include <string>
#include <vector>

#include <STARE.h>

#include "BESInternalError.h"

#define SSC_LAT_NAME "Latitude"
#define SSC_LON_NAME "Longitude"
#define SSC_I_NAME "i"
#define SSC_J_NAME "j"
#define SSC_INDEX_NAME "STARE_index"
#define SSC_COVER_NAME "STARE_cover"
#define SSC_LONG_NAME "long_name"
#define SSC_INDEX_LONG_NAME "SpatioTemporal Adaptive Resolution Encoding (STARE) index"
#define SSC_COVER_LONG_NAME "SpatioTemporal Adaptive Resolution Encoding (STARE) cover"
#define SSC_LAT_LONG_NAME "latitude"
#define SSC_LON_LONG_NAME "longitude"
#define SSC_UNITS "units"
#define SSC_LAT_UNITS "degrees_north"
#define SSC_LON_UNITS "degrees_east"
#define SSC_INDEX_VAR_ATT_NAME "variables"
#define SSC_NUM_GRING 4
#define SSC_MOD05 "mod05"
#define SSC_TITLE_NAME "title"
#define SSC_TITLE "SpatioTemporal Adaptive Resolution Encoding (STARE) sidecar file"
#define SSC_MAX_NAME 256

#define SSC_NDIM1 1
#define SSC_NDIM2 2
#define NDIM2 2

#define SSC_NOT_SIDECAR (-1001)
#define MAX_NUM_INDEX 10 /**< Max number of STARE index vars in a file. */

/**
 * This is the base class for a data file with geolocation.
 */
class GeoFile
{
private:
    int d_ncid; ///< id of the open netCDF4 file
    int d_num_index; ///< Number of STARE indices sets needed for this file.
    std::string d_data_file_name;

    std::vector<std::string> d_stare_index_name;
    std::vector<std::string> stare_cover_name;
    std::vector<std::string> d_variables; ///< Names of vars that use this index.
    std::vector<size_t> d_size_i, d_size_j;
    std::vector<int> d_stare_varid; ///< Use this varid to read the index values

    // int *geo_num_i1; /**< Number of I. */
    // int *geo_num_j1; /**< Number of J. */
    // double **geo_lat1; /**< Array of latitude values. */
    // double **geo_lon1; /**< Array of longitude values. */
    // unsigned long long **geo_index1; /**< Array of STARE index. */

    // TODO These may be used by the STAREmaster library or createSidecarFile.
    //  jhrg 6/17/21
//    int num_cover;
//    unsigned long long **geo_cover1;
//    int *geo_num_cover_values1;
//    STARE_SpatialIntervals cover;
//
//    int cover_level;
//    int perimeter_stride;

protected:
    std::string sanitize_pathname(string path) const;

    /** Get STARE index sidecar filename. */
    std::string sidecar_filename(const std::string &file_name) const;

    int read_sidecar_file(const std::string &file_name);

public:
    GeoFile() : d_ncid(-1), d_num_index(0) {};

    /**
     * @brief Open and read the sidecar file for a given data file
     * @param data_file_name
     */
    explicit GeoFile(const std::string &data_file_name)  : d_ncid(-1), d_num_index(0), d_data_file_name(data_file_name) {
        // load much of the info and set d_ncid
        int ret = read_sidecar_file(sidecar_filename(data_file_name));
        if (ret != NC_NOERR)
            throw BESInternalError("Could not open file " + sanitize_pathname(data_file_name)
                                    + " - " + nc_strerror(ret), __FILE__, __LINE__);
    };

    virtual ~GeoFile() { close_sidecar_file(); };

    // The data types for STARESTARE_ArrayIndexSpatialValue
    // STARE_SpatialIntervals (which is std::vector<STARE_ArrayIndexSpatialValue>
    void get_stare_indices(const std::string &var_name, std::vector<STARE_ArrayIndexSpatialValue> &values);

    size_t get_variable_rows(std::string variable_name) const;
    size_t get_variable_cols(std::string variable_name) const;

    void close_sidecar_file();
};

#endif /* GEO_FILE_H_ */
