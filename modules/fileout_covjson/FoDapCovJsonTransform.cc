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
#include <iomanip> // setprecision
#include <sstream> // stringstream

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

const int int_64_precision = 15; // 15 digits to the right of the decimal point.
const int max_axes = 4; // (x, y , z, and t) :: if axisCount <= max_axes


/**
 * @brief Checks the spacial/temporal dimensions that we've obtained can be
 *    used to convert to a CovJSON file.
 *
 * @note also sets the domainType based on the given dimensions
 *
 * @returns true if can convert to CovJSON, false if cannot convert
 */
bool FoDapCovJsonTransform::canConvert()
{
    if(xExists && yExists && tExists) {
        if(shapeVals[0] > 1 && shapeVals[1] > 1 && shapeVals[2] >= 0) {
            domainType = 0; // Grid
            return true;
        }
        else if(shapeVals[0] == 1 && shapeVals[1] == 1 && (shapeVals[2] <= 1 && shapeVals[2] >= 0)) {
            domainType = 1; // Vertical Profile
            return true;
        }
        else if(shapeVals[0] == 1 && shapeVals[1] == 1 && shapeVals[2] >= 0) {
            domainType = 2; // Point Series
            return true;
        }
        else if(shapeVals[0] == 1 && shapeVals[1] == 1 && shapeVals[2] >= 0) {
            domainType = 3; // Point
            return true;
        }
    }
    return false;
}


/**
 * @brief Writes each of the variables in a given container to the CovJSON stream.
 *    For each variable in the DDS, write out that variable as CovJSON.
 *
 * @param ostrm Write the CovJSON to this output stream
 * @param values Source array of type T which we want to write to stream
 * @param indx variable for storing the current indexed value
 * @param shape a vector storing the shape's dimensional values
 * @param currentDim the current dimension we are getting values from
 * @param a the current axis we are getting values for
 *
 * @returns the most recently completed index
 */
template<typename T>
unsigned int FoDapCovJsonTransform::covjsonSimpleTypeArrayWorker(ostream *strm, T *values, unsigned int indx,
    vector<unsigned int> *shape, unsigned int currentDim, struct Axis *a)
{
    unsigned int currentDimSize = (*shape)[currentDim];
    ostringstream newValues;

    //*strm << "[";
    newValues << "[";
    for (unsigned int i = 0; i < currentDimSize; i++) {
        if (currentDim < shape->size() - 1) {
            BESDEBUG(FoDapCovJsonTransform_debug_key,
                "covjsonSimpleTypeArrayWorker() - Recursing! indx:  " << indx << " currentDim: " << currentDim << " currentDimSize: " << currentDimSize << endl);
            indx = covjsonSimpleTypeArrayWorker<T>(strm, values, indx, shape, currentDim + 1, a);
            if (i + 1 != currentDimSize) {
                //*strm << ", ";
                newValues << ", ";
            }
        }
        else {
            if (i) {
                //*strm << ", ";
                newValues << ", ";
            }
            if (typeid(T) == typeid(std::string)) {
                // Strings need to be escaped to be included in a CovJSON object.
                string val = reinterpret_cast<string*>(values)[indx++];
                //*strm << "\"" << focovjson::escape_for_covjson(val) << "\"";
                newValues << "\"" << focovjson::escape_for_covjson(val) << "\"";
            }
            else {
                //*strm << values[indx++];
                newValues << values[indx++];
            }
        }
    }
    //*strm << "]";
    newValues << "]";

    a->values += newValues.str();

    return indx;
}


/**
 * @brief Writes each of the variables in a given container to the CovJSON stream.
 *    For each variable in the DDS, write out that variable as CovJSON.
 *
 * @param ostrm Write the CovJSON to this output stream
 * @param values Source array of type T which we want to write to stream
 * @param indx variable for storing the current indexed value
 * @param shape a vector storing the shape's dimensional values
 * @param currentDim the current dimension we are printing values from
 * @param p the current parameter we are getting values for
 *
 * @returns the most recently completed index
 */
