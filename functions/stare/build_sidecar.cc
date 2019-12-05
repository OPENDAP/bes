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
#include <unordered_map>
#include <string>
#include <memory>

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

//Struct to store the iterator and lat/lon values that match a specific STARE index
struct coords {
    int x;
    int y;

    float64 lat;
    float64 lon;

    uint64 stareIndex;
};

/**
 * @brief Returns a an array of coords where each entry holds the x and y values
 *  that correlate to the lat and lon values generated from the url
 * @param url
 * @return coords
 */
vector<coords> readUrl(string dataUrl, string latName, string lonName) {
    auto_ptr<libdap::Connect> url(new libdap::Connect(dataUrl));

    string latlonName = latName + "," + lonName;

    std::vector<float> lat;
    std::vector<float> lon;

    vector<coords> indexArray;

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
            || size_x
               != urlLonArray->dimension_size(
                urlLonArray->dim_begin() + 1)) {
            throw libdap::Error(
                    "The size of the latitude and longitude arrays are not the same");
        }

        //Initialize the arrays with the correct length for lat and lon
        lat.resize(urlLatArray->length());
        urlLatArray->value(&lat[0]);
        lon.resize(urlLonArray->length());
        urlLonArray->value(&lon[0]);

        VERBOSE(cerr << "\tsize of lat array: " << lat.size() << endl);
        VERBOSE(cerr << "\tsize of lon array: " << lon.size() << endl);

        coords indexVals = coords();

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

unique_ptr< vector<coords> > readLocal(const string &dataPath, const string &latName, const string &lonName) {
    //Read the file and store the datasets
    hid_t file = H5Fopen(dataPath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0)
        throw BESSyntaxUserError(string("Could not open the file '").append(dataPath).append("'"), __FILE__, __LINE__);

    hid_t latDataset = H5Dopen(file, latName.c_str(), H5P_DEFAULT);
    if (latDataset < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 dataset '").append(latName).append("'"), __FILE__,
                                 __LINE__);

    hid_t lonDataset = H5Dopen(file, lonName.c_str(), H5P_DEFAULT);
    if (lonDataset < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 dataset '").append(lonName).append("'"), __FILE__,
                                 __LINE__);

    //Get the number of dimensions
    //Should be 2, but this is future proofing just in case,
    //that way I don't have to go back and figure it all out again kln 10/17/19
    hid_t dspace = H5Dget_space(latDataset);
    if (dspace < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 data space for '").append(latName).append("'"),
                                 __FILE__, __LINE__);

    const int ndims = H5Sget_simple_extent_ndims(dspace);
    vector<hsize_t> dims(ndims);
    // hsize_t dims[ndims];

    //TODO: Add a comparison similar to the readUrl function that checks to make sure lat and lon are equal size

    //Get the size of the dimensions so that we know how big to make the memory space
    H5Sget_simple_extent_dims(dspace, &dims[0], NULL);

    //We need to get the filespace and memspace before reading the values from each dataset
    hid_t latFilespace = H5Dget_space(latDataset);
    if (latFilespace < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 file space for '").append(latName).append("'"),
                                 __FILE__, __LINE__);

    hid_t lonFilespace = H5Dget_space(lonDataset);
    if (lonFilespace < 0)
        throw BESSyntaxUserError(string("Could not open the HDF5 file space for '").append(lonName).append("'"),
                                 __FILE__, __LINE__);

    //The filespace will tell us what the size of the vectors need to be for reading in the
    // data from the h5 file. We could also use memspace, but they should be the same size.
    hssize_t latSize = H5Sget_select_npoints(latFilespace);
    VERBOSE(cerr << "\n\tlat dataspace size: " << latSize);
    hssize_t lonSize = H5Sget_select_npoints(lonFilespace);
    VERBOSE(cerr << "\n\tlon dataspace size: " << lonSize << endl);

    if (latSize != lonSize)
        throw BESSyntaxUserError(
                string("The size of the Latitude and Longitude arrays must be equal in '").append(dataPath).append("'"),
                __FILE__, __LINE__);

    //TODO: Should this be double?
    vector<float> latArray(latSize);
    vector<float> lonArray(lonSize);

    hid_t memspace = H5Screate_simple(ndims, &dims[0], NULL);
    if (memspace < 0)
        throw BESSyntaxUserError(
                string("Could not make an HDF5 memory space while working with '").append(dataPath).append("'"),
                __FILE__, __LINE__);

    //Read the data file and store the values of each dataset into an array
    herr_t status = H5Dread(latDataset, H5T_NATIVE_FLOAT, memspace, latFilespace, H5P_DEFAULT, &latArray[0]);
    if (status < 0)
        throw BESSyntaxUserError(string("Could not read data for '").append(latName).append("'"), __FILE__, __LINE__);

    status = H5Dread(lonDataset, H5T_NATIVE_FLOAT, memspace, lonFilespace, H5P_DEFAULT, &lonArray[0]);
    if (status < 0)
        throw BESSyntaxUserError(string("Could not read data for '").append(lonName).append("'"), __FILE__, __LINE__);

    VERBOSE(cerr << "\tsize of lat array: " << latArray.size() << endl);
    VERBOSE(cerr << "\tsize of lon array: " << lonArray.size() << endl);

    unique_ptr< vector<coords> > indexArray(new vector<coords>(latArray.size()));
    coords indexVals;

    int size_x = latArray.size();

    auto lat = latArray.begin();
    auto lon = lonArray.begin();
    for (int n = 0; n < size_x; ++n) {
        indexVals.x = n;
        indexVals.y = n;

        indexVals.lat = *lat++;
        indexVals.lon = *lon++;

        indexArray->push_back(indexVals);
    }

    return indexArray;
}

