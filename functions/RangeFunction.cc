// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Authors: Nathan Potter <npotter@opendap.org>
//         James Gallagher <jgallagher@opendap.org>
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

#include <BaseType.h>
#include <Float64.h>
#include <Str.h>
#include <Structure.h>
#include <Array.h>
#include <Grid.h>
#include "D4RValue.h"

#include <Error.h>
#include <DDS.h>

//#include <dods-limits.h>
#include <debug.h>
#include <util.h>

#include "BESDebug.h"

#include "RangeFunction.h"

using namespace libdap;

namespace functions {

string range_info =
        string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
                + "<function name=\"linear_scale\" version=\"1.0b1\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#range\">\n"
                + "</function>";

// These static functions could be moved to a class that provides a more
// general interface for COARDS/CF someday. Assume each BaseType comes bundled
// with an attribute table.

/**
 * @brief Return the attribute value in a double
 * @param val Attribute value as text
 * @return The attribute value in a double
 */
static double string_to_double(const char *val)
{
    istringstream iss(val);
    double v;
    iss >> v;

    double abs_val = fabs(v);
    if (abs_val > DODS_DBL_MAX || (abs_val != 0.0 && abs_val < DODS_DBL_MIN))
        throw Error(malformed_expr, string("Could not convert the string '") + val + "' to a double.");

    return v;
}

#if 0

Not Used

/** Look for any one of a series of attribute values in the attribute table
 for \e var. This function treats the list of attributes as if they are ordered
 from most to least likely/important. It stops when the first of the vector of
 values is found. If the variable (var) is a Grid, this function also looks
 at the Grid's Array for the named attributes. In all cases it returns the
 first value found.
 @param var Look for attributes in this BaseType variable.
 @param attributes A vector of attributes; the first one found will be returned.
 @return The attribute value in a double. */
static double get_attribute_double_value(BaseType *var, vector<string> &attributes)
{
    // This code also builds a list of the attribute values that have been
    // passed in but not found so that an informative message can be returned.
    AttrTable &attr = var->get_attr_table();
    string attribute_value = "";
    string values = "";
    vector<string>::iterator i = attributes.begin();
    while (attribute_value == "" && i != attributes.end()) {
        values += *i;
        if (!values.empty()) values += ", ";
        attribute_value = attr.get_attr(*i++);
    }

    // If the value string is empty, then look at the grid's array (if it's a
    // grid) or throw an Error.
    if (attribute_value.empty()) {
        if (var->type() == dods_grid_c)
            return get_attribute_double_value(dynamic_cast<Grid&>(*var).get_array(), attributes);
        else
            throw Error(malformed_expr,
                    string("No COARDS/CF '") + values.substr(0, values.length() - 2)
                            + "' attribute was found for the variable '" + var->name() + "'.");
    }

    return string_to_double(remove_quotes(attribute_value).c_str());
}
#endif

/**
 * @brief Return the value for an attribute.
 *
 * This function looks up the attribute in the table bound to var. If that
 * fails it assumes the variable is a Grid and looks in the Grid's Array. If the
 * first attempt - looking in the attribute table of var - fails and var is not
 * a Grid, this code throws an exception.
 *
 * @param var Look in this variable's attribute table
 * @param attribute The name of the attribute
 * @return The attribute value in a double
 */
static double get_attribute_double_value(BaseType *var, const string &attribute)
{
    AttrTable &attr = var->get_attr_table();
    string attribute_value = attr.get_attr(attribute);

    // If the value string is empty, then look at the grid's array (if it's a
    // grid or throw an Error.
    if (attribute_value.empty()) {
        if (var->type() == dods_grid_c)
            return get_attribute_double_value(dynamic_cast<Grid&>(*var).get_array(), attribute);
        else
            throw Error(malformed_expr,
                    string("No COARDS '") + attribute + "' attribute was found for the variable '" + var->name()
                            + "'.");
    }

    return string_to_double(remove_quotes(attribute_value).c_str());
}

static double get_missing_value(BaseType *var)
{
    return get_attribute_double_value(var, "missing_value");
}

/**
 * @brief Scan the data and find the max and min values.
 *
 * @param data Pointer to the data (a vector of doubles)
 * @param length Number of elements in data
 * @param use_missing True if the data values matching missing should be excluded
 * @param missing Value to exclude (a double)
 * @return An instance of min_max_t that holds the min and max values
 */
min_max_t find_min_max(double* data, int length, bool use_missing, double missing)
{
    min_max_t v;

    if (use_missing) {
        for (int i = 0; i < length; ++i) {
            if (!double_eq(data[i], missing)) {
                v.max_val = max(v.max_val, data[i]);
                v.min_val = min(v.min_val, data[i]);
            }
        }
    }
    else {
        for (int i = 0; i < length; ++i) {
            v.max_val = max(v.max_val, data[i]);
            v.min_val = min(v.min_val, data[i]);
        }
    }

    return v;
}

// TODO Modify this to include information about monotonicity of vectors.
// That will be useful for geo operations when we use this to look at lat
// and lon extent.

/**
 * @brief Find the min and max values of a DAP variable
 * @param bt Scan the data in this variable
 * @param missing Exclude this value when/if found in data
 * @param use_missing True if the missing value should be excluded from the results
 * @return A DAP Structure with two Float64 variables 'min' and 'max'. This structure
 * is named such that it will be flattened by the BES framework function evaluator
 * code.
 */
BaseType *range_worker(BaseType *bt, double missing, bool use_missing)
{
    // Read the data, determine range and return the result. Must replace the new data
    // in a constructor (i.e., Array part of a Grid).

    min_max_t v;

    if (bt->type() == dods_grid_c) {
        // Grab the whole Grid; note that the scaling is done only on the array part
        Grid &source = dynamic_cast<Grid&>(*bt);

        BESDEBUG("function", "range_worker() - Grid send_p: " << source.send_p() << endl);
        BESDEBUG("function", "range_worker() - Grid Array send_p: " << source.get_array()->send_p() << endl);

        // Read the grid; set send_p since Grid is a kind of constructor and
        // read will only be called on it's fields if their send_p flag is set
        source.set_send_p(true);
        source.read();

        // Get the Array part and read the values
        Array *a = source.get_array();
        double *data = extract_double_array(a);

        // Now determine the range.
        int length = a->length();

        v = find_min_max(data, length, use_missing, missing);

        delete[] data;
    }
    else if (bt->is_vector_type()) {
        Array &source = dynamic_cast<Array&>(*bt);
        // If the array is really a map, make sure to read using the Grid
        // because of the HDF4 handler's odd behavior WRT dimensions.
        if (source.get_parent() && source.get_parent()->type() == dods_grid_c) {
            source.get_parent()->set_send_p(true);
            source.get_parent()->read();
        }
        else
            source.read();

        double *data = extract_double_array(&source);

        // Now determine the range.
        int length = source.length();

        v = find_min_max(data, length, use_missing, missing);

        delete[] data;
    }
    else if (bt->is_simple_type() && !(bt->type() == dods_str_c || bt->type() == dods_url_c)) {
        double data = extract_double_value(bt);
        v.max_val = data;
        v.min_val = data;
    }
    else {
        throw Error(malformed_expr, "The range_worker() function works only for numeric Grids, Arrays and scalars.");
    }

    // TODO Move this down to the dap2/4 versions?
    Structure *rangeResult = new Structure("range_result_unwrap");

    Float64 *rangeMin = new Float64("min");
    rangeMin->set_value(v.min_val);
    rangeResult->add_var_nocopy(rangeMin);

    Float64 *rangeMax = new Float64("max");
    rangeMax->set_value(v.max_val);
    rangeResult->add_var_nocopy(rangeMax);

    return rangeResult;
}

/** Given a BaseType, scale it using 'y = mx + b'. Either provide the
 constants 'm' and 'b' or the function will look for the COARDS attributes
 'scale_factor' and 'add_offset'.

 @param argc A count of the arguments
 @param argv An array of pointers to each argument, wrapped in a child of BaseType
 @param btpp A pointer to the return value; caller must delete.

 @return The scaled variable, represented using Float64
 @exception Error Thrown if scale_factor is not given and the COARDS
 attributes cannot be found OR if the source variable is not a
 numeric scalar, Array or Grid. */
void function_dap2_range(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(range_info);
        *btpp = response;
        return;
    }

