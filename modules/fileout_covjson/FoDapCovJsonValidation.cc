
// -*- mode: c++; c-basic-offset:4 -*-
//
// FoDapCovJsonValidation.cc
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

#include "FoDapCovJsonValidation.h"
#include "focovjson_utils.h"

#define FoDapCovJsonValidation_debug_key "focovjson"


/**
 * @brief Dumps information about this transformation object for debugging
 *   purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including all of the FoJson objects converted from DAP objects that are
 * to be sent to the netcdf file.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FoDapCovJsonValidation::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FoDapCovJsonValidate::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_dds != 0) {
        _dds->print(strm);
    }
    BESIndent::UnIndent();
}


/**
 * @brief Get the CovJSON encoding for a DDS
 *
 * Set up the CovJSON output transform object. This constructor builds
 * an object that will build a CovJSON encoding for a DDS. This class can
 * return both the entire DDS, including data, and a metadata-only
 * response.
 *
 * @note The 'transform' method is used to build the response and a
 *   bool flag is passed to it to select data or metadata. However, if
 *   that flag is true and the DDS does not already contain data, the
 *   result is undefined.
 *
 * @param dds DDS object
 * @throws BESInternalError if the DDS* is null or if localfile is empty.
 */
FoDapCovJsonValidation::FoDapCovJsonValidation(libdap::DDS *dds) : _dds(dds)
{
    if (!_dds) throw BESInternalError("File out COVJSON, null DDS passed to constructor", __FILE__, __LINE__);
}


/**
 * @brief For validating a DDS to see if it is possible to convert the dataset
 *   into the CoverageJson format
 *
 * This function will determine what axis have been provided in the given dds
 * object. Storing this data will help us to build a CovJSON encoding for a DDS.
 *
 * @note Uses global _.
 */
void FoDapCovJsonValidation::validateDataset()
{
    hasX = false;
    hasY = false;
    hasZ = false;
    hasT = false;

    validateDataset(_dds);
}


/**
 * @brief Prints leaf metadata to dds.log file for testing and debugging purposes.
 *
 * Gets the current leaf's attributes, prints the metadata, then runs checkAttribute()
 * format to the CovJSON output stream. The scope of what metadata is retrieved
 * to determine what axis variables we have to work with (t, x, y, and z).
 *
 * @param bt Pointer to a BaseType vector containing values/attributes
 */
void FoDapCovJsonValidation::writeLeafMetadata(libdap::BaseType *bt)
{
    ofstream tempOut;
    string tempFileName = "/home/ubuntu/hyrax/dds.log";
    tempOut.open(tempFileName.c_str(), ios::app);

    if(tempOut.fail()) {
       cout << "Could not open " << tempFileName << endl;
       exit(EXIT_FAILURE);
    }

    // See if the actual array name matches up
    checkAttribute("", bt->name());
    tempOut.close();
}


/**
 * @brief Validates for a CovJSON representation of the DDS to the passed stream.
 *
 * @note w10 sees the world in terms of leaves and nodes. Leaves have data, nodes
 *   have other nodes and leaves.
 *
 * @param dds Pointer to a DDS vector, which contains both w10n leaves and nodes
 */
void FoDapCovJsonValidation::validateDataset(libdap::DDS *dds)
{
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
    transformNodeWorker(leaves, nodes);
}


/**
 * @brief Worker method allows us to recursively traverse a node's range
 *    of values.
 *
 * @param leaves Pointer to a vector of BaseTypes, which represent w10n leaves
 * @param nodes Pointer to a vector of BaseTypes, which represent w10n nodes
 */
void FoDapCovJsonValidation::transformNodeWorker(vector<libdap::BaseType *> leaves, vector<libdap::BaseType *> nodes)
{
    for (std::vector<libdap::BaseType *>::size_type l = 0; l < leaves.size(); l++) {
        libdap::BaseType *v = leaves[l];
        BESDEBUG(FoDapCovJsonValidation_debug_key, "Processing LEAF: " << v->name() << endl);
        validateDataset(v);
    }

    for (std::vector<libdap::BaseType *>::size_type n = 0; n < nodes.size(); n++) {
        libdap::BaseType *v = nodes[n];
        validateDataset(v);
    }

}


/**
 * @brief Validates for a CovJSON representation of the DDS to the passed stream.
 *
 * @param bt Pointer to a BaseType vector containing values/attributes
 */
