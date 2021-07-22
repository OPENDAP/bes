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

#include <BaseType.h>
#include <dods-datatypes.h>

#include "ServerFunction.h"

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

template<class T>
void stare_subset_array_helper(vector<T> &result_data, const vector<T> &src_data,
                               const vector<STARE_ArrayIndexSpatialValue> &target_indices,
                               const vector<STARE_ArrayIndexSpatialValue> &dataset_indices);

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

public:
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

public:
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

public:
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

public:
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

    template<class T>
    static void build_masked_data(libdap::Array *dependent_var,
                                  const std::vector<STARE_ArrayIndexSpatialValue> &dep_var_stare_indices,
                                  const std::vector<STARE_ArrayIndexSpatialValue> &target_s_indices, T mask_value,
                                  unique_ptr<libdap::Array> &result);
};

} // functions namespace