template<typename T>
unsigned int FoDapCovJsonTransform::covjsonSimpleTypeArrayWorker(ostream *strm, T *values, unsigned int indx,
    vector<unsigned int> *shape, unsigned int currentDim, struct Parameter *p)
{
    unsigned int currentDimSize = (*shape)[currentDim];
    ostringstream newValues;

    //*strm << "[";
    newValues << "[";
    for (unsigned int i = 0; i < currentDimSize; i++) {
        if (currentDim < shape->size() - 1) {
            BESDEBUG(FoDapCovJsonTransform_debug_key,
                "covjsonSimpleTypeArrayWorker() - Recursing! indx:  " << indx << " currentDim: " << currentDim << " currentDimSize: " << currentDimSize << endl);
            indx = covjsonSimpleTypeArrayWorker<T>(strm, values, indx, shape, currentDim + 1, p);
            if (i + 1 != currentDimSize) {
                //*strm << ", ";
                newValues << ", ";
            }
        }
        else {
            if (i) {
                //*strm << ", ";
                newValues << ", ";
            }
            if (typeid(T) == typeid(std::string)) {
                // Strings need to be escaped to be included in a CovJSON object.
                string val = reinterpret_cast<string*>(values)[indx++];
                //*strm << "\"" << focovjson::escape_for_covjson(val) << "\"";
                newValues << "\"" << focovjson::escape_for_covjson(val) << "\"";
            }
            else {
                //*strm << values[indx++];
                newValues << values[indx++];
            }
        }
    }
    //*strm << "]";
    newValues << "]";

    p->values += newValues.str();

    return indx;
}


/**
 * @brief Writes the CovJSON representation of the passed DAP Array of simple types.
 *   If the parameter "sendData" evaluates to true then data will also be sent.
 *
 * For each variable in the DDS, write out that variable and its
 * attributes as CovJSON. Each OPeNDAP data type translates into a
 * particular CovJSON type. Also write out any global attributes stored at the
 * top level of the DataDDS.
 *
 * @note If sendData is true but the DDS does not contain data, the result
 * is undefined.
 *
 * @param ostrm Write the CovJSON to this stream
 * @param a Source data array - write out data or metadata from or about this array
 * @param indent Indent the output so humans can make sense of it
 * @param sendData true: send data; false: send metadata
 */
