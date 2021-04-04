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
#include <memory>
#include <cassert>

#include <STARE.h>
#include <hdf5.h>
#include <netcdf.h>

#include <BaseType.h>
#include <Float64.h>
#include <Str.h>
#include <Array.h>
#include <Grid.h>
#include <Structure.h>

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

#include "BESDebug.h"
#include "BESUtil.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"

#include "StareFunctions.h"

// Used with BESDEBUG
#define STARE "stare"

using namespace libdap;
using namespace std;

namespace functions {

// These default values can be overridden using BES keys.
// See StareFunctions.h jhrg 5/21/20
// If stare_storage_path is empty, expect the sidecar file in the same
// place as the data file. jhrg 5/26/20
string stare_storage_path = "";
string stare_sidecar_suffix = "_sidecar";

/**
 * @brief Write a collection of STARE Matches to an ostream
 *
 * STARE matches is not a vector of stare_match objects. It's a self-contained
 * connection of vectors that holds a collection of STARE matches in a way that
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

    auto ti = m.target_indices.begin();
    auto si = m.stare_indices.begin();
    auto xi = m.x_indices.begin();
    auto yi = m.y_indices.begin();

    while (si != m.stare_indices.end()) {
        out << "Target: " << *ti++ << ", Dataset Index: " << *si++ << ", coord: x: " << *xi++ << ", y: " << *yi++ << endl;
    }

    return out;
}

#if 0
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
#endif

void
extract_uint64_array(Array *var, vector<dods_uint64> &values) {

    values.resize(var->length());
    var->value(&values[0]);    // Extract the values of 'var' to 'values'
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
 * @param target_indices - stare values from a constraint expression
 * @param data_stare_indices - stare values being compared, retrieved from the sidecar file. These
 * are the index values that describe the coverage of the dataset.
 */
bool
target_in_dataset(const vector<dods_uint64> &target_indices, const vector<dods_uint64> &data_stare_indices) {
    // Changes to the range-for loop, fixed the type (was unsigned long long
    // which works on OSX but not CentOS7). jhrg 11/5/19
    for (const dods_uint64 &i : target_indices) {
        for (const dods_uint64 &j :data_stare_indices ) {
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
 * @brief How many of the dataset's STARE indices overlap the target STARE indices?
 *
 * This method should return the number of indices of the dataset's spatial coverage
 * that overlap the spatial coverage passed to the server as defined by the target
 * STARE indices.
 *
 * @param target_indices - stare values from a constraint expression
 * @param dataset_indices - stare values being compared, retrieved from the sidecar file. These
 * are the index values that describe the coverage of the dataset.
 * @param all_target_matches If true this function counts every target index that
 * overlaps every dataset index. The default counts 1 for each datset index that matches _any_
 * target index.
 */
unsigned int
count(const vector<dods_uint64> &target_indices, const vector<dods_uint64> &dataset_indices, bool all_target_matches /*= false*/) {
    unsigned int counter = 0;
    for (const dods_uint64 &i : dataset_indices) {
        for (const dods_uint64 &j : target_indices)
            // Here we are counting the number of target indices that overlap the
            // dataset indices.
            if (cmpSpatial(i, j) != 0) {
                counter++;
                BESDEBUG(STARE, "Matching (dataset, target) indices: " << i << ", " << j << endl);
                if (!all_target_matches)
                    break;  // exit the inner loop
            }
    }

    return counter;
}

/**
 * @brief Return a collection of STARE Matches
 * @param target_indices Target STARE indices (passed in by a client)
 * @param dataset_indices STARE indices of this dataset
 * @param dataset_x_coords Matching X indices for the corresponding dataset STARE index
 * @param dataset_y_coords Matching Y indices for the corresponding dataset STARE index
 * @return A STARE Matches collection. Contains the X, Y, and 'matching' dataset indices
 * for the given set of target indices.
 * @see stare_matches
 */
unique_ptr<stare_matches>
stare_subset_helper(const vector<dods_uint64> &target_indices, const vector<dods_uint64> &dataset_indices,
                    const vector<int> &dataset_x_coords, const vector<int> &dataset_y_coords)
{
    assert(dataset_indices.size() == dataset_x_coords.size());
    assert(dataset_indices.size() == dataset_y_coords.size());

    //auto subset = new stare_matches;
    unique_ptr<stare_matches> subset(new stare_matches());

    auto x = dataset_x_coords.begin();
    auto y = dataset_y_coords.begin();
    for (const dods_uint64 &i : dataset_indices) {
        for (const dods_uint64 &j : target_indices) {
            if (cmpSpatial(i, j) != 0) {    // != 0 --> i is in j OR j is in i
                subset->add(*x, *y, i, j);
                // TODO Add a break call here? jhrg 6/17/20
            }
        }
        ++x;
        ++y;
    }

    return subset;
}

/**
 * @brief Build the result data as masked values from src_data
 *
 * The result_data vector is modified. The result_data vector should be
 * passed to this function filled with NaN or the equivalent. This function
 * will transfer only the values from src_data to result_data that have
 * STARE indices which overlap a target index.
 *
 * @tparam T The array element datatype (e.g., Byte, Int32, ...)
 * @param result_data A vector<T> initialized to all mask values
 * @param src_data  The source data. Values are transferred as apropriate to result_data.
 * @param target_indices The STARE indices that define a region of interest.
 * @param dataset_indices The STARE indices that describe the coverage of the data.
 */
template <class T>
void stare_subset_array_helper(vector<T> &result_data, const vector<T> &src_data,
        const vector<dods_uint64> &target_indices, const vector<dods_uint64> &dataset_indices)
{
    assert(dataset_indices.size() == src_data.size());
    assert(dataset_indices.size() == result_data.size());

    auto r = result_data.begin();
    auto s = src_data.begin();
    for (const dods_uint64 &i : dataset_indices) {
        for (const dods_uint64 &j : target_indices) {
            if (cmpSpatial(i, j) != 0) {        // != 0 --> i is in j OR j is in i
                *r = *s;
                break;
            }
        }
        ++r; ++s;
    }
}

/**
 * @brief Mask the data in dependent_var using the target_s_indices
 *
 * Copy values from dependent_var to result that lie at the intersection
 * of the target and dataset stare indices. The libdap::Array 'result' is
 * modified.
 *
 * @tparam T The element type of the dependent_var and result Arrays
 *
 * @param dependent_var The dataset values to subset/mask
 * @param dep_var_stare_indices The stare indices that define the spatial
 * extent of these data
 * @param target_s_indices The stare indices that define the spatial extent
 * of the region of interest
 * @param result A value-result parameter. The masked dependent_var data is
 * returned using this libdap::Array
 */
template <class T>
void StareSubsetArrayFunction::build_masked_data(Array *dependent_var, const vector<dods_uint64> &dep_var_stare_indices,
                                                 const vector<dods_uint64> &target_s_indices, unique_ptr<Array> &result) {
    vector<T> src_data(dependent_var->length());
    dependent_var->read();  // TODO Do we need to call read() here? jhrg 6/16/20
    dependent_var->value(&src_data[0]);

    T mask_value = 0;  // TODO This should use the value in mask_val_var. jhrg 6/16/20
    vector<T> result_data(dependent_var->length(), mask_value);

    stare_subset_array_helper(result_data, src_data, target_s_indices, dep_var_stare_indices);

    result->set_value(result_data, result_data.size());
}

/**
 * @brief Return the pathname to an STARE sidecar file for a given dataset.
 *
 * This uses the value of the BES key FUNCTIONS.stareStoragePath to find
 * a the sidecar file that matches the given dataset. The lookup ignores
 * any path component of the dataset; only the file name is used.
 * @param pathName The dataset pathname
 * @param token Optional extension to the main part of the file name (default '_sidecar').
 * @return The pathname to the matching sidecar file.
 */
string
get_sidecar_file_pathname(const string &pathName, const string &token)
{
    string granuleName = pathName;
    if (!stare_storage_path.empty()) {
        granuleName = pathName.substr(pathName.find_last_of('/') + 1);
    }

    size_t findDot = granuleName.find_last_of('.');
    // Added extraction of the extension since the files won't always be *.h5
    // also switched to .append() instead of '+' because the former is faster.
    // jhrg 11/5/19
    string extension = granuleName.substr(findDot); // ext includes the dot
    granuleName = granuleName.substr(0, findDot).append(token).append(extension);

    if (!stare_storage_path.empty()) {
        // Above the path has been removed
        return BESUtil::pathConcat(stare_storage_path, granuleName);
    }
    else {
        // stare_storage_path is empty, granuleName is the full path
        return granuleName;
    }

}

/**
 * @brief Read the 32-bit integer array data
 * @param file The HDF5 Id of an open file
 * @param values Value-result parameter, a vector that can hold dods_int32 values
 */
void
get_sidecar_int32_values(const string &filename, const string &variable, vector<dods_int32> &values)
{
    //Read the file and store the datasets
    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0)
        throw BESInternalError("Could not open file " + filename, __FILE__, __LINE__);

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
 * @param filename The name of the netCDF/HDF5 sidecar file
 * @param variable Get the stare indices for this dependent variable
 * @param values Value-result parameter, a vector that can hold dods_uint64 values
 */
void
get_sidecar_uint64_values_2(const string &filename, BaseType *variable, vector<dods_uint64> &values)
{
    int ncid;
    GeoFile *gf = new GeoFile();
    vector<long long unsigned int> my_values;
    int ret;

    // Open and scan the sidecar file.
    if ((ret = gf->readSidecarFile(filename.c_str(), 1, ncid)))
        throw BESInternalError("Could not open file " + filename + " - " + nc_strerror(ret), __FILE__, __LINE__);

    // Get the STARE index data for variable.
    if ((ret = gf->getSTAREIndex_2(variable->name(), 1, ncid, my_values)))
        throw BESInternalError("Could get stare indexes from file " + filename + " - " + nc_strerror(ret), __FILE__, __LINE__);

    // Copy vector.
    for (int i = 0; i < my_values.size(); i++)
    	values.push_back(my_values.at(i));

    // Close the sidecar file.
    if ((ret = gf->closeSidecarFile(1, ncid)))
        throw BESInternalError("Could not close file " + filename + " - " + nc_strerror(ret),  __FILE__, __LINE__);
}

/**
 * @brief Read the unsigned 64-bit integer array data
 * @param file The HDF5 Id of an open file
 * @param variable Get the stare indices for this dependent variable
 * @param values Value-result parameter, a vector that can hold dods_uint64 values
 */
void
get_sidecar_uint64_values(const string &filename, BaseType */*variable*/, vector<dods_uint64> &values)
{
    //Read the file and store the datasets
    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0)
        throw BESInternalError("Could not open file " + filename, __FILE__, __LINE__);

    // Here we look up the name of the stare index data for 'variable.' For now,
    // use the name stored in 's_index_name'. jhrg 6/3/20

    hid_t dataset = H5Dopen(file, s_index_name.c_str(), H5P_DEFAULT);
    if (dataset < 0)
        throw BESInternalError(string("Could not open dataset: ").append(s_index_name), __FILE__, __LINE__);

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

void
read_stare_indices_from_function_argument(BaseType *raw_stare_indices, vector<dods_uint64>&s_indices) {

    Array *stare_indices = dynamic_cast<Array *>(raw_stare_indices);
    if (stare_indices == nullptr)
        throw BESSyntaxUserError(
                "Expected an Array but found a " + raw_stare_indices->type_name(), __FILE__, __LINE__);

    if (stare_indices->var()->type() != dods_uint64_c)
        throw BESSyntaxUserError(
                "Expected an Array of UInt64 values but found an Array of  " + stare_indices->var()->type_name(),
                __FILE__, __LINE__);

    stare_indices->read();

    extract_uint64_array(stare_indices, s_indices);
}

 /**
  * @brief Return true/false indicating that the given stare indices intersect the variables
  *
  * Checks to see if there are any Stare values provided from the client that can be found in the sidecar file.
  * If there is at least one match, the function will return a 1. Otherwise returns a 0.
  *
  * @param args Two values: A variable in the dataset and a set of stare indices
  * @param dmr The DMR object for this dataset
  * @return True/False in a DAP Int32.
  */
BaseType *
StareIntersectionFunction::stare_intersection_dap4_function(D4RValueList *args, DMR &dmr)
{
    if (args->size() != 2) {
        ostringstream oss;
        oss << "stare_intersection(): Expected two arguments, but got " << args->size();
        throw BESSyntaxUserError(oss.str(), __FILE__, __LINE__);
    }

    //Find the filename from the dmr
    string fullPath = get_sidecar_file_pathname(dmr.filename(), stare_sidecar_suffix);

    BaseType *dependent_var = args->get_rvalue(0)->value(dmr);
    BaseType *raw_stare_indices = args->get_rvalue(1)->value(dmr);

     //Read the data file and store the values of each dataset into an array
    vector<dods_uint64> dep_var_stare_indices;
    get_sidecar_uint64_values(fullPath, dependent_var, dep_var_stare_indices);

    // TODO: We can dump the values in 'stare_indices' here
    vector<dods_uint64> target_s_indices;
    read_stare_indices_from_function_argument(raw_stare_indices, target_s_indices);

    bool status = target_in_dataset(target_s_indices, dep_var_stare_indices);

#if 0
     Int32 *result = new Int32("result");
#endif
    unique_ptr<Int32> result(new Int32("result"));
    if (status) {
        result->set_value(1);
    }
    else {
        result->set_value(0);
    }

    return result.release();
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
    if (args->size() != 2) {
        ostringstream oss;
        oss << "stare_intersection(): Expected two arguments, but got " << args->size();
        throw BESSyntaxUserError(oss.str(), __FILE__, __LINE__);
    }

    //Find the filename from the dmr
    string fullPath = get_sidecar_file_pathname(dmr.filename(), stare_sidecar_suffix);

    BaseType *dependent_var = args->get_rvalue(0)->value(dmr);
    BaseType *raw_stare_indices = args->get_rvalue(1)->value(dmr);

    //Read the data file and store the values of each dataset into an array
    vector<dods_uint64> dep_var_stare_indices;
    get_sidecar_uint64_values(fullPath, dependent_var, dep_var_stare_indices);

    // TODO: We can dump the values in 'stare_indices' here
    vector<dods_uint64> target_s_indices;
    read_stare_indices_from_function_argument(raw_stare_indices, target_s_indices);

    int num = count(target_s_indices, dep_var_stare_indices);

#if 0
    Int32 *result = new Int32("result");
#endif
    unique_ptr<Int32> result(new Int32("result"));
    result->set_value(num);
    return result.release();
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
    if (args->size() != 2) {
        ostringstream oss;
        oss << "stare_subset(): Expected two arguments, but got " << args->size();
        throw BESSyntaxUserError(oss.str(), __FILE__, __LINE__);
    }

    //Find the filename from the dmr
    string fullPath = get_sidecar_file_pathname(dmr.filename(), stare_sidecar_suffix);

    BaseType *dependent_var = args->get_rvalue(0)->value(dmr);
    BaseType *raw_stare_indices = args->get_rvalue(1)->value(dmr);

    //Read the data file and store the values of each dataset into an array
    vector<dods_uint64> dep_var_stare_indices;
    get_sidecar_uint64_values(fullPath, dependent_var, dep_var_stare_indices);

    // TODO: We can dump the values in 'stare_indices' here
    vector<dods_uint64> target_s_indices;
    read_stare_indices_from_function_argument(raw_stare_indices, target_s_indices);

    vector<dods_int32> dataset_x_coords;
    get_sidecar_int32_values(fullPath, "X", dataset_x_coords);
    vector<dods_int32> dataset_y_coords;
    get_sidecar_int32_values(fullPath, "Y", dataset_y_coords);

    unique_ptr <stare_matches> subset = stare_subset_helper(target_s_indices, dep_var_stare_indices, dataset_x_coords, dataset_y_coords);

    // When no subset is found (none of the target indices match those in the dataset)
    if (subset->stare_indices.size() == 0) {
        subset->stare_indices.push_back(0);
        subset->target_indices.push_back(0);
        subset->x_indices.push_back(-1);
        subset->y_indices.push_back(-1);
    }
    // Transfer values to a Structure
    unique_ptr<Structure> result(new Structure("result"));

    unique_ptr<Array> stare(new Array("stare", new UInt64("stare")));
    stare->set_value(&(subset->stare_indices[0]), subset->stare_indices.size());
    stare->append_dim(subset->stare_indices.size());
    result->add_var_nocopy(stare.release());

    unique_ptr<Array> target(new Array("target", new UInt64("target")));
    target->set_value(&(subset->target_indices[0]), subset->target_indices.size());
    target->append_dim(subset->target_indices.size());
    result->add_var_nocopy(target.release());

    unique_ptr<Array> x(new Array("x", new Int32("x")));
    x->set_value(subset->x_indices, subset->x_indices.size());
    x->append_dim(subset->x_indices.size());
    result->add_var_nocopy(x.release());

    unique_ptr<Array> y(new Array("y", new Int32("y")));
    y->set_value(subset->y_indices, subset->y_indices.size());
    y->append_dim(subset->y_indices.size());
    result->add_var_nocopy(y.release());

    return result.release();
}

BaseType *
StareSubsetArrayFunction::stare_subset_array_dap4_function(D4RValueList *args, DMR &dmr)
{
    if (args->size() != 3) {
        ostringstream oss;
        oss << "stare_subset_array(): Expected three arguments, but got " << args->size();
        throw BESSyntaxUserError(oss.str(), __FILE__, __LINE__);
    }

    //Find the filename from the dmr
    string fullPath = get_sidecar_file_pathname(dmr.filename(), stare_sidecar_suffix);

    Array *dependent_var = dynamic_cast<Array*>(args->get_rvalue(0)->value(dmr));
    if (!dependent_var)
        throw BESSyntaxUserError("stare_subset_array() expected an Array as the first argument.", __FILE__, __LINE__);

    // TODO Get/use this.
    BaseType *mask_val_var = args->get_rvalue(1)->value(dmr);

    Array *raw_stare_indices = dynamic_cast<Array*>(args->get_rvalue(2)->value(dmr));
    if (!raw_stare_indices)
        throw BESSyntaxUserError("stare_subset_array() expected an Array as the third argument.", __FILE__, __LINE__);

    //Read the data file and store the values of each dataset into an array
    vector<dods_uint64> dep_var_stare_indices;
    get_sidecar_uint64_values(fullPath, dependent_var, dep_var_stare_indices);

    vector<dods_uint64> target_s_indices;
    read_stare_indices_from_function_argument(raw_stare_indices, target_s_indices);

    // ptr_duplicate() does not copy data values
    unique_ptr<Array> result(static_cast<Array*>(dependent_var->ptr_duplicate()));

    // TODO Add more types. jhrg 6/17/20
    switch(dependent_var->var()->type()) {
        case dods_int16_c: {
            build_masked_data<dods_int16>(dependent_var, dep_var_stare_indices, target_s_indices, result);
            break;
        }
        case dods_float32_c: {
            build_masked_data<dods_float32>(dependent_var, dep_var_stare_indices, target_s_indices, result);
            break;
        }

        default:
            throw BESInternalError(string("stare_subset_array() failed: Unsupported array element type (")
                + dependent_var->var()->type_name() + ").", __FILE__, __LINE__);
    }

    return result.release();
}

} // namespace functions
