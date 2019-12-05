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

#include "ServerFunction.h"

namespace libdap {
class BaseType;
class DDS;
class D4RValueList;
class DMR;
}

namespace functions {

/// X and Y coordinates of a point
struct point {
    int x;
    int y;

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

/// Hold the result from the subset helper function as a collection of vectors
struct stare_matches {
    std::vector<int> x_indices;
    std::vector<int> y_indices;

    std::vector<libdap::dods_uint64> stare_indices;

    // Pass by value and use move
    stare_matches(std::vector<int> x, const std::vector<int> y, const std::vector<libdap::dods_uint64> si)
        : x_indices(std::move(x)), y_indices(std::move(y)), stare_indices(std::move(si)) {}

    stare_matches() {}

    void add(int x, int y, libdap::dods_uint64 si) {
        x_indices.push_back(x);
        y_indices.push_back(y);
        stare_indices.push_back(si);
    }

    friend std::ostream & operator << (std::ostream &out, const stare_matches &m);
};

std::string get_sidecar_file_pathname(const std::string &pathName);
void get_int32_values(hid_t file, const std::string &variable, std::vector<int> &values);
void get_uint64_values(hid_t file, const std::string &variable, std::vector<libdap::dods_uint64> &values);

bool has_value(const std::vector<libdap::dods_uint64> &stareVal, const std::vector<libdap::dods_uint64> &dataStareIndices);
unsigned int count(const std::vector<libdap::dods_uint64> &stareVal, const std:: vector<libdap::dods_uint64> &stareIndices);

vector<stare_match> *stare_subset_helper(const vector<libdap::dods_uint64> &targetIndices,
                                         const vector<libdap::dods_uint64> &datasetStareIndices,
                                         const vector<int> &xArray, const vector<int> &yArray);

stare_matches *stare_subset_helper2(const vector<libdap::dods_uint64> &targetIndices,
                                    const vector<libdap::dods_uint64> &datasetStareIndices,
                                    const vector<int> &xArray, const vector<int> &yArray);

const std::string STARE_STORAGE_PATH = "FUNCTIONS.stareStoragePath";

class StareIntersectionFunction : public libdap::ServerFunction {
public:
    static libdap::BaseType *stare_intersection_dap4_function(libdap::D4RValueList *args, libdap::DMR &dmr);

    friend class StareFunctionsTest;

public:
    StareIntersectionFunction() {
        setName("stare_intersection");
        setDescriptionString("The stare_intersection: Returns 1 if the coverage of the current dataset includes any of the given STARE indices.");
        setUsageString("stare_intersection(STARE index [, STARE index ...]) | linear_scale($UInt64(<size hint>:STARE index [, STARE index ...]))");
        setRole("http://services.opendap.org/dap4/server-side-function/stare_intersection");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#stare_intersection");
        setFunction(stare_intersection_dap4_function);
        setVersion("0.1");
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
        setDescriptionString("The stare_count: Returns the number of the STARE indices that are included in this dataset.");
        setUsageString("stare_count(STARE index [, STARE index ...]) | stare_count($UInt64(<size hint>:STARE index [, STARE index ...]))");
        setRole("http://services.opendap.org/dap4/server-side-function/stare_count");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#stare_count");
        setFunction(stare_count_dap4_function);
        setVersion("0.1");
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
        setName("stare_subset_helper");
        setDescriptionString("The stare_subset_helper: Returns the number of the STARE indices that are included in this dataset.");
        setUsageString("stare_subset(STARE index [, STARE index ...]) | stare_subset_helper($UInt64(<size hint>:STARE index [, STARE index ...]))");
        setRole("http://services.opendap.org/dap4/server-side-function/stare_subset_helper");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#stare_subset_helper");
        setFunction(stare_subset_dap4_function);
        setVersion("0.1");
    }

    virtual ~StareSubsetFunction() {
    }
};

} // functions namespace
