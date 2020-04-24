// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2019 OPeNDAP, Inc.
// Authors: Kodi Neumiller <kneumiller@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <sstream>
#include <unordered_map>
#include <memory>

#include <STARE.h>
#include <hdf5.h>

#include <BaseType.h>
#include <Float64.h>
#include <Str.h>
#include <Array.h>
#include <Grid.h>
#include <Structure.h>
#include <DDS.h>

#include <DMR.h>
#include <D4RValue.h>

#include <Byte.h>
#include <Int16.h>
#include <Int32.h>
#include <UInt16.h>
#include <UInt32.h>
#include <Float32.h>
#include <Int64.h>
#include <UInt64.h>
#include <Int8.h>

#include <Error.h>

#include <debug.h>
#include <util.h>

#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"

#include "StareFunctions.h"

using namespace libdap;
using namespace std;

namespace functions {

/**
 *
 * @brief Write a functions::point to an ostream.
 * @param out The ostream
 * @param p The point
 * @return A reference to the ostream
 */
ostream & operator << (ostream &out, const point &p)
{
    out << "x: " << p.x;
    out << ", y: " << p.y;
    return out;
}

/**
 * @brief Write a STARE match object to an ostream
 * @param out The ostream
 * @param m The STARE Match
 * @return A reference to the ostream
 */
ostream & operator << (ostream &out, const stare_match &m)
{
    out << "SIndex: " << m.stare_index;
    out << ", coord: " << m.coord;
    return out;
}

/**
 * @brief Write a collection of STARE Matches to an ostream
 *
 * STARE matches is not a vector of stare_match objects. It's a self-contained
 * commection of vectors that holds a collection of STARE matches in a way that
 * can be dumped into libdap::Array instances easily and efficiently.
 *
 * @param out The ostream
 * @param m The STARE Matches
 * @return A reference to the ostream
 */
ostream & operator << (ostream &out, const stare_matches &m)
{
    assert(m.stare_indices.size() == m.x_indices.size()
        && m.x_indices.size() == m.y_indices.size());

    auto si = m.stare_indices.begin();
    auto xi = m.x_indices.begin();
    auto yi = m.y_indices.begin();

    while (si != m.stare_indices.end()) {
        out << "SIndex: " << *si++ << ", coord: x: " << *xi++ << ", y: " << *yi++ << endl;
    }

    return out;
}

// May need to be moved to libdap/util
// This helper function assumes 'var' is the correct size.
// Made this static to limit its scope to this file. jhrg 11/7/19

/**
 * @brief extract values form a libdap::Array and return them in an unsigned 64-bit int vector
 * @param var The array
 * @return An unsigned 64-bit integer vector.
 * @throw libdap::Error if the array does not hold values that can be stored in a 64-bit unsigned integer vector
 */
static vector<dods_uint64> *extract_uint64_array(Array *var) {
    assert(var);

    int length = var->length();

    auto *newVar = new vector<dods_uint64>;
    newVar->resize(length);
    var->value(&(*newVar)[0]);    // Extract the values of 'var' to 'newVar'

    return newVar;
}

/**
 * Compare two spatial array index values a, b.
 *
 * Suffers some overhead, but should handle the overlap. For other
 * kinds of intersection we need to handle intervals or move to a range.
 *
 * Returns
 *   1 if b is in a
 *  -1 if a is in b
 *   0 otherwise.
 *
 */
//int cmpSpatial(STARE_ArrayIndexSpatialValue a_, STARE_ArrayIndexSpatialValue b_) {

/**
 * @brief Do any of the targetIndices STARE indices overlap the dataset's STARE indices?
 * @param targetIndices - stare values from a constraint expression
 * @param dataStareIndices - stare values being compared, retrieved from the sidecar file. These
 * are the index values that describe the coverage of the dataset.
 */
bool
target_in_dataset(const vector<dods_uint64> &targetIndices, const vector<dods_uint64> &dataStareIndices) {
    // Changes to the range-for loop, fixed the type (was unsigned long long
    // which works on OSX but not CentOS7). jhrg 11/5/19
    for (const dods_uint64 &i : targetIndices) {
        for (const dods_uint64 &j :dataStareIndices ) {
            // Check to see if the index 'i' overlaps the index 'j'. The cmpSpatial()
            // function returns -1, 0, 1 depending on i in j, no overlap or, j in i.
            // testing for !0 covers the general overlap case.
            int result = cmpSpatial(i, j);
            if (result != 0)
                return true;
        }
    }

    return false;
}

/**
 * @brief How many of the target STARE indices overlap the dataset's STARE indices?
 *
 * This method should return the number of indices passed to the server that overlap the
 * spatial coverage of the dataset as defined by its set of STARE indices.
 *
 * @param stareVal - stare values from a constraint expression
 * @param dataStareIndices - stare values being compared, retrieved from the sidecar file. These
 * are the index values that describe the coverage of the dataset.
 */
unsigned int
count(const vector<dods_uint64> &stareVal, const vector<dods_uint64> &dataStareIndices) {
    unsigned int counter = 0;
    for (const dods_uint64 &i : stareVal) {
        for (const dods_uint64 &j : dataStareIndices)
            // Here we are counting the number of traget indices that overlap the
            // dataset indices.
            if (cmpSpatial(i, j) != 0) {
                counter++;
                break;  // exit the inner loop
            }
    }

    return counter;
}

/**
 * @brief Return a collection of STARE Matches
 * @param targetIndices Target STARE indices (passed in by a client)
 * @param datasetStareIndices STARE indices of this dataset
 * @param xArray Matching X indices for the corresponding STARE index
 * @param yArray Matching Y indices for the corresponding STARE index
 * @return A STARE Matches collection. Contains the X, Y, and 'matching' target indices.
 * @see stare_matches
 */
unique_ptr<stare_matches>
stare_subset_helper(const vector<dods_uint64> &targetIndices, const vector<dods_uint64> &datasetStareIndices,
                    const vector<int> &xArray, const vector<int> &yArray)
{
    assert(datasetStareIndices.size() == xArray.size());
    assert(datasetStareIndices.size() == yArray.size());

    //auto subset = new stare_matches;
    unique_ptr<stare_matches> subset(new stare_matches());

    auto x = xArray.begin();
    auto y = yArray.begin();

    for (dods_uint64 datasetStareIndex : datasetStareIndices) {
        for (dods_uint64 targetIndex : targetIndices) {
            if (cmpSpatial(datasetStareIndex, targetIndex) != 0) { // (datasetStareIndex == targetIndex) {
                subset->add(*x, *y, targetIndex);
            }
        }
        ++x;
        ++y;
    }

    return subset;
}

/**
 * @brief Return the pathname to an STARE sidecar file for a given dataset.
 *
 * This uses the value of the BES key FUNCTIONS.stareStoragePath to find
 * a the sidecar file that matches the given dataset. The lookup ignores
 * any path component of the dataset; only the file name is used.
 * @param pathName The dataset pathname
 * @return The pathname to the matching sidecar file.
 */
string
get_sidecar_file_pathname(const string &pathName)
{
    size_t granulePos = pathName.find_last_of('/');
    string granuleName = pathName.substr(granulePos + 1);
    size_t findDot = granuleName.find_last_of('.');
    // Added extraction of the extension since the files won't always be *.h5
    // also switched to .append() instead of '+' because the former is faster.
    // jhrg 11/5/19
    string extension = granuleName.substr(findDot); // ext includes the dot
    string newPathName = granuleName.substr(0, findDot).append("_sidecar").append(extension);

    string stareDirectory = TheBESKeys::TheKeys()->read_string_key(STARE_STORAGE_PATH, "/tmp");

    string fullPath = BESUtil::pathConcat(stareDirectory, newPathName);
    return fullPath;
}

/**
 * @brief Read the 32-bit integer array data
 * @param file The HDF5 Id of an open file
 * @param values Value-result parameter, a vector that can hold dods_int32 values
 */
void
get_sidecar_int32_values(hid_t file, const string &variable, vector<dods_int32> &values)
{
    hid_t dataset = H5Dopen(file, variable.c_str(), H5P_DEFAULT);
    if (dataset < 0)
        throw BESInternalError(string("Could not open dataset: ").append(variable), __FILE__, __LINE__);

    hid_t dspace = H5Dget_space(dataset);
    const int ndims = H5Sget_simple_extent_ndims(dspace);
    vector<hsize_t> dims(ndims);

    //Get the size of the dimension so that we know how big to make the memory space
    //Each of the dataspaces should be the same size, if in the future they are different
    // sizes then the size of each dataspace will need to be calculated.
    H5Sget_simple_extent_dims(dspace, &dims[0], NULL);

    //We need to get the filespace and memspace before reading the values from each dataset
    hid_t filespace = H5Dget_space(dataset);

    hid_t memspace = H5Screate_simple(ndims, &dims[0], NULL);

    //Get the number of elements in the dataspace and use that to appropriate the proper size of the vectors
    values.resize(H5Sget_select_npoints(filespace));

    //Read the data file and store the values of each dataset into an array
    H5Dread(dataset, H5T_NATIVE_INT, memspace, filespace, H5P_DEFAULT, &values[0]);
}

/**
 * @brief Read the unsigned 64-bit integer array data
 * @param file The HDF5 Id of an open file
 * @param values Value-result parameter, a vector that can hold dods_uint64 values
 */
void
get_sidecar_uint64_values(hid_t file, const string &variable, vector<dods_uint64> &values)
{
    hid_t dataset = H5Dopen(file, variable.c_str(), H5P_DEFAULT);
    if (dataset < 0)
        throw BESInternalError(string("Could not open dataset: ").append(variable), __FILE__, __LINE__);

    hid_t dspace = H5Dget_space(dataset);
    const int ndims = H5Sget_simple_extent_ndims(dspace);
    vector<hsize_t> dims(ndims);

    //Get the size of the dimension so that we know how big to make the memory space
    //Each of the dataspaces should be the same size, if in the future they are different
    // sizes then the size of each dataspace will need to be calculated.
    H5Sget_simple_extent_dims(dspace, &dims[0], NULL);

    //We need to get the filespace and memspace before reading the values from each dataset
    hid_t filespace = H5Dget_space(dataset);

    hid_t memspace = H5Screate_simple(ndims, &dims[0], NULL);

    //Get the number of elements in the dataspace and use that to appropriate the proper size of the vectors
    values.resize(H5Sget_select_npoints(filespace));

    //Read the data file and store the values of each dataset into an array
    H5Dread(dataset, H5T_NATIVE_ULLONG, memspace, filespace, H5P_DEFAULT, &values[0]);
}

/**
 * -- Intersection server function --
 * Checks to see if there are any Stare values provided from the client that can be found in the sidecar file.
 * If there is at least one match, the function will return a 1. Otherwise returns a 0.
 */
BaseType *
StareIntersectionFunction::stare_intersection_dap4_function(D4RValueList *args, DMR &dmr)
{
    if (args->size() != 1) {
        ostringstream oss;
        oss << "stare_intersection(): Expected a single argument, but got " << args->size();
        throw BESSyntaxUserError(oss.str(), __FILE__, __LINE__);
    }

    //Find the filename from the dmr
    string fullPath = get_sidecar_file_pathname(dmr.filename());

    //Read the file and store the datasets
    hid_t file = H5Fopen(fullPath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0)
        throw BESInternalError("Could not open file " + fullPath, __FILE__, __LINE__);

    //Read the data file and store the values of each dataset into an array
    vector<dods_uint64> dataStareIndices;
    get_sidecar_uint64_values(file, "Stare Index", dataStareIndices);

    BaseType *pBaseType = args->get_rvalue(0)->value(dmr);
    Array *stareSrc = dynamic_cast<Array *>(pBaseType);
    if (stareSrc == nullptr)
        throw BESSyntaxUserError("stare_intersection(): Expected an Array but found a " + pBaseType->type_name(), __FILE__, __LINE__);

    if (stareSrc->var()->type() != dods_uint64_c)
        throw BESSyntaxUserError("Expected an Array of UInt64 values but found an Array of  " + stareSrc->var()->type_name(), __FILE__, __LINE__);

    stareSrc->read();

    vector<dods_uint64> *targetIndices = extract_uint64_array(stareSrc);

    bool status = target_in_dataset(*targetIndices, dataStareIndices);

    delete targetIndices;

    Int32 *result = new Int32("result");
    if (status) {
        result->set_value(1);
    }
    else {
        result->set_value(0);
    }

    return result;
}

/**
 * @brief Count the number of STARE indices in the arg that overlap the indices of this dataset.
 *
 * This function counts the number of 'target' indices (those that are passed into
 * the function) that are contained (oe which contain) the SARE indices of the
 * dataset.
 *
 * In a URL, this will look like: stare_count($UInt64(4 : 3440016633681149966, ...))
 *
 * @note Hyrax also supports POST for data access requests so that very long lists
 * of function arguments may be passed into function like this one. Also note that
 * the notation '$UInt64(<n>:<value 0>,<value 1>,...<value n-1>)' can be written
 * '$UInt64(0:<value 0>,<value 1>,...<value n-1>)' with only minor performance cost.
 *
 * @param args A single vector of Unsigned 64-bit integer STARE Indices.
 * @param dmr The DMR for the given dataset. The dataset name is read from this object.
 * @return The number of indices given that also appear in the dataset, as a DAP Int32 value
 */
BaseType *
StareCountFunction::stare_count_dap4_function(D4RValueList *args, DMR &dmr)
{
    if (args->size() != 1) {
        ostringstream oss;
        oss << "stare_intersection(): Expected a single argument, but got " << args->size();
        throw BESSyntaxUserError(oss.str(), __FILE__, __LINE__);
    }

    //Find the filename from the dmr
    string fullPath = get_sidecar_file_pathname(dmr.filename());

    //Read the file and store the datasets
    hid_t file = H5Fopen(fullPath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0)
        throw BESInternalError("Could not open file " + fullPath, __FILE__, __LINE__);

    vector<dods_uint64> datasetStareIndices;
    get_sidecar_uint64_values(file, "Stare Index", datasetStareIndices);

#if 0
    // I wanted to see the values read from the MYD09.A2019003.2040.006.2019005020913_sidecar.h5
    // file. They are all '3440016191299518474' and there are about 100 indices. jhrg 11/18/19
    cerr << "Values: ";
    ostream_iterator<dods_uint64> it(cerr, ", ");
    std::copy(datasetStareIndices.begin(), datasetStareIndices.end(), it);
    cerr << endl;
#endif

    BaseType *pBaseType = args->get_rvalue(0)->value(dmr);
    Array *stareSrc = dynamic_cast<Array *>(pBaseType);
    if (stareSrc == nullptr)
        throw BESSyntaxUserError("stare_intersection(): Expected an Array but found a " + pBaseType->type_name(), __FILE__, __LINE__);

    if (stareSrc->var()->type() != dods_uint64_c)
        throw BESSyntaxUserError("Expected an Array of UInt64 values but found an Array of  " + stareSrc->var()->type_name(), __FILE__, __LINE__);

    stareSrc->read();

    vector<dods_uint64> *targetIndices = extract_uint64_array(stareSrc);

    int num = count(*targetIndices, datasetStareIndices);

    delete targetIndices;

    Int32 *result = new Int32("result");
    result->set_value(num);

    return result;
}

/**
 * @brief For the given target STARE indices, return the overlapping
 * dataset X, Y, and STARE indices
 *
 * This function will subset the dataset using the given vector of STARE indices. The result
 * of the subset operation is three vectors. The first two contain the X and Y indices of
 * the Latitude and Longitude arrays. The third vector contains the subset of the target
 * indices that overlap the dataset's STARE indices.
 *
 * @param args A single vector of Unsigned 64-bit integer STARE Indices.
 * @param dmr The DMR for the given dataset. The dataset name is read from this object.
 * @return Three arrays holding the three values. The Nth elements of each array
 * are one set of matched values.
 */
BaseType *
StareSubsetFunction::stare_subset_dap4_function(D4RValueList *args, DMR &dmr)
{
    if (args->size() != 1) {
        ostringstream oss;
        oss << "stare_intersection(): Expected a single argument, but got " << args->size();
        throw BESSyntaxUserError(oss.str(), __FILE__, __LINE__);
    }

    //Find the filename from the dmr
    string fullPath = get_sidecar_file_pathname(dmr.filename());

    //Read the file and store the datasets
    hid_t file = H5Fopen(fullPath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0)
        throw BESInternalError("Could not open file " + fullPath, __FILE__, __LINE__);

    // Read values from the sidecar file - stare data about a given dataset
    vector<dods_uint64> datasetStareIndices;
    get_sidecar_uint64_values(file, "Stare Index", datasetStareIndices);
    vector<dods_int32> xArray;
    get_sidecar_int32_values(file, "X", xArray);
    vector<dods_int32> yArray;
    get_sidecar_int32_values(file, "Y", yArray);

#if 0
    // I wanted to see the values read from the MYD09.A2019003.2040.006.2019005020913_sidecar.h5
    // file. They are all '3440016191299518474' and there are about 100 indices. jhrg 11/18/19
    cerr << "Values: ";
    ostream_iterator<dods_uint64> it(cerr, ", ");
    std::copy(datasetStareIndices.begin(), datasetStareIndices.end(), it);
    cerr << endl;
#endif

    BaseType *pBaseType = args->get_rvalue(0)->value(dmr);
    Array *stareSrc = dynamic_cast<Array *>(pBaseType);
    if (stareSrc == nullptr)
        throw BESSyntaxUserError("stare_intersection(): Expected an Array but found a " + pBaseType->type_name(), __FILE__, __LINE__);

    if (stareSrc->var()->type() != dods_uint64_c)
        throw BESSyntaxUserError("Expected an Array of UInt64 values but found an Array of  " + stareSrc->var()->type_name(), __FILE__, __LINE__);

    stareSrc->read();

    vector<dods_uint64> *targetIndices = extract_uint64_array(stareSrc);

    unique_ptr <stare_matches> subset = stare_subset_helper(*targetIndices, datasetStareIndices, xArray, yArray);

    delete targetIndices;

    // Transfer values to a Structure
    auto result = new Structure("result");

    Array *stare = new Array("stare", new UInt64("stare"));
    stare->set_value(&(subset->stare_indices[0]), subset->stare_indices.size());
    stare->append_dim(subset->stare_indices.size());
    result->add_var_nocopy(stare);

    auto x = new Array("x", new Int32("x"));
    x->set_value(subset->x_indices, subset->x_indices.size());
    x->append_dim(subset->x_indices.size());
    result->add_var_nocopy(x);

    auto y = new Array("y", new Int32("y"));
    y->set_value(subset->y_indices, subset->y_indices.size());
    y->append_dim(subset->y_indices.size());
    result->add_var_nocopy(y);

    return result;
}

} // namespace functions
