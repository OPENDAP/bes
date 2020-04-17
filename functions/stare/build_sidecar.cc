/*********************************************************************
 * build_sidecar.cc													*
 *																	*
 *  Created on: Mar 7, 2019											*
 *  																*
 *  	Purpose: Index the STARE value that corresponds to a given	*
 *  		 iterator from an array, as well as its lat/lon value.	*
 *  			 												    *
 *      Author: Kodi Neumiller, kneumiller@opendap.org				*
 *      															*
 ********************************************************************/

#include <unistd.h>
#include <string>
#include <memory>
#include <algorithm>
#include <cassert>
#include <chrono>

#include <hdf5.h>

#include "STARE.h"

#include <D4Connect.h>
#include <Connect.h>
#include <Response.h>
#include <Array.h>
#include <Error.h>

#include <BESSyntaxUserError.h>

using namespace std;

static bool verbose = false;
#define VERBOSE(x) do { if (verbose) x; } while(false)
static bool very_verbose = false;
#define VERY_VERBOSE(x) do { if (very_verbose) x; } while(false)

/**
 * @brief Store the lon,lat values that match a specific STARE index
 * @note I reordered these so that x,y and lon,lat will more obviously line up.
 */
struct coord {
    unsigned long x;  /// The x index in the lon and lat arrays
    unsigned long y;  /// The y index for the lon and lat arrays

    float64 lon;    /// The Longitude values
    float64 lat;    /// The Latitude values

    // The STARE library's name for the type is STARE_ArrayIndexSpatialValue
    uint64 s_index; /// The corresponding level 27 STARE index
};

/**
 * @brief This structure holds data for a collection of latitude and longitude values
 * The benefit of this structure is that the lat and lon values don't have to
 * be copied as much as with the 'coord' structure since HDF5 will read/write
 * all of the arrays at once.
 *
 * To access a set of related values, use the same index for each vector.
 *
 * To load the structure with lat/lon values, use the ctor or pass the addresses
 * of the lat and lon vectors to reader code (eliminating one copy operation).
 */
struct coordinates {
    vector<hsize_t> dims;     /// The x, y dimensions of lon and lat

    vector<unsigned long> x;  /// The x index in the lon and lat arrays
    vector<unsigned long> y;  /// The y index for the lon and lat arrays

    vector<float64> lon;    /// The Longitude values
    vector<float64> lat;    /// The Latitude values

    STARE_ArrayIndexSpatialValues s_index; /// The corresponding level 27 STARE index

    coordinates() {}

    /**
     * Simple ctor used with lat and lon have already been read.
     * @param latitude
     * @param longitude
     */
    coordinates(vector<float64>latitude, vector<float64>longitude) {
        assert(latitude.size() == longitude.size());
        set_size(latitude.size());

        copy(latitude.begin(), latitude.end(), lat.begin());
        copy(longitude.begin(), longitude.end(), lon.begin());
    }

    /**
     * Set the size for all the vectors so that other code can write to them directly
     * @param size The number of values the vectors will hold
     */
    void set_size(size_t size) {
        x.resize(size);
        y.resize(size);
        lon.resize(size);
        lat.resize(size);
        s_index.resize(size);
    }

    /**
     * @defgroup Get addresses for the vector data
     * These provide a way to avoid tortured expression syntax to get the
     * addresses of the memory blocks for these vectors.
     * @{
     */
    hsize_t *get_dims() { return &dims[0]; }

    unsigned long *get_x() { return &x[0]; }
    unsigned long *get_y() { return &y[0]; }

    float64 *get_lon() { return &lon[0]; }
    float64 *get_lat() { return &lat[0]; }

    STARE_ArrayIndexSpatialValue *get_s_index() { return &s_index[0]; }
    /** @} */
};

/**
 * @brief Returns a an array of coords where each entry holds the x and y values
 *  that correlate to the lat and lon values generated from the url
 * @param url
 * @return coords
 * @todo refactor - break apart the code that reads the lat/lon and the code that
 * computes the s-index
 */
