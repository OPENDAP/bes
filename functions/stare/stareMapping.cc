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
#include "H5Cpp.h"

using namespace H5;

//Struct to store the iterator and lat/lon values that match a specific STARE index
struct coords {
	int x;
	int y;

	float lat;
	float lon;

	int stareIndex;
};

void findLatLon(std::string dataUrl, const float64 level,
		const float64 buildlevel) {
	//Create an htmInterface that will be used to get the STARE index
	htmInterface htm(level, buildlevel);
	const SpatialIndex &index = htm.index();

	auto_ptr < libdap::Connect > url(new libdap::Connect(dataUrl));

	std::vector<float> lat;
	std::vector<float> lon;

	try {
		libdap::BaseTypeFactory factory;
		libdap::DataDDS dds(&factory);

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

		int dims = urlLatArray->dimensions();

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


		coords indexVals = coords();
		std::unordered_map<float, struct coords> indexMap;

		//Array to store the key and values of the indexMap that will be used to write the hdf5 file
		cout << "y length: " << size_y << endl;
		vector<coords> keyVals(size_x * size_y);

		//Declare the beginning of the vectors here rather than inside the loop to save compute time
		vector<float>::iterator i_begin = lat.begin();
		vector<float>::iterator j_begin = lon.begin();

		int arrayLoc = 0;

		//Make a separate iterator to point at the lat vector and the lon vector inside the same loop.
		// Then, loop through the lat and lon arrays at the same time
		for (vector<float>::iterator i = lat.begin(), e = lat.end(), j =
				lon.begin(); i != e; ++i, ++j) {
			//Use an offset since we are using a 1D array and treating it like a 2D array
			int offset = j - j_begin;
			indexVals.x = offset / (size_x - 1);	//Get the current index for the lon
			indexVals.y = offset % (size_x - 1);	//Get the current index for the lat
			indexVals.lat = *i;
			indexVals.lon = *j;
			indexVals.stareIndex = index.idByLatLon(*i, *j);//Use the lat/lon values to calculate the Stare index

			//Map the STARE index value to the x,y indices
			indexMap[indexVals.stareIndex] = indexVals;

			//Store the same previous values inside the coords vector for use in the hdf5 file
			keyVals[arrayLoc].x = indexVals.x;
			keyVals[arrayLoc].y = indexVals.y;
			keyVals[arrayLoc].lat = *i;
			keyVals[arrayLoc].lon = *j;
			keyVals[arrayLoc].stareIndex = indexVals.stareIndex;

			arrayLoc++;
		}


		 /*************
		 * HDF5 Stuff *
		 * ***********/

		//DataSpace takes in how many dimensions the provided array will have,
		//followed by each dimension's size.
		//An array is used to store each dimensions size ("arrayLength[]" in this case)
		hsize_t arrayLength[] = { keyVals.size() };
		DataSpace space(1, arrayLength);

		//Locate the granule name inside the provided url.
		// Once the granule is found, add ".h5" to the granule name
		// Rename the new H5 file to be <granulename.h5>
		std::size_t granulePos = dataUrl.find_last_of("/");
		std::string granuleName = dataUrl.substr(granulePos+1);
		std::size_t findDot = granuleName.find_last_of(".");
		std::string newName = granuleName.substr(0,findDot)+ ".h5";
		H5File* file = new H5File(newName, H5F_ACC_TRUNC);

		//Allocate appropriate memory
		CompType mtype(sizeof(coords));
		mtype.insertMember("x", HOFFSET(coords, x), PredType::NATIVE_INT);
		mtype.insertMember("y", HOFFSET(coords, y), PredType::NATIVE_INT);
		mtype.insertMember("lat", HOFFSET(coords, lat), PredType::NATIVE_FLOAT);
		mtype.insertMember("lon", HOFFSET(coords, lon), PredType::NATIVE_FLOAT);
		mtype.insertMember("stareIndex", HOFFSET(coords, stareIndex), PredType::NATIVE_INT);

		DataSet* dataset = new DataSet(
				file->createDataSet("StareIndexSet", mtype, space));
		dataset->write(&keyVals[0], mtype);

		delete dataset;
		delete file;

	} catch (libdap::Error &e) {
		cerr << "ERROR: " << e.get_error_message() << endl;
	}
}


int main(int argc, char *argv[]) {
	findLatLon(argv[1], atof(argv[2]), atof(argv[3]));
}
