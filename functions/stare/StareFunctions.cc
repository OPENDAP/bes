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
//#include <SpatialRange.h>

//#include <hdf5.h>
#include <netcdf.h>

#include <libdap/BaseType.h>
#include <libdap/Float64.h>
#include <libdap/Str.h>
#include <libdap/Array.h>
#include <libdap/Grid.h>
#include <libdap/Structure.h>

#include <libdap/DMR.h>
#include <libdap/D4RValue.h>

#include <libdap/Byte.h>
#include <libdap/Int16.h>
#include <libdap/Int32.h>
#include <libdap/UInt16.h>
#include <libdap/UInt32.h>
#include <libdap/Float32.h>
#include <libdap/UInt64.h>

#include "BESDebug.h"
#include "BESUtil.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"

#include "StareFunctions.h"
#include "GeoFile.h"

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

// TODO Remove this once change-over is complete. jhrg 6/17/21
string stare_sidecar_suffix = "_stare.nc";

/**
 * @brief Write a collection of STARE Matches to an ostream
 *
 * STARE matches is not a vector of stare_match objects. It's a self-contained
 * collection of vectors that holds a collection of STARE matches in a way that
 * can be dumped into libdap::Array instances easily and efficiently.
 *
 * @param out The ostream
 * @param m The STARE Matches
 * @return A reference to the ostream
 */
ostream & operator << (ostream &out, const stare_matches &m)
{
    assert(m.stare_indices.size() == m.row_indices.size()
        && m.row_indices.size() == m.col_indices.size());

    auto ti = m.target_indices.begin();
    auto si = m.stare_indices.begin();
    auto xi = m.row_indices.begin();
    auto yi = m.col_indices.begin();

    while (si != m.stare_indices.end()) {
        out << "Target: " << *ti++ << ", Dataset Index: " << *si++ << ", coord: row: " << *xi++ << ", col: " << *yi++ << endl;
    }

    return out;
}