vector<coord> readUrl(const string &dataUrl, const string &latName, const string &lonName) {
    unique_ptr<libdap::Connect> url(new libdap::Connect(dataUrl));

    string latlonName = latName + "," + lonName;

    std::vector<float> lat;
    std::vector<float> lon;

    vector<coord> indexArray;

    try {
        libdap::BaseTypeFactory factory;
        libdap::DataDDS dds(&factory);

        VERBOSE(cerr << "\n\n\tRequesting data from " << dataUrl << endl);

        //Makes sure only the Latitude and Longitude variables are requested
        url->request_data(dds, latlonName);

        //Create separate libdap arrays to store the lat and lon arrays individually
        libdap::Array *urlLatArray = dynamic_cast<libdap::Array *>(dds.var(latName));
        libdap::Array *urlLonArray = dynamic_cast<libdap::Array *>(dds.var(lonName));

        // ----Error checking---- //
        if (urlLatArray == 0 || urlLonArray == 0) {
            throw libdap::Error("Expected both lat and lon arrays");
        }

        unsigned int dims = urlLatArray->dimensions();

        if (dims != 2) {
            throw libdap::Error("Incorrect latitude dimensions");
        }

        if (dims != urlLonArray->dimensions()) {
            throw libdap::Error("Incorrect longitude dimensions");
        }

        int size_y = urlLatArray->dimension_size(urlLatArray->dim_begin());
        int size_x = urlLatArray->dimension_size(urlLatArray->dim_begin() + 1);

        if (size_y != urlLonArray->dimension_size(urlLonArray->dim_begin())
            || size_x != urlLonArray->dimension_size(urlLonArray->dim_begin() + 1)) {
            throw libdap::Error("The size of the latitude and longitude arrays are not the same");
        }

        //Initialize the arrays with the correct length for lat and lon
        lat.resize(urlLatArray->length());
        urlLatArray->value(&lat[0]);
        lon.resize(urlLonArray->length());
        urlLonArray->value(&lon[0]);

        VERBOSE(cerr << "\tsize of lat array: " << lat.size() << endl);
        VERBOSE(cerr << "\tsize of lon array: " << lon.size() << endl);

        coord indexVals = coord();

        //Declare the beginning of the vectors here rather than inside the loop to save compute time
        //vector<float>::iterator i_begin = lat.begin();		FIXME: Probably not needed since we just use a single dimension array
        vector<float>::iterator j_begin = lon.begin();

        for (vector<float>::iterator i = lat.begin(), e = lat.end(), j =
                lon.begin(); i != e; ++i, ++j) {
            //Use an offset since we are using a 1D array and treating it like a 2D array
            int offset = j - j_begin;
            indexVals.x = offset / (size_x - 1);//Get the current index for the lon
            indexVals.y = offset % (size_x - 1);//Get the current index for the lat
            indexVals.lat = *i;
            indexVals.lon = *j;

            indexArray.push_back(indexVals);
        }

    }
    catch (libdap::Error &e) {
        cerr << "ERROR: " << e.get_error_message() << endl;
    }

    return indexArray;
}

/**
 * @brief Read the latitude and longitude vectors from a level 1B/2 HDF5 file
 * @param filename Read from this file
 * @param lat_name The name of the 2D array that holds latitude values
 * @param lon_name The name of the 2D array that holds longitude values
 * @param lat Value-result parameter for the latitude data
 * @param lon Value-result parameter for the longtitude data
 * @return A vector<hsize_t> that holds the size of the y and x dimensions
 * of the lat and lon value-result parameters
 */