void FoDapCovJsonValidation::validateDataset(libdap::BaseType *bt)
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
        validateDataset(bt->get_attr_table());
        break;

    case libdap::dods_structure_c:
        validateDataset((libdap::Structure *) bt);
        break;

    case libdap::dods_grid_c:
        //validateDataset((libdap::Grid *) bt);
        break;

    case libdap::dods_sequence_c:
        validateDataset((libdap::Sequence *) bt);
        break;

    case libdap::dods_array_c:
        validateDataset((libdap::Array *) bt);
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
 * @brief Contructs a new set of leaves and nodes derived from a previous set of nodes.
 *
 * @note DAP Constructor types are semantically equivalent to a w10n node type so they
 *   must be represented as a collection of child nodes and leaves.
 *
 * @param cnstrctr a new collection of child nodes and leaves
 */
void FoDapCovJsonValidation::validateDataset(libdap::Constructor *cnstrctr)
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

    validateDataset(cnstrctr->get_attr_table());

    transformNodeWorker(leaves, nodes);
}


/**
 * @brief Writes the CovJSON representation of the passed DAP Array of string types
 *   to the dds.log file.
 *
 * For each variable in the DDS, write out that variable and its
 * attributes as CovJSON. Each OPeNDAP data type translates into a
 * particular CovJSON type. Also write out any global attributes stored at the
 * top level of the DataDDS.
 *
 * @param a Source data array - write out data or metadata from or about this array
 */
void FoDapCovJsonValidation::covjsonStringArray(libdap::Array *a)
{
    ofstream tempOut;
    string tempFileName = "/home/ubuntu/hyrax/dds.log";
    tempOut.open(tempFileName.c_str(), ios::app);

    if(tempOut.fail()) {
       cout << "Could not open " << tempFileName << endl;
       exit(EXIT_FAILURE);
    }

    int numDim = a->dimensions(true);
    vector<unsigned int> shape(numDim);
    //long length = focovjson::computeConstrainedShape(a, &shape);

    for (std::vector<unsigned int>::size_type i = 0; i < shape.size(); i++) {
        shapeOrig = shape[i];
    }

    vector<std::string> sourceValues;
    a->value(sourceValues);

    validateDataset(a->get_attr_table());

    tempOut.close();
}


/**
 * @brief Validates for a CovJSON representation of the DDS to the passed stream.
 *
 * Write the CovJSON representation of the passed DAP Array instance - which had better be one
 * of the atomic DAP types.
 *
 * @param a Source data array containing simple type values
 */