static void
extract_stare_index_array(Array *var, vector<STARE_ArrayIndexSpatialValue> &values)
{
    if (var->var()->type() != dods_uint64_c)
        throw BESSyntaxUserError("STARE server function passed an invalid Index array (" + var->name()
        + " is type: " + var->var()->type_name() + ").", __FILE__, __LINE__);

    values.resize(var->length());
    var->value((dods_uint64*)&values[0]);    // Extract the values of 'var' to 'values'
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
//int cmpSpatial(STARE_ArrayIndexSpatialValue a_, STARE_ArrayIndexSpatialValue b_);

/**
 * @brief Do any of the targetIndices STARE indices overlap the dataset's STARE indices?
 * @param target_indices - stare values from a constraint expression
 * @param data_stare_indices - stare values being compared, retrieved from the sidecar file. These
 * are the index values that describe the coverage of the dataset.
 * @return true if sny of the target indices appear in the dataset, otherwise false.
 */
bool
target_in_dataset(const vector<STARE_ArrayIndexSpatialValue> &target_indices,
                  const vector<STARE_ArrayIndexSpatialValue> &data_stare_indices) {
#if 1
    // this took 0.23s and worked.
    // Changes to the range-for loop, fixed the type (was unsigned long long
    // which works on OSX but not CentOS7). jhrg 11/5/19
    for (const STARE_ArrayIndexSpatialValue &i : target_indices) {
        for (const STARE_ArrayIndexSpatialValue &j :data_stare_indices ) {
            // Check to see if the index 'i' overlaps the index 'j'. The cmpSpatial()
            // function returns -1, 0, 1 depending on i in j, no overlap or, j in i.
            // testing for !0 covers the general overlap case.
            int result = cmpSpatial(i, j);
            if (result != 0)
                return true;
        }
    }
#else
    // For one sample MOD05 file, this took 5.6s and failed to work.
    // Problem: seg fault.

    // Initialize the skiplists for the search
    auto r = SpatialRange(data_stare_indices);
    cerr << "built spatial range" << endl;
    for (const dods_uint64 &sid : target_indices) {
        if( r.contains(sid) ) { return true; }
    }
#endif
    return false;
}

/**
 * @brief How many of the dataset's STARE indices overlap the target STARE indices?
 *
 * This method should return the number of indices from the target set
 * that overlap the of the dataset's spatial coverage.
 *
 * @todo Currently does not use the coverage but the larger index set.
 *
 * @param target_indices - stare values from a constraint expression
 * @param dataset_indices - stare values being compared, retrieved from the sidecar file. These
 * are the index values that describe the coverage of the dataset.
 * @param all_dataset_matches If true this function counts every dataset index that
 * overlaps every target index. The default counts 1 for each target index that matches _any_
 * dataset index.
 * @return The number of target indices in the dataset.
 */
unsigned int
count(const vector<STARE_ArrayIndexSpatialValue> &target_indices,
      const vector<STARE_ArrayIndexSpatialValue> &dataset_indices,
      bool all_dataset_matches /*= false*/) {
    unsigned int counter = 0;
    for (const auto &i : target_indices) {
        for (const auto &j : dataset_indices)
            // Here we are counting the number of target indices that overlap the
            // dataset indices.
            if (cmpSpatial(i, j) != 0) {
                counter++;
                BESDEBUG(STARE, "Matching (dataset, target) indices: " << i << ", " << j << endl);
                if (!all_dataset_matches)
                    break;  // exit the inner loop
            }
    }

    return counter;
}

/**
 * @brief Return a set of stare matches
 * @param target_indices Look for these indices
 * @param dataset_indices Look in these indices
 * @param dataset_rows The number of rows in the dataset
 * @param dataset_cols The number of columns in the dataset
 * @return
 */
unique_ptr<stare_matches>
stare_subset_helper(const vector<STARE_ArrayIndexSpatialValue> &target_indices,
                    const vector<STARE_ArrayIndexSpatialValue> &dataset_indices,
                    size_t dataset_rows, size_t dataset_cols)
{
    //auto subset = new stare_matches;
    unique_ptr<stare_matches> subset(new stare_matches());

   auto sid_iter = dataset_indices.begin();
    for (size_t i = 0; i < dataset_rows; ++i) {
        for (size_t j = 0; j < dataset_cols; ++j) {
            auto sid = *sid_iter++;
            for (const auto &target : target_indices) {
                if (cmpSpatial(sid, target) != 0) {     // != 0 --> sid is in target OR target is in sid
                    subset->add(i, j, sid, target);
                }
            }
        }
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
        const vector<STARE_ArrayIndexSpatialValue> &target_indices,
        const vector<STARE_ArrayIndexSpatialValue> &dataset_indices)
{
    assert(dataset_indices.size() == src_data.size());
    assert(dataset_indices.size() == result_data.size());

    auto r = result_data.begin();
    auto s = src_data.begin();
    for (const auto &i : dataset_indices) {
        for (const auto &j : target_indices) {
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
 *
 * @todo FIXME jhrg 6/17/21
 */
template <class T>
void StareSubsetArrayFunction::build_masked_data(Array *dependent_var,
                                                 const vector<STARE_ArrayIndexSpatialValue> &dep_var_stare_indices,
                                                 const vector<STARE_ArrayIndexSpatialValue> &target_s_indices, T mask_value,
                                                 unique_ptr<Array> &result) {
    vector<T> src_data(dependent_var->length());
    dependent_var->read();  // TODO Do we need to call read() here? jhrg 6/16/20
    dependent_var->value(&src_data[0]);

    //T mask_value = 0;  // TODO This should use the value in mask_val_var. jhrg 6/16/20
    vector<T> result_data(dependent_var->length(), mask_value);

    stare_subset_array_helper(result_data, src_data, target_s_indices, dep_var_stare_indices);

    result->set_value(result_data, result_data.size());
}

void
read_stare_indices_from_function_argument(BaseType *raw_stare_indices,
                                          vector<STARE_ArrayIndexSpatialValue> &s_indices) {

    Array *stare_indices = dynamic_cast<Array *>(raw_stare_indices);
    if (stare_indices == nullptr)
        throw BESSyntaxUserError(
                "Expected an Array but found a " + raw_stare_indices->type_name(), __FILE__, __LINE__);

    if (stare_indices->var()->type() != dods_uint64_c)
        throw BESSyntaxUserError(
                "Expected an Array of UInt64 values but found an Array of  " + stare_indices->var()->type_name(),
                __FILE__, __LINE__);

    stare_indices->read();

    extract_stare_index_array(stare_indices, s_indices);
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

    BaseType *dependent_var = args->get_rvalue(0)->value(dmr);
    BaseType *raw_stare_indices = args->get_rvalue(1)->value(dmr);

    unique_ptr<GeoFile> gf(new GeoFile(dmr.filename()));

    // Read the stare indices for the dependent var from the sidecar file.
    vector<STARE_ArrayIndexSpatialValue> dep_var_stare_indices;
    gf->get_stare_indices(dependent_var->name(), dep_var_stare_indices);

    // Put the stare indices passed into the function into a vector<>
    vector<STARE_ArrayIndexSpatialValue> target_s_indices;
    read_stare_indices_from_function_argument(raw_stare_indices, target_s_indices);

    // Are any of the target indices covered by this variable
    bool status = target_in_dataset(target_s_indices, dep_var_stare_indices);

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

    BaseType *dependent_var = args->get_rvalue(0)->value(dmr);
    BaseType *raw_stare_indices = args->get_rvalue(1)->value(dmr);

    unique_ptr<GeoFile> gf(new GeoFile(dmr.filename()));

    // Read the stare indices for the dependent var from the sidecar file.
    vector<STARE_ArrayIndexSpatialValue> dep_var_stare_indices;
    gf->get_stare_indices(dependent_var->name(), dep_var_stare_indices);

    // Put the stare indices passed into the function into a vector<>
    vector<STARE_ArrayIndexSpatialValue> target_s_indices;
    read_stare_indices_from_function_argument(raw_stare_indices, target_s_indices);

    int num = count(target_s_indices, dep_var_stare_indices, false);

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

    BaseType *dependent_var = args->get_rvalue(0)->value(dmr);
    BaseType *raw_stare_indices = args->get_rvalue(1)->value(dmr);

    unique_ptr<GeoFile> gf(new GeoFile(dmr.filename()));

    // Read the stare indices for the dependent var from the sidecar file.
    vector<STARE_ArrayIndexSpatialValue> dep_var_stare_indices;
    gf->get_stare_indices(dependent_var->name(), dep_var_stare_indices);

    // Put the stare indices passed into the function into a vector<>
    vector<STARE_ArrayIndexSpatialValue> target_s_indices;
    read_stare_indices_from_function_argument(raw_stare_indices, target_s_indices);

    unique_ptr <stare_matches> subset = stare_subset_helper(target_s_indices, dep_var_stare_indices,
                                                            gf->get_variable_rows(dependent_var->name()),
                                                            gf->get_variable_cols(dependent_var->name()));

    // When no subset is found (none of the target indices match those in the dataset)
    if (subset->stare_indices.size() == 0) {
        subset->stare_indices.push_back(0);
        subset->target_indices.push_back(0);
        subset->row_indices.push_back(-1);
        subset->col_indices.push_back(-1);
    }
    // Transfer values to a Structure
    unique_ptr<Structure> result(new Structure("result"));

    unique_ptr<Array> stare(new Array("stare", new UInt64("stare")));
    stare->set_value((dods_uint64*)&(subset->stare_indices[0]), subset->stare_indices.size());
    stare->append_dim(subset->stare_indices.size());
    result->add_var_nocopy(stare.release());

    unique_ptr<Array> target(new Array("target", new UInt64("target")));
    target->set_value((dods_uint64*)&(subset->target_indices[0]), subset->target_indices.size());
    target->append_dim(subset->target_indices.size());
    result->add_var_nocopy(target.release());

    unique_ptr<Array> x(new Array("row", new Int32("row")));
    x->set_value(subset->row_indices, subset->row_indices.size());
    x->append_dim(subset->row_indices.size());
    result->add_var_nocopy(x.release());

    unique_ptr<Array> y(new Array("col", new Int32("col")));
    y->set_value(subset->col_indices, subset->col_indices.size());
    y->append_dim(subset->col_indices.size());
    result->add_var_nocopy(y.release());

    return result.release();
}

// FIXME jhrg 7/6/21
double get_mask_value(BaseType *)
{
    return 17;
}

BaseType *
StareSubsetArrayFunction::stare_subset_array_dap4_function(D4RValueList *args, DMR &dmr)
{
    if (args->size() != 3) {
        ostringstream oss;
        oss << "stare_subset_array(): Expected three arguments, but got " << args->size();
        throw BESSyntaxUserError(oss.str(), __FILE__, __LINE__);
    }

    Array *dependent_var = dynamic_cast<Array*>(args->get_rvalue(0)->value(dmr));
    if (!dependent_var)
        throw BESSyntaxUserError("Expected an Array as teh first argument to stare_subset_array()", __FILE__, __LINE__);

    BaseType *mask_val_var = args->get_rvalue(1)->value(dmr);
    double mask_value = get_mask_value(mask_val_var);

    BaseType *raw_stare_indices = args->get_rvalue(2)->value(dmr);

    unique_ptr<GeoFile> gf(new GeoFile(dmr.filename()));

    // Read the stare indices for the dependent var from the sidecar file.
    vector<STARE_ArrayIndexSpatialValue> dep_var_stare_indices;
    gf->get_stare_indices(dependent_var->name(), dep_var_stare_indices);

    // Put the stare indices passed into the function into a vector<>
    vector<STARE_ArrayIndexSpatialValue> target_s_indices;
    read_stare_indices_from_function_argument(raw_stare_indices, target_s_indices);

    // ptr_duplicate() does not copy data values
    unique_ptr<Array> result(dynamic_cast<Array*>(dependent_var->ptr_duplicate()));

    // FIXME Add more types. jhrg 6/17/20
    switch(dependent_var->var()->type()) {
        case dods_int16_c: {
            build_masked_data<dods_int16>(dependent_var, dep_var_stare_indices, target_s_indices, mask_value, result);
            break;
        }
        case dods_float32_c: {
            build_masked_data<dods_float32>(dependent_var, dep_var_stare_indices, target_s_indices, mask_value, result);
            break;
        }

        default:
            throw BESInternalError(string("stare_subset_array() failed: Unsupported array element type (")
                + dependent_var->var()->type_name() + ").", __FILE__, __LINE__);
    }

    return result.release();
}

// TODO code to build a lat/lon box. From Mike Rilee. 7/6/21

#if 0
// STARE.h
// Signature: STARE::STARE_SpatialIntervals CoverBoundingBoxFromLatLonDegrees(LatLonDegrees64ValueVector corners, int force_resolution_level = -1);

LatLonDegrees64ValueVector latlonbox;
latlonbox.push_back(LatLonDegrees64(0,0));
latlonbox.push_back(LatLonDegrees64(2,0));
latlonbox.push_back(LatLonDegrees64(2,2));
latlonbox.push_back(LatLonDegrees64(0,2));
int force_resolution = 6; // defaults to -1
STARE_SpatialIntervals sivs = sIndex.CoverBoundingBoxFromLatLonDegrees(latlonbox,force_resolution);
#endif

} // namespace functions