vector<hsize_t> read_lat_lon(const string &filename, const string &lat_name, const string &lon_name,
        vector<float64> &lat, vector<float64> &lon) {

    //Read the file and store the datasets
    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0)
        throw BESSyntaxUserError(string("Could not open the file '").append(filename).append("'"), __FILE__, __LINE__);

    hid_t latDataset = H5Dopen(file, lat_name.c_str(), H5P_DEFAULT);
    if (latDataset < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 dataset '").append(lat_name).append("'"), __FILE__,
                                 __LINE__);

    hid_t lonDataset = H5Dopen(file, lon_name.c_str(), H5P_DEFAULT);
    if (lonDataset < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 dataset '").append(lon_name).append("'"), __FILE__,
                                 __LINE__);

    //Get the number of dimensions
    //Should be 2, but this is future proofing just in case,
    //that way I don't have to go back and figure it all out again kln 10/17/19
    hid_t dspace = H5Dget_space(latDataset);
    if (dspace < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 data space for '").append(lat_name).append("'"),
                                 __FILE__, __LINE__);

    const int ndims = H5Sget_simple_extent_ndims(dspace);
    if (ndims != 2)
        throw BESSyntaxUserError(string("The latitude variable '").append(lat_name).append("' should be a 2D array"),
                                 __FILE__, __LINE__);

    vector<hsize_t> dims(ndims);

    //Get the size of the dimensions so that we know how big to make the memory space
    //dims holds the size of the ndims dimensions.
    H5Sget_simple_extent_dims(dspace, &dims[0], NULL);

    //We need to get the filespace and memspace before reading the values from each dataset
    hid_t latFilespace = H5Dget_space(latDataset);
    if (latFilespace < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 file space for '").append(lat_name).append("'"),
                                 __FILE__, __LINE__);

    hid_t lonFilespace = H5Dget_space(lonDataset);
    if (lonFilespace < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 file space for '").append(lon_name).append("'"),
                                 __FILE__, __LINE__);

    //The filespace will tell us what the size of the vectors need to be for reading in the
    // data from the h5 file. We could also use memspace, but they should be the same size.
    hssize_t latSize = H5Sget_select_npoints(latFilespace);
    VERBOSE(cerr << "\n\tlat dataspace size: " << latSize);
    hssize_t lonSize = H5Sget_select_npoints(lonFilespace);
    VERBOSE(cerr << "\n\tlon dataspace size: " << lonSize << endl);

    if (latSize != lonSize)
        throw BESSyntaxUserError(
                string("The size of the Latitude and Longitude arrays must be equal in '").append(filename).append("'"),
                __FILE__, __LINE__);

    lat.resize(latSize);
    lon.resize(lonSize);

    hid_t memspace = H5Screate_simple(ndims, &dims[0], NULL);
    if (memspace < 0)
        throw BESSyntaxUserError(
                string("Could not make an HDF5 memory space while working with '").append(filename).append("'"),
                __FILE__, __LINE__);

    //Read the data file and store the values of each dataset into an array
    // was H5T_NATIVE_FLOAT. jhrg 4/17/20
    herr_t status = H5Dread(latDataset, H5T_NATIVE_DOUBLE, memspace, latFilespace, H5P_DEFAULT, &lat[0]);
    if (status < 0)
        throw BESSyntaxUserError(string("Could not read data for '").append(lat_name).append("'"), __FILE__, __LINE__);


    status = H5Dread(lonDataset, H5T_NATIVE_DOUBLE, memspace, lonFilespace, H5P_DEFAULT, &lon[0]);
    if (status < 0)
        throw BESSyntaxUserError(string("Could not read data for '").append(lon_name).append("'"), __FILE__, __LINE__);

    VERBOSE(cerr << "\tsize of lat array: " << lat.size() << endl);
    VERBOSE(cerr << "\tsize of lon array: " << lon.size() << endl);

    return dims;
}

/**
 * @brief Read the latitude and longitude vectors from a level 1B/2 HDF5 file
 * @param filename Read from this file
 * @param lat_name The name of the 2D array that holds latitude values
 * @param lon_name The name of the 2D array that holds longitude values
 * @param c Put the Latitude, Longitude and dims in this value-result parameter
 */
