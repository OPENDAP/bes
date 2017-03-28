// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003,2011,2012 OPeNDAP, Inc.
// Authors: James Gallagher <jgallagher@opendap.org>
//         Scott Moe <smeest1@gmail.com>
//         Bill Howe <billhowe@cs.washington.edu>
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

// NOTE: This file is built only when the gridfields library is linked with
// the netcdf_handler (i.e., the handler's build is configured using the
// --with-gridfields=... option to the 'configure' script).

#if 0 // Disabling cruft code from earlier ugrid work - ndp 03/25/2013

#include "config.h"

#include <limits.h>

#include <cstdlib>      // used by strtod()
#include <cerrno>
#include <cmath>
#include <iostream>
#include <sstream>

//#define DODS_DEBUG

#include "BaseType.h"
#include "Byte.h"
#include "Int16.h"
#include "UInt16.h"
#include "Int32.h"
#include "UInt32.h"
#include "Float32.h"
#include "Float64.h"
#include "Str.h"
#include "Url.h"
#include "Array.h"
#include "Structure.h"
#include "Sequence.h"
#include "Grid.h"
#include "Error.h"

#include "debug.h"
#include "util.h"

#include <gridfields/restrict.h>
#include <gridfields/gridfield.h>
#include <gridfields/grid.h>
#include <gridfields/cell.h>
#include <gridfields/cellarray.h>
#include <gridfields/array.h>
#include <gridfields/implicit0cells.h>
#include <gridfields/gridfieldoperator.h>

//  We wrapped VC++ 6.x strtod() to account for a short coming
//  in that function in regards to "NaN".  I don't know if this
//  still applies in more recent versions of that product.
//  ROM - 12/2007
#ifdef WIN32
#include <limits>
double w32strtod(const char *, char **);
#endif

using namespace std;
using namespace libdap;

/** Given a BaseType pointer, extract the string value it contains and return
 it.

 @param arg The BaseType pointer
 @return A C++ string
 @exception Error thrown if the referenced BaseType object does not contain
 a DAP String. */
static string extract_string_argument(BaseType * arg)
{
    if (arg->type() != dods_str_c)
        throw Error(malformed_expr, "The function requires a DAP string argument.");

    if (!arg->read_p())
        throw InternalErr(__FILE__, __LINE__,
                "The CE Evaluator built an argument list where some constants held no values.");

    string s = dynamic_cast<Str&>(*arg).value();

    DBG(cerr << "s: " << s << endl);

    return s;
}

template<class T>
static void set_array_using_double_helper(Array * a, double *src, int src_len)
{
    T *values = new T[src_len];
    for (int i = 0; i < src_len; ++i)
        values[i] = (T) src[i];

    a->set_value(values, src_len);

    delete[] values;
}

template<typename DODS, typename T>
static T *extract_array_helper(Array *a)
{
    int length = a->length();

    DBG(cerr << "Allocating..." << length << endl);
    DODS *b = new DODS[length];

    DBG(cerr << "Assigning value..." << endl);
    a->value(b);

    DBG(cerr << "array values extracted.  Casting..." << endl);
    T *dest = new T[length];

    for (int i = 0; i < length; ++i)
        dest[i] = (T) b[i];
    delete[]b;

    DBG(cerr << "Returning extracted values." << endl);

    return dest;
}

/**
 * Extract data from a DAP array and return those values in a gridfields
 * array. This function sets the \e send_p property of the DAP Array and
 * uses its \e read() member function to get values. Thus, it should work
 * for values stored in any type of data source (e.g., file) for which the
 * Array class has been specialized.
 *
 * @param a The DAP Array. Extract values from this array
 * @return A GF::Array
 */
