// -*- mode: c++; c-basic-offset:4 -*-
//
// FoDapCovJsonTransform.cc
//
// This file is part of BES CovJSON File Out Module
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Corey Hemphill <hemphilc@oregonstate.edu>
// Author: River Hendriksen <hendriri@oregonstate.edu>
// Author: Riley Rimer <rrimer@oregonstate.edu>
//
// Adapted from the File Out JSON module implemented by Nathan Potter
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
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "config.h"

#include <cassert>

#include <sstream>
#include <iostream>
#include <fstream>
#include <stddef.h>
#include <string>
#include <typeinfo>

using std::ostringstream;
using std::istringstream;

#include <DDS.h>
#include <Structure.h>
#include <Constructor.h>
#include <Array.h>
#include <Grid.h>
#include <Sequence.h>
#include <Float64.h>
#include <Str.h>
#include <Url.h>

#include <BESDebug.h>
#include <BESInternalError.h>

#include <DapFunctionUtils.h>

#include "FoDapCovJsonTransform.h"
#include "focovjson_utils.h"

#define FoDapCovJsonTransform_debug_key "focovjson"

const int int_64_precision = 15; // 15 digits to the right of the decimal point. jhrg 9/14/15

/**
 *  @TODO Handle String and URL Arrays including backslash escaping double quotes in values.
 *
 */
template<typename T>
unsigned int FoDapCovJsonTransform::covjsonSimpleTypeArrayWorker(ostream *strm, T *values, unsigned int indx,
    vector<unsigned int> *shape, unsigned int currentDim)
{
    *strm << "[";

    unsigned int currentDimSize = (*shape)[currentDim];

    for (unsigned int i = 0; i < currentDimSize; i++) {
        if (currentDim < shape->size() - 1) {
            BESDEBUG(FoDapCovJsonTransform_debug_key,
                "covjsonSimpleTypeArrayWorker() - Recursing! indx:  " << indx << " currentDim: " << currentDim << " currentDimSize: " << currentDimSize << endl);
            indx = covjsonSimpleTypeArrayWorker<T>(strm, values, indx, shape, currentDim + 1);
            if (i + 1 != currentDimSize) *strm << ", ";
        }
        else {
            if (i) *strm << ", ";
            if (typeid(T) == typeid(std::string)) {
                // Strings need to be escaped to be included in a JSON object.
                string val = reinterpret_cast<string*>(values)[indx++]; // ((string *) values)[indx++];
                *strm << "\"" << focovjson::escape_for_covjson(val) << "\"";
            }
            else {
                *strm << values[indx++];
            }
        }
    }
    *strm << "]";

    return indx;
}

/**
 * Writes the CovJSON representation of the passed DAP Array of simple types. If the
 * parameter "sendData" evaluates to true then data will also be sent.
 */
template<typename T>
void FoDapCovJsonTransform::covjsonSimpleTypeArray(ostream *strm, libdap::Array *a, string indent, bool sendData)
{
    //*strm << indent << "{" << endl;
    string childindent = indent + _indent_increment;

    //writeLeafMetadata(strm, a, childindent);
    writeLeafMetadata(strm, a, indent);

    int numDim = a->dimensions(true);
    vector<unsigned int> shape(numDim);
    long length = focovjson::computeConstrainedShape(a, &shape);

    if (sendData) {
        *strm << childindent << "\"values\": ";
        unsigned int indx = 0;
        vector<T> src(length);
        a->value(&src[0]);

        // I added this, and a corresponding block in FoInstance... because I fixed
        // an issue in libdap::Float64 where the precision was not properly reset
        // in it's print_val() method. Because of that error, precision was (left at)
        // 15 when this code was called until I fixed that method. Then this code
        // was not printing at the required precision. jhrg 9/14/15
        if (typeid(T) == typeid(libdap::dods_float64)) {
            streamsize prec = strm->precision(int_64_precision);
            try {
                indx = covjsonSimpleTypeArrayWorker(strm, &src[0], 0, &shape, 0);
                strm->precision(prec);
            }
            catch(...) {
                strm->precision(prec);
                throw;
            }
        }
        else {
            indx = covjsonSimpleTypeArrayWorker(strm, &src[0], 0, &shape, 0);
        }

        assert(length == indx);
    }

    *strm << endl << indent << "}";
}