void read_lat_lon(const string &filename, const string &lat_name, const string &lon_name, coordinates *c) {

    //Read the file and store the datasets
    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0)
        throw BESSyntaxUserError(string("Could not open the file '").append(filename).append("'"), __FILE__, __LINE__);

    hid_t latDataset = H5Dopen(file, lat_name.c_str(), H5P_DEFAULT);
    if (latDataset < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 dataset '").append(lat_name).append("'"), __FILE__,
                                 __LINE__);

    hid_t lonDataset = H5Dopen(file, lon_name.c_str(), H5P_DEFAULT);
    if (lonDataset < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 dataset '").append(lon_name).append("'"), __FILE__,
                                 __LINE__);

    //Get the number of dimensions
    //Should be 2, but this is future proofing just in case,
    //that way I don't have to go back and figure it all out again kln 10/17/19
    hid_t dspace = H5Dget_space(latDataset);
    if (dspace < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 data space for '").append(lat_name).append("'"),
                                 __FILE__, __LINE__);

    const int ndims = H5Sget_simple_extent_ndims(dspace);
    if (ndims != 2)
        throw BESSyntaxUserError(string("The latitude variable '").append(lat_name).append("' should be a 2D array"),
                                 __FILE__, __LINE__);

    c->dims.resize(ndims);

    //Get the size of the dimensions so that we know how big to make the memory space
    //dims holds the size of the ndims dimensions.
    H5Sget_simple_extent_dims(dspace, c->get_dims(), NULL);

    //We need to get the filespace and memspace before reading the values from each dataset
    hid_t latFilespace = H5Dget_space(latDataset);
    if (latFilespace < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 file space for '").append(lat_name).append("'"),
                                 __FILE__, __LINE__);

    hid_t lonFilespace = H5Dget_space(lonDataset);
    if (lonFilespace < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 file space for '").append(lon_name).append("'"),
                                 __FILE__, __LINE__);

    //The filespace will tell us what the size of the vectors need to be for reading in the
    // data from the h5 file. We could also use memspace, but they should be the same size.
    hssize_t latSize = H5Sget_select_npoints(latFilespace);
    VERBOSE(cerr << "\n\tlat dataspace size: " << latSize);
    hssize_t lonSize = H5Sget_select_npoints(lonFilespace);
    VERBOSE(cerr << "\n\tlon dataspace size: " << lonSize << endl);

    if (latSize != lonSize)
        throw BESSyntaxUserError(
                string("The size of the Latitude and Longitude arrays must be equal in '").append(filename).append("'"),
                __FILE__, __LINE__);

    c->set_size(latSize);

    hid_t memspace = H5Screate_simple(ndims, c->get_dims(), NULL);
    if (memspace < 0)
        throw BESSyntaxUserError(
                string("Could not make an HDF5 memory space while working with '").append(filename).append("'"),
                __FILE__, __LINE__);

    //Read the data file and store the values of each dataset into an array
    // was H5T_NATIVE_FLOAT. jhrg 4/17/20
    herr_t status = H5Dread(latDataset, H5T_NATIVE_DOUBLE, memspace, latFilespace, H5P_DEFAULT, c->get_lat());
    if (status < 0)
        throw BESSyntaxUserError(string("Could not read data for '").append(lat_name).append("'"), __FILE__, __LINE__);


    status = H5Dread(lonDataset, H5T_NATIVE_DOUBLE, memspace, lonFilespace, H5P_DEFAULT, c->get_lon());
    if (status < 0)
        throw BESSyntaxUserError(string("Could not read data for '").append(lon_name).append("'"), __FILE__, __LINE__);

    VERBOSE(cerr << "\tsize of lat array: " << c->lat.size() << endl);
    VERBOSE(cerr << "\tsize of lon array: " << c->lon.size() << endl);
}

/**
 * @brief Build the STARE index information given latitude and longitude data
 * @param stare
 * @param dims
 * @param latitude
 * @param longitude
 * @return The STARE index information
 */
unique_ptr< vector<coord> > build_coords(STARE &stare, const vector<hsize_t> &dims,
                                         const vector<float64> &latitude, const vector<float64> &longitude) {

    unique_ptr< vector<coord> > coords(new vector<coord>(latitude.size()));

    auto lat = latitude.begin();
    auto lon = longitude.begin();
    //Assume data are stored in row-major order; dims[0] is the row, dims[1] is the column
    unsigned long long n = 0;
    for (unsigned long r = 0; r < dims[0]; ++r) {
        for (unsigned long c = 0; c < dims[1]; ++c) {
            (*coords)[n].x = c;
            (*coords)[n].y = r;

            (*coords)[n].lat = *lat;
            (*coords)[n].lon = *lon;

            (*coords)[n].s_index = stare.ValueFromLatLonDegrees(*lat, *lon);

            VERY_VERBOSE(cerr << "Coord: " << *lat << ", " << *lon << " -> " << hex << (*coords)[n].s_index << dec << endl);

            ++n;
            ++lat;
            ++lon;
        }
    }

    return coords;
}

