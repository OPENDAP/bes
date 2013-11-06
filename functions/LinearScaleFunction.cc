
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
#include <Array.h>
#include <Grid.h>

#include <Error.h>
#include <DDS.h>

#include <debug.h>
#include <util.h>

#include "LinearScaleFunction.h"

namespace libdap {

// These static functions could be moved to a class that provides a more
// general interface for COARDS/CF someday. Assume each BaseType comes bundled
// with an attribute table.

static double string_to_double(const char *val)
{
#if 0
	char *ptr;
    errno = 0;
    // Clear previous value. 5/21/2001 jhrg

#ifdef WIN32
    double v = w32strtod(val, &ptr);
#else
    double v = strtod(val, &ptr);
#endif

    if ((v == 0.0 && (val == ptr || errno == HUGE_VAL || errno == ERANGE))
            || *ptr != '\0') {
        throw Error(malformed_expr,string("Could not convert the string '") + val + "' to a double.");
    }
#endif

    istringstream iss(val);
    double v;
    iss >> v;

    double abs_val = fabs(v);
    if (abs_val > DODS_DBL_MAX || (abs_val != 0.0 && abs_val < DODS_DBL_MIN))
        throw Error(malformed_expr,string("Could not convert the string '") + val + "' to a double.");

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
        if (!values.empty())
            values += ", ";
        attribute_value = attr.get_attr(*i++);
    }