/**
 * String version of covjsonSimpleTypeArray(). This version exists because of the differing
 * type signatures of the libdap::Vector::value() methods for numeric and c++ string types.
 *
 * @param strm Write to this stream
 * @param a Source Array - write out data or metadata from or about this Array
 * @param indent Indent the output so humans can make sense of it
 * @param sendData True: send data; false: send metadata
 */
void FoDapCovJsonTransform::covjsonStringArray(std::ostream *strm, libdap::Array *a, string indent, bool sendData)
{
    *strm << indent << "{" << endl;\
    string childindent = indent + _indent_increment;

    //writeLeafMetadata(strm, a, childindent);
    writeLeafMetadata(strm, a, indent);

    int numDim = a->dimensions(true);
    vector<unsigned int> shape(numDim);
    long length = focovjson::computeConstrainedShape(a, &shape);

    *strm << childindent << "\"shape\": [";

    for (std::vector<unsigned int>::size_type i = 0; i < shape.size(); i++) {
        if (i > 0) *strm << ",";
        *strm << shape[i];
    }
    *strm << "]";

    if (sendData) {
        *strm << "," << endl;

        // Data array gets printed to strm
        *strm << childindent << "\"values\": ";
        unsigned int indx;

        // The string type utilizes a specialized version of libdap:Array.value()
        vector<std::string> sourceValues;
        a->value(sourceValues);
        indx = covjsonSimpleTypeArrayWorker(strm, (std::string *) (&sourceValues[0]), 0, &shape, 0);

        if (length != indx)
            BESDEBUG(FoDapCovJsonTransform_debug_key,
                "covjsonStringArray() - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);

    }

    *strm << endl << indent << "}";
}

/**
 * Writes the covjson opener for the Dataset, including name and top level DAP attributes.
 */
void FoDapCovJsonTransform::writeDatasetMetadata(ostream *strm, libdap::DDS *dds, string indent)
{
    // Name
    *strm << indent << "\"name\": \"" << dds->get_dataset_name() << "\"," << endl;

    //Attributes
    transform(strm, dds->get_attr_table(), indent);
    *strm << "," << endl;
}

/**
 * Writes CovJSON opener for a DAP object that is seen as a "node" in w10n semantics.
 * Header includes object name and attributes
 */
void FoDapCovJsonTransform::writeNodeMetadata(ostream *strm, libdap::BaseType *bt, string indent)
{
    // Name
    *strm << indent << "\"name\": \"" << bt->name() << "\"," << endl;

    //Attributes
    transform(strm, bt->get_attr_table(), indent);
    *strm << "," << endl;
}

/**
 * Writes CovJSON opener for a DAP object that is seen as a "leaf" in w10n semantics.
 * Header includes object name. attributes, and  type.
 */
void FoDapCovJsonTransform::writeLeafMetadata(ostream *strm, libdap::BaseType *bt, string indent)
{
    // Name
    //*strm << indent << "\"name\": \"" << bt->name() << "\"," << endl;
    *strm << indent << "\"" << bt->name() << "\": {" << endl;

    // type
    //if (bt->type() == libdap::dods_array_c) {
    //    libdap::Array *a = (libdap::Array *) bt;
    //    *strm << indent << "\"type\": \"" << a->var()->type_name() << "\"," << endl;
    //}
    //else {
    //    *strm << indent << "\"type\": \"" << bt->type_name() << "\"," << endl;
    //}

    //Attributes
    //transform(strm, bt->get_attr_table(), indent);
    //*strm << "," << endl;
}

/**
 * will find value name x,y,z,t for the axis of the coverage and return it.
 */
// char FoDapCovJsonTransform::Axis_Unit_value(ostream *strm, libdap::AttrTable &attr_table, string indent)
// {
//
//     string child_indent = indent + _indent_increment;
//
//     // Start the attributes block
//     *strm << indent << "\"attributes\": [";
//
// //	if(attr_table.get_name().length()>0)
// //		*strm  << endl << child_indent << "{\"name\": \"name\", \"value\": \"" << attr_table.get_name() << "\"},";
//
// // Only do more if there are actually attributes in the table
//     if (attr_table.get_size() != 0) {
//         *strm << endl;
//         libdap::AttrTable::Attr_iter begin = attr_table.attr_begin();
//         libdap::AttrTable::Attr_iter end = attr_table.attr_end();
//
//         for (libdap::AttrTable::Attr_iter at_iter = begin; at_iter != end; at_iter++) {
//
//             switch (attr_table.get_attr_type(at_iter)) {
//             case libdap::Attr_container: {
//                 libdap::AttrTable *atbl = attr_table.get_attr_table(at_iter);
//
//                 // not first thing? better use a comma...
//                 if (at_iter != begin) *strm << "," << endl;
//
//                 // Attribute Containers need to be opened and then a recursive call gets made
//                 *strm << child_indent << "{" << endl;
//
//                 // If the table has a name, write it out as a CovJSON property.
//                 if (atbl->get_name().length() > 0)
//                     *strm << child_indent + _indent_increment << "\"name\": \"" << atbl->get_name() << "\"," << endl;
//
//                 // Recursive call for child attribute table.
//                 Axis_Unit_value(strm, *atbl, child_indent + _indent_increment);
//                 *strm << endl << child_indent << "}";
//
//                 break;
//
//             }
//             default: {
//                 // not first thing? better use a comma...
//                 if (at_iter != begin) *strm << "," << endl;
//
//                 // Open attribute object, write name
//                 *strm << child_indent << "{\"name\": \"" << attr_table.get_name(at_iter) << "\", ";
//
//                 // Open value array
//                 *strm << "\"value\": [";
//                 vector<std::string> *values = attr_table.get_attr_vector(at_iter);
//                 // write values
//                 for (std::vector<std::string>::size_type i = 0; i < values->size(); i++) {
//
//                     // not first thing? better use a comma...
//                     if (i > 0) *strm << ",";
//
//                     // Escape the double quotes found in String and URL type attribute values.
//                     if (attr_table.get_attr_type(at_iter) == libdap::Attr_string
//                         || attr_table.get_attr_type(at_iter) == libdap::Attr_url) {
//                         *strm << "\"";
//                         // string value = (*values)[i] ;
//                         *strm << focovjson::escape_for_covjson((*values)[i]);
//                         *strm << "\"";
//                     }
//                     else {
//
//                         *strm << (*values)[i];
//                     }
//
//                 }
//                 // close value array
//                 *strm << "]}";
//                 break;
//             }
//
//             }
//         }
//
//         *strm << endl << indent;
//     }
//
//     // close AttrTable COVJSON
//
//     *strm << "]";
// }

/**
 * @brief Get the JSON encoding for a DDS
 *
 * Set up the JSON output transform object. This constructor builds
 * an object that will build a JSON encoding for a DDS. This class can
 * return both the entire DDS, including data, and a metadata-only
 * response.
 *
 * @note The 'transform' method is used to build the response and a
 * bool flag is passed to it to select data or metadata. However, if
 * that flag is true and the DDS does not already contain data, the
 * result is undefined.
 *
 * @param dds DDS object
 * @throws BESInternalError if the DDS* is null or if localfile is empty.
 */
FoDapCovJsonTransform::FoDapCovJsonTransform(libdap::DDS *dds) : _dds(dds), _indent_increment("  ")
{
    if (!_dds) throw BESInternalError("File out COVJSON, null DDS passed to constructor", __FILE__, __LINE__);
}

/** @brief dumps information about this transformation object for debugging
 * purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including all of the FoJson objects converted from DAP objects that are
 * to be sent to the netcdf file.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FoDapCovJsonTransform::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FoDapCovJsonTransform::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_dds != 0) {
        _dds->print(strm);
    }
    BESIndent::UnIndent();
}

/**
 * @brief Transforms each of the marked variables of the DDS to JSON
 *
 * For each variable in the DDS, write out that variable and its
 * attributes as JSON. Each OPeNDAP data type translates into a
 * particular JSON type. Also write out any global attributes stored at the
 * top level of the DataDDS.
 *
 * @note If sendData is true but the DDS does not contain data, the result
 * is undefined.
 *
 * @param ostrm Write the JSON to this stream
 * @param sendData True if data should be sent, False to send only metadata.
 */
void FoDapCovJsonTransform::transform(ostream &ostrm, bool sendData, FoDapCovJsonValidation fv)
{
    transform(&ostrm, _dds, "", sendData, fv);
}

/**
 * DAP Constructor types are semantically equivalent to a w10n node type so they
 * must be represented as a collection of child nodes and leaves.
 */
void FoDapCovJsonTransform::transform(ostream *strm, libdap::Constructor *cnstrctr, string indent, bool sendData)
{
    vector<libdap::BaseType *> leaves;
    vector<libdap::BaseType *> nodes;

    // Sort the variables into two sets/
    libdap::DDS::Vars_iter vi = cnstrctr->var_begin();
    libdap::DDS::Vars_iter ve = cnstrctr->var_end();
    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            libdap::BaseType *v = *vi;

            libdap::Type type = v->type();
            if (type == libdap::dods_array_c) {
                type = v->var()->type();
            }
            if (v->is_constructor_type() || (v->is_vector_type() && v->var()->is_constructor_type())) {
                nodes.push_back(v);
            }
            else {
                leaves.push_back(v);
            }
        }
    }

    // Declare this node
    *strm << indent << "{" << endl;
    string child_indent = indent + _indent_increment;

    // Write this node's metadata (name & attributes)
    writeNodeMetadata(strm, cnstrctr, child_indent);

    //transformAxesWorker(strm, leaves, child_indent, sendData);

    //transformReferenceWorker(strm, child_indent);

    //transformParameterWorker(strm, nodes, child_indent, sendData);

    *strm << indent << "}" << endl;
}