static GF::Array *extract_gridfield_array(Array *a) {
    if ((a->type() == dods_array_c && !a->var()->is_simple_type())
            || a->var()->type() == dods_str_c || a->var()->type() == dods_url_c)
    throw Error(malformed_expr,
            "The function requires a DAP numeric-type array argument.");

    a->set_send_p(true);
    a->read();

    // Construct a GridField array from a DODS array
    GF::Array *gfa;

    switch (a->var()->type()) {
        case dods_byte_c:
            gfa = new GF::Array(a->var()->name(), GF::INT);
            gfa->shareIntData(extract_array_helper<dods_byte, int>(a), a->length());
            break;
        case dods_uint16_c:
            gfa = new GF::Array(a->var()->name(), GF::INT);
            gfa->shareIntData(extract_array_helper<dods_uint16, int>(a), a->length());
            break;
        case dods_int16_c:
            gfa = new GF::Array(a->var()->name(), GF::INT);
            gfa->shareIntData(extract_array_helper<dods_int16, int>(a), a->length());
            break;
        case dods_uint32_c:
            gfa = new GF::Array(a->var()->name(), GF::INT);
            gfa->shareIntData(extract_array_helper<dods_uint32, int>(a), a->length());
            break;
        case dods_int32_c:
            gfa = new GF::Array(a->var()->name(), GF::INT);
            gfa->shareIntData(extract_array_helper<dods_int32, int>(a), a->length());
            break;
        case dods_float32_c:
            gfa = new GF::Array(a->var()->name(), GF::FLOAT);
            gfa->shareFloatData(extract_array_helper<dods_float32, float>(a), a->length());
            break;
        case dods_float64_c:
            gfa = new GF::Array(a->var()->name(), GF::FLOAT);
            gfa->shareFloatData(extract_array_helper<dods_float64, float>(a), a->length());
            break;
        default:
            throw InternalErr(__FILE__, __LINE__, "Unknown DDS type encountered when converting to gridfields array");
    }
    return gfa;
};

/*
 If the array has the exact dimensions in the vector dims, in the same order,
 return true.  Otherwise return false.

 */
static bool same_dimensions(Array *arr, vector<Array::dimension> &dims) {
    vector<Array::dimension>::iterator dit;
    Array::Dim_iter ait;
    DBG(cerr << "same_dimensions test for array " << arr->name() << endl);
    DBG(cerr << "  array dims: ");
    for (ait = arr->dim_begin(); ait!=arr->dim_end(); ++ait) {
        DBG(cerr << (*ait).name << ", ");
    }
    DBG(cerr << endl);
    DBG(cerr << "  rank dims: ");
    for (dit = dims.begin(); dit!=dims.end(); ++dit) {
        DBG(cerr << (*dit).name << ", " << endl);
        for (ait = arr->dim_begin(); ait!=arr->dim_end(); ++ait) {
            Array::dimension dd = *dit;
            Array::dimension ad = *ait;
            DBG(cout<<dd.name<<" "<<ad.name<<" "<<dd.size<<" "<<ad.size<<endl);
            if (dd.name != ad.name
                    or dd.size != ad.size
                    or dd.stride != ad.stride
                    or dd.stop != ad.stop)
            return false;
        }
        DBG(cerr << endl);
    }
    return true;
}

/** Given a pointer to an Array that holds a numeric type, extract the
 values and return in an array of T. This function allocates the
 array using 'new T[n]' so delete[] can be used when you are done
 the data. */
template<typename T>
static T *extract_array(Array * a)
{
    // Simple types are Byte, ..., Float64, String and Url.
    if ((a->type() == dods_array_c && !a->var()->is_simple_type())
            || a->var()->type() == dods_str_c || a->var()->type() == dods_url_c)
    throw Error(malformed_expr,
            "The function requires a DAP numeric-type array argument.");

    a->set_send_p(true);
    a->read();
    // This test should never pass due to the previous two lines; 
    // reading here seems to make 
    // sense rather than letting the caller forget to do so.
    // is read() idemopotent?
    if (!a->read_p())
    throw InternalErr(__FILE__, __LINE__,
            string("The Array '") + a->name() +
            "'does not contain values. send_read_p() not called?");

    // The types of arguments that the CE Parser will build for numeric
    // constants are limited to Uint32, Int32 and Float64. See ce_expr.y.
    // Expanded to work for any numeric type so it can be used for more than
    // just arguments.
    switch (a->var()->type()) {
        case dods_byte_c:
        return extract_array_helper<dods_byte, T>(a);

        case dods_uint16_c:
        DBG(cerr << "dods_uint32_c" << endl);
        return extract_array_helper<dods_uint16, T>(a);

        case dods_int16_c:
        DBG(cerr << "dods_int16_c" << endl);
        return extract_array_helper<dods_int16, T>(a);

        case dods_uint32_c:
        DBG(cerr << "dods_uint32_c" << endl);
        return extract_array_helper<dods_uint32, T>(a);

        case dods_int32_c:
        DBG(cerr << "dods_int32_c" << endl);
        return extract_array_helper<dods_int32, T>(a);

        case dods_float32_c:
        DBG(cerr << "dods_float32_c" << endl);
        // Added the following line. jhrg 8/7/12
        return extract_array_helper<dods_float32, T>(a);

        case dods_float64_c:
        DBG(cerr << "dods_float64_c" << endl);
        return extract_array_helper<dods_float64, T>(a);

        default:
        throw InternalErr(__FILE__, __LINE__,
                "The argument list built by the CE parser contained an unsupported numeric type.");
    }
}