template<typename T>
void FoDapCovJsonTransform::covjsonSimpleTypeArray(ostream *strm, libdap::Array *a, string indent, bool sendData)
{
    string childindent = indent + _indent_increment;

    bool *axisRetrieved = new bool;
    bool *parameterRetrieved = new bool;
    *axisRetrieved = false;
    *parameterRetrieved = false;
    getAttributes(strm, a->get_attr_table(), a->name(), axisRetrieved, parameterRetrieved);

    // sendData = false; // For testing purposes

    // If we are dealing with an Axis
    if((*axisRetrieved == true && *parameterRetrieved == false) && (axisCount <= max_axes)) {
        struct Axis *currAxis;
        currAxis = axes[axisCount - 1];

        int numDim = a->dimensions(true);
        vector<unsigned int> shape(numDim);
        long length = focovjson::computeConstrainedShape(a, &shape);

        if(sendData) {
            // *strm << childindent << "\"values\": ";;
            currAxis->values += "\"values\": ";

            unsigned int indx = 0;
            vector<T> src(length);
            a->value(&src[0]);

            // I added this, and a corresponding block in FoInstance... because I fixed
            // an issue in libdap::Float64 where the precision was not properly reset
            // in it's print_val() method. Because of that error, precision was (left at)
            // 15 when this code was called until I fixed that method. Then this code
            // was not printing at the required precision. jhrg 9/14/15
            if(typeid(T) == typeid(libdap::dods_float64)) {
                //streamsize prec = strm->precision(int_64_precision);
                try {
                    indx = covjsonSimpleTypeArrayWorker(strm, &src[0], 0, &shape, 0, currAxis);
                    //strm->precision(prec);
                }
                catch(...) {
                    //strm->precision(prec);
                    throw;
                }
            }
            else {
                indx = covjsonSimpleTypeArrayWorker(strm, &src[0], 0, &shape, 0, currAxis);
            }
            assert(length == indx);
        }
        else {
            // *strm << childindent << "\"values\": []";
            currAxis->values += "\"values\": []";
        }
    }

    // If we are dealing with a Parameter
    else if(*axisRetrieved == false && *parameterRetrieved == true) {
        struct Parameter *currParameter;
        currParameter = parameters[parameterCount - 1];

        int numDim = a->dimensions(true);
        vector<unsigned int> shape(numDim);
        long length = focovjson::computeConstrainedShape(a, &shape);

        // *strm << childindent << "\"shape\": [";
        currParameter->shape += "\"shape\": [";
        for(std::vector<unsigned int>::size_type i = 0; i < shape.size(); i++) {
            if (i > 0) {
                // *strm << ", ";
                currParameter->shape += ", ";
            }

            ostringstream otemp;
            istringstream itemp;
            int tempVal = 0;

            // *strm << shape[i];
            otemp << shape[i];

            istringstream (otemp.str());
            istringstream (otemp.str()) >> tempVal;

            shapeVals.push_back(tempVal);

            currParameter->shape += otemp.str();
        }
        // *strm << "]," << endl;
        currParameter->shape += "],";

        if(sendData) {
            // *strm << childindent << "\"values\": ";
            currParameter->values += "\"values\": ";
            unsigned int indx = 0;
            vector<T> src(length);
            a->value(&src[0]);

            // I added this, and a corresponding block in FoInstance... because I fixed
            // an issue in libdap::Float64 where the precision was not properly reset
            // in it's print_val() method. Because of that error, precision was (left at)
            // 15 when this code was called until I fixed that method. Then this code
            // was not printing at the required precision. jhrg 9/14/15
            if(typeid(T) == typeid(libdap::dods_float64)) {
                // streamsize prec = strm->precision(int_64_precision);
                try {
                    indx = covjsonSimpleTypeArrayWorker(strm, &src[0], 0, &shape, 0, currParameter);
                    // strm->precision(prec);
                }
                catch(...) {
                    // strm->precision(prec);
                    throw;
                }
            }
            else {
                indx = covjsonSimpleTypeArrayWorker(strm, &src[0], 0, &shape, 0, currParameter);
            }
            assert(length == indx);
        }
        else {
            // *strm << childindent << "\"values\": []";
            currParameter->values += "\"values\": []";
        }
    }
    free(axisRetrieved);
    free(parameterRetrieved);
}


/**
 * @brief Writes the CovJSON representation of the passed DAP Array of string types.
 *   If the parameter "sendData" evaluates to true, then data will also be sent.
 *
 * For each variable in the DDS, write out that variable and its
 * attributes as CovJSON. Each OPeNDAP data type translates into a
 * particular CovJSON type. Also write out any global attributes stored at the
 * top level of the DataDDS.
 *
 * @note This version exists because of the differing type signatures of the
 *   libdap::Vector::value() methods for numeric and c++ string types.
 *
 * @note If sendData is true but the DDS does not contain data, the result
 *   is undefined.
 *
 * @param strm Write to this output stream
 * @param a Source data array - write out data or metadata from or about this array
 * @param indent Indent the output so humans can make sense of it
 * @param sendData true: send data; false: send metadata
 */
void FoDapCovJsonTransform::covjsonStringArray(std::ostream *strm, libdap::Array *a, string indent, bool sendData)
{
    string childindent = indent + _indent_increment;

    *strm << indent << "{" << endl;
    //writeLeafMetadata(strm, a, indent);

    int numDim = a->dimensions(true);
    vector<unsigned int> shape(numDim);
    //long length = focovjson::computeConstrainedShape(a, &shape);

    *strm << childindent << "\"shape\": [";
    for (std::vector<unsigned int>::size_type i = 0; i < shape.size(); i++) {
        if (i > 0) *strm << ", ";
        *strm << shape[i];
    }

    *strm << "],";
    if (sendData) {
        *strm << "," << endl;
        // Data array gets printed to strm
        *strm << childindent << "\"values\": ";
        // unsigned int indx;

        // The string type utilizes a specialized version of libdap:Array.value()
        vector<std::string> sourceValues;
        a->value(sourceValues);
        //indx = covjsonSimpleTypeArrayWorker(strm, (std::string *) (&sourceValues[0]), 0, &shape, 0);

        // if (length != indx) {
        //     BESDEBUG(FoDapCovJsonTransform_debug_key,
        //         "covjsonStringArray() - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);
        // }
    }
    *strm << endl << indent << "}";
}