/**
 * This worker method allows us to recursively traverse a "node" variables contents and
 * any child nodes will be traversed as well.
 */
void FoDapCovJsonTransform::transformAxesWorker(ostream *strm, vector<libdap::BaseType *> leaves,
    string indent, bool sendData)
{
    // Write the axes to strm
    *strm << indent << "\"axes\": {";

    if (leaves.size() > 0) *strm << endl;
    for (std::vector<libdap::BaseType *>::size_type l = 0; l < leaves.size(); l++) {
        libdap::BaseType *v = leaves[l];
        BESDEBUG(FoDapCovJsonTransform_debug_key, "Processing LEAF: " << v->name() << endl);
        if (l > 0) {
            *strm << ",";
            *strm << endl;
        }
        transform(strm, v, indent + _indent_increment, sendData);
    }
    if (leaves.size() > 0) *strm << endl << indent;

    // Write down the parameters and values
    //*strm << _indent_increment << "\"parameters\": [";
    //if (nodes.size() > 0) *strm << endl;
    //for (std::vector<libdap::BaseType *>::size_type n = 0; n < nodes.size(); n++) {
    //    libdap::BaseType *v = nodes[n];
    //    transform(strm, v, _indent_increment + _indent_increment, sendData);
    //}
    //if (nodes.size() > 0) *strm << endl << indent;

    //*strm << "]" << endl;
}

