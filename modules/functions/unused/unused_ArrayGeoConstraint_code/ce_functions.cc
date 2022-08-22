// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

// (c) COPYRIGHT URI/MIT 1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>


// These functions are used by the CE evaluator
//
// 1/15/99 jhrg

#include "config.h"

#include <limits.h>

#include <cstdlib>      // used by strtod()
#include <cerrno>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>

//#define DODS_DEBUG

#undef FUNCTION_DAP	// undef so the dap() function always returns an error;
// use keywords instead.

#include <libdap/BaseType.h>
#include <libdap/Byte.h>
#include <libdap/Int16.h>
#include <libdap/UInt16.h>
#include <libdap/Int32.h>
#include <libdap/UInt32.h>
#include <libdap/Float32.h>
#include <libdap/Float64.h>
#include <libdap/Str.h>
#include <libdap/Url.h>
#include <libdap/Array.h>
#include <libdap/Structure.h>
#include <libdap/Sequence.h>
#include <libdap/Grid.h>
#include <libdap/ServerFunctionsList.h>

#include "BESDebug.h"
#include <libdap/Error.h>

#include "grid_utils.h"

#include <libdap/RValue.h>
#include "GSEClause.h"
#include "GridGeoConstraint.h"
//#include "ArrayGeoConstraint.h"

#include <functions/unused_ArrayGeoConstraint_code/ce_functions.h>
#ifdef GRIDFIELDS
#include "gridfields_functions.h"
#include "ugridFunctions/gf2.h"
#include "ugridFunctions/gf3.h"
#endif

#include "gse_parser.h"
#include "gse.tab.hh"
#include <libdap/debug.h>
#include <libdap/util.h>

// GRIDFIELDS should never be defined since this was moved to the netcdf
// handler. 4/19/12 jhrg
#ifdef GRIDFIELDS
#include <gridfields/restrict.h>
#include <gridfields/gridfield.h>
#include <gridfields/grid.h>
#include <gridfields/cell.h>
#include <gridfields/cellarray.h>
#include <gridfields/array.h>
#include <gridfields/implicit0cells.h>
#include <gridfields/gridfieldoperator.h>
#endif

//  We wrapped VC++ 6.x strtod() to account for a short coming
//  in that function in regards to "NaN".  I don't know if this
//  still applies in more recent versions of that product.
//  ROM - 12/2007
#ifdef WIN32
#include <limits>
double w32strtod(const char *, char **);
#endif

using namespace std;

#if 0
int gse_parse(void *arg);
void gse_restart(FILE * in);

// Glue routines declared in gse.lex
// void gse_switch_to_buffer(void *new_buffer);
void gse_delete_buffer(void *buffer);
void *gse_string(const char *yy_str);
#endif