/**
 * @brief Gets a leaf's attribute metadata and stores them to private
 *   class variables to make them accessible for printing.
 *
 * Gets the current attribute values and stores the metadata the data in
 * the corresponding private class variables . Will logically search
 * for value names (ie "longitude") and store them as required.
 *
 * @note logic to determine z axis does not yet exist
 *
 * @note strm is included here for debugging purposes. Otherwise, there is no
 *   absolute need to require it as an argument. May remove strm as an arg if
 *   necessary.
 *
 * @param ostrm Write the CovJSON to this stream (TEST/DEBUGGING)
 * @param attr_table Reference to an AttrTable containing Axis attribute values
 * @param name Name of a given parameter
 * @param axisRetrieved true if axis is retrieved, false if not
 * @param parameterRetrieved true if parameter is retrieved, false if not
 */
void FoDapCovJsonTransform::getAttributes(ostream *strm, libdap::AttrTable &attr_table, string name,
    bool *axisRetrieved, bool *parameterRetrieved)
{
    std::string currAxisName;
    std::string currParameterUnit;
    std::string currParameterLongName;

    // Only do more if there are actually attributes in the table
    if (attr_table.get_size() != 0) {
        libdap::AttrTable::Attr_iter begin = attr_table.attr_begin();
        libdap::AttrTable::Attr_iter end = attr_table.attr_end();

        for (libdap::AttrTable::Attr_iter at_iter = begin; at_iter != end; at_iter++) {
            switch (attr_table.get_attr_type(at_iter)) {
            case libdap::Attr_container: {
                libdap::AttrTable *atbl = attr_table.get_attr_table(at_iter);
                // Recursive call for child attribute table
                getAttributes(strm, *atbl, name, axisRetrieved, parameterRetrieved);
                break;
            }
            default: {
                vector<std::string> *values = attr_table.get_attr_vector(at_iter);

                for (std::vector<std::string>::size_type i = 0; i < values->size(); i++) {
                    string currName = attr_table.get_name(at_iter);
                    string currValue = (*values)[i];

                    // FOR TESTING AND DEBUGGING PURPOSES
                    // *strm << "\"currName\": \"" << currName << "\", \"currValue\": \"" << currValue << "\"" << endl;

                    if(((currValue.compare("lon") == 0) || (currValue.compare("longitude") == 0)
                        || (currValue.compare("LONGITUDE") == 0) || (currValue.compare("Longitude") == 0)
                        || (currValue.compare("x") == 0) || (currValue.compare("X") == 0)) && !xExists) {
                        xExists = true;
                        isAxis = true;
                        isParam = false;
                        currAxisName = "x";
                    }
                    else if(((currName.compare("units") == 0) && ((currValue.compare("degrees_east") == 0)
                        || currValue.compare("degree East") || currValue.compare("degrees East"))) && !xExists)  {
                        xExists = true;
                        isAxis = true;
                        isParam = false;
                        currAxisName = "x";
                    }
                    else if(((currValue.compare("lat") == 0) || (currValue.compare("latitude") == 0)
                        || (currValue.compare("LATITUDE") == 0) || (currValue.compare("Latitude") == 0)
                        || (currValue.compare("y") == 0) || (currValue.compare("Y") == 0)) && !yExists) {
                        yExists = true;
                        isAxis = true;
                        isParam = false;
                        currAxisName = "y";
                    }
                    else if(((currName.compare("units") == 0) && ((currValue.compare("degrees_north") == 0)
                        || currValue.compare("degree North") || currValue.compare("degrees North"))) && !yExists) {
                        yExists = true;
                        isAxis = true;
                        isParam = false;
                        currAxisName = "y";
                    }
                    else if (((currName.compare("t") == 0) || (currName.compare("TIME") == 0)
                        || (currName.compare("time") == 0) || (currName.compare("s") == 0)
                        || (currName.compare("seconds") == 0) || (currName.compare("Seconds") == 0)
                        || (currName.compare("time_origin") == 0)) && !tExists) {
                        tExists = true;
                        isAxis = true;
                        isParam = false;
                        currAxisName = "t";
                    }
                    else if((currName.compare("units") == 0) && (currValue.find("hour") || currValue.find("hours")
                        || currValue.find("seconds") || currValue.find("time")) && !tExists) {
                        tExists = true;
                        isAxis = true;
                        isParam = false;
                        currAxisName = "t";
                    }
                    else if(currName.compare("units") == 0) {
                        isAxis = false;
                        isParam = true;
                        currParameterUnit = currValue;
                    }
                    else if(currName.compare("long_name") == 0) {
                        isAxis = false;
                        isParam = true;
                        currParameterLongName = currValue;
                    }
                    else {
                        isAxis = false;
                        isParam = false;
                    }
                }

                if(isAxis == true && isParam == false) {
                    if(currAxisName.compare("") != 0) {
                        struct Axis *newAxis = new Axis;
                        newAxis->name = currAxisName;
                        axes.push_back(newAxis);
                        axisCount++;
                        *axisRetrieved = true;
                        *parameterRetrieved = false;
                    }
                }
                else if(isAxis == false && isParam == true) {
                    if(currParameterUnit.compare("") != 0 && currParameterLongName.compare("") != 0) {
                        struct Parameter *newParameter = new Parameter;
                        newParameter->name = name;
                        newParameter->unit = currParameterUnit;
                        newParameter->longName = currParameterLongName;
                        parameters.push_back(newParameter);
                        parameterCount++;
                        *axisRetrieved = false;
                        *parameterRetrieved = true;
                    }
                }
                else {
                    // Do nothing
                }

                break;
            }
            }
        }
    }
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
FoDapCovJsonTransform::FoDapCovJsonTransform(libdap::DDS *dds)
    : _dds(dds), _indent_increment("  "), xExists(false), yExists(false), zExists(false), tExists(false), isParam(false), isAxis(false), axisCount(0), parameterCount(0)
{
    if (!_dds) throw BESInternalError("File out COVJSON, null DDS passed to constructor", __FILE__, __LINE__);
}


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
 * @brief Transforms each of the marked variables of the DDS to CovJSON
 *
 * For each variable in the DDS, write out that variable and its
 * attributes as CovJSON. Each OPeNDAP data type translates into a
 * particular CovJSON type. Also write out any global attributes stored at the
 * top level of the DataDDS.
 *
 * @param ostrm Write the CovJSON to this stream
 * @param sendData True if data should be sent, False to send only metadata.
 */
void FoDapCovJsonTransform::transform(ostream &ostrm, bool sendData)
{
    transform(&ostrm, _dds, "", sendData);
}


/**
 * @brief Contructs a new set of leaves and nodes derived from a previous set of nodes.
 *
 * Contructs a new set of leaves and nodes derived from a previous set of nodes, then
 * runs the transformRangesWorker() on the new set of leaves so that the range values
 * can be printed to the CovJSON output stream.
 *
 * @note The new derived nodes vector is not used for anything at this time.
 *
 * @note DAP Constructor types are semantically equivalent to a w10n node type so they
 *   must be represented as a collection of child nodes and leaves.
 *
 * @param strm Write to this output stream
 * @param cnstrctr a new collection of child nodes and leaves
 * @param indent Indent the output so humans can make sense of it
 * @param sendData true: send data; false: send metadata
 */
void FoDapCovJsonTransform::transform(ostream *strm, libdap::Constructor *cnstrctr, string indent, bool sendData)
{
    vector<libdap::BaseType *> leaves;
    vector<libdap::BaseType *> nodes;
    // Sort the variables into two sets
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

    transformNodeWorker(strm, leaves, nodes, indent, sendData);
}


/**
 * @brief Worker method allows us to recursively traverse an Node's variable
 *   contents and any child nodes will be traversed as well.
 *
 * @param strm Write to this output stream
 * @param leaves Pointer to a vector of BaseTypes, which represent w10n leaves
 * @param leaves Pointer to a vector of BaseTypes, which represent w10n nodes
 * @param indent Indent the output so humans can make sense of it
 * @param sendData true: send data; false: send metadata
 */
void FoDapCovJsonTransform::transformNodeWorker(ostream *strm, vector<libdap::BaseType *> leaves,
    vector<libdap::BaseType *> nodes, string indent, bool sendData)
{
    // Get this node's leaves
    for (std::vector<libdap::BaseType *>::size_type l = 0; l < leaves.size(); l++) {
        libdap::BaseType *v = leaves[l];
        BESDEBUG(FoDapCovJsonTransform_debug_key, "Processing LEAF: " << v->name() << endl);
        transform(strm, v, indent + _indent_increment, sendData);
    }

    // Get this node's child nodes
    for (std::vector<libdap::BaseType *>::size_type n = 0; n < nodes.size(); n++) {
        libdap::BaseType *v = nodes[n];
        BESDEBUG(FoDapCovJsonTransform_debug_key, "Processing NODE: " << v->name() << endl);
        transform(strm, v, indent + _indent_increment, sendData);
    }
}


/**
 * @brief Worker method prints the CoverageJSON file header, which
 *   includes domainType, to stream
 *
 * @param strm Write to this output stream
 * @param indent Indent the output so humans can make sense of it
 */
void FoDapCovJsonTransform::printCoverageHeaderWorker(ostream *strm, string indent)
{
    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;

    *strm << indent << "{" << endl;
    *strm << child_indent1 << "\"type\": \"Coverage\"," << endl;
    *strm << child_indent1 << "\"domain\": {" << endl;

    if(domainType == Grid) {
        *strm << child_indent2 << "\"domainType\": \"Grid\"," << endl;
    }
    else if(domainType == VerticalProfile) {
        *strm << child_indent2 << "\"domainType\": \"Vertical Profile\"," << endl;
    }
    else if(domainType == PointSeries) {
        *strm << child_indent2 << "\"domainType\": \"Point Series\"," << endl;
    }
    else if(domainType == Point) {
        *strm << child_indent2 << "\"domainType\": \"Point\"," << endl;
    }
    else {
        *strm << child_indent2 << "\"domainType\": \"Grid\"," << endl;
    }
}


/**
 * @brief Worker method prints the CoverageJSON file Axes to stream
 *
 * @param strm Write to this output stream
 * @param indent Indent the output so humans can make sense of it
 */
void FoDapCovJsonTransform::printAxesWorker(ostream *strm, string indent)
{
    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;

    BESDEBUG(FoDapCovJsonTransform_debug_key, "Printing AXES" << endl);

    // Write the axes to strm
    *strm << indent << "\"axes\": {" << endl;
    for(unsigned int i = 0; i < axisCount; i++) {
        *strm << child_indent1 << "\"" << axes[i]->name << "\": {" << endl;
        *strm << child_indent2 << axes[i]->values << endl;

        if(i == axisCount - 1) {
            *strm << child_indent1 << "}" << endl;
        }
        else {
            *strm << child_indent1 << "}," << endl;
        }
    }
    *strm << indent << "}," << endl;
}


/**
 * @brief Worker method prints the CoverageJSON file Axes reference to stream
 *
 * @param strm Write to this output stream
 * @param indent Indent the output so humans can make sense of it
 */
void FoDapCovJsonTransform::printReferenceWorker(ostream *strm, string indent)
{
    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;

    BESDEBUG(FoDapCovJsonTransform_debug_key, "Printing REFERENCES" << endl);

    // According to CovJSON spec, there should never be a case where there is no x and y
    // coordinate. In other words, the only thing we need to determine here is whether
    // or not there is a z coordinate variable.
    string coordVars = "\"x\", \"y\"";
    if(zExists == true) {
        coordVars += ", \"z\"";
    }

    // "referencing": [{
    //   "coordinates": ["t"],
    //   "system": {
    //     "type": "TemporalRS",
    //     "calendar": "Gregorian"
    //   }
    // },
    // {
    //   "coordinates": ["x", "y", "z"],
    //   "system": {
    //     "type": "GeographicCRS",
    //     "id": "http://www.opengis.net/def/crs/OGC/1.3/CRS84"
    //   }
    // }]

    *strm << indent << "\"referencing\": [{" << endl;
    *strm << child_indent1 << "\"coordinates\": [\"t\"]," << endl;
    *strm << child_indent1 << "\"system\": {" << endl;
    *strm << child_indent2 << "\"type\": \"TemporalRS\"," << endl;
    *strm << child_indent2 << "\"calendar\": \"Gregorian\"," << endl;
    *strm << child_indent1 << "}" << endl;
    *strm << indent << "}," << endl;
    *strm << indent << "{" << endl;
    *strm << child_indent1 << "\"coordinates\": [" << coordVars << "]," << endl;
    *strm << child_indent1 << "\"system\": {" << endl;
    *strm << child_indent2 << "\"type\": \"GeographicCRS\"," << endl;
    *strm << child_indent2 << "\"id\": \"http://www.opengis.net/def/crs/OGC/1.3/CRS84\"," << endl;
    *strm << child_indent1 << "}" << endl;
    *strm << indent << "}]" << endl;
    *strm << _indent_increment << "}," << endl;
}


/**
 * @brief Worker method prints the CoverageJSON file Parameters to stream
 *
 * @param strm Write to this output stream
 * @param indent Indent the output so humans can make sense of it
 */
void FoDapCovJsonTransform::printParametersWorker(ostream *strm, string indent)
{
    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;
    string child_indent3 = child_indent2 + _indent_increment;
    string child_indent4 = child_indent3 + _indent_increment;

    BESDEBUG(FoDapCovJsonTransform_debug_key, "Printing PARAMETERS" << endl);

    string axisNames = "\"t\", ";
    if(zExists) {
        axisNames += "\"z\", ";
    }
    axisNames += "\"y\", \"x\"";

    // Write down the parameters and values
    for(unsigned int i = 0; i < parameterCount; i++) {
        *strm << indent << "\"parameters\": {" << endl;
        *strm << child_indent1 << "\"" << parameters[i]->name << "\": {" << endl;
        *strm << child_indent2 << "\"type\": \"Parameter\"," << endl;
        *strm << child_indent2 << "\"description\": \"" << parameters[i]->name << "\"," << endl;
        *strm << child_indent2 << "\"unit\": {" << endl;
        *strm << child_indent3 << "\"label\": {" << endl;
        *strm << child_indent4 << "\"en\": \"" << parameters[i]->unit << "\"" << endl;
        *strm << child_indent3 << "}" << endl;
        *strm << child_indent2 << "}," << endl;
        *strm << child_indent2 << "\"symbol\": {" << endl;
        *strm << child_indent3 << "\"value\": \"" << parameters[i]->unit << "\"," << endl;
        *strm << child_indent3 << "\"type\": \"\"," << endl;
        *strm << child_indent2 << "}," << endl;
        *strm << child_indent2 << "\"observedProperty\": {" << endl;
        *strm << child_indent3 << "\"id\": null," << endl;
        *strm << child_indent3 << "\"label\": {" << endl;
        *strm << child_indent4 << "\"en\": \"" << parameters[i]->longName << "\"" << endl;
        *strm << child_indent3 << "}" << endl;
        *strm << child_indent2 << "}" << endl;
        *strm << child_indent1 << "}" << endl;
        *strm << indent << "}," << endl;

        // Axis name (x, y, or z)
        *strm << indent << "\"ranges\": {" << endl;
        *strm << child_indent1 << "\"" << parameters[i]->name << "\": {" << endl;
        *strm << child_indent2 << "\"type\": \"NdArray\"," << endl;
        *strm << child_indent2 << "\"dataType\": \"float\"," << endl;
        *strm << child_indent2 << "\"axisNames\": [" << axisNames << "]," << endl;
        *strm << child_indent2 << parameters[i]->shape << endl;
        *strm << child_indent2 << parameters[i]->values << endl;
        *strm << child_indent1 << "}" << endl;

        if(i == parameterCount - 1) {
            *strm << indent << "}" << endl;
        }
        else {
            *strm << indent << "}," << endl;
        }

    }
}


/**
 * @brief Worker method prints the CoverageJSON file footer to stream
 *
 * @param strm Write to this output stream
 * @param indent Indent the output so humans can make sense of it
 */
void FoDapCovJsonTransform::printCoverageFooterWorker(std::ostream *strm, std::string indent)
{
    *strm << indent << "}" << endl;
}


/**
 * @brief Writes a CovJSON representation of the DDS to the passed stream. Data
 *   is sent if the sendData flag is true. Otherwise, only metadata is sent.
 *
 * @note w10 sees the world in terms of leaves and nodes. Leaves have data, nodes
 *   have other nodes and leaves.
 *
 * @param strm Write to this output stream
 * @param dds Pointer to a DDS vector, which contains both w10n leaves and nodes
 * @param indent Indent the output so humans can make sense of it
 * @param sendData true: send data; false: send metadata
 */
void FoDapCovJsonTransform::transform(ostream *strm, libdap::DDS *dds, string indent, bool sendData)
{
    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;

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

    transformNodeWorker(strm, leaves, nodes, child_indent2, sendData);

    bool canConvertToCovJson = canConvert();

    // Only print to stream if this file can be converted to CovJSON
    if (canConvertToCovJson) {
        // Print header and domain type
        printCoverageHeaderWorker(strm, indent);

        // Prints the axes metadata and range values
        printAxesWorker(strm, child_indent2);

        // Prints the references for the given Axes
        printReferenceWorker(strm, child_indent2);

        // Prints parameter metadata and range values
        printParametersWorker(strm, child_indent1);

        // Print footer
        printCoverageFooterWorker(strm, indent);
    }
    else {
        // @TODO Do nothing?? Or throw BESInternalError?? hmmm
    }
}


/**
 * @brief  Write the CovJSON representation of the passed BaseType instance. If the
 *   parameter sendData is true then include the data. Otherwise, only metadata is
 *   sent.
 *
 * @note Structure, Grid, and Sequence cases are not implemented at this time.
 *
 * @param strm Write to this output stream
 * @param bt Pointer to a BaseType vector containing values and/or attributes
 * @param indent Indent the output so humans can make sense of it
 * @param sendData true: send data; false: send metadata
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
 * @brief  Write the CovJSON representation of the passed BaseType instance,
 *   which had better be one of the atomic DAP types. If the parameter sendData
 *   is true then include the data.
 *
 * @note writeLeafMetadata() no longer exists, thus it is commented out. It
 *   should be replaced with a different metadata writing function.
 *
 * @param strm Write to this output stream
 * @param b Pointer to a BaseType vector containing atomic type variables
 * @param indent Indent the output so humans can make sense of it
 * @param sendData true: send data; false: send metadata
 */
void FoDapCovJsonTransform::transformAtomic(ostream *strm, libdap::BaseType *b, string indent, bool sendData)
{
    string childindent = indent + _indent_increment;

    *strm << indent << "\"\": {" << endl;

    if (sendData) {
        // Print Data
        *strm << childindent << "\"values\": [";

        if (b->type() == libdap::dods_str_c || b->type() == libdap::dods_url_c) {
            libdap::Str *strVar = (libdap::Str *) b;
            std::string tmpString = strVar->value();
            *strm << "\"" << focovjson::escape_for_covjson(tmpString) << "\"";
        }
        else {
            b->print_val(*strm, "", false);
        }

        *strm << "]" << endl << indent << "}";
    }
    else {
        *strm << childindent << "\"values\": []" << endl << indent << "}";
    }
}


/**
 * @brief  Write the CovJSON representation of the passed DAP Array instance,
 *   which had better be one of the atomic DAP types. If the parameter sendData
 *   is true then include the data.
 *
 * @param strm Write to this output stream
 * @param a Pointer to an Array containing atomic type variables
 * @param indent Indent the output so humans can make sense of it
 * @param sendData true: send data; false: send metadata
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