/**
 * This worker method allows us to recursively traverse a "node" variables contents and
 * any child nodes will be traversed as well.
 */
void FoDapCovJsonTransform::transformParametersWorker(ostream *strm, vector<libdap::BaseType *> nodes,
    string indent, bool sendData)
{
    // Write the axes to strm
    *strm << indent << "\"parameters\": {";

    //if (leaves.size() > 0) *strm << endl;
    //for (std::vector<libdap::BaseType *>::size_type l = 0; l < leaves.size(); l++) {
    //    libdap::BaseType *v = leaves[l];
    //    BESDEBUG(FoDapCovJsonTransform_debug_key, "Processing LEAF: " << v->name() << endl);
    //    if (l > 0) {
    //        *strm << ",";
    //        *strm << endl;
    //    }
    //    transform(strm, v, indent + _indent_increment, sendData);
    //}
    //if (leaves.size() > 0) *strm << endl << indent;

    // Write down the parameters and values
    //*strm << _indent_increment << "\"parameters\": [";
    if (nodes.size() > 0) *strm << endl;
    for (std::vector<libdap::BaseType *>::size_type n = 0; n < nodes.size(); n++) {
        libdap::BaseType *v = nodes[n];
        transform(strm, v, _indent_increment + _indent_increment, sendData);
    }
    if (nodes.size() > 0) *strm << endl << indent;

    //*strm << "]" << endl;
}