/**
 * @brief Use the coords struct to calculate the stare index based on the lat/lon values
 * @param latlonVals
 * @param level
 * @param buildlevel
 */
unique_ptr< vector<uint64> > calculateStareIndex(const float64 level, const float64 buildlevel, unique_ptr< vector<coords> > &latlonVals) {
    //Create an htmInterface that will be used to get the STARE index
    //htmInterface htm(level, buildlevel);
    //const SpatialIndex &index = htm.index();
    STARE index(level, buildlevel);

    unique_ptr< vector<uint64> > stareVals(new vector<uint64>(latlonVals->size()));
    uint64 stareIndex;

    for (vector<coords>::iterator i = latlonVals->begin(); i != latlonVals->end(); ++i) {
        stareIndex = index.ValueFromLatLonDegrees(i->lat, i->lon, level);

        VERBOSE(cerr << "Coord: " << i->lat << ", " << i->lon << " -> " << stareIndex << endl);

        stareVals->push_back(stareIndex);
    }

    return stareVals;
}

// TODO Add error checking to this function's HDF5 API calls.
/*************
 * HDF5 Stuff *
 *************/
void writeHDF5(const string &filename, string tmpStorage, unique_ptr< vector<coords> > &coordVals,
        unique_ptr< vector<uint64> > &stareVals) {
    hid_t file, datasetX, datasetY, datasetLat, datasetLon, datasetStare;
    hid_t dataspace; /* handle */

    //Need to store each value in its own array
    vector<int> xArray;
    vector<int> yArray;
    vector<float> latArray;
    vector<float> lonArray;

    for (vector<coords>::iterator i = coordVals->begin(); i != coordVals->end(); ++i) {
        xArray.push_back(i->x);
        yArray.push_back(i->y);
        latArray.push_back(i->lat);
        lonArray.push_back(i->lon);
    }

    //Used to store the the size of the array.
    // In other cases where the array is more than 1 dimension each dimension's length would be stored in this array.
    hsize_t arrayLength[1] = {xArray.size()};

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
    file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);

    //The rank is used to determine the dimensions for the dataspace
    //	Since we only use a one dimension array we can always assume the Rank should be 1
    dataspace = H5Screate_simple(1 /*RANK*/, arrayLength, NULL);

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
    datasetX = H5Dcreate2(file, datasetName, H5T_NATIVE_INT, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    datasetName = "Y";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    datasetY = H5Dcreate2(file, datasetName, H5T_NATIVE_INT, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    datasetName = "Latitude";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    datasetLat = H5Dcreate2(file, datasetName, H5T_NATIVE_FLOAT, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    datasetName = "Longitude";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    datasetLon = H5Dcreate2(file, datasetName, H5T_NATIVE_FLOAT, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    datasetName = "Stare Index";
    VERBOSE(cerr << "Creating dataset: " << datasetName << " -> ");
    datasetStare = H5Dcreate2(file, datasetName, H5T_NATIVE_INT64, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write the data to the datasets
     */
    VERBOSE(cerr << "Writing data to dataset" << endl);
    H5Dwrite(datasetX, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &xArray[0]);
    H5Dwrite(datasetY, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &yArray[0]);
    H5Dwrite(datasetLat, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &latArray[0]);
    H5Dwrite(datasetLon, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &lonArray[0]);
    vector<uint64> stareValsVector = *stareVals;
    H5Dwrite(datasetStare, H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, &stareValsVector[0]);

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

static void usage()
{
    cerr << "\nbuild_sidecar [options] <filename> latitude-name longitude-name level buildlevel\n\n";
    cerr << "-o output file: \tOutput the STARE data to the given output file\n\n";
    cerr << "-v verbose\n\n";
    cerr << "-t transfer location: \tTransfer the generated sidecar file to the given directory\n" << endl;
}

/** -h	help
 *  -o	output file
 *	-v	verbose
 *
 *  build_sidecar [options] [file|DAP_URL] latitude_var_name longitude_var_name level buildlevel
 */
int main(int argc, char *argv[]) {
    int c;
    extern char *optarg;
    extern int optind;

    string newName;
    string tmpStorage;

    while ((c = getopt(argc, argv, "hvo:t:")) != -1) {
        switch (c) {
            case 'h':
                usage();
                exit(EXIT_SUCCESS);
                break;
            case 'o':
                newName = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            case 't':
                tmpStorage = optarg;
                break;
        }
    }

    // This resets argc and argv once the optional arguments have been processed. jhrg 12/5/19
    argc -= optind;
    argv += optind;

    if (argc != 5) {
        cerr << "Expected 5 required arguments" << endl;
        usage();
        exit(EXIT_FAILURE);
    }

    //Required argument values
    string dataUrl = argv[argc - 5];
    string latName = argv[argc - 4];
    string lonName = argv[argc - 3];
    float level = atof(argv[argc - 2]);
    float build_level = atof(argv[argc - 1]);

    try {
        unique_ptr< vector<coords> > coordResults;
        if (dataUrl.find("https://") != string::npos || dataUrl.find("http://") != string::npos ||
            dataUrl.find("www.") != string::npos)
            // FIXME Match logic of the local file function
            cerr << "URL support disabled." << endl;
#if 0
            coordResults = readUrl(dataUrl, latName, lonName);
#endif
        else
            coordResults = readLocal(dataUrl, latName, lonName);

        unique_ptr< vector<uint64> > stareResults = calculateStareIndex(level, build_level, coordResults);

        // FIXME The data file's extension might not be 'h5'. jhrg 12/5/19
        if (newName.empty()) {
            //Locate the granule name inside the provided url.
            // Once the granule is found, add ".h5" to the granule name
            // Rename the new H5 file to be <granulename.h5>
            size_t granulePos = dataUrl.find_last_of("/");
            string granuleName = dataUrl.substr(granulePos + 1);
            size_t findDot = granuleName.find_last_of(".");
            newName = granuleName.substr(0, findDot) + "_sidecar.h5";
        }

        writeHDF5(newName, tmpStorage, coordResults, stareResults);
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