namespace libdap {
#if 0
/** Is \e lhs equal to \e rhs? Use epsilon to determine equality. */
inline bool double_eq(double lhs, double rhs, double epsilon = 1.0e-5)
{
    return fabs(lhs - rhs) < epsilon;
}

/** Given a BaseType pointer, extract the string value it contains and return
 it.

 @param arg The BaseType pointer
 @return A C++ string
 @exception Error thrown if the referenced BaseType object does not contain
 a DAP String. */
string extract_string_argument(BaseType * arg)
{
    if (arg->type() != dods_str_c)
        throw Error(malformed_expr,
                "The function requires a DAP string argument.");

    if (!arg->read_p())
        throw InternalErr(__FILE__, __LINE__,
                "The CE Evaluator built an argument list where some constants held no values.");

    string s = dynamic_cast<Str&>(*arg).value();

    DBG(cerr << "s: " << s << endl);

    return s;
}

// @todo Replace new with vector<T> (vector<T> values(src_len);)
template<class T> static void set_array_using_double_helper(Array * a, double *src, int src_len)
{
    T *values = new T[src_len];
    for (int i = 0; i < src_len; ++i)
        values[i] = (T) src[i];

#ifdef VAL2BUF
    a->val2buf(values, true);
#else
    a->set_value(values, src_len);
#endif

    delete[]values;
}

/** Given an array that holds some sort of numeric data, load it with values
 using an array of doubles. This function makes several assumptions. First,
 it assumes the caller really wants to put the doubles into whatever types
 the array holds! Caveat emptor. Second, it assumes that if the size of
 source (\e src) array is different than the destination (\e dest) the
 caller has made a mistake. In that case it will throw an Error object.

 After setting the values, this method sets the \c read_p property for
 \e dest. Setting \e read_p tells the serialization methods in libdap
 that this variable already holds data values and, given that, the
 serialization code will not try to read the values.

 @param dest An Array. The values are written to this array, reusing
 its storage. Existing values are lost.
 @param src The source data.
 @param src_len The number of elements in the \e src array.
 @exception Error Thrown if \e dest is not a numeric-type array (Byte, ...,
 Float64) or if the number of elements in \e src does not match the number
 is \e dest. */
void set_array_using_double(Array * dest, double *src, int src_len)
{
    // Simple types are Byte, ..., Float64, String and Url.
    if ((dest->type() == dods_array_c && !dest->var()->is_simple_type()) 
	|| dest->var()->type() == dods_str_c 
	|| dest->var()->type() == dods_url_c)
        throw InternalErr(__FILE__, __LINE__,
                "The function requires a DAP numeric-type array argument.");

    // Test sizes. Note that Array::size() takes any constraint into account
    // when it returns the length. Even if this was removed, the 'helper'
    // function this uses calls Vector::val2buf() which uses Vector::width()
    // which in turn uses size().
    if (dest->size() != src_len)
        throw InternalErr(__FILE__, __LINE__,
                "The source and destination array sizes don't match ("
                + long_to_string(src_len) + " versus "
                + long_to_string(dest->size()) + ").");

    // The types of arguments that the CE Parser will build for numeric
    // constants are limited to Uint32, Int32 and Float64. See ce_expr.y.
    // Expanded to work for any numeric type so it can be used for more than
    // just arguments.
    switch (dest->var()->type()) {
    case dods_byte_c:
        set_array_using_double_helper<dods_byte>(dest, src, src_len);
        break;
    case dods_uint16_c:
        set_array_using_double_helper<dods_uint16>(dest, src, src_len);
        break;
    case dods_int16_c:
        set_array_using_double_helper<dods_int16>(dest, src, src_len);
        break;
    case dods_uint32_c:
        set_array_using_double_helper<dods_uint32>(dest, src, src_len);
        break;
    case dods_int32_c:
        set_array_using_double_helper<dods_int32>(dest, src, src_len);
        break;
    case dods_float32_c:
        set_array_using_double_helper<dods_float32>(dest, src, src_len);
        break;
    case dods_float64_c:
        set_array_using_double_helper<dods_float64>(dest, src, src_len);
        break;
    default:
        throw InternalErr(__FILE__, __LINE__,
                "The argument list built by the CE parser contained an unsupported numeric type.");
    }

    // Set the read_p property.
    dest->set_read_p(true);
}

#ifdef GRIDFIELDS
template<typename DODS, typename T> T *extract_array_helper(Array *a) 
{
  DBG(cerr << "Extracting array values..." << endl);
  int length = a->size();

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
#endif // GRIDFIELDS

#ifdef GRIDFIELDS
GF::Array *extract_gridfield_array(Array *a) {
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
       gfa->shareIntData(extract_array_helper<dods_byte, int>(a), a->size());
       break;
    case dods_uint16_c:
       gfa = new GF::Array(a->var()->name(), GF::INT);
       gfa->shareIntData(extract_array_helper<dods_uint16, int>(a), a->size());
       break;
    case dods_int16_c:
       gfa = new GF::Array(a->var()->name(), GF::INT);
       gfa->shareIntData(extract_array_helper<dods_int16, int>(a), a->size());
       break;
    case dods_uint32_c:
       gfa = new GF::Array(a->var()->name(), GF::INT);
       gfa->shareIntData(extract_array_helper<dods_uint32, int>(a), a->size());
       break;
    case dods_int32_c:
       gfa = new GF::Array(a->var()->name(), GF::INT);
       gfa->shareIntData(extract_array_helper<dods_int32, int>(a), a->size());
       break;
    case dods_float32_c:
       gfa = new GF::Array(a->var()->name(), GF::FLOAT);
       gfa->shareFloatData(extract_array_helper<dods_float32, float>(a), a->size());
       break;
    case dods_float64_c:
       gfa = new GF::Array(a->var()->name(), GF::FLOAT);
       gfa->shareFloatData(extract_array_helper<dods_float64, float>(a), a->size());
       break;
    default:
        throw InternalErr(__FILE__, __LINE__,
                "Unknown DDS type encountered when converting to gridfields array");
    }
    return gfa;
};
#endif // GRIDFIELDS

#ifdef GRIDFIELDS
/*
If the array has the exact dimensions in the vector dims, in the same order, 
return true.  Otherwise return false. 

*/
bool same_dimensions(Array *arr, vector<Array::dimension> &dims) {
  vector<Array::dimension>::iterator dit;
  Array::Dim_iter ait;
  DBG(cerr << "same_dimensions test for array " << arr->name() << endl);
  DBG(cerr << << "  array dims: ");
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
#endif // GRIDFIELDS

#ifdef GRIDFIELDS
/** Given a pointer to an Array which holds a numeric type, extract the
 values and return in an array of T. This function allocates the
 array using 'new T[n]' so delete[] can be used when you are done
 the data. */
template<typename T>
T *extract_array(Array * a)
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
        return extract_array_helper<dods_float32, T>(a);
    case dods_float64_c:
        DBG(cerr << "dods_float64_c" << endl);
        return extract_array_helper<dods_float64, T>(a);
    default:
        throw InternalErr(__FILE__, __LINE__,
                "The argument list built by the CE parser contained an unsupported numeric type.");
    }
}

#endif // GRIDFIELDS

template<class T> static double *extract_double_array_helper(Array * a)
{
    int length = a->size();
    // Could improve this using vector<T>. jhrg
    T *b = new T[length];
    a->value(b);

    double *dest = new double[length];
    for (int i = 0; i < length; ++i)
        dest[i] = (double) b[i];
    delete[]b;

    return dest;
}

/** Given a pointer to an Array which holds a numeric type, extract the
 values and return in an array of doubles. This function allocates the
 array using 'new double[n]' so delete[] MUST be used when you are done
 the data. */
