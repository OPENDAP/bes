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
#include <cstring>
#include <typeinfo>
#include <iomanip> // setprecision
#include <sstream> // stringstream
#include <vector>
#include <ctime>

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

/**
 * @brief Checks the spacial/temporal dimensions that we've obtained, if we've
 *    obtained any at all, can be used to convert to a CovJSON file. If x, y,
 *    and t exist, then we determine domainType based on the shape values and we
 *    return true. If x, y, and/or t don't exist, we simply return false
 *
 * @note see CovJSON domain type spec: https://covjson.org/domain-types/ for
 *    further details on determining domain type
 *
 * @returns true if can convert to CovJSON, false if cannot convert
 */
bool FoDapCovJsonTransform::canConvert()
{
    // If x, y, z, and t all exist
    // We are assuming the following is true:
    //    - shapeVals[0] = x axis
    //    - shapeVals[1] = y axis
    //    - shapeVals[2] = z axis
    //    - shapeVals[3] = t axis
    if(xExists && yExists && zExists && tExists) {
        // A domain with Grid domain type MUST have the axes "x" and "y"
        // and MAY have the axes "z" and "t".
        if((shapeVals[0] > 1) && (shapeVals[1] > 1) && (shapeVals[2] >= 1) && (shapeVals[3] >= 0)) {
            domainType = Grid;
            return true;
        }

        // A domain with VerticalProfile domain type MUST have the axes "x",
        // "y", and "z", where "x" and "y" MUST have a single coordinate only.
        else if((shapeVals[0] == 1) && (shapeVals[1] == 1) && (shapeVals[2] >= 1) && ((shapeVals[3] <= 1) && (shapeVals[3] >= 0))) {
            domainType = VerticalProfile;
            return true;
        }

        // A domain with PointSeries domain type MUST have the axes "x", "y",
        // and "t" where "x" and "y" MUST have a single coordinate only. A
        // domain with PointSeries domain type MAY have the axis "z" which
        // MUST have a single coordinate only.
        else if((shapeVals[0] == 1) && (shapeVals[1] == 1) && (shapeVals[2] == 1) && (shapeVals[3] >= 0)) {
            domainType = PointSeries;
            return true;
        }

        // A domain with Point domain type MUST have the axes "x" and "y" and MAY
        // have the axes "z" and "t" where all MUST have a single coordinate only.
        else if((shapeVals[0] == 1) && (shapeVals[1] == 1) && (shapeVals[2] == 1) && (shapeVals[3] == 1)) {
            domainType = Point;
            return true;
        }
    }

    // If just x, y, and t exist
    // We are assuming the following is true:
    //    - shapeVals[0] = x axis
    //    - shapeVals[1] = y axis
    //    - shapeVals[2] = t axis
    else if(xExists && yExists && !zExists && tExists) {
        // A domain with Grid domain type MUST have the axes "x" and "y"
        // and MAY have the axes "z" and "t".
        if((shapeVals[0] > 1) && (shapeVals[1] > 1) && (shapeVals[2] >= 0)) {
            domainType = Grid;
            return true;
        }

        // A domain with PointSeries domain type MUST have the axes "x", "y",
        // and "t" where "x" and "y" MUST have a single coordinate only. A
        // domain with PointSeries domain type MAY have the axis "z" which
        // MUST have a single coordinate only.
        else if((shapeVals[0] == 1) && (shapeVals[1] == 1) && (shapeVals[2] >= 0)) {
            domainType = PointSeries;
            return true;
        }

        // A domain with Point domain type MUST have the axes "x" and "y" and MAY
        // have the axes "z" and "t" where all MUST have a single coordinate only.
        else if((shapeVals[0] == 1) && (shapeVals[1] == 1) && (shapeVals[2] == 1)) {
            domainType = Point;
            return true;
        }
    }

    // If just x and y exist
    // We are assuming the following is true:
    //    - shapeVals[0] = x axis
    //    - shapeVals[1] = y axis
    else if(xExists && yExists && !zExists && !tExists) {
        // A domain with Grid domain type MUST have the axes "x" and "y"
        // and MAY have the axes "z" and "t".
        if((shapeVals[0] > 1) && (shapeVals[1] > 1)) {
            domainType = Grid;
            return true;
        }

        // A domain with Point domain type MUST have the axes "x" and "y" and MAY
        // have the axes "z" and "t" where all MUST have a single coordinate only.
        else if((shapeVals[0] == 1) && (shapeVals[1] == 1)) {
            domainType = Point;
            return true;
        }
    }
    return false; // This source DDS is not valid as CovJSON
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
 *
 * @returns the most recently completed index
 */
template<typename T>
unsigned int FoDapCovJsonTransform::covjsonSimpleTypeArrayWorker(ostream *strm, T *values, unsigned int indx,
    vector<unsigned int> *shape, unsigned int currentDim)
{
    unsigned int currentDimSize = (*shape)[currentDim];

    *strm << "[";
    for(unsigned int i = 0; i < currentDimSize; i++) {
        if(currentDim < shape->size() - 1) {
            BESDEBUG(FoDapCovJsonTransform_debug_key,
                "covjsonSimpleTypeArrayWorker() - Recursing! indx:  " << indx << " currentDim: " << currentDim << " currentDimSize: " << currentDimSize << endl);
            indx = covjsonSimpleTypeArrayWorker<T>(strm, values, indx, shape, currentDim + 1);
            if(i + 1 != currentDimSize) {
                *strm << ", ";
            }
        }
        else {
            if(i) {
                *strm << ", ";
            }
            if(typeid(T) == typeid(string)) {
                // Strings need to be escaped to be included in a CovJSON object.
                string val = reinterpret_cast<string*>(values)[indx++];
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
 * @param ostrm Write the CovJSON to this stream (for debugging purposes)
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
    currDataType = a->var()->type_name();

    getAttributes(strm, a->get_attr_table(), a->name(), axisRetrieved, parameterRetrieved);

    // a->print_val(*strm, "\n", true); // For testing purposes

    // sendData = false; // For testing purposes

    // If we are dealing with an Axis
    if((*axisRetrieved == true) && (*parameterRetrieved == false)) {
        struct Axis *currAxis;
        currAxis = axes[axisCount - 1];

        int numDim = a->dimensions(true);
        vector<unsigned int> shape(numDim);
        long length = focovjson::computeConstrainedShape(a, &shape);

        if(currAxis->name.compare("t") != 0) {
            if(sendData) {
                currAxis->values += "\"values\": ";
                unsigned int indx = 0;
                vector<T> src(length);
                a->value(&src[0]);

                try {
                    ostringstream *astrm = new ostringstream;
                    indx = covjsonSimpleTypeArrayWorker(astrm, &src[0], 0, &shape, 0);
                    currAxis->values += astrm->str();
                    free(astrm);
                }
                catch(...) {
                    throw;
                }

                if(length != indx) {
                    BESDEBUG(FoDapCovJsonTransform_debug_key, "covjsonSimpleTypeArray(Axis) - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);
                }
                assert(length == indx);
            }
            else {
                currAxis->values += "\"values\": []";
            }
        }
    }

    // If we are dealing with a Parameter
    else if(*axisRetrieved == false && *parameterRetrieved == true) {
        struct Parameter *currParameter;
        currParameter = parameters[parameterCount - 1];

        int numDim = a->dimensions(true);
        vector<unsigned int> shape(numDim);
        long length = focovjson::computeConstrainedShape(a, &shape);

        currParameter->shape += "\"shape\": [";
        for(vector<unsigned int>::size_type i = 0; i < shape.size(); i++) {
            if(i > 0) {
                currParameter->shape += ", ";
            }

            // Process the shape's values, which are strings,
            // convert them into integers, and store them
            ostringstream otemp;
            istringstream itemp;
            int tempVal = 0;
            otemp << shape[i];
            istringstream (otemp.str());
            istringstream (otemp.str()) >> tempVal;
            shapeVals.push_back(tempVal);

            // t may only have 1 value: the origin timestamp
            // DANGER: t may not yet be defined
            if((i == 0) && tExists) {
                currParameter->shape += "1";
            }
            else {
                currParameter->shape += otemp.str();
            }
        }
        currParameter->shape += "],";

        if(sendData) {
            currParameter->values += "\"values\": ";
            unsigned int indx = 0;
            vector<T> src(length);
            a->value(&src[0]);

            try {
                ostringstream *pstrm = new ostringstream;
                indx = covjsonSimpleTypeArrayWorker(pstrm, &src[0], 0, &shape, 0);
                currParameter->values += pstrm->str();
                free(pstrm);
            }
            catch(...) {
                throw;
            }
            if(length != indx) {
                BESDEBUG(FoDapCovJsonTransform_debug_key, "covjsonSimpleTypeArray(Parameter) - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);
            }
            assert(length == indx);
        }
        else {
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
void FoDapCovJsonTransform::covjsonStringArray(ostream *strm, libdap::Array *a, string indent, bool sendData)
{
    string childindent = indent + _indent_increment;
    bool *axisRetrieved = new bool;
    bool *parameterRetrieved = new bool;
    *axisRetrieved = false;
    *parameterRetrieved = false;

    getAttributes(strm, a->get_attr_table(), a->name(), axisRetrieved, parameterRetrieved);

    // a->print_val(*strm, "\n", true); // For testing purposes

    // sendData = false; // For testing purposes

    // If we are dealing with an Axis
    if((*axisRetrieved == true) && (*parameterRetrieved == false)) {
        struct Axis *currAxis;
        currAxis = axes[axisCount - 1];

        int numDim = a->dimensions(true);
        vector<unsigned int> shape(numDim);
        long length = focovjson::computeConstrainedShape(a, &shape);

        if(currAxis->name.compare("t") != 0) {
            if(sendData) {
                currAxis->values += "\"values\": ";
                unsigned int indx = 0;
                // The string type utilizes a specialized version of libdap:Array.value()
                vector<string> sourceValues;
                a->value(sourceValues);

                try {
                    ostringstream *astrm = new ostringstream;
                    indx = covjsonSimpleTypeArrayWorker(astrm, (string *) (&sourceValues[0]), 0, &shape, 0);
                    currAxis->values += astrm->str();
                    free(astrm);
                }
                catch(...) {
                    throw;
                }
                if(length != indx) {
                    BESDEBUG(FoDapCovJsonTransform_debug_key, "covjsonStringArray(Axis) - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);
                }
                assert(length == indx);
            }
            else {
                currAxis->values += "\"values\": []";
            }
        }
    }

    // If we are dealing with a Parameter
    else if(*axisRetrieved == false && *parameterRetrieved == true) {
        struct Parameter *currParameter;
        currParameter = parameters[parameterCount - 1];

        int numDim = a->dimensions(true);
        vector<unsigned int> shape(numDim);
        long length = focovjson::computeConstrainedShape(a, &shape);

        currParameter->shape += "\"shape\": [";
        for(vector<unsigned int>::size_type i = 0; i < shape.size(); i++) {
            if(i > 0) {
                currParameter->shape += ", ";
            }

            // Process the shape's values, which are strings,
            // convert them into integers, and store them
            ostringstream otemp;
            istringstream itemp;
            int tempVal = 0;
            otemp << shape[i];
            istringstream (otemp.str());
            istringstream (otemp.str()) >> tempVal;
            shapeVals.push_back(tempVal);

            // t may only have 1 value: the origin timestamp
            // DANGER: t may not yet be defined
            if((i == 0) && tExists) {
                currParameter->shape += "1";
            }
            else {
                currParameter->shape += otemp.str();
            }
        }
        currParameter->shape += "],";

        if(sendData) {
            currParameter->values += "\"values\": ";
            unsigned int indx = 0;
            // The string type utilizes a specialized version of libdap:Array.value()
            vector<string> sourceValues;
            a->value(sourceValues);

            try {
                ostringstream *pstrm = new ostringstream;
                indx = covjsonSimpleTypeArrayWorker(pstrm, (string *) (&sourceValues[0]), 0, &shape, 0);
                currParameter->values += pstrm->str();
                free(pstrm);
            }
            catch(...) {
                throw;
            }
            if(length != indx) {
                BESDEBUG(FoDapCovJsonTransform_debug_key, "covjsonStringArray(Parameter) - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);
            }
            assert(length == indx);
        }
        else {
            currParameter->values += "\"values\": []";
        }
    }

    free(axisRetrieved);
    free(parameterRetrieved);
}


/**
 * @brief Gets a leaf's attribute metadata and stores them to private
 *   class variables to make them accessible for printing.
 *
 * Gets the current attribute values and stores the metadata the data in
 * the corresponding private class variables . Will logically search
 * for value names (ie "longitude") and store them as required.
 *
 * @note strm is included here for debugging purposes. Otherwise, there is no
 *   absolute need to require it as an argument. May remove strm as an arg if
 *   necessary.
 *
 * @note CoverageJSON specification for Temporal Reference Systems
 *   https://covjson.org/spec/#temporal-reference-systems
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
    string currAxisName;
    string currAxisTimeOrigin;
    string currParameterUnit;
    string currParameterLongName;

    // Only do more if there are actually attributes in the table
    if(attr_table.get_size() != 0) {
        libdap::AttrTable::Attr_iter begin = attr_table.attr_begin();
        libdap::AttrTable::Attr_iter end = attr_table.attr_end();

        for(libdap::AttrTable::Attr_iter at_iter = begin; at_iter != end; at_iter++) {
            switch (attr_table.get_attr_type(at_iter)) {
            case libdap::Attr_container: {
                libdap::AttrTable *atbl = attr_table.get_attr_table(at_iter);
                // Recursive call for child attribute table
                getAttributes(strm, *atbl, name, axisRetrieved, parameterRetrieved);
                break;
            }
            default: {
                vector<string> *values = attr_table.get_attr_vector(at_iter);

                for(vector<string>::size_type i = 0; i < values->size(); i++) {
                    string currName = attr_table.get_name(at_iter);
                    string currValue = (*values)[i];

                    // FOR TESTING AND DEBUGGING PURPOSES
                    // *strm << "\"currName\": \"" << currName << "\", \"currValue\": \"" << currValue << "\"" << endl;

                    // Parse the attribute table values and try to determine what variables AND
                    // metadata are present -- its not an exact science, and its a little dirty.
                    if(currName.compare("units") == 0) {
                        if(((currValue.compare("degrees_east") == 0) || (currValue.compare("degree East") == 0)
                            || (currValue.compare("degrees East") == 0)) && !xExists) {
                            xExists = true;
                            isAxis = true;
                            isParam = false;
                            currAxisName = "x";
                        }
                        else if(((currValue.compare("degrees_north") == 0) || (currValue.compare("degree North") == 0)
                            || (currValue.compare("degrees North") == 0)) && !yExists) {
                            yExists = true;
                            isAxis = true;
                            isParam = false;
                            currAxisName = "y";
                        }
                        else if(((currValue.find("z") == 0) || (currValue.find("Z") == 0)) && !zExists) {
                            zExists = true;
                            isAxis = true;
                            isParam = false;
                            currAxisName = "z";
                        }
                        else if(((currValue.find("day") == 0) || (currValue.find("hour") == 0)
                            || (currValue.find("minute") == 0) || (currValue.find("second") == 0)) && !tExists) {
                            tExists = true;
                            isAxis = true;
                            isParam = false;
                            currAxisTimeOrigin = currValue;
                            currAxisName = "t";
                        }
                        else {
                            isAxis = false;
                            isParam = true;
                            currParameterUnit = currValue;
                        }
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
                    // Push a new axis
                    if(currAxisName.compare("") != 0) {
                        struct Axis *newAxis = new Axis;
                        newAxis->name = currAxisName;

                        // If we're dealing with the time axis, capture the time
                        // origin timestamp value with the appropriate formatting
                        // for printing.

                        // See https://covjson.org/spec/#temporal-reference-systems
                        if(currAxisName.compare("t") == 0) {
                            // If the calendar is based on years, months, days,
                            // then the referenced values SHOULD use one of the
                            // following ISO8601-based lexical representations:

                            //    YYYY
                            //    ±XYYYY (where X stands for extra year digits)
                            //    YYYY-MM
                            //    YYYY-MM-DD
                            //    YYYY-MM-DDTHH:MM:SS[.F]Z where Z is either “Z”
                            //              or a time scale offset + -HH:MM

                            // If calendar dates with reduced precision are
                            // used in a lexical representation (e.g. "2016"),
                            // then a client SHOULD interpret those dates in
                            // that reduced precision.

                            string item;
                            vector<string> tokens;
                            char *dup = strdup(currAxisTimeOrigin.c_str());
                            char *token = strtok(dup, " ");
                            while(token != NULL){
                                tokens.push_back(string(token));
                                token = strtok(NULL, " ");
                            }

                            free(token);
                            free(dup);
                            free(token);

                            // For testing purposes
                            // for(unsigned int i = 0; i < tokens.size(); i++) {
                            //     *strm << tokens[i] << endl;
                            // }

                            // @TODO Need to figure out a way to dynamically parse
                            // origin timestamps and convert them to an appropriate
                            // format for CoverageJSON

                            newAxis->values += "\"values\": [\"";
                            newAxis->values += currAxisTimeOrigin;
                            newAxis->values += "\"]";
                        }

                        axes.push_back(newAxis);
                        axisCount++;
                        *axisRetrieved = true;
                        *parameterRetrieved = false;
                    }
                }
                else if(isAxis == false && isParam == true) {
                    // Push a new parameter
                    if(currParameterUnit.compare("") != 0 && currParameterLongName.compare("") != 0) {
                        struct Parameter *newParameter = new Parameter;
                        newParameter->name = name;
                        newParameter->dataType = currDataType;
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
    : _dds(dds), _indent_increment("  "), xExists(false), yExists(false), zExists(false), tExists(false), isParam(false), isAxis(false), canConvertToCovJson(false), axisCount(0), parameterCount(0)
{
    if(!_dds) throw BESInternalError("File out COVJSON, null DDS passed to constructor", __FILE__, __LINE__);
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
    if(_dds != 0) {
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
 * @param testOverride true: print to stream regardless of whether the file can
 *    be converted to CoverageJSON (for testing purposes) false: run normally
 */
void FoDapCovJsonTransform::transform(ostream &ostrm, bool sendData, bool testOverride)
{
    transform(&ostrm, _dds, "", sendData, testOverride);
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

    for(; vi != ve; vi++) {
        if((*vi)->send_p()) {
            libdap::BaseType *v = *vi;
            libdap::Type type = v->type();

            if(type == libdap::dods_array_c) {
                type = v->var()->type();
            }
            if(v->is_constructor_type() || (v->is_vector_type() && v->var()->is_constructor_type())) {
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
 * @param nodes Pointer to a vector of BaseTypes, which represent w10n nodes
 * @param indent Indent the output so humans can make sense of it
 * @param sendData true: send data; false: send metadata
 */
void FoDapCovJsonTransform::transformNodeWorker(ostream *strm, vector<libdap::BaseType *> leaves,
    vector<libdap::BaseType *> nodes, string indent, bool sendData)
{
    // Get this node's leaves
    for(vector<libdap::BaseType *>::size_type l = 0; l < leaves.size(); l++) {
        libdap::BaseType *v = leaves[l];
        BESDEBUG(FoDapCovJsonTransform_debug_key, "Processing LEAF: " << v->name() << endl);
        transform(strm, v, indent + _indent_increment, sendData);
    }

    // Get this node's child nodes
    for(vector<libdap::BaseType *>::size_type n = 0; n < nodes.size(); n++) {
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
 * @param isCoverageCollection true if CoverageCollection format needed, false if normal Coverage
 */
void FoDapCovJsonTransform::printCoverageHeaderWorker(ostream *strm, string indent, bool isCoverageCollection)
{
    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;

    BESDEBUG(FoDapCovJsonTransform_debug_key, "Printing COVERAGE HEADER" << endl);

    // A lot of conditional printing based on whether we have a single
    // Coverage or a Coverage Collection. Nothing fancy here.
    if(parameterCount > 1 && isCoverageCollection) {
        *strm << indent << "{" << endl;
        *strm << child_indent1 << "\"type\": \"CoverageCollection\"," << endl;
    }
    else if(parameterCount > 1 && !isCoverageCollection) {
        *strm << indent << "\"coverages\": [{" << endl;
        *strm << child_indent1 << "\"type\": \"Coverage\"," << endl;
        *strm << child_indent1 << "\"domain\": {" << endl;
    }
    else {
        *strm << indent << "{" << endl;
        *strm << child_indent1 << "\"type\": \"Coverage\"," << endl;
        *strm << child_indent1 << "\"domain\": {" << endl;
    }

    if(parameterCount > 1 && !isCoverageCollection) {
        *strm << child_indent2 << "\"type\" : \"Domain\"," << endl;
    }

    if(parameterCount == 1 && !isCoverageCollection) {
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
            *strm << child_indent2 << "\"domainType\": \"Unknown\"," << endl;
        }
    }
    else if(parameterCount > 1 && isCoverageCollection) {
        if(domainType == Grid) {
            *strm << child_indent1 << "\"domainType\": \"Grid\"," << endl;
        }
        else if(domainType == VerticalProfile) {
            *strm << child_indent1 << "\"domainType\": \"Vertical Profile\"," << endl;
        }
        else if(domainType == PointSeries) {
            *strm << child_indent1 << "\"domainType\": \"Point Series\"," << endl;
        }
        else if(domainType == Point) {
            *strm << child_indent1 << "\"domainType\": \"Point\"," << endl;
        }
        else {
            *strm << child_indent1 << "\"domainType\": \"Unknown\"," << endl;
        }
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
        for(unsigned int j = 0; j < axisCount; j++) {
            // Logic for printing axes in the appropriate order

            // If x, y, z, and t all exist (x, y, z, t)
            if(xExists && yExists && zExists && tExists) {
                if((i == 0) && (axes[j]->name.compare("x") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                }
                else if((i == 1) && (axes[j]->name.compare("y") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                }
                else if((i == 2) && (axes[j]->name.compare("z") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                }
                else if((i == 3) && (axes[j]->name.compare("t") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                }
            }
            // If just x, y, and t exist (x, y, t)
            else if(xExists && yExists && !zExists && tExists) {
                if((i == 0) && (axes[j]->name.compare("x") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                }
                else if((i == 1) && (axes[j]->name.compare("y") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                }
                else if((i == 2) && (axes[j]->name.compare("t") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                }
            }
            // If just x and y exist (x, y)
            else if(xExists && yExists && !zExists && !tExists) {
                if((i == 0) && (axes[j]->name.compare("x") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                }
                else if((i == 1) && (axes[j]->name.compare("y") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                }
            }
        }
        if(i == axisCount - 1) {
            *strm << child_indent1 << "}" << endl;
        }
        else {
            *strm << child_indent1 << "}," << endl;
        }
    }
    if(parameterCount == 1) {
        *strm << indent << "}," << endl;
    }
    else {
        *strm << indent << "}" << endl;
    }
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
    string coordVars;

    BESDEBUG(FoDapCovJsonTransform_debug_key, "Printing REFERENCES" << endl);

    if(xExists) {
        coordVars += "\"x\"";
    }

    if(yExists) {
        if(coordVars.length() > 0) {
            coordVars += ", ";
        }
        coordVars += "\"y\"";
    }

    if(zExists) {
        if(coordVars.length() > 0) {
            coordVars += ", ";
        }
        coordVars += "\"z\"";
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
    if(tExists) {
        *strm << child_indent1 << "\"coordinates\": [\"t\"]," << endl;
        *strm << child_indent1 << "\"system\": {" << endl;
        *strm << child_indent2 << "\"type\": \"TemporalRS\"," << endl;
        *strm << child_indent2 << "\"calendar\": \"Gregorian\"" << endl;
        *strm << child_indent1 << "}" << endl;
        *strm << indent << "}," << endl;
        *strm << indent << "{" << endl;
    }
    *strm << child_indent1 << "\"coordinates\": [" << coordVars << "]," << endl;
    *strm << child_indent1 << "\"system\": {" << endl;
    *strm << child_indent2 << "\"type\": \"GeographicCRS\"," << endl;
    *strm << child_indent2 << "\"id\": \"http://www.opengis.net/def/crs/OGC/1.3/CRS84\"" << endl;
    *strm << child_indent1 << "}" << endl;

    if(parameterCount > 1) {
        *strm << indent << "}]," << endl;
    }
    else {
        *strm << indent << "}]" << endl;
    }

    if(parameterCount == 1) {
        *strm << _indent_increment << "}," << endl;
    }
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

    // Write down the parameter metadata
    *strm << indent << "\"parameters\": {" << endl;
    for(unsigned int i = 0; i < parameterCount; i++) {
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
        *strm << child_indent3 << "\"type\": \"\"" << endl;
        *strm << child_indent2 << "}," << endl;
        *strm << child_indent2 << "\"observedProperty\": {" << endl;
        *strm << child_indent3 << "\"id\": null," << endl;
        *strm << child_indent3 << "\"label\": {" << endl;
        *strm << child_indent4 << "\"en\": \"" << parameters[i]->longName << "\"" << endl;
        *strm << child_indent3 << "}" << endl;
        *strm << child_indent2 << "}" << endl;

        if(i == parameterCount - 1) {
            *strm << child_indent1 << "}" << endl;
        }
        else {
            *strm << child_indent1 << "}," << endl;
        }
    }
    if(parameterCount > 1) {
        *strm << indent << "}," << endl;
    }
}


/**
 * @brief Worker method prints the CoverageJSON file Parameter's ranges to stream
 *
 * @note CoverageJSON specification for N-dimensional Array Objects (NdArray)
 *    https://covjson.org/spec/#ndarray-objects
 *
 * @param strm Write to this output stream
 * @param indent Indent the output so humans can make sense of it
 */
void FoDapCovJsonTransform::printRangesWorker(ostream *strm, string indent)
{
    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;
    string child_indent3 = child_indent2 + _indent_increment;
    string axisNames;

    BESDEBUG(FoDapCovJsonTransform_debug_key, "Printing RANGES" << endl);

    if(tExists) {
         axisNames += "\"t\"";
    }

    if(zExists) {
        if(axisNames.length() > 0) {
            axisNames += ", ";
        }
        axisNames += "\"z\"";
    }

    if(yExists) {
        if(axisNames.length() > 0) {
            axisNames += ", ";
        }
        axisNames += "\"y\"";
    }

    if(xExists) {
        if(axisNames.length() > 0) {
            axisNames += ", ";
        }
        axisNames += "\"x\"";
    }

    // Axis name (x, y, or z)
    *strm << indent << "}," << endl;
    *strm << indent << "\"ranges\": {" << endl;
    for(unsigned int i = 0; i < parameterCount; i++) {
        string dataType;
        // See spec: https://covjson.org/spec/#ndarray-objects
        if(parameters[i]->dataType.find("int") == 0 || parameters[i]->dataType.find("Int") == 0
            || parameters[i]->dataType.find("integer") == 0 || parameters[i]->dataType.find("Integer") == 0) {
            dataType = "integer";
        }
        else if(parameters[i]->dataType.find("float") == 0 || parameters[i]->dataType.find("Float") == 0) {
            dataType = "float";
        }
        else if(parameters[i]->dataType.find("string") == 0 || parameters[i]->dataType.find("String") == 0) {
            dataType = "string";
        }
        else {
            dataType = "unknown";
        }

        *strm << child_indent1 << "\"" << parameters[i]->name << "\": {" << endl;
        *strm << child_indent2 << "\"type\": \"NdArray\"," << endl;
        *strm << child_indent2 << "\"dataType\": \"" << dataType << "\", " << endl;
        *strm << child_indent2 << "\"axisNames\": [" << axisNames << "]," << endl;
        *strm << child_indent2 << parameters[i]->shape << endl;
        *strm << child_indent2 << parameters[i]->values << endl;

        if(i == parameterCount - 1) {
            *strm << child_indent1 << "}" << endl;
        }
        else {
            *strm << child_indent1 << "}," << endl;
        }
    }
    if(parameterCount == 1) {
        *strm << indent << "}" << endl;
    }
}


/**
 * @brief Worker method prints the CoverageJSON file footer to stream
 *
 * @param strm Write to this output stream
 * @param indent Indent the output so humans can make sense of it
 */
void FoDapCovJsonTransform::printCoverageFooterWorker(ostream *strm, string indent)
{
    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;

    BESDEBUG(FoDapCovJsonTransform_debug_key, "Printing COVERAGE FOOTER" << endl);

    if(parameterCount > 1) {
        *strm << child_indent2 << "}" << endl;
        *strm << child_indent1 << "}]" << endl;
    }

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
 * @param testOverride true: print to stream regardless of whether the file can
 *    be converted to CoverageJSON (for testing purposes) false: run canConvert
 *    function to determine if the source DDS can be converted to CovJSON
 */
void FoDapCovJsonTransform::transform(ostream *strm, libdap::DDS *dds, string indent, bool sendData, bool testOverride)
{
    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;
    string child_indent3 = child_indent2 + _indent_increment;
    // Sort the variables into two sets
    vector<libdap::BaseType *> leaves;
    vector<libdap::BaseType *> nodes;

    libdap::DDS::Vars_iter vi = dds->var_begin();
    libdap::DDS::Vars_iter ve = dds->var_end();
    for(; vi != ve; vi++) {
        if((*vi)->send_p()) {
            libdap::BaseType *v = *vi;
            libdap::Type type = v->type();
            if(type == libdap::dods_array_c) {
                type = v->var()->type();
            }
            if(v->is_constructor_type() || (v->is_vector_type() && v->var()->is_constructor_type())) {
                nodes.push_back(v);
            }
            else {
                leaves.push_back(v);
            }
        }
    }

    // Read through the source DDS leaves and nodes, extract all axes and
    // parameter data, and store that data as Axis and Parameters
    transformNodeWorker(strm, leaves, nodes, child_indent2, sendData);

    // Determine if the attribute values we read can be converted to CovJSON.
    // Test override forces printing output to stream regardless of whether
    // or not the file can be converted into CoverageJSON format.
    if(testOverride) {
        canConvertToCovJson = true;
    }
    else {
        canConvertToCovJson = canConvert();
    }

    // Only print if this file can be converted to CovJSON
    if(canConvertToCovJson) {
        // If we have more than one parameter, we are dealing with a
        // Coverage Collection, so we must print accordingly
        if(parameterCount > 1) {
            // Prints header and domain type
            printCoverageHeaderWorker(strm, indent, true);

            // Prints parameter metadata
            printParametersWorker(strm, child_indent1);

            // Prints the references for the given Axes
            printReferenceWorker(strm, child_indent1);

            // Prints header and domain type
            printCoverageHeaderWorker(strm, child_indent1, false);

            // Prints the axes metadata and range values
            printAxesWorker(strm, child_indent3);

            // Prints the parameter range values
            printRangesWorker(strm, child_indent2);

            // Prints footer
            printCoverageFooterWorker(strm, indent);
        }
        // If we have a single parameter, or even no parameters,
        // we will print as a "normal" coverage
        else {
            // Prints header and domain type
            printCoverageHeaderWorker(strm, indent, false);

            // Prints the axes metadata and range values
            printAxesWorker(strm, child_indent2);

            // Prints the references for the given Axes
            printReferenceWorker(strm, child_indent2);

            // Prints parameter metadata
            printParametersWorker(strm, child_indent1);

            // Prints the parameter range values
            printRangesWorker(strm, child_indent1);

            // Prints footer
            printCoverageFooterWorker(strm, indent);
        }
    }
    else {
        // If this file can't be converted, then its failing spacial/temporal requirements
        throw BESInternalError("File cannot be converted to COVJSON format due to missing or incompatible spacial dimensions", __FILE__, __LINE__);
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
    switch(bt->type()) {
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
        transformAtomic(bt, indent, sendData);
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
 * @note @TODO need to handle these types appropriately - make proper printing
 *
 * @param b Pointer to a BaseType vector containing atomic type variables
 * @param indent Indent the output so humans can make sense of it
 * @param sendData true: send data; false: send metadata
 */
void FoDapCovJsonTransform::transformAtomic(libdap::BaseType *b, string indent, bool sendData)
{
    string childindent = indent + _indent_increment;
    struct Axis *newAxis = new Axis;

    newAxis->name = "test";
    if(sendData) {
        newAxis->values += "\"values\": [";
        if(b->type() == libdap::dods_str_c || b->type() == libdap::dods_url_c) {
            libdap::Str *strVar = (libdap::Str *) b;
            string tmpString = strVar->value();
            newAxis->values += "\"";
            newAxis->values += focovjson::escape_for_covjson(tmpString);
            newAxis->values += "\"";
        }
        else {
            ostringstream otemp;
            istringstream itemp;
            int tempVal = 0;
            b->print_val(otemp, "", false);
            istringstream (otemp.str());
            istringstream (otemp.str()) >> tempVal;
            newAxis->values += otemp.str();
        }
        newAxis->values += "]";
    }
    else {
        newAxis->values += "\"values\": []";
    }

    axes.push_back(newAxis);
    axisCount++;
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

    switch(a->var()->type()) {
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