/**
 * @brief Build the STARE index information given latitude and longitude data
 * @param stare
 * @param dims
 * @param latitude
 * @param longitude
 * @return The STARE index information
 */
unique_ptr<coordinates>
build_coordinates(STARE &stare, const vector<hsize_t> &dims,
        const vector<float64> &latitude, const vector<float64> &longitude) {

    // This ctor sets the size of the vectors in the 'coordinates' struct.
    unique_ptr<coordinates> c(new coordinates(latitude, longitude));

    //Assume data are stored in row-major order; dims[0] is the row, dims[1] is the column
    unsigned long n = 0;
    for (unsigned long row = 0; row < dims[0]; ++row) {
        for (unsigned long col = 0; col < dims[1]; ++col) {
            c->x[n] = col;
            c->y[n] = row;

            c->s_index[n] = stare.ValueFromLatLonDegrees(c->lat[n], c->lon[n]);

            VERY_VERBOSE(cerr << "Coord: " << c->lat[n] << ", " << c->lon[n] << " -> " << hex << c->s_index[n] << dec << endl);

            ++n;
        }
    }

    return c;
}

/**
 * @brief Build the STARE indices given coordinates that include latitude and longitude data
 * @param stare
 * @param dims
 * @param latitude
 * @param longitude
 * @return The STARE index information
 */
void
compute_coordinates(STARE &stare, coordinates *c) {

    //Assume data are stored in row-major order; dims[0] is the row, dims[1] is the column
    unsigned long n = 0;
    for (unsigned long row = 0; row < c->dims[0]; ++row) {
        for (unsigned long col = 0; col < c->dims[1]; ++col) {
            c->x[n] = col;
            c->y[n] = row;

            c->s_index[n] = stare.ValueFromLatLonDegrees(c->lat[n], c->lon[n]);

            VERY_VERBOSE(cerr << "Coord: " << c->lat[n] << ", " << c->lon[n] << " -> " << hex << c->s_index[n] << dec << endl);

            ++n;
        }
    }
}

/**
 * @brief Write the STARE index information to an HDF5 file.
 *
 * @todo Add error checking to this function's HDF5 API calls.
 * @param filename
 * @param tmpStorage
 * @param coords
 */
void writeHDF5(const string &filename, string tmpStorage, vector<coord> *coords) {
    //Allows us to use hdf5 1.10 generated files with hdf5 1.8
    // !---Must be removed if a feature from 1.10 is required to use---!
    // -kln 5/16/19
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
#ifdef H5F_LIBVER_V18
    H5Pset_libver_bounds (fapl, H5F_LIBVER_EARLIEST, H5F_LIBVER_V18);
#else
    H5Pset_libver_bounds(fapl, H5F_LIBVER_EARLIEST, H5F_LIBVER_LATEST);
#endif

    //H5Fcreate returns a file id that will be saved in variable "file"
    hid_t file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);

    //The rank is used to determine the dimensions for the dataspace
    //	Since we only use a one dimension array we can always assume the Rank should be 1
    hsize_t coords_size = coords->size();
    hid_t dataspace = H5Screate_simple(1 /*RANK*/, &coords_size, NULL);

