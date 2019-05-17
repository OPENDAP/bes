/*********************************************************************
 * stareMapping.cc													*
 *																	*
 *  Created on: Mar 7, 2019											*
 *  																	*
 *  		Purpose: Index the STARE value that corresponds to a given	*
 *  				 iterator from an array, as well as its lat/lon 		*
 *  				 value.												*
 *      Author: Kodi Neumiller, kneumiller@opendap.org				*
 *      																*
 ********************************************************************/

#include "SpatialIndex.h"
#include "SpatialInterface.h"

#include <D4Connect.h>
#include <Connect.h>
#include <Response.h>
#include <Array.h>

#include <Error.h>

#include <unordered_map>
#include <array>
#include <memory>
#include "hdf5.h"

using namespace std;

static bool verbose = false;
#define VERBOSE(x) do { if (verbose) x; } while(false)

//Struct to store the iterator and lat/lon values that match a specific STARE index
struct coords {
	int x;
	int y;

	float lat;
	float lon;

	int stareIndex;
};

/**
 * @brief Compute STARE indices and tie the values to x/y coords using lat/lon referenced by data url.
 * @param dataUrl
 * @param level
 * @param buildlevel
 * @param keyVals return value param containing the STARE info
 */
void findLatLon(std::string dataUrl, const float64 level,
	const float64 buildlevel, vector<int> &xArray, vector<int> &yArray, vector<float> &latArray, vector<float> &lonArray, vector<int> &stareArray) {
	//Create an htmInterface that will be used to get the STARE index
	htmInterface htm(level, buildlevel);
	const SpatialIndex &index = htm.index();

	auto_ptr < libdap::Connect > url(new libdap::Connect(dataUrl));

	std::vector<float> lat;
	std::vector<float> lon;

	try {
		libdap::BaseTypeFactory factory;
		libdap::DataDDS dds(&factory);

		VERBOSE(cerr << "\n\n\tRequesting data from " << dataUrl << endl);

		//Makes sure only the Latitude and Longitude variables are requested
		url->request_data(dds, "Latitude,Longitude");

		//Create separate libdap arrays to store the lat and lon arrays individually
		libdap::Array *urlLatArray = dynamic_cast<libdap::Array *>(dds.var(
				"Latitude"));
		libdap::Array *urlLonArray = dynamic_cast<libdap::Array *>(dds.var(
				"Longitude"));

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
		std::unordered_map<float, struct coords> indexMap;

#if 0
		//Taken out because CF doesn't support compound types,
		//	so each variable will need to be stored in its own array
		// -kln 5/17/19

		//Array to store the key and values of the indexMap that will be used to write the hdf5 file
		keyVals.resize(size_x * size_y);
#endif

		xArray.resize(lat.size());
		yArray.resize(lat.size());
		latArray.resize(lat.size());
		lonArray.resize(lat.size());
		stareArray.resize(lat.size());

		//Declare the beginning of the vectors here rather than inside the loop to save compute time
		//vector<float>::iterator i_begin = lat.begin();		FIXME: Probably not needed since we just use a single dimension array
		vector<float>::iterator j_begin = lon.begin();

		int arrayLoc = 0;

		VERBOSE (cerr << "\nCalculating the STARE indices" << endl);

		//Make a separate iterator to point at the lat vector and the lon vector inside the same loop.
		// Then, loop through the lat and lon arrays at the same time
		for (vector<float>::iterator i = lat.begin(), e = lat.end(), j =
				lon.begin(); i != e; ++i, ++j) {
			//Use an offset since we are using a 1D array and treating it like a 2D array
			int offset = j - j_begin;
			indexVals.x = offset / (size_x - 1);//Get the current index for the lon
			indexVals.y = offset % (size_x - 1);//Get the current index for the lat
			indexVals.lat = *i;
			indexVals.lon = *j;
			indexVals.stareIndex = index.idByLatLon(*i, *j);//Use the lat/lon values to calculate the Stare index

			//Map the STARE index value to the x,y indices
			indexMap[indexVals.stareIndex] = indexVals;

			//Store the values calculated for indexVals and store them in their arrays to be used in the hdf5 file
			xArray[arrayLoc] = indexVals.x;
			yArray[arrayLoc] = indexVals.y;
			latArray[arrayLoc] = *i;
			lonArray[arrayLoc] = *j;
			stareArray[arrayLoc] = indexVals.stareIndex;

#if 0
			//Taken out because CF doesn't support compound types,
			//	so each variable will need to be stored in its own array
			// -kln 5/17/19

			//Store the same previous values inside the coords vector for use in the hdf5 file
			keyVals[arrayLoc].x = indexVals.x;
			keyVals[arrayLoc].y = indexVals.y;
			keyVals[arrayLoc].lat = *i;
			keyVals[arrayLoc].lon = *j;
			keyVals[arrayLoc].stareIndex = indexVals.stareIndex;
#endif

			arrayLoc++;
		}

	} catch (libdap::Error &e) {
		cerr << "ERROR: " << e.get_error_message() << endl;
	}
}