void FoDapCovJsonTransform::transformReferenceWorker(ostream *strm, string indent)
{
    //     "referencing": [
    //     {
    //       "coordinates": ["t"],
    //       "system": {
    //         "type": "TemporalRS",
    //         "calendar": "Gregorian"
    //       }
    //     },
    //     {
    //       "coordinates": ["x", "y"],
    //       "system": {
    //         "type": "GeographicCRS",
    //         "id": "http://www.opengis.net/def/crs/OGC/1.3/CRS84"
    //       }
    //     }
    //     ]
    //   },
    //   "parameters": {

    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;

    *strm << "}," << endl << indent << "\"referencing\": [" << endl;
    *strm << indent << "{" << endl;
    *strm << child_indent1 << "\"coordinates\": [\"t\"]," << endl;
    *strm << child_indent1 << "\"system\": {" << endl;
    *strm << child_indent2 << "\"type\": \"TemporalRS\"," << endl;
    *strm << child_indent2 << "\"calendar\": \"Gregorian\"," << endl;
    *strm << child_indent1 << "}" << endl;
    *strm << indent << "}," << endl;

    *strm << indent << "{" << endl;
    *strm << child_indent1 << "\"coordinates\": [\"x\", \"y\"]," << endl;
    *strm << child_indent1 << "\"system\": {" << endl;
    *strm << child_indent2 << "\"type\": \"GeographicCRS\"," << endl;
    *strm << child_indent2 << "\"id\": \"http://www.opengis.net/def/crs/OGC/1.3/CRS84\"," << endl;
    *strm << child_indent1 << "}" << endl;
    *strm << indent << "}" << endl;
    *strm << indent << "]" << endl;
    *strm << _indent_increment << "}," << endl;
}

/**
 * Writes a CovJSON representation of the DDS to the passed stream. Data is sent if the sendData
 * flag is true. Otherwise, only metadata is sent.
 */
void FoDapCovJsonTransform::transform(ostream *strm, libdap::DDS *dds, string indent, bool sendData, FoDapCovJsonValidation fv)
{
    /*
     * w10 sees the world in terms of leaves and nodes. Leaves have data, nodes have other nodes and leaves.
     */
    vector<libdap::BaseType *> leaves;
    vector<libdap::BaseType *> nodes;

    libdap::DDS::Vars_iter vi = dds->var_begin();
    libdap::DDS::Vars_iter ve = dds->var_end();
    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            libdap::BaseType *v = *vi;
            libdap::Type type = v->type();
            if (type == libdap::dods_array_c) {
                type = v->var()->type();
            }
            if (v->is_constructor_type() || (v->is_vector_type() && v->var()->is_constructor_type())) {
                nodes.push_back(v);
            }
            else {
                leaves.push_back(v);
            }
        }
    }

    // TODO Begin CovJSON file formatting here ?? This where the first bracket is printed
    //      so I believe we should probably print the following here before we print the
    //      axes and referencing. I'm not quite sure where to print referencing yet,
    //      though. We could create a worker to print this formatting or just implement
    //      it right here.
    //
    // {
    //   "type": "Coverage",
    //   "domain": {
    //     "domainType": "Grid",
    //

    *strm << indent << "{" << endl; // beginning of the file

    string child_indent1 = indent + _indent_increment;

    *strm << child_indent1 << "\"type\": \"Coverage\"," << endl;

    *strm << child_indent1 << "\"domain\": {" << endl;

    string child_indent2 = child_indent1 + _indent_increment;

    /*
    * if:
    * 0 grid
    * 1 vertical profile
    * 2 pointseries
    * 3 point
    */
    if(fv.domainType == Grid) {
        *strm << child_indent2 << "\"domainType\": \"Grid\"," << endl;
    }
    else if(fv.domainType == VerticalProfile) {
        *strm << child_indent2 << "\"domainType\": \"VerticalProfile\"," << endl;
    }
    else if(fv.domainType == PointSeries) {
        *strm << child_indent2 << "\"domainType\": \"PointSeries\"," << endl;
    }
    else if(fv.domainType == Point) {
        *strm << child_indent2 << "\"domainType\": \"Point\"," << endl;
    }
    else {
		    throw BESInternalError("File out COVJSON, Could not define a domainType", __FILE__, __LINE__);
    }

    // Write this node's metadata (name & attributes)
    //writeDatasetMetadata(strm, dds, child_indent);

    // The axes are the first 3 leaves - the transformAxesWorker call should parse and
    // print these values. We need to format them accordingly.

    //     "axes": {
    //       "x": { "values": [values...] },
    //       "y": { "values": [values...] },
    //       "t": { "values": [values...] }
    //     },
    //

    transformAxesWorker(strm, leaves, child_indent2, sendData);

    transformReferenceWorker(strm, child_indent2);

    transformParametersWorker(strm, nodes, child_indent1, sendData);

    *strm << child_indent1 << "}" << endl;

    *strm << indent << "}" << endl; // end of the file
}