/** Given a BaseType pointer, extract the numeric value it contains and return
 it in a C++ double.

 @param arg The BaseType pointer
 @return A C++ double
 @exception Error thrown if the referenced BaseType object does not contain
 a DAP numeric value. */
static double extract_double_value(BaseType * arg)
{
    // Simple types are Byte, ..., Float64, String and Url.
    if (!arg->is_simple_type() || arg->type() == dods_str_c || arg->type() == dods_url_c)
        throw Error(malformed_expr, "The function requires a DAP numeric-type argument.");

    if (!arg->read_p())
        throw InternalErr(__FILE__, __LINE__,
                "The CE Evaluator built an argument list where some constants held no values.");

    // The types of arguments that the CE Parser will build for numeric
    // constants are limited to Uint32, Int32 and Float64. See ce_expr.y.
    // Expanded to work for any numeric type so it can be used for more than
    // just arguments.
    switch (arg->type()) {
        case dods_byte_c:
            return (double) (dynamic_cast<Byte&>(*arg).value());
        case dods_uint16_c:
            return (double) (dynamic_cast<UInt16&>(*arg).value());
        case dods_int16_c:
            return (double) (dynamic_cast<Int16&>(*arg).value());
        case dods_uint32_c:
            return (double) (dynamic_cast<UInt32&>(*arg).value());
        case dods_int32_c:
            return (double) (dynamic_cast<Int32&>(*arg).value());
        case dods_float32_c:
            return (double) (dynamic_cast<Float32&>(*arg).value());
        case dods_float64_c:
            return dynamic_cast<Float64&>(*arg).value();
        default:
            throw InternalErr(__FILE__, __LINE__,
                    "The argument list built by the CE parser contained an unsupported numeric type.");
    }
}

#if 0
// These static functions could be moved to a class that provides a more
// general interface for COARDS/CF someday. Assume each BaseType comes bundled
// with an attribute table.

// These are included here because the ugrid code might want to use attribute
// values bound to various variables and this illustrates how that could be
// done. jhrg 8/20/12