/*************
 * HDF5 Stuff *
 *************/
void writeHDF5(const string &filename, const vector<int> &xArray, const vector<int> &yArray, const vector<float> &latArray, const vector<float> &lonArray, const vector<int> &stareArray) {
	hid_t file, datasetX, datasetY, datasetLat, datasetLon, datasetStare;
	hid_t dataspace; /* handle */

	//Used to store the the size of the array.
	// In other cases where the array is more than 1 dimension each dimension's length would be stored in this array.
	hsize_t arrayLength[1] = { xArray.size() };

	//Allows us to use hdf5 1.10 generated files with hdf5 1.8
	// !---Must be removed if a feature from 1.10 is required to use---!
	// -kln 5/16/19
	hid_t fapl = H5Pcreate (H5P_FILE_ACCESS);
#ifdef H5F_LIBVER_V18
	H5Pset_libver_bounds (fapl, H5F_LIBVER_EARLIEST, H5F_LIBVER_V18);
#else
	H5Pset_libver_bounds (fapl, H5F_LIBVER_EARLIEST, H5F_LIBVER_LATEST);
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
	datasetStare = H5Dcreate2(file, datasetName, H5T_NATIVE_INT, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

	/*
	 * Write the data to the datasets
	 */
	VERBOSE(cerr << "Writing data to dataset" << endl);
	H5Dwrite(datasetX, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &xArray[0]);
	H5Dwrite(datasetY, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &yArray[0]);
	H5Dwrite(datasetLat, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &latArray[0]);
	H5Dwrite(datasetLon, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &lonArray[0]);
	H5Dwrite(datasetStare, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &stareArray[0]);

	/*
	 * Close/release resources.
	 */
	H5Sclose(dataspace);
	H5Fclose(file);

	VERBOSE(cerr << "Data written to file: " << filename << endl);
}


/** -h	help
 *  -o	output file
 *	-v	verbose
 *
 *  build_sidecar [options] filename level buildlevel
 */
int main(int argc, char *argv[]) {
	int c;
	string newName;

	while ((c = getopt(argc, argv, "hvo:")) != -1) {
		switch (c) {
		case 'h':
			cerr << "\nbuild_sidecar [options] <filename> level buildlevel\n\n";
			cerr << "-o output file: \tOutput the STARE data to the given output file\n\n";
			cerr << "-v verbose\n" << endl;
			exit(1);
			break;
		case 'o':
			newName = optarg;
			break;
		case 'v':
			verbose = true;
			break;
		}
	}

	//Argument values
	string dataUrl = argv[argc-3];
	float level = atof(argv[argc-2]);
	float build_level = atof(argv[argc-1]);

	/*if (argc != 4) {
		cerr << "Expected 3 arguments" << endl;
		exit(EXIT_FAILURE);
	}*/

	try {
		//Need to store each value in its own array
		vector<int> xVals;
		vector<int> yVals;
		vector<float> latVals;
		vector<float> lonVals;
		vector<int> stareVals;

		findLatLon(dataUrl, level, build_level, xVals, yVals, latVals, lonVals, stareVals);
#if 0
		//Taken out because CF doesn't support compound types,
		//	so each variable will need to be stored in its own array
		// -kln 5/17/19

		vector<coords> keyVals;
		findLatLon(dataUrl, level, build_level, keyVals);
#endif

		if (newName.empty()) {
			//Locate the granule name inside the provided url.
			// Once the granule is found, add ".h5" to the granule name
			// Rename the new H5 file to be <granulename.h5>
			size_t granulePos = dataUrl.find_last_of("/");
			string granuleName = dataUrl.substr(granulePos + 1);
			size_t findDot = granuleName.find_last_of(".");
			newName = granuleName.substr(0, findDot) + "_sidecar.h5";
		}

		writeHDF5(newName, xVals, yVals, latVals, lonVals, stareVals);
	}
	catch(libdap::Error &e) {
		cerr << "Error: " << e.get_error_message() << endl;
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);

}