/**
 * Write the CovJSON representation of the passed BaseType instance. If the
 * parameter sendData is true then include the data.
 */
void FoDapCovJsonTransform::transform(ostream *strm, libdap::BaseType *bt, string indent, bool sendData)
{
    switch (bt->type()) {
    // Handle the atomic types - that's easy!
    case libdap::dods_byte_c:
    case libdap::dods_int16_c:
    case libdap::dods_uint16_c:
    case libdap::dods_int32_c:
    case libdap::dods_uint32_c:
    case libdap::dods_float32_c:
    case libdap::dods_float64_c:
    case libdap::dods_str_c:
    case libdap::dods_url_c:
        transformAtomic(strm, bt, indent, sendData);
        break;

    case libdap::dods_structure_c:
        transform(strm, (libdap::Structure *) bt, indent, sendData);
        break;

    case libdap::dods_grid_c:
        transform(strm, (libdap::Grid *) bt, indent, sendData);
        break;

    case libdap::dods_sequence_c:
        transform(strm, (libdap::Sequence *) bt, indent, sendData);
        break;

    case libdap::dods_array_c:
        transform(strm, (libdap::Array *) bt, indent, sendData);
        break;

    case libdap::dods_int8_c:
    case libdap::dods_uint8_c:
    case libdap::dods_int64_c:
    case libdap::dods_uint64_c:
    case libdap::dods_enum_c:
    case libdap::dods_group_c: {
        string s = (string) "File out COVJSON, " + "DAP4 types not yet supported.";
        throw BESInternalError(s, __FILE__, __LINE__);
        break;
    }

    default: {
        string s = (string) "File out COVJSON, " + "Unrecognized type.";
        throw BESInternalError(s, __FILE__, __LINE__);
        break;
    }

    }
}

/**
 * Write the CovJSON representation of the passed BaseType instance - which had better be one of the
 * atomic DAP types. If the parameter sendData is true then include the data.
 */
void FoDapCovJsonTransform::transformAtomic(ostream *strm, libdap::BaseType *b, string indent, bool sendData)
{
    *strm << indent << "{" << endl;

    string childindent = indent + _indent_increment;

    //writeLeafMetadata(strm, b, childindent);
    writeLeafMetadata(strm, b, indent);

    *strm << childindent << "\"shape\": [1]," << endl;

    if (sendData) {
        // Data
        *strm << childindent << "\"data\": [";

        if (b->type() == libdap::dods_str_c || b->type() == libdap::dods_url_c) {
            libdap::Str *strVar = (libdap::Str *) b;
            std::string tmpString = strVar->value();
            *strm << "\"" << focovjson::escape_for_covjson(tmpString) << "\"";
        }
        else {
            b->print_val(*strm, "", false);
        }

        *strm << "]";
    }

}

/**
 * Write the CovJSON representation of the passed DAP Array instance - which had better be one of
 * atomic DAP types. If the parameter sendData is true then include the data.
 */