#if 0
    //Taken out because CF doesn't support compound types,
    //	so each variable will need to be stored in its own array
    // -kln 5/17/19

    /*
     * Create the memory datatype.
     *  Because the "coords" struct has ints and floats we need to make sure we put
     *  the correct offset onto memory for each data type
     */
    VERBOSE(cerr << "\nCreating datatypes: x, y, lat, lon, stareIndex -> ");

    dataTypes = H5Tcreate (H5T_COMPOUND, sizeof(coords));
    H5Tinsert(dataTypes, "x", HOFFSET(coords, x), H5T_NATIVE_INT);
    H5Tinsert(dataTypes, "y", HOFFSET(coords, y), H5T_NATIVE_INT);
    H5Tinsert(dataTypes, "lat", HOFFSET(coords, lat), H5T_NATIVE_FLOAT);
    H5Tinsert(dataTypes, "lon", HOFFSET(coords, lon), H5T_NATIVE_FLOAT);
    H5Tinsert(dataTypes, "stareIndex", HOFFSET(coords, stareIndex), H5T_NATIVE_INT);

    /*
     * Create the dataset
     */
    const char *datasetName = "StareData";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    dataset = H5Dcreate2(file, datasetName, dataTypes, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    VERBOSE(cerr << "Writing data to dataset" << endl);
    H5Dwrite(dataset, dataTypes, H5S_ALL, H5S_ALL, H5P_DEFAULT, &keyVals[0]);
#endif

    /*
     * Create the datasets
     */
    const char *datasetName = "X";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    hid_t datasetX = H5Dcreate2(file, datasetName, H5T_NATIVE_INT, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    datasetName = "Y";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    hid_t datasetY = H5Dcreate2(file, datasetName, H5T_NATIVE_INT, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    datasetName = "Latitude";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    // was H5T_NATIVE_FLOAT. jhrg 4/17/20
    hid_t datasetLat = H5Dcreate2(file, datasetName, H5T_NATIVE_DOUBLE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    datasetName = "Longitude";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    hid_t datasetLon = H5Dcreate2(file, datasetName, H5T_NATIVE_DOUBLE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    datasetName = "Stare Index";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    hid_t datasetStare = H5Dcreate2(file, datasetName, H5T_NATIVE_INT64, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write the data to the datasets
     */
    //Need to store each value in its own array
    vector<unsigned long> xArray;
    vector<unsigned long> yArray;
    vector<float64> latArray;
    vector<float64> lonArray;
    vector<uint64> s_indices;

    for (vector<coord>::iterator i = coords->begin(), e = coords->end(); i != e; ++i) {
        xArray.push_back(i->x);
        yArray.push_back(i->y);
        latArray.push_back(i->lat);
        lonArray.push_back(i->lon);
        s_indices.push_back(i->s_index);
    }


    VERBOSE(cerr << "Writing data to dataset" << endl);
    H5Dwrite(datasetX, H5T_NATIVE_ULONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &xArray[0]);
    H5Dwrite(datasetY, H5T_NATIVE_ULONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &yArray[0]);
    H5Dwrite(datasetLat, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &latArray[0]);
    H5Dwrite(datasetLon, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &lonArray[0]);
    H5Dwrite(datasetStare, H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &s_indices[0]);

    /*
     * Close/release resources.
     */
    H5Sclose(dataspace);
    H5Fclose(file);

    VERBOSE(cerr << "\nData written to file: " << filename << endl);

    //Store the sidecar files in /tmp/ (or the provided directory) so that it can easily be found for the
    // server functions.
    if (tmpStorage.empty())
        tmpStorage = "/tmp/" + filename;
    else
        tmpStorage = tmpStorage + filename;

    rename(filename.c_str(), tmpStorage.c_str());
    VERBOSE(cerr << "Data moved to: " << tmpStorage << endl);
}

/**
 * @brief Write the STARE index information to an HDF5 file.
 *
 * @todo Add error checking to this function's HDF5 API calls.
 * @param filename
 * @param tmpStorage
 * @param coords
 */
void writeHDF5(const string &filename, string tmpStorage, coordinates *c) {
    //Allows us to use hdf5 1.10 generated files with hdf5 1.8
    // !---Must be removed if a feature from 1.10 is required to use---!
    // -kln 5/16/19
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
#ifdef H5F_LIBVER_V18
    H5Pset_libver_bounds (fapl, H5F_LIBVER_EARLIEST, H5F_LIBVER_V18);
#else
    H5Pset_libver_bounds(fapl, H5F_LIBVER_EARLIEST, H5F_LIBVER_LATEST);
#endif

    //H5Fcreate returns a file id that will be saved in variable "file"
    hid_t file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);

    //The rank is used to determine the dimensions for the dataspace
    //	Since we only use a one dimension array we can always assume the Rank should be 1
    hsize_t coords_size = c->x.size();
    hid_t dataspace = H5Screate_simple(1 /*RANK*/, &coords_size, NULL);

#if 0
    //Taken out because CF doesn't support compound types,
    //	so each variable will need to be stored in its own array
    // -kln 5/17/19

    /*
     * Create the memory datatype.
     *  Because the "coords" struct has ints and floats we need to make sure we put
     *  the correct offset onto memory for each data type
     */
    VERBOSE(cerr << "\nCreating datatypes: x, y, lat, lon, stareIndex -> ");

    dataTypes = H5Tcreate (H5T_COMPOUND, sizeof(coords));
    H5Tinsert(dataTypes, "x", HOFFSET(coords, x), H5T_NATIVE_INT);
    H5Tinsert(dataTypes, "y", HOFFSET(coords, y), H5T_NATIVE_INT);
    H5Tinsert(dataTypes, "lat", HOFFSET(coords, lat), H5T_NATIVE_FLOAT);
    H5Tinsert(dataTypes, "lon", HOFFSET(coords, lon), H5T_NATIVE_FLOAT);
    H5Tinsert(dataTypes, "stareIndex", HOFFSET(coords, stareIndex), H5T_NATIVE_INT);

    /*
     * Create the dataset
     */
    const char *datasetName = "StareData";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    dataset = H5Dcreate2(file, datasetName, dataTypes, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    VERBOSE(cerr << "Writing data to dataset" << endl);
    H5Dwrite(dataset, dataTypes, H5S_ALL, H5S_ALL, H5P_DEFAULT, &keyVals[0]);
#endif

    /*
     * Create the datasets
     */
    const char *datasetName = "X";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    hid_t datasetX = H5Dcreate2(file, datasetName, H5T_NATIVE_INT, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    datasetName = "Y";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    hid_t datasetY = H5Dcreate2(file, datasetName, H5T_NATIVE_INT, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    datasetName = "Latitude";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    // was H5T_NATIVE_FLOAT. jhrg 4/17/20
    hid_t datasetLat = H5Dcreate2(file, datasetName, H5T_NATIVE_DOUBLE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    datasetName = "Longitude";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    hid_t datasetLon = H5Dcreate2(file, datasetName, H5T_NATIVE_DOUBLE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    datasetName = "Stare Index";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    hid_t datasetStare = H5Dcreate2(file, datasetName, H5T_NATIVE_INT64, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write the data to the datasets
     */

    VERBOSE(cerr << "Writing data to dataset" << endl);
    H5Dwrite(datasetX, H5T_NATIVE_ULONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(c->x[0]));
    H5Dwrite(datasetY, H5T_NATIVE_ULONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(c->y[0]));
    H5Dwrite(datasetLat, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(c->lat[0]));
    H5Dwrite(datasetLon, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(c->lon[0]));
    H5Dwrite(datasetStare, H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &(c->s_index[0]));

    /*
     * Close/release resources.
     */
    H5Sclose(dataspace);
    H5Fclose(file);

    VERBOSE(cerr << "\nData written to file: " << filename << endl);

    //Store the sidecar files in /tmp/ (or the provided directory) so that it can easily be found for the
    // server functions.
    if (tmpStorage.empty())
        tmpStorage = "/tmp/" + filename;
    else
        tmpStorage = tmpStorage + filename;
    rename(filename.c_str(), tmpStorage.c_str());
    VERBOSE(cerr << "Data moved to: " << tmpStorage << endl);
}

static void usage() {
    cerr << "build_sidecar [options] <filename> <latitude-name> <longitude-name>" << endl;
    cerr << "-o output file: \tOutput the STARE data to the given output file" << endl;
    cerr << "-v|V verbose/very verbose" << endl;
    cerr << "-t transfer location: \tTransfer the generated sidecar file to the given directory" << endl;
    cerr << "-b STARE Build Level: \tHigher levels -> longer initialization time. (default is 5)" << endl;
    cerr << "-s STARE default Level: \tHigher levels -> finer resolution. (default is 27)" << endl;
    cerr << "-a Algotithm: \t1, 2 or 3 (default is 3)" << endl;
}

static string
get_sidecar_filename(const string &dataUrl, const string &suffix = "_sidecar.h5") {
    // Assume the granule is called .../path/file.ext where both the
    // slashes and dots might actually not be there. Assume also that
    // the data are in an HDF5 file. jhrg 1/15/20

    // Locate the granule name inside the provided url.
    // Once the granule is found, add ".h5" to the granule name
    // Rename the new H5 file to be <granulename.h5>
    size_t granulePos = dataUrl.find_last_of('/');
    string granuleName = dataUrl.substr(granulePos + 1);
    size_t findDot = granuleName.find_last_of('.');
    return granuleName.substr(0, findDot).append(suffix);
}

/**
 * -h help
 * -o <output file>
 * -v verbose
 * -b <int> STARE library build level (default is 5, very fast library initialization time)
 * -s <int> STARE index max level (default is 27, the highest possible)
 * -a <int>
 *
 *  build_sidecar [options] [file|DAP_URL] latitude_var_name longitude_var_name level
 */
int main(int argc, char *argv[]) {
    int c;
    extern char *optarg;
    extern int optind;

    string newName = "";
    string tmpStorage = "./"; // Default is the CWD.
    float build_level = 5.0;  // The default build level, fast start time, longer index lookup.
    float level = 27.0;
    int alg = 3;

    while ((c = getopt(argc, argv, "hvVo:t:b:s:a:")) != -1) {
        switch (c) {
            case 'o':
                newName = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            case 'V':
                verbose = true;
                very_verbose = true;
                break;
            case 't':
                tmpStorage = optarg;
                break;
            case 'b':
                build_level = atof(optarg);
                break;
            case 's':
                level = atof(optarg);
                break;
            case 'a':
                alg = atoi(optarg);
                break;
            case 'h':
            default:
                usage();
                exit(EXIT_SUCCESS);
                break;
        }
    }

    // This resets argc and argv once the optional arguments have been processed. jhrg 12/5/19
    argc -= optind;
    argv += optind;

    if (argc != 3) {
        cerr << "Expected 3 required arguments" << endl;
        usage();
        exit(EXIT_FAILURE);
    }

    //Required argument values
    string dataUrl = argv[0];
    string latName = argv[1];
    string lonName = argv[2];

    if (newName.empty())
        newName = get_sidecar_filename(dataUrl);

    try {
        STARE stare(level, build_level);
        vector<float64> lat;
        vector<float64> lon;
        vector<hsize_t> dims;

#if 0
        if (dataUrl.find("https://") != string::npos
            || dataUrl.find("http://") != string::npos
            || dataUrl.find("www.") != string::npos) {
            // FIXME Match logic of the local file function
            coords = readUrl(dataUrl, latName, lonName);
        }
        else {
             dims = read_lat_lon(dataUrl, latName, lonName, lat, lon);
        }
#endif
        using namespace std::chrono;
        auto start = high_resolution_clock::now();

        switch (alg) {
            case 1: {
                dims = read_lat_lon(dataUrl, latName, lonName, lat, lon);
                unique_ptr<vector<coord> > coords = build_coords(stare, dims, lat, lon);
                writeHDF5(newName, tmpStorage, coords.get());
                break;
            }

            case 2: {
                dims = read_lat_lon(dataUrl, latName, lonName, lat, lon);
                unique_ptr<coordinates> c = build_coordinates(stare, dims, lat, lon);
                writeHDF5(newName, tmpStorage, c.get());
                break;
            }

            default:
            case 3: {
                unique_ptr<coordinates> c(new coordinates());
                read_lat_lon(dataUrl, latName, lonName, c.get());
                compute_coordinates(stare, c.get());
                writeHDF5(newName, tmpStorage, c.get());
                break;
            }
        }

        auto total = duration_cast<milliseconds>(high_resolution_clock::now()-start).count();
        VERBOSE(cerr << "Time for the algorithm " << alg << ": " << total << "ms." << endl);
    }
    catch (libdap::Error &e) {
        cerr << "Error: " << e.get_error_message() << endl;
        exit(EXIT_FAILURE);
    }
    catch (BESError &e) {
        cerr << "Error: " << e.get_message() << endl;
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
