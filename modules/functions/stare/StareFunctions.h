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

#include <string>
#include <utility>

#include <STARE.h>
#include <hdf5.h>

#include <libdap/BaseType.h>
#include <libdap/dods-datatypes.h>

#include <libdap/ServerFunction.h>

#define STARE_STORAGE_PATH_KEY "FUNCTIONS.stareStoragePath"
#define STARE_SIDECAR_SUFFIX_KEY "FUNCTIONS.stareSidecarSuffix"

namespace libdap {
class BaseType;
class DDS;
class D4RValueList;
class DMR;
}

namespace functions {

// These default values can be overridden using BES keys.
// See DapFunctions.cc. jhrg 5/21/20
extern string stare_storage_path;
extern string stare_sidecar_suffix;

bool target_in_dataset(const std::vector<STARE_ArrayIndexSpatialValue> &target_indices,
                       const std::vector<STARE_ArrayIndexSpatialValue> &data_stare_indices);

unsigned int count(const std::vector<STARE_ArrayIndexSpatialValue> &target_indices,
                   const std::vector<STARE_ArrayIndexSpatialValue> &dataset_indices, bool all_target_matches = false);

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
 */
template<class T>
void build_masked_data(libdap::Array *dependent_var,
                       const vector<STARE_ArrayIndexSpatialValue> &dep_var_stare_indices,
                       const vector<STARE_ArrayIndexSpatialValue> &target_s_indices,
                       T mask_value,
                       unique_ptr<libdap::Array> &result)
{
    vector<T> src_data(dependent_var->length());
    dependent_var->read();  // TODO Do we need to call read() here? jhrg 6/16/20
    dependent_var->value(src_data.data());

    //T mask_value = 0;  // TODO This should use the value in mask_val_var. jhrg 6/16/20
    vector<T> result_data(dependent_var->length(), mask_value);

    stare_subset_array_helper(result_data, src_data, target_s_indices, dep_var_stare_indices);

    result->set_value(result_data, result_data.size());
}

/// Hold the result from the subset helper function as a collection of vectors
struct stare_matches {
    std::vector<libdap::dods_int32> row_indices;
    std::vector<libdap::dods_int32> col_indices;

    std::vector<STARE_ArrayIndexSpatialValue> stare_indices;
    std::vector<STARE_ArrayIndexSpatialValue> target_indices;

    // Pass by value and use move
    stare_matches(std::vector<libdap::dods_int32> row, std::vector<libdap::dods_int32> col,
                  std::vector<STARE_ArrayIndexSpatialValue> si, std::vector<STARE_ArrayIndexSpatialValue> ti)
            : row_indices(std::move(row)), col_indices(std::move(col)), stare_indices(std::move(si)),
              target_indices(std::move(ti)) {}

    stare_matches() = default;

    void add(libdap::dods_int32 row, libdap::dods_int32 col, STARE_ArrayIndexSpatialValue si, STARE_ArrayIndexSpatialValue ti) {
        row_indices.push_back(row);
        col_indices.push_back(col);
        stare_indices.push_back(si);
        target_indices.push_back(ti);
    }

    friend std::ostream &operator<<(std::ostream &out, const stare_matches &m);
};

unique_ptr<stare_matches> stare_subset_helper(const std::vector<STARE_ArrayIndexSpatialValue> &target_indices,
                                              const std::vector<STARE_ArrayIndexSpatialValue> &dataset_indices,
                                              size_t row, size_t cols);

class StareIntersectionFunction : public libdap::ServerFunction {
public:
    static libdap::BaseType *stare_intersection_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);

    friend class StareFunctionsTest;

    StareIntersectionFunction() {
        setName("stare_intersection");
        setDescriptionString(
                "The stare_intersection: Returns 1 if the coverage of the current dataset includes any of the given STARE indices, 0 otherwise.");
        setUsageString(
                "stare_intersection(var, STARE index [, STARE index ...]) | stare_intersection(var, $UInt64(<size hint>:STARE index [, STARE index ...]))");
        setRole("http://services.opendap.org/dap4/server-side-function/stare_intersection");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#stare_intersection");
        setFunction(stare_intersection_dap4_function);
        setVersion("0.3");
    }

    ~StareIntersectionFunction() override = default;
};

class StareCountFunction : public libdap::ServerFunction {
public:
    static libdap::BaseType *stare_count_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);

    friend class StareFunctionsTest;

    StareCountFunction() {
        setName("stare_count");
        setDescriptionString(
                "The stare_count: Returns the number of the STARE indices that are included in the coverage of this dataset.");
        setUsageString(
                "stare_count(var, STARE index [, STARE index ...]) | stare_count(var, $UInt64(<size hint>:STARE index [, STARE index ...]))");
        setRole("http://services.opendap.org/dap4/server-side-function/stare_count");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#stare_count");
        setFunction(stare_count_dap4_function);
        setVersion("0.3");
    }

    ~StareCountFunction() override = default;
};

class StareSubsetFunction : public libdap::ServerFunction {
public:
    static libdap::BaseType *stare_subset_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);

    friend class StareFunctionsTest;

    StareSubsetFunction() {
        setName("stare_subset");
        setDescriptionString(
                "The stare_subset: Returns the set of the STARE indices that are included in the coverage of this dataset.");
        setUsageString("stare_subset(var, $UInt64(<size hint>:STARE index [, STARE index ...]))");
        setRole("http://services.opendap.org/dap4/server-side-function/stare_subset");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#stare_subset");
        setFunction(stare_subset_dap4_function);
        setVersion("0.3");
    }

    ~StareSubsetFunction() override = default;
};

class StareSubsetArrayFunction : public libdap::ServerFunction {
public:
    static libdap::BaseType *stare_subset_array_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);

    friend class StareFunctionsTest;

    StareSubsetArrayFunction() {
        setName("stare_subset_array");
        setDescriptionString(
                "The stare_subset: Returns a masked copy of 'var' for the subset given the set of STARE indices that are included in the coverage of this dataset.");
        setUsageString("stare_subset_array(var, mask-val, $UInt64(<size hint>:STARE index [, STARE index ...]))");
        setRole("http://services.opendap.org/dap4/server-side-function/stare_subset_array");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#stare_subset_array");
        setFunction(stare_subset_array_dap4_function);
        setVersion("0.1");
    }

    ~StareSubsetArrayFunction() override = default;
};

/**
 * Wrapper for a lat/lon point. Used by the Stare Box code.
 */
struct point {
    double lat;
    double lon;
    point() = default;
    point(double lat_, double lon_) : lat(lat_), lon(lon_) {}
};

STARE_SpatialIntervals stare_box_helper(const vector<point> &points, int resolution = 6);
STARE_SpatialIntervals stare_box_helper(const point &top_left, const point &bottom_right, int resolution = 6);

class StareBoxFunction : public libdap::ServerFunction {
public:
    static libdap::BaseType *stare_box_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);

    friend class StareFunctionsTest;

    StareBoxFunction() {
        setName("stare_box");
        setDescriptionString(
                "The stare_box() function: Returns a STARE cover for the region within the four lat/lon corner points.");
        setUsageString("stare_box(tl_lat, tl_lon, br_lat, br_lon) or stare_box(p1_lat, p1_lon, ..., p4_lat, p4_lon)");
        setRole("http://services.opendap.org/dap4/server-side-function/stare_box");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#stare_box");
        setFunction(stare_box_dap4_function);
        setVersion("0.1");
    }

    ~StareBoxFunction() override = default;
};

} // functions namespace