    // If the value string is empty, then look at the grid's array (if it's a
    // grid) or throw an Error.
    if (attribute_value.empty()) {
        if (var->type() == dods_grid_c)
            return get_attribute_double_value(dynamic_cast<Grid&>(*var).get_array(), attributes);
        else
            throw Error(malformed_expr,string("No COARDS/CF '") + values.substr(0, values.length() - 2)
                    + "' attribute was found for the variable '"
                    + var->name() + "'.");
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
            throw Error(malformed_expr,string("No COARDS '") + attribute
                    + "' attribute was found for the variable '"
                    + var->name() + "'.");
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
void
function_linear_scale(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    string info =
    string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
    "<function name=\"linear_scale\" version=\"1.0b1\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#linear_scale\">\n" +
    "</function>";

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        return;
    }

    // Check for 1 or 3 arguments: 1 --> use attributes; 3 --> m & b supplied
    DBG(cerr << "argc = " << argc << endl);
    if (!(argc == 1 || argc == 3 || argc == 4))
        throw Error(malformed_expr,"Wrong number of arguments to linear_scale(). See linear_scale() for more information");

    // Get m & b
    bool use_missing = false;
    double m, b, missing = 0.0;
    if (argc == 4) {
        m = extract_double_value(argv[1]);
        b = extract_double_value(argv[2]);
        missing = extract_double_value(argv[3]);
        use_missing = true;
    } else if (argc == 3) {
        m = extract_double_value(argv[1]);
        b = extract_double_value(argv[2]);
        use_missing = false;
    } else {
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

    DBG(cerr << "m: " << m << ", b: " << b << endl);DBG(cerr << "use_missing: " << use_missing << ", missing: " << missing << endl);

    // Read the data, scale and return the result. Must replace the new data
    // in a constructor (i.e., Array part of a Grid).
    BaseType *dest = 0;
    double *data;
    if (argv[0]->type() == dods_grid_c) {
#if 0
        // For a Grid, the function scales only the Array part.
        Array *source = dynamic_cast<Grid*>(argv[0])->get_array();
        //argv[0]->set_send_p(true);
             //source->set_send_p(true);
        source->read();
        data = extract_double_array(source);
        int length = source->length();
        for (int i = 0; i < length; ++i)
            data[i] = data[i] * m + b;
#if 0
        int i = 0;
        while (i < length) {
            DBG2(cerr << "data[" << i << "]: " << data[i] << endl);
            if (!use_missing || !double_eq(data[i], missing))
                data[i] = data[i] * m + b;
            DBG2(cerr << " >> data[" << i << "]: " << data[i] << endl);
            ++i;
        }
#endif
        // Vector::add_var will delete the existing 'template' variable
        Float64 *temp_f = new Float64(source->name());
        source->add_var(temp_f);

#ifdef VAL2BUF
        source.val2buf(static_cast<void*>(data), false);
#else
        source->set_value(data, length);
#endif
        delete [] data; // val2buf copies.
        delete temp_f; // add_var copies and then adds.
        dest = argv[0];
        dest->set_send_p(true);
#endif
        // Grab the whole Grid; note that the scaling is done only on the array part
        Grid &source = dynamic_cast<Grid&>(*argv[0]);

        DBG(cerr << "Grid send_p: " << source.send_p() << endl);
        DBG(cerr << "Grid Array send_p: " << source.get_array()->send_p() << endl);

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
#if 0
        // read the maps so that those values will be copied when the source Grid
        // is copied to the dest Grid
        Grid::Map_iter s = source.map_begin();
        while (s != source.map_end()) {
            static_cast<Array*>(*s)->read();
            ++s;
        }
#endif
        // Copy source Grid to result Grid. Could improve on this by not using this
        // trick since it copies all of 'source' to 'dest', including the main Array.
        // The next bit of code will replace those values with the newly scaled ones.
        Grid *result = new Grid(source);

        // Now load the transferred values; use Float64 as the new type of the result
        // Grid Array.
        result->get_array()->add_var_nocopy(new Float64(source.name()));
        result->get_array()->set_value(data, length);
        delete[] data;

#if 0
        // Now set the maps (NB: the copy constructor does not copy data)
        Grid::Map_iter s = source.map_begin();
        Grid::Map_iter d = result->map_begin();
        while (s != source.map_end()) {
            Array *a = static_cast<Array*>(*s);
            a->read();
            switch(a->var()->type()) {
            case dods_byte_c: {
                vector<dods_byte> v(a->length());
                a->value(&v[0]);
                static_cast<Array*>(*d)->set_value(v, v.size());
                break;
            }
            case dods_float32_c: {
                vector<dods_float32> v(a->length());
                a->value(&v[0]);
                static_cast<Array*>(*d)->set_value(v, a->length());
                break;
            }
            default:
                throw Error("Non-numeric Grid Map not supported by linear_scale().");
            }
            ++s; ++d;
        }
#endif

        // FIXME result->set_send_p(true);
        DBG(cerr << "Grid send_p: " << result->send_p() << endl);
        DBG(cerr << "Grid Array send_p: " << result->get_array()->send_p() << endl);

        dest = result;
    }
    else if (argv[0]->is_vector_type()) {
#if 0
        Array &source = dynamic_cast<Array&> (*argv[0]);
        source.set_send_p(true);
        // If the array is really a map, make sure to read using the Grid
        // because of the HDF4 handler's odd behavior WRT dimensions.
        if (source.get_parent() && source.get_parent()->type() == dods_grid_c)
            source.get_parent()->read();
        else
            source.read();

        data = extract_double_array(&source);
        int length = source.length();
        int i = 0;
        while (i < length) {
            if (!use_missing || !double_eq(data[i], missing))
                data[i] = data[i] * m + b;
            ++i;
        }

        Float64 *temp_f = new Float64(source.name());
        source.add_var(temp_f);

        source.val2buf(static_cast<void*>(data), false);

        delete [] data; // val2buf copies.
        delete temp_f; // add_var copies and then adds.

        dest = argv[0];
#endif
        Array &source = dynamic_cast<Array&>(*argv[0]);
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
    else if (argv[0]->is_simple_type() && !(argv[0]->type() == dods_str_c || argv[0]->type() == dods_url_c)) {
        double data = extract_double_value(argv[0]);
        if (!use_missing || !double_eq(data, missing))
            data = data * m + b;

        Float64 *fdest = new Float64(argv[0]->name());

        fdest->set_value(data);
        // dest->val2buf(static_cast<void*> (&data));
        dest = fdest;
    } else {
        throw Error(malformed_expr,"The linear_scale() function works only for numeric Grids, Arrays and scalars.");
    }

    *btpp = dest;
    return;
}

} // namesspace libdap