void FoDapCovJsonTransform::transform(ostream *strm, libdap::Array *a, string indent, bool sendData)
{

    BESDEBUG(FoDapCovJsonTransform_debug_key,
        "FoCovJsonTransform::transform() - Processing Array. " << " a->type(): " << a->type() << " a->var()->type(): " << a->var()->type() << endl);

    switch (a->var()->type()) {
    // Handle the atomic types - that's easy!
    case libdap::dods_byte_c:
        covjsonSimpleTypeArray<libdap::dods_byte>(strm, a, indent, sendData);
        break;

    case libdap::dods_int16_c:
        covjsonSimpleTypeArray<libdap::dods_int16>(strm, a, indent, sendData);
        break;

    case libdap::dods_uint16_c:
        covjsonSimpleTypeArray<libdap::dods_uint16>(strm, a, indent, sendData);
        break;

    case libdap::dods_int32_c:
        covjsonSimpleTypeArray<libdap::dods_int32>(strm, a, indent, sendData);
        break;

    case libdap::dods_uint32_c:
        covjsonSimpleTypeArray<libdap::dods_uint32>(strm, a, indent, sendData);
        break;

    case libdap::dods_float32_c:
        covjsonSimpleTypeArray<libdap::dods_float32>(strm, a, indent, sendData);
        break;

    case libdap::dods_float64_c:
        covjsonSimpleTypeArray<libdap::dods_float64>(strm, a, indent, sendData);
        break;

    case libdap::dods_str_c: {
        covjsonStringArray(strm, a, indent, sendData);
        break;
    }

    case libdap::dods_url_c: {
        covjsonStringArray(strm, a, indent, sendData);
        break;
    }

    case libdap::dods_structure_c: {
        throw BESInternalError("File out COVJSON, Arrays of Structure objects not a supported return type.", __FILE__, __LINE__);
        break;
    }
    case libdap::dods_grid_c: {
        throw BESInternalError("File out COVJSON, Arrays of Grid objects not a supported return type.", __FILE__, __LINE__);
        break;
    }

    case libdap::dods_sequence_c: {
        throw BESInternalError("File out COVJSON, Arrays of Sequence objects not a supported return type.", __FILE__, __LINE__);
        break;
    }

    case libdap::dods_array_c: {
        throw BESInternalError("File out COVJSON, Arrays of Array objects not a supported return type.", __FILE__, __LINE__);
        break;
    }
    case libdap::dods_int8_c:
    case libdap::dods_uint8_c:
    case libdap::dods_int64_c:
    case libdap::dods_uint64_c:
        // case libdap::dods_url4_c:
    case libdap::dods_enum_c:
    case libdap::dods_group_c: {
        throw BESInternalError("File out COVJSON, DAP4 types not yet supported.", __FILE__, __LINE__);
        break;
    }

    default: {
        throw BESInternalError("File out COVJSON, Unrecognized type.", __FILE__, __LINE__);
        break;
    }

    }

}

/**
 * Write the CovJSON representation of the passed DAP AttrTable instance.
 * Supports multi-valued attributes and nested attributes.
 */
void FoDapCovJsonTransform::transform(ostream *strm, libdap::AttrTable &attr_table, string indent)
{

    string child_indent = indent + _indent_increment;

    // Start the attributes block
    *strm << indent << "\"attributes\": [";

//	if(attr_table.get_name().length()>0)
//		*strm  << endl << child_indent << "{\"name\": \"name\", \"value\": \"" << attr_table.get_name() << "\"},";

// Only do more if there are actually attributes in the table
    if (attr_table.get_size() != 0) {
        *strm << endl;
        libdap::AttrTable::Attr_iter begin = attr_table.attr_begin();
        libdap::AttrTable::Attr_iter end = attr_table.attr_end();

        for (libdap::AttrTable::Attr_iter at_iter = begin; at_iter != end; at_iter++) {

            switch (attr_table.get_attr_type(at_iter)) {
            case libdap::Attr_container: {
                libdap::AttrTable *atbl = attr_table.get_attr_table(at_iter);

                // not first thing? better use a comma...
                if (at_iter != begin) *strm << "," << endl;

                // Attribute Containers need to be opened and then a recursive call gets made
                *strm << child_indent << "{" << endl;

                // If the table has a name, write it out as a CovJSON property.
                if (atbl->get_name().length() > 0)
                    *strm << child_indent + _indent_increment << "\"" << atbl->get_name() << "\" {" << endl;

                // Recursive call for child attribute table.
                transform(strm, *atbl, child_indent + _indent_increment);
                *strm << endl << child_indent << "}";

                break;

            }
            default: {
                // not first thing? better use a comma...
                if (at_iter != begin) *strm << "," << endl;

                // Open attribute object, write name
                *strm << child_indent << "{\"name\": \"" << attr_table.get_name(at_iter) << "\", ";

                // Open value array
                *strm << "\"value\": [";
                vector<std::string> *values = attr_table.get_attr_vector(at_iter);
                // write values
                for (std::vector<std::string>::size_type i = 0; i < values->size(); i++) {

                    // not first thing? better use a comma...
                    if (i > 0) *strm << ",";

                    // Escape the double quotes found in String and URL type attribute values.
                    if (attr_table.get_attr_type(at_iter) == libdap::Attr_string
                        || attr_table.get_attr_type(at_iter) == libdap::Attr_url) {
                        *strm << "\"";
                        // string value = (*values)[i] ;
                        *strm << focovjson::escape_for_covjson((*values)[i]);
                        *strm << "\"";
                    }
                    else {

                        *strm << (*values)[i];
                    }

                }
                // close value array
                *strm << "]}";
                break;
            }

            }
        }

        *strm << endl << indent;
    }

    // close AttrTable COVJSON

    *strm << "]";
}
