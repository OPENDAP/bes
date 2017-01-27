// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003,2011,2012 OPeNDAP, Inc.
// Authors: Nathan Potter <ndp@opendap.org>
//          James Gallagher <jgallagher@opendap.org>
//          Scott Moe <smeest1@gmail.com>
//          Bill Howe <billhowe@cs.washington.edu>
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

//#define DODS_DEBUG

#include "config.h"

#include <vector>
#include <sstream>

#include <gridfields/array.h>

#include "BaseType.h"
#include "Array.h"
#include "debug.h"
#include "util.h"

#include "BESDebug.h"

#include "ugrid_utils.h"

#ifdef NDEBUG
#undef BESDEBUG
#define BESDEBUG( x, y )
#endif

using namespace std;
using namespace libdap;

namespace ugrid {

GF::e_Type getGridfieldsInternalTypeMap(Type type)
{

    switch (type) {
    case dods_byte_c:
    case dods_uint16_c:
    case dods_int16_c:
    case dods_uint32_c:
    case dods_int32_c: {
        return GF::INT;
        break;
    }
    case dods_float32_c:
    case dods_float64_c: {
        return GF::FLOAT;
        break;
    }
    default:
        throw InternalErr(__FILE__, __LINE__,
            "Unknown DAP type encountered when converting to gridfields internal type.");
    }
}

Type getGridfieldsReturnType(Type type)
{
    GF::e_Type gfInternalType = getGridfieldsInternalTypeMap(type);

    Type retType;
    switch (gfInternalType) {
    case GF::INT: {
        retType = dods_int32_c;
        break;
    }
    case GF::FLOAT: {
        retType = dods_float64_c;
        break;
    }
    default:
        throw InternalErr(__FILE__, __LINE__,
            "Unknown GF::e_Type type encountered when resolving gridfields result type mapping for dap type "
                + libdap::type_name(type));
    }DBG(cerr << " getGridfieldsReturnType() - Return type for " << libdap::type_name(type) <<
        " is " << libdap::type_name(retType) << endl);

    return retType;
}

Type getGridfieldsReturnType(libdap::Array *a)
{
    return getGridfieldsReturnType(a->var()->type());
}

GF::Array *newGFIndexArray(string name, long size, vector<int*> *sharedIntArrays)
{
    GF::Array *gfa = new GF::Array(name, GF::INT);
    int *values = new int[size];
    for (long i = 0; i < size; i++) {
        values[i] = i;
    }
    gfa->shareIntData(values, size);
    sharedIntArrays->push_back(values);
    return gfa;
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
GF::Array *extractGridFieldArray(libdap::Array *a, vector<int*> *sharedIntArrays, vector<float*> *sharedFloatArrays)
{
    if ((a->type() == dods_array_c && !a->var()->is_simple_type()) || a->var()->type() == dods_str_c
        || a->var()->type() == dods_url_c)
        throw Error(malformed_expr, "The function requires a DAP numeric-type array argument.");

    DBG(cerr << "extract_gridfield_array() - " << "Reading data values into DAP Array '" << a->name() <<"'"<< endl);
    a->set_send_p(true);
    a->read();

    // Construct a GridField array from a DODS array
    GF::Array *gfa;

    switch (a->var()->type()) {
    case dods_byte_c: {
        gfa = new GF::Array(a->var()->name(), GF::INT);
        int *values = extract_array_helper<dods_byte, int>(a);
        gfa->shareIntData(values, a->length());
        sharedIntArrays->push_back(values);
        break;
    }
    case dods_uint16_c: {
        gfa = new GF::Array(a->var()->name(), GF::INT);
        int *values = extract_array_helper<dods_uint16, int>(a);
        gfa->shareIntData(values, a->length());
        sharedIntArrays->push_back(values);
        break;
    }
    case dods_int16_c: {
        gfa = new GF::Array(a->var()->name(), GF::INT);
        int *values = extract_array_helper<dods_int16, int>(a);
        gfa->shareIntData(values, a->length());
        sharedIntArrays->push_back(values);
        break;
    }
    case dods_uint32_c: {
        gfa = new GF::Array(a->var()->name(), GF::INT);
        int *values = extract_array_helper<dods_uint32, int>(a);
        gfa->shareIntData(values, a->length());
        sharedIntArrays->push_back(values);
        break;
    }
    case dods_int32_c: {
        gfa = new GF::Array(a->var()->name(), GF::INT);
        int *values = extract_array_helper<dods_int32, int>(a);
        gfa->shareIntData(values, a->length());
        sharedIntArrays->push_back(values);
        break;
    }
    case dods_float32_c: {
        gfa = new GF::Array(a->var()->name(), GF::FLOAT);
        float *values = extract_array_helper<dods_float32, float>(a);
        gfa->shareFloatData(values, a->length());
        sharedFloatArrays->push_back(values);
        break;
    }
    case dods_float64_c: {
        gfa = new GF::Array(a->var()->name(), GF::FLOAT);
        float *values = extract_array_helper<dods_float64, float>(a);
        gfa->shareFloatData(values, a->length());
        sharedFloatArrays->push_back(values);
        break;
    }
    default:
        throw InternalErr(__FILE__, __LINE__, "Unknown DAP type encountered when converting to gridfields array");
    }
    return gfa;
}

/**
 * Splits the string on the passed char. Returns vector of substrings.
 * TODO make this work on situations where multiple spaces doesn't hose the split()
 */
vector<string> &split(const string &s, char delim, vector<string> &elems)
{
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

/**
 * Splits the string on the passed char. Returns vector of substrings.
 */
vector<string> split(const string &s, char delim)
{
    vector<string> elems;
    return split(s, delim, elems);
}

// Returns the string value of the attribute called aName, 0 otherwise.
string getAttributeValue(BaseType *bt, string aName)
{
    AttrTable &at = bt->get_attr_table();
    DBG(cerr << "getAttributeValue() - " << "Checking to see if the variable " << bt->name()
        << "' has an attribute '"<< aName << "'"<<endl);

    // Confirm that submitted variable has an attribute called aName and return its value.
    AttrTable::Attr_iter loc = at.simple_find(aName);
    if (loc != at.attr_end()) {
        return at.get_attr(loc, 0);
    }

    return "";
}

/**
 * Checks the passed BaseType attributes as follows: If the BaseType has a "cf_role" attribute and it's value is the same as
 * aValue return true. If it doesn't have a "cf_role" attribute, then if there is a "standard_name" attribute and it's value is
 * the same as aValue then  return true. All other outcomes return false.
 */
bool matchesCfRoleOrStandardName(BaseType *bt, string aValue)
{
    // Confirm that submitted variable has a 'location' attribute whose value is "node".
    if (!checkAttributeValue(bt, CF_ROLE, aValue)) {
        // Missing the 'cf_role' attribute? Check for a 'standard_name' attribute whose value is "aValue".
        if (!checkAttributeValue(bt, CF_STANDARD_NAME, aValue)) {
            return false;
        }
    }

    return true;
}

// Returns true iff the submitted BaseType variable has an attribute called aName attribute whose value is aValue.
bool checkAttributeValue(BaseType *bt, string aName, string aValue)
{

    AttrTable &at = bt->get_attr_table();
    DBG(cerr << "checkAttributeValue() - " << "Checking to see if the variable " << bt->name()
        << "' has an attribute '"<< aName << "' with value '" << aValue << "'"<<endl);

    // Confirm that submitted variable has an attribute called aName whose value is aValue.
    AttrTable::Attr_iter loc = at.simple_find(aName);
    if (loc != at.attr_end()) {
        DBG(cerr << "checkAttributeValue() - " << "'" << bt->name() << "' has a attribute named '" << aName << "'"<< endl);
        string value = at.get_attr(loc, 0);
        DBG(cerr << "checkAttributeValue() - " << "Attribute '"<< aName <<"' has value of '" << value << "'"<< endl);
        if (value != aValue) {
            return false;
        }
        return true;
    }
    return false;

}

} // namespace ugrid