double *extract_double_array(Array * a)
{
    // Simple types are Byte, ..., Float64, String and Url.
    if ((a->type() == dods_array_c && !a->var()->is_simple_type())
	|| a->var()->type() == dods_str_c || a->var()->type() == dods_url_c)
        throw Error(malformed_expr,
                "The function requires a DAP numeric-type array argument.");

    if (!a->read_p())
        throw InternalErr(__FILE__, __LINE__,
                string("The Array '") + a->name() +
                "'does not contain values.");

    // The types of arguments that the CE Parser will build for numeric
    // constants are limited to Uint32, Int32 and Float64. See ce_expr.y.
    // Expanded to work for any numeric type so it can be used for more than
    // just arguments.
    switch (a->var()->type()) {
    case dods_byte_c:
        return extract_double_array_helper<dods_byte>(a);
    case dods_uint16_c:
        return extract_double_array_helper<dods_uint16>(a);
    case dods_int16_c:
        return extract_double_array_helper<dods_int16>(a);
    case dods_uint32_c:
        return extract_double_array_helper<dods_uint32>(a);
    case dods_int32_c:
        return extract_double_array_helper<dods_int32>(a);
    case dods_float32_c:
        return extract_double_array_helper<dods_float32>(a);
    case dods_float64_c:
        return extract_double_array_helper<dods_float64>(a);
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
double extract_double_value(BaseType * arg)
{
    // Simple types are Byte, ..., Float64, String and Url.
    if (!arg->is_simple_type() || arg->type() == dods_str_c || arg->type()
            == dods_url_c)
        throw Error(malformed_expr,
                "The function requires a DAP numeric-type argument.");

    if (!arg->read_p())
        throw InternalErr(__FILE__, __LINE__,
                "The CE Evaluator built an argument list where some constants held no values.");

    // The types of arguments that the CE Parser will build for numeric
    // constants are limited to Uint32, Int32 and Float64. See ce_expr.y.
    // Expanded to work for any numeric type so it can be used for more than
    // just arguments.
    switch (arg->type()) {
    case dods_byte_c:
        return (double)(dynamic_cast<Byte&>(*arg).value());
    case dods_uint16_c:
        return (double)(dynamic_cast<UInt16&>(*arg).value());
    case dods_int16_c:
        return (double)(dynamic_cast<Int16&>(*arg).value());
    case dods_uint32_c:
        return (double)(dynamic_cast<UInt32&>(*arg).value());
    case dods_int32_c:
        return (double)(dynamic_cast<Int32&>(*arg).value());
    case dods_float32_c:
        return (double)(dynamic_cast<Float32&>(*arg).value());
    case dods_float64_c:
        return dynamic_cast<Float64&>(*arg).value();
    default:
        throw InternalErr(__FILE__, __LINE__,
                "The argument list built by the CE parser contained an unsupported numeric type.");
    }
}
#endif



#if 0
/**
 * The passed DDS parameter dds is evaluated to see if it contains Grid objects.
 *
 * @param dds The DDS to be evaluated.
 */
bool GridFunction::canOperateOn(DDS &dds){
	BESDEBUG( "GridFunction","GridFunction::canOperateOn() - Checking DDS for suitable input variables." << endl);

	vector<Grid *> *grids = new vector<Grid *>();

	getGrids(dds,grids);

    bool usable = !grids->empty();

    delete grids;

	return usable;
}
#endif
#if 0
/**
 * The passed DDS parameter dds is evaluated to see if it contains Grid objects whose semantics allow them
 * to be operated on by function_geogrid()
 *
 * @param dds The DDS to be evaluated.
 */
bool GeoGridFunction::canOperateOn(DDS &dds){

	BESDEBUG( "GeoGridFunction","GeoGridFunction::canOperateOn: Checking DDS for suitable input variables." << endl);

    bool usable = false;

    // Go find all the Grid variables.
	vector<Grid *> *grids = new vector<Grid *>();
	getGrids(dds,grids);

	// Were there any?
    if(!grids->empty()){
    	// Apparently so...

    	// See if any one of them looks like suitable GeoGrid
    	vector<Grid *>::iterator git;
    	for(git=grids->begin(); !usable && git!=grids->end() ; git++){
    		Grid *grid = *git;
    		usable = isGeoGrid(grid);
    	}
    }
    delete grids;

	return usable;
}
#endif

#if 0
/** This server-side function returns version information for the server-side
 functions. Note that this function takes no arguments and returns a
 String using the BaseType value/result parameter.

 @param btpp A pointer to the return value; caller must delete.
 @TODO Change implementation to use libxml2 objects and NOT strings.
*/
void
function_version(int, BaseType *[], DDS &dds, BaseType **btpp)
{
    string xml_value = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";



    ServerFunction *sf;
    string functionType;

    ServerFunctionsList *sfList = libdap::ServerFunctionsList::TheList();
    std::multimap<string,libdap::ServerFunction *>::iterator begin = sfList->begin();
    std::multimap<string,libdap::ServerFunction *>::iterator end = sfList->end();
    std::multimap<string,libdap::ServerFunction *>::iterator sfit;

    xml_value += "<ds:functions xmlns:ds=\"http://xml.opendap.org/ns/DAP/4.0/dataset-services#\">\n";
    for(sfit=begin; sfit!=end; sfit++){
    	sf = sfList->getFunction(sfit);
    	if(sf->canOperateOn(dds)){
			xml_value += "    <ds:function  name=\"" + sf->getName() +"\""+
						 " version=\"" + sf->getVersion() + "\""+
						 " type=\"" + sf->getTypeString() + "\""+
						 " role=\"" + sf->getRole() + "\""+
						 " >\n" ;
			xml_value += "        <ds:Description href=\"" + sf->getDocUrl() + "\">" + sf->getDescriptionString() + "</ds:Description>\n";
			xml_value += "    </ds:function>\n";
    	}
    }
	xml_value += "</functions>\n";

    Str *response = new Str("version");

    response->set_value(xml_value);
    *btpp = response;
    return;
}
#endif
#if 0
void
function_dap(int, BaseType *[], DDS &dds, ConstraintEvaluator &ce)
{
#ifdef FUNCTION_DAP
    if (argc != 1) {
	throw Error("The 'dap' function must be called with a version number.\n\
	see http://docs.opendap.org/index.php/Server_Side_Processing_Functions#dap");
    }

    double pv = extract_double_value(argv[0]);
    dds.set_dap_version(pv);
#else
    throw Error("The 'dap' function is not supported in lieu of Constraint expression 'keywords.'\n\
see http://docs.opendap.org/index.php/Server_Side_Processing_Functions#keywords");
#endif
}
#endif

#if 0
static void parse_gse_expression(gse_arg * arg, BaseType * expr)
{
    gse_restart(0); // Restart the scanner.
    void *cls = gse_string(extract_string_argument(expr).c_str());
    // gse_switch_to_buffer(cls); // Get set to scan the string.
    bool status = gse_parse((void *) arg) == 0;
    gse_delete_buffer(cls);
    if (!status)
        throw Error(malformed_expr, "Error parsing grid selection.");
}

static void apply_grid_selection_expr(Grid *grid, GSEClause *clause)
{
    // Basic plan: For each map, look at each clause and set start and stop
    // to be the intersection of the ranges in those clauses.
    Grid::Map_iter map_i = grid->map_begin();
    while (map_i != grid->map_end() && (*map_i)->name() != clause->get_map_name())
        ++map_i;

    if (map_i == grid->map_end())
        throw Error(malformed_expr,"The map vector '" + clause->get_map_name()
                + "' is not in the grid '" + grid->name() + "'.");

    // Use pointer arith & the rule that map order must match array dim order
    Array::Dim_iter grid_dim = (grid->get_array()->dim_begin() + (map_i - grid->map_begin()));

    Array *map = dynamic_cast < Array * >((*map_i));
    if (!map)
        throw InternalErr(__FILE__, __LINE__, "Expected an Array");
    int start = max(map->dimension_start(map->dim_begin()), clause->get_start());
    int stop = min(map->dimension_stop(map->dim_begin()), clause->get_stop());

    if (start > stop) {
        ostringstream msg;
        msg
                << "The expressions passed to grid() do not result in an inclusive \n"
                << "subset of '" << clause->get_map_name()
                << "'. The map's values range " << "from "
                << clause->get_map_min_value() << " to "
                << clause->get_map_max_value() << ".";
        throw Error(malformed_expr,msg.str());
    }

    DBG(cerr << "Setting constraint on " << map->name()
            << "[" << start << ":" << stop << "]" << endl);

    // Stride is always one.
    map->add_constraint(map->dim_begin(), start, 1, stop);
    grid->get_array()->add_constraint(grid_dim, start, 1, stop);
}

static void apply_grid_selection_expressions(Grid * grid,
        vector < GSEClause * >clauses)
{
    vector < GSEClause * >::iterator clause_i = clauses.begin();
    while (clause_i != clauses.end())
        apply_grid_selection_expr(grid, *clause_i++);

    grid->set_read_p(false);
}
#endif

#if 0
/** This is an example function for use by the MIIC project team. The first version
 * of this function simply looked for the Latitude and Longitude arrays of a MODIS
 * granule and returned them if found. This version, miic_ex2, takes two optional
 * arguments. If present, these two arguments are the names of the variables in the
 * dataset that should be used for the latitude and longitude data. If they are not
 * present, the function reverts to the old behavior. If only one argument is given,
 * an error is returned.
 *
 */
void function_miic_ex2(int argc, BaseType *argv[], DDS &dds, BaseType **btpp)
{
    Array *l_lat = 0;
    Array *l_lon = 0;
    switch (argc) {
    case 0: {
        // First find the latitude and longitude variables. This assumes the file is
        // CF. Note that the names of these are not passed into the function so it
        // looks up the variables in the DDS. If the names were passed in, this step
        // would be skipped.
        if (!(l_lat = dynamic_cast<Array *>(dds.var("Latitude")))
                || !(l_lon = dynamic_cast<Array *>(dds.var("Latitude"))))
            throw Error(malformed_expr, "Could not find the Latitude or Longitude data!");
        break;
    }
    case 2: {
        l_lat = dynamic_cast<Array*>(argv[0]);
        l_lon = dynamic_cast<Array*>(argv[1]);
        if (!l_lat || !l_lon)
            throw Error(malformed_expr, "Expected two Array variables as arguments.");
        break;
    }
    default:
        throw Error(malformed_expr, "Expected either zero or two arguments.");
    }

    // Now read the data values into C arrays the function can use. The length of the
    // data is l_lat->size() and l_lon->size() resp. Use delete[] to release the
    // storage. Also note that the Array* must be used to determine the number of
    // dimensions of the arrays - extract_double_array() returns a simple vector.
    l_lat->read();
    double *lat = extract_double_array(l_lat);
    l_lon->read();
    double *lon = extract_double_array(l_lon);

    // For this example, make a Structure, add two new variables to it and stuff
    // these values in them. Make the new variable one-dimensional arrays (vectors)
    // just to keep the code simple.
    Structure *dest = new Structure("MODIS_Geo_information");

    Array *new_lat = new Array("MODIS_Latitude", new Float64("MODIS_Latitude"));
    new_lat->append_dim(l_lat->size());
    new_lat->set_value(lat, l_lat->size());
    dest->add_var(new_lat);

    Array *new_lon = new Array("MODIS_Longtude", new Float64("MODIS_Longtude"));
    new_lon->append_dim(l_lon->size());
    new_lon->set_value(lon, l_lon->size());
    dest->add_var(new_lon);

    *btpp = dest;
}
#endif

#if 0
/** The grid function uses a set of relational expressions to form a selection
 within a Grid variable based on the values in the Grid's map vectors.
 Thus, if a Grid has a 'temperature' map which ranges from 0.0 to 32.0
 degrees, it's possible to request the values of the Grid that fall between
 10.5 and 12.5 degrees without knowing to which array indexes those values
 correspond. The function takes one or more arguments:<ul>
 <li>The name of a Grid.</li>
 <li>Zero or more strings which hold relational expressions of the form:<ul>
 <li><code>&lt;map var&gt; &lt;relop&gt; &lt;constant&gt;</code></li>
 <li><code>&lt;constant&gt; &lt;relop&gt; &lt;map var&gt; &lt;relop&gt;
 &lt;constant&gt;</code></li>
 </ul></li>
 </ul>

 Each of the relation expressions is applied to the Grid and the result is
 returned.

 @note Since this is a function and one of the arguments is the grid, the
 grid is read (using the Grid::read() method) at the time the argument list
 is built.

 @todo In order to be used by geogrid() , this code may have to be modified
 so that the maps and array are not re-read by the serialize() method. It
 might also be a good idea to change from the '?grid(SST,"10<time<20")'
 syntax in a URL to '?SST&grid(SST,"10<time<20")' even though it's more
 verbose in the URL, it would make the function a true 'selection operator'
 and allow several grids to be returned with selections in one request.

 @param argc The number of values in argv.
 @param argv An array of BaseType pointers which hold the arguments to be
 passed to geogrid. The arguments may be Strings, Integers, or Reals, subject
 to the above constraints.
 @param btpp A pointer to the return value; caller must delete.

 @see geogrid() (func_geogrid_select) A function which has logic specific
 to longitude/latitude selection. */
void
function_grid(int argc, BaseType * argv[], DDS &dds, BaseType **btpp)
{
    DBG(cerr << "Entering function_grid..." << endl);

    string info =
    string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
    "<function name=\"grid\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#grid\">\n" +
    "</function>\n";

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        return;
    }

    Grid *original_grid = dynamic_cast < Grid * >(argv[0]);
    if (!original_grid)
        throw Error(malformed_expr,"The first argument to grid() must be a Grid variable!");

    // Duplicate the grid; ResponseBuilder::send_data() will delete the variable
    // after serializing it.
    BaseType *btp = original_grid->ptr_duplicate();
    Grid *l_grid = dynamic_cast < Grid * >(btp);
    if (!l_grid) {
    	delete btp;
        throw InternalErr(__FILE__, __LINE__, "Expected a Grid.");
    }
    
    DBG(cerr << "grid: past initialization code" << endl);

    // Read the maps. Do this before calling parse_gse_expression(). Avoid
    // reading the array until the constraints have been applied because it
    // might be really large.

    // This version makes sure to set the send_p flags which is needed for
    // the hdf4 handler (and is what should be done in general).
    Grid::Map_iter i = l_grid->map_begin();
    while (i != l_grid->map_end())
        (*i++)->set_send_p(true);

    l_grid->read();

    DBG(cerr << "grid: past map read" << endl);

    // argv[1..n] holds strings; each are little expressions to be parsed.
    // When each expression is parsed, the parser makes a new instance of
    // GSEClause. GSEClause checks to make sure the named map really exists
    // in the Grid and that the range of values given makes sense.
    vector < GSEClause * > clauses;
    gse_arg *arg = new gse_arg(l_grid);
    for (int i = 1; i < argc; ++i) {
        parse_gse_expression(arg, argv[i]);
        clauses.push_back(arg->get_gsec());
    }
    delete arg;
    arg = 0;

    apply_grid_selection_expressions(l_grid, clauses);

    DBG(cerr << "grid: past gse application" << endl);

    l_grid->get_array()->set_send_p(true);

    l_grid->read();

    // Make a new grid here and copy just the parts of the Grid
    // that are in the current projection - this means reading
    // the array slicing information, extracting the correct
    // values and building destination arrays with just those
    // values.

    *btpp = l_grid;
    return;
}
#endif
#if 0
/** The geogrid function returns the part of a Grid which includes a
 geographically specified rectangle. The arguments to the function are the
 name of a Grid, the left-top and right-bottom points of the rectangle and
 zero or more relational expressions of the sort that the grid function
 accepts. The constraints on the arguments are:<ul> <li>The Grid must have
 Latitude and Longitude map vectors. Those are discovered by looking for
 map vectors which satisfy enough of any one of a set of conventions to
 make the identification of those map vectors positive or by guessing
 which maps are which. The set of conventions supported is: COARDS, CF
 1.0, GDT and CSC (see
 http://www.unidata.ucar.edu/software/netcdf/conventions.html). If the
 geogrid guesses at the maps, it adds an attribute (geogrid_warning) which
 says so. (in version 1.1)</li> <li>The rectangle corner points are in
 Longitude-Latitude. Longitude may be given using -180 to 180 or 0 to 360.
 For data sources with global coverage, geogrid assumes that the Longitude
 axis is circular. For requests made using 0/359 notation, it assumes it
 is modulus 360. Requests made using -180/179 notation cannot use values
 outside that range.</li> <li>The notation used to specify the rectangular
 region determines the notation used in the longitude/latitude map vectors
 of the Grid returned by the function.</li> <li>There are no restrictions
 on the relational expressions beyond those for the grid() (see
 func_grid_select()) function.</li> </ul>

 @note The geogrid() function is implemented as a 'BaseType function'
 which means that there can be only one function per request and no other
 variables may be named in the request.

 @param argc The number of values in argv.
 @param argv An array of BaseType pointers which hold the arguments to be
 passed to geogrid. The arguments may be Strings, Integers, or Reals,
 subject to the above constraints.
 @param btpp A pointer to the return value; caller must delete.

 @return The constrained and read Grid, ready to be sent. */
void
function_geogrid(int argc, BaseType *argv[], DDS &dds, BaseType **btpp)
{
    string info =
    string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
    "<function name=\"geogrid\" version=\"1.2\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#geogrid\">\n"+
    "</function>";

    if (argc == 0) {
        Str *response = new Str("version");
        response->set_value(info);
        *btpp = response;
        return ;
    }

    // There are two main forms of this function, one that takes a Grid and one
    // that takes a Grid and two Arrays. The latter provides a way to explicitly
    // tell the function which maps contain lat and lon data. The remaining
    // arguments are the same for both versions, although that includes a
    // varying argument list.

    // Look at the types of the first three arguments to determine which of the
    // two forms were used to call this function.
    Grid *l_grid = 0;
    if (argc < 1 || !(l_grid = dynamic_cast < Grid * >(argv[0]->ptr_duplicate())))
	throw Error(malformed_expr,"The first argument to geogrid() must be a Grid variable!");

    // Both forms require at least this many args
    if (argc < 5)
        throw Error(malformed_expr,"Wrong number of arguments to geogrid() (expected at least 5 args). See geogrid() for more information.");

    bool grid_lat_lon_form;
    Array *l_lat = 0;
    Array *l_lon = 0;
    if (!(l_lat = dynamic_cast < Array * >(argv[1]))) //->ptr_duplicate())))
	grid_lat_lon_form = false;
    else if (!(l_lon = dynamic_cast < Array * >(argv[2]))) //->ptr_duplicate())))
	throw Error(malformed_expr,"When using the Grid, Lat, Lon form of geogrid() both the lat and lon maps must be given (lon map missing)!");
    else
	grid_lat_lon_form = true;

    if (grid_lat_lon_form && argc < 7)
        throw Error(malformed_expr,"Wrong number of arguments to geogrid() (expected at least 7 args). See geogrid() for more information.");

#if 0
    Grid *l_grid = dynamic_cast < Grid * >(argv[0]->ptr_duplicate());
    if (!l_grid)
        throw Error(malformed_expr,"The first argument to geogrid() must be a Grid variable!");
#endif
    // Read the maps. Do this before calling parse_gse_expression(). Avoid
    // reading the array until the constraints have been applied because it
    // might be really large.
    //
    // Trick: Some handlers build Grids from a combination of Array
    // variables and attributes. Those handlers (e.g., hdf4) use the send_p
    // property to determine which parts of the Grid to read *but they can
    // only read the maps from within Grid::read(), not the map's read()*.
    // Since the Grid's array does not have send_p set, it will not be read
    // by the call below to Grid::read().
    Grid::Map_iter i = l_grid->map_begin();
    while (i != l_grid->map_end())
        (*i++)->set_send_p(true);

    l_grid->read();
    // Calling read() above sets the read_p flag for the entire grid; clear it
    // for the grid's array so that later on the code will be sure to read it
    // under all circumstances.
    l_grid->get_array()->set_read_p(false);
    DBG(cerr << "geogrid: past map read" << endl);

    // Look for Grid Selection Expressions tacked onto the end of the BB
    // specification. If there are any, evaluate them before evaluating the BB.
    int min_arg_count = (grid_lat_lon_form) ? 7 : 5;
    if (argc > min_arg_count) {
        // argv[5..n] holds strings; each are little Grid Selection Expressions
        // to be parsed and evaluated.
        vector < GSEClause * > clauses;
        gse_arg *arg = new gse_arg(l_grid);
        for (int i = min_arg_count; i < argc; ++i) {
            parse_gse_expression(arg, argv[i]);
            clauses.push_back(arg->get_gsec());
        }
        delete arg;
        arg = 0;

        apply_grid_selection_expressions(l_grid, clauses);
    }

    try {
        // Build a GeoConstraint object. If there are no longitude/latitude
        // maps then this constructor throws Error.
        GridGeoConstraint gc(l_grid);

        // This sets the bounding box and modifies the maps to match the
        // notation of the box (0/359 or -180/179)
        int box_index_offset = (grid_lat_lon_form) ? 3 : 1;
        double top = extract_double_value(argv[box_index_offset]);
        double left = extract_double_value(argv[box_index_offset + 1]);
        double bottom = extract_double_value(argv[box_index_offset + 2]);
        double right = extract_double_value(argv[box_index_offset + 3]);
        gc.set_bounding_box(top, left, bottom, right);
        DBG(cerr << "geogrid: past bounding box set" << endl);

        // This also reads all of the data into the grid variable
        gc.apply_constraint_to_data();
        DBG(cerr << "geogrid: past apply constraint" << endl);

        // In this function the l_grid pointer is the same as the pointer returned
        // by this call. The caller of the function must free the pointer.
        *btpp = gc.get_constrained_grid();
        return;
    }
    catch (Error &e) {
        throw e;
    }
    catch (exception & e) {
        throw
        InternalErr(string
                ("A C++ exception was thrown from inside geogrid(): ")
                + e.what());
    }
}
#endif

#if 0
// These static functions could be moved to a class that provides a more
// general interface for COARDS/CF someday. Assume each BaseType comes bundled
// with an attribute table.

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

    if ((v == 0.0 && (val == ptr || errno == HUGE_VAL || errno == ERANGE))
            || *ptr != '\0') {
        throw Error(malformed_expr,string("Could not convert the string '") + val + "' to a double.");
    }

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
static double get_attribute_double_value(BaseType *var,
        vector<string> &attributes)
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
            throw Error(malformed_expr,string("No COARDS/CF '") + values.substr(0, values.size() - 2)
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
        int length = source->size();
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
        int length = a->size();
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
                vector<dods_byte> v(a->size());
                a->value(v.data());
                static_cast<Array*>(*d)->set_value(v, v.size());
                break;
            }
            case dods_float32_c: {
                vector<dods_float32> v(a->size());
                a->value(v.data());
                static_cast<Array*>(*d)->set_value(v, a->size());
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
        int length = source.size();
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
        int length = source.size();
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
#endif

#if 0
/** Perform a selection on the array using geographical coordinates. This
 function takes several groups of arguments.
 <ul>
 <li>geoarray(var, top, left, bottom, right)</li>
 <li>geoarray(var, top, left, bottom, right, var_top, v_left, v_bottom, v_right)</li>
 <li>geoarray(var, top, left, bottom, right, var_top, v_left, v_bottom, v_right, projection, datum)</li>
 </ul>

 @note Only the plat-carre projection and wgs84 datum are currently
 supported.

 @param argc A count of the arguments 
 @param argv An array of pointers to each argument, wrapped in a child of BaseType
 @param btpp A pointer to the return value; caller must delete.

 @return The Array, constrained by the selection
 @exception Error Thrown if thins go awry. */
void
function_geoarray(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    string info =
    string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
    "<function name=\"geoarray\" version=\"0.9b1\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#geoarray\">\n" +
    "</function>";

    if (argc == 0) {
        Str *response = new Str("version");
        response->set_value(info);
        *btpp = response;
        return;
    }

    DBG(cerr << "argc = " << argc << endl);
    if (!(argc == 5 || argc == 9 || argc == 11))
        throw Error(malformed_expr,"Wrong number of arguments to geoarray(). See geoarray() for more information.");

    // Check the Array (and dup because the caller will free the variable).
    Array *l_array = dynamic_cast < Array * >(argv[0]->ptr_duplicate());
    if (!l_array)
        throw Error(malformed_expr,"The first argument to geoarray() must be an Array variable!");

    try {

        // Read the bounding box and variable extents from the params
        double bb_top = extract_double_value(argv[1]);
        double bb_left = extract_double_value(argv[2]);
        double bb_bottom = extract_double_value(argv[3]);
        double bb_right = extract_double_value(argv[4]);

        switch (argc) {
            case 5: {
            	ArrayGeoConstraint agc(l_array);

         		agc.set_bounding_box(bb_left, bb_top, bb_right, bb_bottom);
				// This also reads all of the data into the grid variable
        		agc.apply_constraint_to_data();
        		*btpp = agc.get_constrained_array();
        		return;
            	break;
            }
            case 9: {
                double var_top = extract_double_value(argv[5]);
                double var_left = extract_double_value(argv[6]);
                double var_bottom = extract_double_value(argv[7]);
                double var_right = extract_double_value(argv[8]);
                ArrayGeoConstraint agc (l_array, var_left, var_top, var_right, var_bottom);

        		agc.set_bounding_box(bb_left, bb_top, bb_right, bb_bottom);
				// This also reads all of the data into the grid variable
        		agc.apply_constraint_to_data();
        		*btpp =  agc.get_constrained_array();
        		return;
                break;
            }
            case 11: {
                double var_top = extract_double_value(argv[5]);
                double var_left = extract_double_value(argv[6]);
                double var_bottom = extract_double_value(argv[7]);
                double var_right = extract_double_value(argv[8]);
                string projection = extract_string_argument(argv[9]);
                string datum = extract_string_argument(argv[10]);
                ArrayGeoConstraint agc(l_array,
                        var_left, var_top, var_right, var_bottom,
                        projection, datum);

        		agc.set_bounding_box(bb_left, bb_top, bb_right, bb_bottom);
				// This also reads all of the data into the grid variable
        		agc.apply_constraint_to_data();
        		*btpp = agc.get_constrained_array();
        		return;
                break;
            }
            default:
            	throw InternalErr(__FILE__, __LINE__, "Wrong number of args to geoarray.");
        }
    }
    catch (Error & e) {
        throw e;
    }
    catch (exception & e) {
        throw
        InternalErr(string
                ("A C++ exception was thrown from inside geoarray(): ")
                + e.what());

    }

    throw InternalErr(__FILE__, __LINE__, "Impossible condition in geoarray.");
}
#endif

#ifdef GRIDFIELDS

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
function_ugrid_demo(int argc, BaseType * argv[], DDS &dds, BaseType **btpp)
{

    // This is the nascent on-line help for these functions. 
    static string info =
	string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
	"<function name=\"ugrid_demo\" version=\"0.1\">\n" +
	"Fledgling code for Unstructured grid operations.\n" +
	"</function>";
    
    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        return;
    }

    // Check number of arguments; DBG is a macro. Use #define
    // DODS_DEBUG to activate the debugging stuff.
    if (argc != 2)
        throw Error(malformed_expr,"Wrong number of arguments to ugrid_demo. ugrid_demo(dim:int32, condition:string); was passed " + long_to_string(argc) + " argument(s)");

    //FIXME Neither of the input parameters are used.
    if (argv[0]->type() != dods_int32_c) {
        throw Error(malformed_expr,"Wrong type for first argument. ugrid_demo(dim:int32, condition:string); was passed a/an " + argv[1]->type_name());
    }
    if (argv[1]->type() != dods_str_c) {
        throw Error(malformed_expr,"Wrong type for second argument. ugrid_demo(dim:int32, condition:string); was passed a/an " + argv[1]->type_name());
    }

    BaseType *result = 0;		// This will hold the result
    
    // keep track of which DDS dimensions correspond to GF dimensions
    map<GF::Dim_t, vector<Array::dimension> > rank_dimensions; 

    GF::Grid *G = new GF::Grid("result");
  
    // 1) Find the nodes
    DBG(cerr << "Reading 0-cells" << endl);
    GF::AbstractCellArray *nodes = NULL;
    for (DDS::Vars_iter vi = dds.var_begin(); vi != dds.var_end(); vi++) {
      BaseType *bt = *vi;
      Array &arr = dynamic_cast<Array&>(*bt);
      AttrTable &at = arr.get_attr_table();
      DBG(cerr << "Array: " << arr.name() << endl);

      int node_count = -1;  //error condition
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
        } // Bound to nodes?  
      } // Has a "grid_location" attribute?
      /* 
We should use implicit0cells here, but that's limited to 
cell ids that are numbered from zero.  The class can 
and should be generalized, but I don't want to get 
distracted fixing it.  
Instead, we materialize the nodes as explciit cells.  Wastes memory.

Update: We do use implicit0cells, noting that the index_origin attribute
is currently only defined for k-cells, where k>0.
For now, we do NOT preserve index_origin the result.
We can therefore guarantee that nodes are numbered from 0.
       */  
      nodes = new GF::Implicit0Cells(node_count);
/*

      nodes = new GF::CellArray();
      for (int i=index_origin; i<node_count; i++) {
        // insert a cell of one node with id i
        nodes->addCellNodes(&i, 1);
      }
*/    

      // We've figured out which dimensions are associated with
      // the nodes, so stop 
      // TODO: Assumes only one grid per file!
      break; 
    }

    // Attach the nodes to the grid
    G->setKCells(nodes, 0);

    // 2) For each k, find the k-cells
    // k = 2, for now
    DBG(cerr << "Reading 2-cells" << endl);
    GF::CellArray *twocells = NULL;
    for (DDS::Vars_iter vi = dds.var_begin(); vi != dds.var_end(); vi++) {
      BaseType *bt = *vi;
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
            } else {
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
 
    // 3) For each var, bind it to the appropriate dimension

    // For each variable in the data source:
    GF::GridField *input = new GF::GridField(G);
    for (DDS::Vars_iter vi = dds.var_begin(); vi != dds.var_end(); vi++) {
      BaseType *bt = *vi;
      if (bt->type() == dods_array_c) {
        Array *arr = (Array *)bt;
        DBG(cerr << "Data Array: " << arr->name() << endl);
        // Each rank is associated with a sequence of dimensions
        // Vars that have the same dimensions should be bound to the grid at that rank
        // (Note that in gridfields, Dimension and rank are synonyms.  We
        // use the latter here to avoid confusion).
        map<GF::Dim_t, vector<Array::dimension> >::iterator iter;
        for( iter = rank_dimensions.begin(); iter != rank_dimensions.end(); ++iter ) {
          bool same = same_dimensions(arr, iter->second);
          if (same) {
            // This var should be bound to rank k
            GF::Array *gfa = extract_gridfield_array(arr);
            DBG(cerr << "Adding Attribute: " << gfa->sname() << endl);
            input->AddAttribute(iter->first, gfa);
          } else {
            //This array does not appear to be associated with any rank of the unstructured grid.  Ignore for now.  Anything else we should do?
          }
        }
      } // Ignore if not an array type.  Anything else we should do?
    }
    input->print();
/*
    input->print();
    GF::GridFieldOperator *op = GF::RestrictOp(expr, dim, input);
    GF::GridField *R = op->getResult(); 
        
    R->print();
*/

    // 4) Convert back to a DDS BaseType

    /*
    // Create variables for each cell dimension
    // Create variables for each attribute at each rank
    */

#if 1
    //FIXME HACKHACKHACK
    result = new Str("result");
    string result_string = "The gridfields function is not complete yet.";
    dynamic_cast<Str&>(*result).set_value(result_string);
#endif

    *btpp = result;
    return;
}
#endif // GRIDFIELDS

#if 0
void register_functions(ConstraintEvaluator & ce)
{
#ifdef GRIDFIELDS
    ce.add_function("ugrid_demo", function_ugrid_demo);
#endif
    ce.add_function("grid", function_grid);
    ce.add_function("geogrid", function_geogrid);
    ce.add_function("linear_scale", function_linear_scale);
    ce.add_function("version", function_version);

#ifdef GRIDFIELDS
    ce.add_function("ugrid_restrict", function_ugrid_restrict);
    ce.add_function("ugr", function_ugr);
    ce.add_function("ugr2", function_ugr2);
    ce.add_function("ugr3", function_ugr3);
#endif

#if 0
    ce.add_function("miic_ex2", function_miic_ex2);
    ce.add_function("dap", function_dap);
#endif
}
#endif
} // namespace libdap
