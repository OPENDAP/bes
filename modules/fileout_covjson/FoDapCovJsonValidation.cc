
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

/** @brief dumps information about this transformation object for debugging
 * purposes
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

FoDapCovJsonValidation::FoDapCovJsonValidation(libdap::DDS *dds) : _dds(dds)
{
    if (!_dds) throw BESInternalError("File out COVJSON, null DDS passed to constructor", __FILE__, __LINE__);
}

/*
* This method is for validating a dds to see if it is possible to convert the dataset into the coverageJson format
*/
void FoDapCovJsonValidation::validateDataset()
{
    hasX = false;
    hasY = false;
    hasT = false;

    // This is just here to clear out the log file
    //ofstream tempOut;
    //string tempFileName = "/home/ubuntu/hyrax/dds.log";
    //tempOut.open(tempFileName.c_str(), ios::trunc);
    //if(tempOut.fail()) {cout << "Could not open " << tempFileName << endl;exit(EXIT_FAILURE);}
    //tempOut.close();
    validateDataset(_dds);
}

/**
 * Writes json opener for a DAP object that is seen as a "leaf" in w10n semantics.
 * Header includes object name. attributes, and  type.
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

    //tempOut  << "\"name\": \"" << bt->name() << "\"," << endl;

    // See if the actual array name matches up
    checkAttribute("",bt->name());
    tempOut.close();
}

template<typename T>
unsigned int FoDapCovJsonValidation::covjson_simple_type_array_worker(T *values, unsigned int indx,
    vector<unsigned int> *shape, unsigned int currentDim)
{
    fstream tempOut;
    string tempFileName = "/home/ubuntu/hyrax/dds.log";

    if(tempOut.fail()) {
       cout << "Could not open " << tempFileName << endl;
       exit(EXIT_FAILURE);
    }

    //tempOut << "[";

    unsigned int currentDimSize = (*shape)[currentDim];

    for (unsigned int i = 0; i < currentDimSize; i++) {
        if (currentDim < shape->size() - 1) {
            BESDEBUG(FoDapCovJsonValidation_debug_key,
                "covjson_simple_type_array_worker() - Recursing! indx:  " << indx << " currentDim: " << currentDim << " currentDimSize: " << currentDimSize << endl);
            indx = covjson_simple_type_array_worker<T>(values, indx, shape, currentDim + 1);
            //if (i + 1 != currentDimSize) //tempOut << ", ";
        }
        else {
            //if (i) //tempOut << ", ";
            if (typeid(T) == typeid(std::string)) {
                // Strings need to be escaped to be included in a JSON object.
                string val = reinterpret_cast<string*>(values)[indx++]; // ((string *) values)[indx++];
                //tempOut << "\"" << focovjson::escape_for_covjson(val) << "\"";
            }
            //else {
                //tempOut << values[indx++];
            //}
        }
    }
    //tempOut << "]";
    tempOut.close();
    return indx;
}


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
    transform_node_worker(leaves, nodes);
}

void FoDapCovJsonValidation::transform_node_worker(vector<libdap::BaseType *> leaves, vector<libdap::BaseType *> nodes)
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
        // case libdap::dods_url4_c:
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
 * DAP Constructor types are semantically equivalent to a w10n node type so they
 * must be represented as a collection of child nodes and leaves.
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

    transform_node_worker(leaves, nodes);
}

void FoDapCovJsonValidation::covjson_string_array(libdap::Array *a)
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
        if (i > 0) //tempOut << ",";
        shapeOrig = shape[i];
        //tempOut << shapeOrig;
    }
    //tempOut << "]";

        //tempOut << "," << endl;
        //unsigned int indx;

        vector<std::string> sourceValues;
        a->value(sourceValues);

    validateDataset(a->get_attr_table());

    tempOut.close();
}

/**
 * Write the json representation of the passed DAP Array instance - which had better be one of
 * atomic DAP types. If the parameter sendData is true then include the data.
 */
void FoDapCovJsonValidation::validateDataset(libdap::Array *a)
{

    BESDEBUG(FoDapCovJsonValidation_debug_key,
        "FoCovJsonTransform::validateDataset() - Processing Array. " << " a->type(): " << a->type() << " a->var()->type(): " << a->var()->type() << endl);

    switch (a->var()->type()) {
    // Handle the atomic types - that's easy!
    case libdap::dods_byte_c:
        covjson_simple_type_array<libdap::dods_byte>(a);
        break;

    case libdap::dods_int16_c:
        covjson_simple_type_array<libdap::dods_int16>(a);
        break;

    case libdap::dods_uint16_c:
        covjson_simple_type_array<libdap::dods_uint16>(a);
        break;

    case libdap::dods_int32_c:
        covjson_simple_type_array<libdap::dods_int32>(a);
        break;

    case libdap::dods_uint32_c:
        covjson_simple_type_array<libdap::dods_uint32>(a);
        break;

    case libdap::dods_float32_c:
        covjson_simple_type_array<libdap::dods_float32>(a);
        break;

    case libdap::dods_float64_c:
        covjson_simple_type_array<libdap::dods_float64>(a);
        break;

    case libdap::dods_str_c: {
        covjson_string_array(a);
        break;
    }

    case libdap::dods_url_c: {
        covjson_string_array(a);
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

template<typename T>
void FoDapCovJsonValidation::covjson_simple_type_array(libdap::Array *a)
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
    //long length = focovjson::computeConstrainedShape(a, &shape);

    //tempOut << "\"shape\": [";
    for (std::vector<unsigned int>::size_type i = 0; i < shape.size(); i++) {
        if (i > 0) tempOut << ",";
        //tempOut << shape[i];
        shapeOrig = shape[i];
    }
    //tempOut << "]" << "," << endl;

    tempOut.close();
    validateDataset(a->get_attr_table());
}

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
                        //tempOut << attr_table.get_name(at_iter) << endl;
                        //tempOut << (*values)[i].c_str() << endl;
                        checkAttribute(attr_table.get_name(at_iter),(*values)[i].c_str());
                        printf("\n%s\n", (*values)[i].c_str());
                    }
                    break;
                }
            }
        }
    }
    tempOut.close();
}

/*
This function checks the passed in attribute to see if it is one of the required attributes for covjson
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
    else if(name == "units" && value =="degrees_north"){
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

/*
This function checks to see if the attributes needed to create covjson have been found
*/
bool FoDapCovJsonValidation::canConvert()
{
    ofstream tempOut;
    string tempFileName = "/home/ubuntu/hyrax/dds.log";
    tempOut.open(tempFileName.c_str(), ios::app);

    if(tempOut.fail()) {
       cout << "Could not open " << tempFileName << endl;
       exit(EXIT_FAILURE);
    }

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
    tempOut.close();
    return canConvert;
}