    // Check for 1 or 2 arguments: 1 --> use attributes; 2 --> use missing value
    DBG(cerr << "argc = " << argc << endl);
    if (!(argc == 1 || argc == 2 ))
        throw Error(malformed_expr,
                "Wrong number of arguments to range(). See range() for more information");

    // Get m & b
    bool use_missing = false;
    double  missing = 0.0;
    if (argc == 2) {
        missing = extract_double_value(argv[1]);
        use_missing = true;
    }
    else {
        // This is not the best plan; the get_missing_value() function should
        // do something other than throw, but to do that would require mayor
        // surgery on get_attribute_double_value().
        try {
            missing = get_missing_value(argv[0]);
            use_missing = true;
        }
        catch (Error &) {   // Ignore the libdap::Error thrown (but not other errors). jhrg 6/6/17
            use_missing = false;
        }
    }

    BESDEBUG("function",
            "function_dap2_range() -  use_missing: " << use_missing << ", missing: " << missing << endl);

    *btpp = range_worker(argv[0], missing, use_missing);
}

/** Given a BaseType, scale it using 'y = mx + b'. Either provide the
 constants 'm' and 'b' or the function will look for the COARDS attributes
 'scale_factor' and 'add_offset'.

 @param argc A count of the arguments
 @param argv An array of pointers to each argument, wrapped in a child of BaseType
 @param btpp A pointer to the return value; caller must delete.

 @return The scaled variable, represented using Float64
 @exception Error Thrown if scale_factor is not given and the COARDS
 attributes cannot be found OR if the source variable is not a
 numeric scalar, Array or Grid. */
BaseType *function_dap4_range(D4RValueList *args, DMR &dmr)
{
    BESDEBUG("function", "function_dap4_range()  BEGIN " << endl);

    // DAP4 function porting information: in place of 'argc' use 'args.size()'
    if (args == 0 || args->size() == 0) {
        Str *response = new Str("info");
        response->set_value(range_info);
        // DAP4 function porting: return a BaseType* instead of using the value-result parameter
        return response;
    }

    // Check for 2 arguments
    DBG(cerr << "args.size() = " << args.size() << endl);
    if (!(args->size() == 1 || args->size() == 2))
        throw Error(malformed_expr,
                "Wrong number of arguments to linear_scale(). See linear_scale() for more information");

    // Get m & b
    bool use_missing = false;
    double missing = 0.0;
    if (args->size() == 2) {
        missing = extract_double_value(args->get_rvalue(3)->value(dmr));
        use_missing = true;
    }
    else {
        try {
            missing = get_missing_value(args->get_rvalue(0)->value(dmr));
            use_missing = true;
        }
        catch (Error &) {
            use_missing = false;
        }
    }
    BESDEBUG("function",
            "function_dap4_range() - use_missing: " << use_missing << ", missing: " << missing << endl);

    BESDEBUG("function", "function_dap4_range()  END " << endl);

    return range_worker(args->get_rvalue(0)->value(dmr), missing, use_missing);
}

} // namesspace functions