// This was ripped from parser-util.cc
static double string_to_double(const char *val)
{
    char *ptr;
    errno = 0;
    // Clear previous value. 5/21/2001 jhrg

#ifdef WIN32
    double v = w32strtod(val, &ptr);
#else
    double v = strtod(val, &ptr);
#endif

    if ((v == 0.0 && (val == ptr || errno == HUGE_VAL || errno == ERANGE)) || *ptr != '\0') {
        throw Error(malformed_expr, string("Could not convert the string '") + val + "' to a double.");
    }

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
#endif

/** This is a stub Constraint Expression (i.e., server-side) function
 that will evolve into an interface for Unstructured Grid
 operations.

 The function takes a single variable that should be a one
 dimensional array variable and scales it by a factor of ten. The
 result is returned as the value of the CE expression.

 @param argc Count of the function's arguments
 @param argv Array of pointers to the functions arguments
 @param dds Reference to the DDS object for the complete dataset.
 This holds pointers to all of the variables and attributes in the
 dataset.
 @param btpp Return the function result in an instance of BaseType
 referenced by this pointer to a pointer. We could have used a
 BaseType reference, instead of pointer to a pointer, but we didn't.
 This is a value-result parameter.

 @return void

 @exception Error Thrown If the Array is not a one dimensional
 array. */
void
function_ugrid_restrict(int argc, BaseType * argv[], DDS &dds, BaseType **btpp)
{
    static string info =
    string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
    "<function name=\"ugrid_demo\" version=\"0.1\">\n" +
    "Fledgling code for Unstructured grid operations.\n" +
    "</function>";

    static string info2 =
    string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
    "<function name=\"ugrid_demo\" version=\"0.1\">\n" +
    "code for Unstructured grid operations.\n" +
    "</function>";
    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        return;
    }

    if (argc == 7) {
        Str *response = new Str("info2");
        response->set_value(info);
        *btpp = response;
        return;
    }

    // Check number of arguments; DBG is a macro. Use #define
    // DODS_DEBUG to activate the debugging stuff.
    if (argc != 2)
        throw Error(malformed_expr,"Wrong number of arguments to ugrid_demo. ugrid_demo(dim:int32, condition:string); was passed " + long_to_string(argc) + " argument(s)");

    if (argv[0]->type() != dods_int32_c)
        throw Error(malformed_expr,"Wrong type for first argument. ugrid_demo(dim:int32, condition:string); was passed a/an " + argv[0]->type_name());

    if (argv[1]->type() != dods_str_c)
        throw Error(malformed_expr,"Wrong type for second argument. ugrid_demo(dim:int32, condition:string); was passed a/an " + argv[1]->type_name());

    // keep track of which DDS dimensions correspond to GF dimensions

    map<GF::Dim_t, vector<Array::dimension> > rank_dimensions;

    GF::Grid *G = new GF::Grid("result");

    // 1) Find the nodes
    DBG(cerr << "Reading 0-cells" << endl);
    GF::AbstractCellArray *nodes = NULL;

    for (DDS::Vars_iter vi = dds.var_begin(); vi != dds.var_end(); vi++) {

        BaseType *bt = *vi;
        // TODO allow variables that are not arrays; just ignore them
        Array &arr = dynamic_cast<Array&>(*bt);
        AttrTable &at = arr.get_attr_table();
        DBG(cerr << "Array: " << arr.name() << endl);

        int node_count = -1;    //error condition
        AttrTable::Attr_iter loc = at.simple_find("grid_location");

        if (loc != at.attr_end()) {

            if (at.get_attr(loc, 0) == "node") {
                node_count = 1;
                Array::Dim_iter di = arr.dim_begin();
                DBG(cerr << "Interpreting 0-cells from dimensions: ");
                rank_dimensions[0] = vector<Array::dimension>();
                for (Array::Dim_iter di = arr.dim_begin(); di!= arr.dim_end(); di++) {
                    // These dimensions define the nodes.
                    DBG(cerr << di->name << ", ");
                    rank_dimensions[0].push_back(*di);
                    node_count *= di->c_size;
                }
                DBG(cerr << endl);

                nodes = new GF::Implicit0Cells(node_count);
                break;
            } // Bound to nodes?
        } // Has a "grid_location" attribute?
    }

    if (!nodes)
        throw Error("Could not find a grid_location attribute and/or its node set.");

    // Attach the nodes to the grid
    G->setKCells(nodes, 0);

    // 2) For each k, find the k-cells
    // k = 2, for now
    DBG(cerr << "Reading 2-cells" << endl);
    GF::CellArray *twocells = NULL;

    for (DDS::Vars_iter vi = dds.var_begin(); vi != dds.var_end(); vi++) {
        BaseType *bt = *vi;
        // TODO Allow variables that are note Arrays; ignore as above
        Array &arr = dynamic_cast<Array&>(*bt);
        DBG(cerr << "Array: " << arr.name() << endl);

        AttrTable &at = arr.get_attr_table();

        AttrTable::Attr_iter iter_cell_type = at.simple_find("cell_type");

        if (iter_cell_type != at.attr_end()) {
            string cell_type = at.get_attr(iter_cell_type, 0);
            DBG(cerr << cell_type << endl);
            if (cell_type == "tri_ccw") {
                // Ok, we expect triangles
                // which means a shape of 3xN
                int twocell_count = -1, i=0;
                int total_size = 1;
                rank_dimensions[2] = vector<Array::dimension>();
                for (Array::Dim_iter di = arr.dim_begin(); di!= arr.dim_end(); di++) {
                    total_size *= di->c_size;
                    rank_dimensions[2].push_back(*di);
                    if (i == 0) {
                        if (di->c_size != 3) {
                            DBG(cerr << "Cell array of type 'tri_ccw' must have a shape of 3xN, since triangles have three nodes." << endl);
                            throw Error(malformed_expr,"Cell array of type 'tri_ccw' must have a shape of 3xN, since triangles have three nodes.");
                        }
                    }
                    if (i == 1) {
                        twocell_count = di->c_size;
                    }
                    if (i>1) {
                        DBG(cerr << "Too many dimensions for a cell array of type 'tri_ccw'.  Expected shape of 3XN" << endl);
                        throw Error(malformed_expr,"Too many dimensions for a cell array of type 'tri_ccw'.  Expected shape of 3XN");
                    }
                    i++;
                }

                // interpret the array data as triangles
                GF::Node *cellids = extract_array<GF::Node>(&arr);
                GF::Node *cellids2 = extract_array<GF::Node>(&arr);
                for (int j=0;j<twocell_count;j++)
                {   cellids[3*j]=cellids2[j];
                    cellids[3*j+1]=cellids2[j+twocell_count];
                    cellids[3*j+2]=cellids2[j+2*twocell_count];
                }

                // adjust for index origin
                AttrTable::Attr_iter iter_index_origin = at.simple_find("index_origin");
                if (iter_index_origin != at.attr_end()) {
                    DBG(cerr << "Found an index origin attribute." << endl);
                    AttrTable::entry *index_origin_entry = *iter_index_origin;
                    int index_origin;
                    if (index_origin_entry->attr->size() == 1) {
                        AttrTable::entry *index_origin_entry = *iter_index_origin;
                        string val = (*index_origin_entry->attr)[0];
                        DBG(cerr << "Value: " << val << endl);
                        stringstream buffer(val);
                        // what happens if string cannot be converted to an integer?
                        buffer >> index_origin;
                        DBG(cerr << "converted: " << index_origin << endl);
                        if (index_origin != 0) {
                            for (int j=0; j<total_size; j++) {
                                cellids[j] -= index_origin;
                            }
                        }
                    }
                    else {
                        throw Error(malformed_expr,"Index origin attribute exists, but either no value supplied, or more than one value supplied.");
                    }
                }

                // Create the cell array
                twocells = new GF::CellArray(cellids, twocell_count, 3);

                // Attach it to the grid
                G->setKCells(twocells, 2);
            }
        }
    }

    if (!twocells)
        throw Error("Could not find cell array of CCW triangles");

    // 3) For each var, bind it to the appropriate dimension

    // For each variable in the data source:
    GF::GridField *input = new GF::GridField(G);

    for (DDS::Vars_iter vi = dds.var_begin(); vi != dds.var_end(); vi++) {
        BaseType *bt = *vi;

        if (bt->type() == dods_array_c) {
            Array *arr = (Array *)bt;
            DBG(cerr << "Data Array: " << arr->name() << endl);
            GF::Array *gfa = extract_gridfield_array(arr);

            // Each rank is associated with a sequence of dimensions
            // Vars that have the same dimensions should be bound to the grid at that rank
            // (Note that in gridfields, Dimension and rank are synonyms.  We
            // use the latter here to avoid confusion).
            map<GF::Dim_t, vector<Array::dimension> >::iterator iter;
            for( iter = rank_dimensions.begin(); iter != rank_dimensions.end(); ++iter ) {
                bool same = same_dimensions(arr, iter->second);
                if (same) {
                    // This var should be bound to rank k
                    DBG(cerr << "Adding Attribute: " << gfa->sname() << endl);
                    input->AddAttribute(iter->first, gfa);
                }
                else {
                    // This array does not appear to be associated with any
                    // rank of the unstructured grid. Ignore for now.
                    // TODO Anything else we should do?
                }
            }
        } // Ignore if not an array type. Anything else we should do?
    }

    int dim = extract_double_value(argv[0]);
    string projection = extract_string_argument(argv[1]);
    int nodenumber=input->Card(0);

    GF::RestrictOp op = GF::RestrictOp(projection, dim, input);
    GF::GridField *R = new GF::GridField(op.getResult());

    // 4) Convert back to a DDS BaseType

    // Create variables for each cell dimension
    // Create variables for each attribute at each rank

    R->GetGrid()->normalize();

    Structure *construct = new Structure("construct");
    for (DDS::Vars_iter vi = dds.var_begin(); vi != dds.var_end(); vi++) {
        BaseType *bt = *vi;
        if (bt->type() == dods_array_c) {
            Array *arr = (Array *)bt;
            map<GF::Dim_t, vector<Array::dimension> >::iterator iter;

            AttrTable &arrattr2 = arr->get_attr_table();

            if(arrattr2.simple_find("cell_type")!=arrattr2.attr_end())
            {
                GF::CellArray* Inb=(GF::CellArray*)(R->GetGrid()->getKCells(2));
                Int32 *witnessn4 = new Int32(arr->name());
                Array *Nodes = new Array(arr->name(),witnessn4);
                vector< vector<int> > nodes2 = Inb->makeArrayInts();
                vector<dods_int32> node1;
                vector<dods_int32> node2;
                vector<dods_int32> node3;
                for (int j=0;j<nodes2.size();j++) {
                    node1.push_back(nodes2.at(j).at(0));
                    node2.push_back(nodes2.at(j).at(1));
                    node3.push_back(nodes2.at(j).at(2));
                }
                Int32 *witnessn1=new Int32("nodes1");
                Int32 *witnessn2=new Int32("nodes2");
                Int32 *witnessn3=new Int32("nodes3");
                Array *Node1=new Array("trinode1",witnessn1);
                Array *Node2=new Array("trinode2",witnessn2);
                Array *Node3=new Array("trinode3",witnessn3);
                Node1->append_dim(node1.size(),"dim-1");

                Node2->append_dim(node2.size(),"dim-1");
                Node3->append_dim(node3.size(),"dim-1");

                Node1->set_value(node1,node1.size());
                Node2->set_value(node2,node2.size());
                Node3->set_value(node3,node3.size());

                Nodes->append_dim(3,"three");
                Nodes->append_dim(node1.size(),"tris");
                Nodes->reserve_value_capacity(3*node1.size());
                Nodes->set_value_slice_from_row_major_vector(*Node1,0);
                Nodes->set_value_slice_from_row_major_vector(*Node2,Node1->length());
                Nodes->set_value_slice_from_row_major_vector(*Node3,Node1->length()+Node2->length());
                AttrTable &arrattr1 = arr->get_attr_table();
                Nodes->set_attr_table(arrattr1);
                construct->add_var_nocopy(Nodes);
            }
            else {
                for( iter = rank_dimensions.begin(); iter != rank_dimensions.end(); ++iter ) {
                    bool same = same_dimensions(arr, iter->second);
                    if (same) {
                        // This var should be bound to rank k
                        Float64 *witness2=new Float64(arr->name());

                        GF::Array* gfa=R->GetAttribute(iter->first, arr->name());

                        vector<dods_float64> GFA = gfa->makeArrayf();

                        Array *Nodes = new Array(arr->name(), witness2);
                        Nodes->append_dim(GFA.size(), "nodes");
                        Nodes->set_value(GFA,GFA.size());

                        AttrTable &arrattr1 = arr->get_attr_table();
                        Nodes->set_attr_table(arrattr1);
                        // AttrTable &arrattr = Nodes->get_attr_table();
                        construct->add_var_nocopy(Nodes);
                    }
                    else {
                        // This array does not appear to be associated with
                        // any rank of the unstructured grid. Ignore for now.
                        // Anything else we should do?
                    }
                }
            }
        }
    }

#if 0
    for (DDS::Vars_iter vi = dds.var_begin(); vi != dds.var_end(); vi++) {
        BaseType *bt = *vi;
        if (bt->type() == dods_array_c) {
            Array *arr = (Array *)bt;
            map<GF::Dim_t, vector<Array::dimension> >::iterator iter;
            for( iter = rank_dimensions.begin(); iter != rank_dimensions.end(); ++iter ) {
                bool same = same_dimensions(arr, iter->second);
                if (same) {
                    GF::Array* gfa = R->GetAttribute(iter->first, arr->name());
                }
                else {
                    //This array does not appear to be associated with any
                    // rank of the unstructured grid.Ignore for now.
                    // Anything else we should do?
                }

            }
        }
    }
#endif
    // TODO Needed?
    //GF::Grid *newgrid = R->GetGrid();

    *btpp = construct;

    return;
}

//#endif


#endif // Disabling cruft code from earlier ugrid work - ndp 03/25/2013
