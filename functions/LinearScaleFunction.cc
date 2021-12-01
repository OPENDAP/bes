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

#include <libdap/BaseType.h>
#include <Float64.h>
#include <Str.h>
#include <libdap/Array.h>
#include <libdap/Grid.h>
#include "D4RValue.h"

#include <Error.h>
#include <DDS.h>

#include <debug.h>
#include <util.h>

#include "BESDebug.h"

#include "LinearScaleFunction.h"

using namespace libdap;

namespace functions {

string linear_scale_info =
        string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
                + "<function name=\"linear_scale\" version=\"1.0b1\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#linear_scale\">\n"
                + "</function>";

// These static functions could be moved to a class that provides a more
// general interface for COARDS/CF someday. Assume each BaseType comes bundled
// with an attribute table.

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

static double get_y_intercept(BaseType *var)
{
    vector<string> attributes;
    attributes.push_back("add_offset");
    attributes.push_back("add_off");
    return get_attribute_double_value(var, attributes);
}

static double get_slope(BaseType *var)
{
    return get_attribute_double_value(var, "scale_factor");
}

static double get_missing_value(BaseType *var)
{
    return get_attribute_double_value(var, "missing_value");
}

BaseType *function_linear_scale_worker(BaseType *bt, double m, double b, double missing, bool use_missing)
{
    // Read the data, scale and return the result. Must replace the new data
    // in a constructor (i.e., Array part of a Grid).
    BaseType *dest = nullptr;
    double *data;
    if (bt->type() == dods_grid_c) {
        // Grab the whole Grid; note that the scaling is done only on the array part
        Grid &source = dynamic_cast<Grid&>(*bt);

        BESDEBUG("function", "function_linear_scale_worker() - Grid send_p: " << source.send_p() << endl);
        BESDEBUG("function",
                "function_linear_scale_worker() - Grid Array send_p: " << source.get_array()->send_p() << endl);

        // Read the grid; set send_p since Grid is a kind of constructor and
        // read will only be called on it's fields if their send_p flag is set
        source.set_send_p(true);
        source.read();

        // Get the Array part and read the values
        Array *a = source.get_array();
        //a->read();
        data = extract_double_array(a);

        // Now scale the data.
        int length = a->length();
        for (int i = 0; i < length; ++i)
            data[i] = data[i] * m + b;

        // Copy source Grid to result Grid. Could improve on this by not using this
        // trick since it copies all of 'source' to 'dest', including the main Array.
        // The next bit of code will replace those values with the newly scaled ones.
        Grid *result = new Grid(source);

        // Now load the transferred values; use Float64 as the new type of the result
        // Grid Array.
        result->get_array()->add_var_nocopy(new Float64(source.name()));
        result->get_array()->set_value(data, length);
        delete[] data;

        // FIXME result->set_send_p(true);
        BESDEBUG("function", "function_linear_scale_worker() - Grid send_p: " << source.send_p() << endl);
        BESDEBUG("function",
                "function_linear_scale_worker() - Grid Array send_p: " << source.get_array()->send_p() << endl);

        dest = result;
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

        data = extract_double_array(&source);
        int length = source.length();
        for (int i = 0; i < length; ++i)
            data[i] = data[i] * m + b;

        Array *result = new Array(source);

        result->add_var_nocopy(new Float64(source.name()));
        result->set_value(data, length);

        delete[] data; // val2buf copies.

        dest = result;
    }
    else if (bt->is_simple_type() && !(bt->type() == dods_str_c || bt->type() == dods_url_c)) {
        double data = extract_double_value(bt);
        if (!use_missing || !double_eq(data, missing)) data = data * m + b;

        Float64 *fdest = new Float64(bt->name());

        fdest->set_value(data);
        // dest->val2buf(static_cast<void*> (&data));
        dest = fdest;
    }
    else {
        throw Error(malformed_expr, "The linear_scale() function works only for numeric Grids, Arrays and scalars.");
    }

    return dest;
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
void function_dap2_linear_scale(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(linear_scale_info);
        *btpp = response;
        return;
    }

    // Check for 1 or 3 arguments: 1 --> use attributes; 3 --> m & b supplied
    DBG(cerr << "argc = " << argc << endl);
    if (!(argc == 1 || argc == 3 || argc == 4))
        throw Error(malformed_expr,
                "Wrong number of arguments to linear_scale(). See linear_scale() for more information");

    // Get m & b
    bool use_missing = false;
    double m, b, missing = 0.0;
    if (argc == 4) {
        m = extract_double_value(argv[1]);
        b = extract_double_value(argv[2]);
        missing = extract_double_value(argv[3]);
        use_missing = true;
    }
    else if (argc == 3) {
        m = extract_double_value(argv[1]);
        b = extract_double_value(argv[2]);
        use_missing = false;
    }
    else {
        m = get_slope(argv[0]);

        // This is really a hack; on a fair number of datasets, the y intercept
        // is not given and is assumed to be 0. Here the function looks and
        // catches the error if a y intercept is not found.
        try {
            b = get_y_intercept(argv[0]);
        }
        catch (Error &e) {
            b = 0.0;
        }

        // This is not the best plan; the get_missing_value() function should
        // do something other than throw, but to do that would require mayor
        // surgery on get_attribute_double_value().
        try {
            missing = get_missing_value(argv[0]);
            use_missing = true;
        }
        catch (Error &e) {
            use_missing = false;
        }
    }

    BESDEBUG("function",
            "function_dap2_linear_scale() - m: " << m << ", b: " << b << ", use_missing: " << use_missing << ", missing: " << missing << endl);

    *btpp = function_linear_scale_worker(argv[0], m, b, missing, use_missing);
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
BaseType *function_dap4_linear_scale(D4RValueList *args, DMR &dmr)
{
    BESDEBUG("function", "function_dap4_linear_scale()  BEGIN " << endl);

    // DAP4 function porting information: in place of 'argc' use 'args.size()'
    if (args == 0 || args->size() == 0) {
        Str *response = new Str("info");
        response->set_value(linear_scale_info);
        // DAP4 function porting: return a BaseType* instead of using the value-result parameter
        return response;
    }

    // Check for 2 arguments
    DBG(cerr << "args.size() = " << args.size() << endl);
    if (!(args->size() == 1 || args->size() == 3 || args->size() == 4))
        throw Error(malformed_expr,
                "Wrong number of arguments to linear_scale(). See linear_scale() for more information");

    // Get m & b
    bool use_missing = false;
    double m, b, missing = 0.0;
    if (args->size() == 4) {
        m = extract_double_value(args->get_rvalue(1)->value(dmr));
        b = extract_double_value(args->get_rvalue(2)->value(dmr));
        missing = extract_double_value(args->get_rvalue(3)->value(dmr));
        use_missing = true;
    }
    else if (args->size() == 3) {
        m = extract_double_value(args->get_rvalue(1)->value(dmr));
        b = extract_double_value(args->get_rvalue(2)->value(dmr));
        use_missing = false;
    }
    else {
        m = get_slope(args->get_rvalue(0)->value(dmr));

        // This is really a hack; on a fair number of datasets, the y intercept
        // is not given and is assumed to be 0. Here the function looks and
        // catches the error if a y intercept is not found.
        try {
            b = get_y_intercept(args->get_rvalue(0)->value(dmr));
        }
        catch (Error &e) {
            b = 0.0;
        }

        // This is not the best plan; the get_missing_value() function should
        // do something other than throw, but to do that would require mayor
        // surgery on get_attribute_double_value().
        try {
            missing = get_missing_value(args->get_rvalue(0)->value(dmr));
            use_missing = true;
        }
        catch (Error &e) {
            use_missing = false;
        }
    }
    BESDEBUG("function",
            "function_dap4_linear_scale() - m: " << m << ", b: " << b << ", use_missing: " << use_missing << ", missing: " << missing << endl);

    BESDEBUG("function", "function_dap4_linear_scale()  END " << endl);

    return function_linear_scale_worker(args->get_rvalue(0)->value(dmr), m, b, missing, use_missing);
}

} // namesspace functions
