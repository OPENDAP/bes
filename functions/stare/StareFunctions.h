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

#include <dods-datatypes.h>

#include <STARE.h>
#include <hdf5.h>
#include <BaseType.h>
#include "ServerFunction.h"
#include <stare/GeoFile.h>

namespace libdap {
class BaseType;
class DDS;
class D4RValueList;
class DMR;
}

namespace functions {

const string s_index_name = "Stare_Index";

const std::string STARE_STORAGE_PATH_KEY = "FUNCTIONS.stareStoragePath";
const std::string STARE_SIDECAR_SUFFIX_KEY = "FUNCTIONS.stareSidecarSuffix";

// These default values can be overridden using BES keys.
// See DapFunctions.cc. jhrg 5/21/20
extern string stare_storage_path;
extern string stare_sidecar_suffix;

std::string get_sidecar_file_pathname(const std::string &pathName, const string &token = "_sidecar");
void get_sidecar_int32_values(hid_t file, const std::string &variable, std::vector<libdap::dods_int32> &values);
void get_sidecar_uint64_values(hid_t file, const std::string &variable, std::vector<libdap::dods_uint64> &values);
void get_sidecar_uint64_values_2(const std::string &filename, libdap::BaseType *variable, std::vector<libdap::dods_uint64> &values);

bool target_in_dataset(const std::vector<libdap::dods_uint64> &target_indices,
        const std::vector<libdap::dods_uint64> &data_stare_indices);
unsigned int count(const std::vector<libdap::dods_uint64> &target_indices,
        const std:: vector<libdap::dods_uint64> &dataset_indices, bool all_target_matches = false);

template <class T>
void stare_subset_array_helper(vector<T> &result_data, const vector<T> &src_data,
                               const vector<libdap::dods_uint64> &target_indices,
                               const vector<libdap::dods_uint64> &dataset_indices);
#if 0
/// X and Y coordinates of a point
struct point {
    libdap::dods_int32 x;
    libdap::dods_int32 y;

    point(int x, int y): x(x), y(y) {}
    friend std::ostream & operator << (std::ostream &out, const point &c);
};

/// one STARE index and the corresponding point for this dataset
struct stare_match {
    point coord;      /// The X and Y indices that match the...
    libdap::dods_uint64 stare_index; /// STARE index in this dataset

    stare_match(const point &p, libdap::dods_uint64 si): coord(p), stare_index(si) {}
    stare_match(int x, int y, libdap::dods_uint64 si): coord(x, y), stare_index(si) {}
    friend std::ostream & operator << (std::ostream &out, const stare_match &m);
};
#endif

/// Hold the result from the subset helper function as a collection of vectors
struct stare_matches {
    std::vector<libdap::dods_int32> x_indices;
    std::vector<libdap::dods_int32> y_indices;

    std::vector<libdap::dods_uint64> stare_indices;
    std::vector<libdap::dods_uint64> target_indices;

    // Pass by value and use move
    stare_matches(std::vector<libdap::dods_int32> x, const std::vector<libdap::dods_int32> y,
            const std::vector<libdap::dods_uint64> si, const std::vector<libdap::dods_uint64> ti)
        : x_indices(std::move(x)), y_indices(std::move(y)), stare_indices(std::move(si)), target_indices(std::move(ti)) {}

    stare_matches() {}

    void add(libdap::dods_int32 x, libdap::dods_int32 y, libdap::dods_uint64 si, libdap::dods_uint64 ti) {
        x_indices.push_back(x);
        y_indices.push_back(y);
        stare_indices.push_back(si);
        target_indices.push_back(ti);
    }

    friend std::ostream & operator << (std::ostream &out, const stare_matches &m);
};

unique_ptr<stare_matches> stare_subset_helper(const std::vector<libdap::dods_uint64> &target_indices,
                                              const std::vector<libdap::dods_uint64> &dataset_indices,
                                              const std::vector<int> &dataset_x_coords, const std::vector<int> &dataset_y_coords);

class StareIntersectionFunction : public libdap::ServerFunction {
public:
    static libdap::BaseType *stare_intersection_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);

    friend class StareFunctionsTest;

public:
    StareIntersectionFunction() {
        setName("stare_intersection");
        setDescriptionString("The stare_intersection: Returns 1 if the coverage of the current dataset includes any of the given STARE indices, 0 otherwise.");
        setUsageString("stare_intersection(var, STARE index [, STARE index ...]) | stare_intersection(var, $UInt64(<size hint>:STARE index [, STARE index ...]))");
        setRole("http://services.opendap.org/dap4/server-side-function/stare_intersection");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#stare_intersection");
        setFunction(stare_intersection_dap4_function);
        setVersion("0.3");
    }

    virtual ~StareIntersectionFunction() {
    }
};

class StareCountFunction : public libdap::ServerFunction {
public:
    static libdap::BaseType *stare_count_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);

    friend class StareFunctionsTest;

public:
    StareCountFunction() {
        setName("stare_count");
        setDescriptionString("The stare_count: Returns the number of the STARE indices that are included in the coverage of this dataset.");
        setUsageString("stare_count(var, STARE index [, STARE index ...]) | stare_count(var, $UInt64(<size hint>:STARE index [, STARE index ...]))");
        setRole("http://services.opendap.org/dap4/server-side-function/stare_count");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#stare_count");
        setFunction(stare_count_dap4_function);
        setVersion("0.3");
    }

    virtual ~StareCountFunction() {
    }
};

class StareSubsetFunction : public libdap::ServerFunction {
public:
    static libdap::BaseType *stare_subset_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);

    friend class StareFunctionsTest;

public:
    StareSubsetFunction() {
        setName("stare_subset");
        setDescriptionString("The stare_subset: Returns the set of the STARE indices that are included in the coverage of this dataset.");
        setUsageString("stare_subset(var, $UInt64(<size hint>:STARE index [, STARE index ...]))");
        setRole("http://services.opendap.org/dap4/server-side-function/stare_subset");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#stare_subset");
        setFunction(stare_subset_dap4_function);
        setVersion("0.3");
    }

    virtual ~StareSubsetFunction() {
    }
};

class StareSubsetArrayFunction : public libdap::ServerFunction {
public:
    static libdap::BaseType *stare_subset_array_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);

    friend class StareFunctionsTest;

public:
    StareSubsetArrayFunction() {
        setName("stare_subset_array");
        setDescriptionString("The stare_subset: Returns a masked copy of 'var' for the subset given the set of STARE indices that are included in the coverage of this dataset.");
        setUsageString("stare_subset_array(var, mask-val, $UInt64(<size hint>:STARE index [, STARE index ...]))");
        setRole("http://services.opendap.org/dap4/server-side-function/stare_subset_array");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#stare_subset_array");
        setFunction(stare_subset_array_dap4_function);
        setVersion("0.1");
    }

    virtual ~StareSubsetArrayFunction() {
    }

    template <class T>
    static void build_masked_data(libdap::Array *dependent_var, const vector<libdap::dods_uint64> &dep_var_stare_indices,
                                const vector<libdap::dods_uint64> &target_s_indices, unique_ptr<libdap::Array> &result);
};

} // functions namespace
