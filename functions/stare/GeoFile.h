/// @file

/// This class contains the main functionality for reading a file that
/// may be given a STARE sidecar file.

/// Ed Hartnett 4/4/20

#ifndef GEO_FILE_H_ /**< Protect file from double include. */
#define GEO_FILE_H_

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
//#include "ssc.h"
#include "STARE.h"
using namespace std;

#define SSC_LAT_NAME "Latitude"
#define SSC_LON_NAME "Longitude"
#define SSC_I_NAME "i"
#define SSC_J_NAME "j"
#define SSC_K_NAME "k"
#define SSC_L_NAME "l"
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
public:
    GeoFile();
    ~GeoFile();
    
    /** Read file. */
    int readFile(const string fileName, int verbose, int quiet, int build_level);

    /** Get STARE index sidecar filename. */
    string sidecarFileName(const string fileName);

    int readSidecarFile_int(const std::string fileName, int verbose, int &num_index,
			    vector<string> &stare_index_name, vector<size_t> &size_i,
			    vector<size_t> &size_j, vector<string> &variables,
			    vector<int> &stare_varid, int &ncid);
    int readSidecarFile(const std::string fileName, int verbose, int &ncid);

    /** Get STARE index for data varaible. */
    int getSTAREIndex(const std::string varName, int verbose, int ncid, int &varid,
		      size_t &my_size_i, size_t &my_size_j);
    int getSTAREIndex_2(const std::string varName, int verbose, int ncid,
			vector<unsigned long long> &values);

    /** Close sidecar file. */
    int closeSidecarFile(int verbose, int ncid);

    int num_index; /**< Number of STARE indicies needed for this file. */
    int *geo_num_i1; /**< Number of I. */
    int *geo_num_j1; /**< Number of J. */
    double **geo_lat1; /**< Array of latitude values. */
    double **geo_lon1; /**< Array of longitude values. */
    unsigned long long **geo_index1; /**< Array of STARE index. */

    int num_cover;
    unsigned long long **geo_cover1; /**< Array of STARE index intervals. */
    int *geo_num_cover_values1;
    vector<string> var_name[MAX_NUM_INDEX]; /**< Names of vars that use this index. */
    STARE_SpatialIntervals cover;

    int cover_level;
    int perimeter_stride;

    vector<string> stare_index_name;
    vector<string> stare_cover_name;
    vector<string> variables;

    vector<size_t> size_i, size_j;
    vector<int> stare_varid;
};

#endif /* GEO_FILE_H_ */