void FoDapCovJsonValidation::validateDataset(libdap::Array *a)
{

    BESDEBUG(FoDapCovJsonValidation_debug_key,
        "FoCovJsonTransform::validateDataset() - Processing Array. " << " a->type(): " << a->type() << " a->var()->type(): " << a->var()->type() << endl);

    switch (a->var()->type()) {
    // Handle the atomic types - that's easy!
    case libdap::dods_byte_c:
        covjsonSimpleTypeArray<libdap::dods_byte>(a);
        break;

    case libdap::dods_int16_c:
        covjsonSimpleTypeArray<libdap::dods_int16>(a);
        break;

    case libdap::dods_uint16_c:
        covjsonSimpleTypeArray<libdap::dods_uint16>(a);
        break;

    case libdap::dods_int32_c:
        covjsonSimpleTypeArray<libdap::dods_int32>(a);
        break;

    case libdap::dods_uint32_c:
        covjsonSimpleTypeArray<libdap::dods_uint32>(a);
        break;

    case libdap::dods_float32_c:
        covjsonSimpleTypeArray<libdap::dods_float32>(a);
        break;

    case libdap::dods_float64_c:
        covjsonSimpleTypeArray<libdap::dods_float64>(a);
        break;

    case libdap::dods_str_c: {
        covjsonStringArray(a);
        break;
    }

    case libdap::dods_url_c: {
        covjsonStringArray(a);
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
 * @brief Writes the CovJSON representation of the passed DAP Array of simple types
 *   to the dds.log file.
 *
 * For each variable in the DDS, write out that variable and its
 * attributes as CovJSON. Each OPeNDAP data type translates into a
 * particular CovJSON type. Also write out any global attributes stored at the
 * top level of the DataDDS.
 *
 * @param a Source data array - write out data or metadata from or about this array
 */
template<typename T>
void FoDapCovJsonValidation::covjsonSimpleTypeArray(libdap::Array *a)
{
    writeLeafMetadata(a);
    ofstream tempOut;
    string tempFileName = "/home/ubuntu/hyrax/dds.log";
    tempOut.open(tempFileName.c_str(), ios::app);

    if(tempOut.fail()) {
       cout << "Could not open " << tempFileName << endl;
       exit(EXIT_FAILURE);
    }

    int numDim = a->dimensions(true);
    vector<unsigned int> shape(numDim);

    for (std::vector<unsigned int>::size_type i = 0; i < shape.size(); i++) {
        shapeOrig = shape[i];
    }

    tempOut.close();
    validateDataset(a->get_attr_table());
}


/**
 * @brief  Validates for a CovJSON representation of the DDS to the to the dds.log
 *   file.
 *
 * Write the CovJSON representation of the passed DAP AttrTable instance.
 * Supports multi-valued attributes and nested attributes.
 *
 * @note This function may be completely removed at some point.
 *
 * @param attr_table Reference to an AttrTable containing attribute values
 */
void FoDapCovJsonValidation::validateDataset(libdap::AttrTable &attr_table)
{
    ofstream tempOut;
    string tempFileName = "/home/ubuntu/hyrax/dds.log";
    tempOut.open(tempFileName.c_str(), ios::app);

    if(tempOut.fail()) {
        cout << "Could not open " << tempFileName << endl;
        exit(EXIT_FAILURE);
    }

    if (attr_table.get_size() != 0) {
        libdap::AttrTable::Attr_iter begin = attr_table.attr_begin();
        libdap::AttrTable::Attr_iter end = attr_table.attr_end();

        for (libdap::AttrTable::Attr_iter at_iter = begin; at_iter != end; at_iter++) {
            switch (attr_table.get_attr_type(at_iter)) {

                case libdap::Attr_container: {
                    libdap::AttrTable *atbl = attr_table.get_attr_table(at_iter);
                    validateDataset(*atbl);
                    break;
                }

                default: {
                    vector<std::string> *values = attr_table.get_attr_vector(at_iter);
                    // write values
                    for (std::vector<std::string>::size_type i = 0; i < values->size(); i++) {
                        checkAttribute(attr_table.get_name(at_iter),(*values)[i].c_str());
                    }
                    break;
                }
            }
        }
    }
    tempOut.close();
}


/**
 * @brief Validates the passed in attribute name and value to see if it is one of the
 *    required attributes for a CoverageJSON file.
 *
 * @param name an attribute's name
 * @param value an attribute's value
 */
void FoDapCovJsonValidation::checkAttribute(std::string name, std::string value)
{
    if(value == "lon" || value == "longitude"|| value == "LONGITUDE"|| value == "Longitude"|| value == "x"){
        hasX = true;
        if(shapeOrig != NULL) {
            shapeX = shapeOrig;
        }
    }
    else if(name=="units" && value == "degrees_east") {
        hasX = true;
        if(shapeOrig != NULL) {
            shapeX = shapeOrig;
        }
    }

    if(value == "lat" || value == "latitude" || value == "LATITUDE" || value == "Latitude" || value == "y") {
        if(shapeOrig != NULL) {
            shapeY = shapeOrig;
        }
        hasY = true;
    }
    else if(name == "units" && value =="degrees_north") {
        if(shapeOrig != NULL) {
            shapeY = shapeOrig;
        }
        hasY = true;
    }

    if (value == "t" || value == "TIME" || value == "time" || value == "s" || value == "seconds" || value == "Seconds"){
        if(shapeOrig != NULL) {
            shapeT = shapeOrig;
        }
        hasT = true;
    }

    shapeOrig = NULL;
}


/**
 * @brief Validates the passed in attribute names and values to ensure the necessary
 *   attributes are present for generating a CoverageJSON file.
 */
bool FoDapCovJsonValidation::canConvert()
{
    bool canConvert = false;

    if(hasX && hasY && hasT) {
        if(shapeX > 1 && shapeY > 1 && shapeT >= 0) {
            domainType = 0; // Grid
            canConvert = true;
        }
        else if(shapeX == 1 && shapeY == 1 && (shapeT <= 1 && shapeT >= 0)) {
            domainType = 1; // Vertical Profile
            canConvert = true;
        }
        else if(shapeX == 1 && shapeY == 1 && shapeT >= 0) {
            domainType = 2; // Point Series
            canConvert = true;
        }
        else if(shapeX == 1 && shapeY == 1 && shapeT >= 0) {
            domainType = 3; // Point
            canConvert = true;
        }
    }
    return canConvert;
}
