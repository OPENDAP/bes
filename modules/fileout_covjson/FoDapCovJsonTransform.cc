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


bool FoDapCovJsonTransform::canConvert()
{
    // If x, y, z, and t all exist
    // We are assuming the following is true:
    //    - shapeVals[0] = x axis
    //    - shapeVals[1] = y axis
    //    - shapeVals[2] = z axis
    //    - shapeVals[3] = t axis
    if(xExists && yExists && zExists && tExists) {

        if (shapeVals.size() < 4)
            return false;

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

        if (shapeVals.size() < 3)
            return false;

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

        if (shapeVals.size() < 2)
            return false;

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

template<typename T>
unsigned int FoDapCovJsonTransform::covjsonSimpleTypeArrayWorker(ostream *strm, T *values, unsigned int indx,
    vector<unsigned int> *shape, unsigned int currentDim)
{
    unsigned int currentDimSize = (*shape)[currentDim];

    // FOR TESTING AND DEBUGGING PURPOSES
    // *strm << "\"currentDim\": \"" << currentDim << "\"" << endl;
    // *strm << "\"currentDimSize\": \"" << currentDimSize << "\"" << endl;

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

    return indx;
}

template<typename T>
void FoDapCovJsonTransform::covjsonSimpleTypeArray(ostream *strm, libdap::Array *a, string indent, bool sendData)
{
    string childindent = indent + _indent_increment;
    bool axisRetrieved = false;
    bool parameterRetrieved = false;

    currDataType = a->var()->type_name();

    // FOR TESTING AND DEBUGGING PURPOSES
    // *strm << "\"type_name\": \"" << a->var()->type_name() << "\"" << endl;

    getAttributes(strm, a->get_attr_table(), a->name(), &axisRetrieved, &parameterRetrieved);

    // a->print_val(*strm, "\n", true); // For testing purposes

    // sendData = false; // For testing purposes

    // If we are dealing with an Axis
    if((axisRetrieved == true) && (parameterRetrieved == false)) {
        struct Axis *currAxis;
        currAxis = axes[axisCount - 1];

        int numDim = a->dimensions(true);
        vector<unsigned int> shape(numDim);
        long length = focovjson::computeConstrainedShape(a, &shape);

        // FOR TESTING AND DEBUGGING PURPOSES
        // *strm << "\"numDimensions\": \"" << numDim << "\"" << endl;
        // *strm << "\"length\": \"" << length << "\"" << endl << endl;

        if (currAxis->name.compare("t") != 0) {
            if (sendData) {
                currAxis->values += "\"values\": [";
                unsigned int indx = 0;
                vector<T> src(length);
                a->value(&src[0]);

                ostringstream astrm;
                indx = covjsonSimpleTypeArrayWorker(&astrm, &src[0], 0, &shape, 0);
                currAxis->values += astrm.str();

                currAxis->values += "]";

                if (length != indx) {
                    BESDEBUG(FoDapCovJsonTransform_debug_key,
                        "covjsonSimpleTypeArray(Axis) - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);
                }
                assert(length == indx);
            }
            else {
                currAxis->values += "\"values\": []";
            }
        }
    }

    // If we are dealing with a Parameter
    else if(axisRetrieved == false && parameterRetrieved == true) {
        struct Parameter *currParameter;
        currParameter = parameters[parameterCount - 1];

        int numDim = a->dimensions(true);
        vector<unsigned int> shape(numDim);
        long length = focovjson::computeConstrainedShape(a, &shape);

        // FOR TESTING AND DEBUGGING PURPOSES
        // *strm << "\"numDimensions\": \"" << a->dimensions(true) << "\"" << endl;
        // *strm << "\"length\": \"" << length << "\"" << endl << endl;

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

        if (sendData) {
            currParameter->values += "\"values\": [";
            unsigned int indx = 0;
            vector<T> src(length);
            a->value(&src[0]);

            ostringstream pstrm;
            indx = covjsonSimpleTypeArrayWorker(&pstrm, &src[0], 0, &shape, 0);
            currParameter->values += pstrm.str();

            currParameter->values += "]";

            if (length != indx) {
                BESDEBUG(FoDapCovJsonTransform_debug_key,
                    "covjsonSimpleTypeArray(Parameter) - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);
            }
            assert(length == indx);
        }
        else {
            currParameter->values += "\"values\": []";
        }
    }
}

void FoDapCovJsonTransform::covjsonStringArray(ostream *strm, libdap::Array *a, string indent, bool sendData)
{
    string childindent = indent + _indent_increment;
    bool axisRetrieved = false;
    bool parameterRetrieved = false;

    currDataType = a->var()->type_name();

    // FOR TESTING AND DEBUGGING PURPOSES
    // *strm << "\"attr_tableName\": \"" << a->name() << "\"" << endl;

    // FOR TESTING AND DEBUGGING PURPOSES
    // *strm << "\"type_name\": \"" << a->var()->type_name() << "\"" << endl;

    getAttributes(strm, a->get_attr_table(), a->name(), &axisRetrieved, &parameterRetrieved);

    // a->print_val(*strm, "\n", true); // For testing purposes

    // sendData = false; // For testing purposes

    // If we are dealing with an Axis
    if((axisRetrieved == true) && (parameterRetrieved == false)) {
        struct Axis *currAxis;
        currAxis = axes[axisCount - 1];

        int numDim = a->dimensions(true);
        vector<unsigned int> shape(numDim);
        long length = focovjson::computeConstrainedShape(a, &shape);

        if (currAxis->name.compare("t") != 0) {
            if (sendData) {
                currAxis->values += "\"values\": ";
                unsigned int indx = 0;
                // The string type utilizes a specialized version of libdap:Array.value()
                vector<string> sourceValues;
                a->value(sourceValues);

                ostringstream astrm;
                indx = covjsonSimpleTypeArrayWorker(&astrm, (string *) (&sourceValues[0]), 0, &shape, 0);
                currAxis->values += astrm.str();

                if (length != indx) {
                    BESDEBUG(FoDapCovJsonTransform_debug_key,
                        "covjsonStringArray(Axis) - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);
                }
                assert(length == indx);
            }
            else {
                currAxis->values += "\"values\": []";
            }
        }
    }

    // If we are dealing with a Parameter
    else if(axisRetrieved == false && parameterRetrieved == true) {
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

        if (sendData) {
            currParameter->values += "\"values\": ";
            unsigned int indx = 0;
            // The string type utilizes a specialized version of libdap:Array.value()
            vector<string> sourceValues;
            a->value(sourceValues);

            ostringstream pstrm;
            indx = covjsonSimpleTypeArrayWorker(&pstrm, (string *) (&sourceValues[0]), 0, &shape, 0);
            currParameter->values += pstrm.str();

            if (length != indx) {
                BESDEBUG(FoDapCovJsonTransform_debug_key,
                    "covjsonStringArray(Parameter) - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);
            }
            assert(length == indx);
        }
        else {
            currParameter->values += "\"values\": []";
        }
    }
}

void FoDapCovJsonTransform::addAxis(string name, string values) 
{
    struct Axis *newAxis = new Axis;

    newAxis->name = name;
    newAxis->values = values;

    this->axes.push_back(newAxis);
    this->axisCount++;
}

void FoDapCovJsonTransform::addParameter(string id, string name, string type, string dataType, string unit,
    string longName, string standardName, string shape, string values) 
{
    struct Parameter *newParameter = new Parameter;

    newParameter->id = id;
    newParameter->name = name;
    newParameter->type = type;
    newParameter->dataType = dataType;
    newParameter->unit = unit;
    newParameter->longName = longName;
    newParameter->standardName = standardName;
    newParameter->shape = shape;
    newParameter->values = values;

    this->parameters.push_back(newParameter);
    this->parameterCount++;
}

void FoDapCovJsonTransform::getAttributes(ostream *strm, libdap::AttrTable &attr_table, string name,
    bool *axisRetrieved, bool *parameterRetrieved)
{
    string currAxisName;
    string currAxisTimeOrigin;
    string currUnit;
    string currLongName;
    string currStandardName;

    isAxis = false;
    isParam = false;

    *axisRetrieved = false;
    *parameterRetrieved = false;

    // FOR TESTING AND DEBUGGING PURPOSES
    // *strm << "\"attr_tableName\": \"" << name << "\"" << endl;

    // Using CF-1.6 naming conventions -- Also checks for Coads Climatology conventions
    // http://cfconventions.org/Data/cf-conventions/cf-conventions-1.7/cf-conventions.html
    if((name.compare("lon") == 0) || (name.compare("LON") == 0)
        || (name.compare("longitude") == 0) || (name.compare("LONGITUDE") == 0)
        || (name.compare("COADSX") == 0)) {

        // FOR TESTING AND DEBUGGING PURPOSES
        // *strm << "\"Found X-Axis\": \"" << name << "\"" << endl;

        if(!xExists) {
            xExists = true;
            isAxis = true;
            currAxisName = "x";
        }
    }
    else if((name.compare("lat") == 0) || (name.compare("LAT") == 0)
        || (name.compare("latitude") == 0) || (name.compare("LATITUDE") == 0)
        || (name.compare("COADSY") == 0)) {

        // FOR TESTING AND DEBUGGING PURPOSES
        // *strm << "\"Found Y-Axis\": \"" << name << "\"" << endl;

        if(!yExists) {
            yExists = true;
            isAxis = true;
            currAxisName = "y";
        }
    }
    else if((name.compare("lev") == 0) || (name.compare("LEV") == 0)
        || (name.compare("height") == 0) || (name.compare("HEIGHT") == 0)
        || (name.compare("depth") == 0) || (name.compare("DEPTH") == 0)
        || (name.compare("pres") == 0) || (name.compare("PRES") == 0)) {

        // FOR TESTING AND DEBUGGING PURPOSES
        // *strm << "\"Found Z-Axis\": \"" << name << "\"" << endl;

        if(!zExists) {
            zExists = true;
            isAxis = true;
            currAxisName = "z";
        }
    }
    else if((name.compare("time") == 0) || (name.compare("TIME") == 0)) {

        // FOR TESTING AND DEBUGGING PURPOSES
        // *strm << "\"Found T-Axis\": \"" << name << "\"" << endl;

        if(!tExists) {
            tExists = true;
            isAxis = true;
            currAxisName = "t";
        }
    }
    else {
        isParam = true;
    }

    // Only do more if there are actually attributes in the table
    if(attr_table.get_size() != 0) {
        libdap::AttrTable::Attr_iter begin = attr_table.attr_begin();
        libdap::AttrTable::Attr_iter end = attr_table.attr_end();

        for(libdap::AttrTable::Attr_iter at_iter = begin; at_iter != end; at_iter++) {
            // FOR TESTING AND DEBUGGING PURPOSES 
            // attr_table.print(*strm);

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

                    // From Climate and Forecast (CF) Conventions:
                    // http://cfconventions.org/Data/cf-conventions/cf-conventions-1.7/cf-conventions.html#_description_of_the_data

                    // We continue to support the use of the units and long_name attributes as defined in COARDS.
                    // We extend COARDS by adding the optional standard_name attribute which is used to provide unique
                    // identifiers for variables. This is important for data exchange since one cannot necessarily
                    // identify a particular variable based on the name assigned to it by the institution that provided
                    // the data.

                    // The standard_name attribute can be used to identify variables that contain coordinate data. But since it is an
                    // optional attribute, applications that implement these standards must continue to be able to identify coordinate
                    // types based on the COARDS conventions.

                    // See http://cfconventions.org/Data/cf-conventions/cf-conventions-1.7/cf-conventions.html#units
                    if(currName.compare("units") == 0) {
                        if(isAxis) {
                            if(currAxisName.compare("t") == 0) {
                                currAxisTimeOrigin = currValue;
                            }
                        }
                        else if(isParam) {
                            currUnit = currValue;
                        }
                    }
                    // See http://cfconventions.org/Data/cf-conventions/cf-conventions-1.7/cf-conventions.html#long-name
                    else if(currName.compare("long_name") == 0) {
                        currLongName = currValue;
                    }
                    // See http://cfconventions.org/Data/cf-conventions/cf-conventions-1.7/cf-conventions.html#standard-name
                    else if(currName.compare("standard_name") == 0) {
                        currStandardName = currValue;
                    }
                }

                break;
            }
            }
        }
    }

    if(isAxis) {
        // If we're dealing with the time axis, capture the time
        // origin timestamp value with the appropriate formatting
        // for printing.
        // @TODO See https://covjson.org/spec/#temporal-reference-systems
        if(currAxisName.compare("t") == 0) {
            // If the calendar is based on years, months, days,
            // then the referenced values SHOULD use one of the
            // following ISO8601-based lexical representations:

            //  YYYY
            //  ±XYYYY (where X stands for extra year digits)
            //  YYYY-MM
            //  YYYY-MM-DD
            //  YYYY-MM-DDTHH:MM:SS[.F]Z where Z is either “Z”
            //      or a time scale offset + -HH:MM

            // If calendar dates with reduced precision are
            // used in a lexical representation (e.g. "2016"),
            // then a client SHOULD interpret those dates in
            // that reduced precision.

            // @TODO Need to figure out a way to dynamically parse
            // origin timestamps and convert them to an appropriate
            // format for CoverageJSON

            // ex: "2018-01-01T00:12:20Z"
            addAxis(currAxisName, "\"values\": [\"" + sanitizeTimeOriginString(currAxisTimeOrigin) + "\"]");
        }
        else {
            addAxis(currAxisName, "");
        }

        *axisRetrieved = true;
    }
    else if(isParam) {
        // Kent says: Use LongName to select the new Parameter is too strict.
        // but when the test 'currLongName.compare("") != 0' is removed,
        // all of the tests fail and do so by generating output that looks clearly
        // wrong. I'm going to hold off on this part of the patch for now. jhrg 3/28/19

        // Removed the 'currLongName.compare("") != 0' test statement. The removed
        // statement was a constraint to ensure that any parameter retrieved would
        // have a descriptive long_name attribute for printing observedProperty->label.
        //
        // Per Jon Blower:
        // observedProperty->label comes from:
        //    - The CF long_name, if it exists
        //    - If not, the CF standard_name, perhaps with underscores removed
        //    - If the standard_name doesn’t exist, use the variable ID
        //
        // This constraint is now evaluated in the printParameters worker
        // rather than within this function where the parameter is retrieved.
        // -CH 5/11/2019
        addParameter("", name, "", currDataType, currUnit, currLongName, currStandardName, "", "");

        *parameterRetrieved = true;
    }
    else {
        // Do nothing
    }
}

string FoDapCovJsonTransform::sanitizeTimeOriginString(string timeOrigin) {
    string cleanTimeOrigin = timeOrigin;

    return cleanTimeOrigin;
}

FoDapCovJsonTransform::FoDapCovJsonTransform(libdap::DDS *dds) :
    _dds(dds), _returnAs(""), _indent_increment("  "), atomicVals(""), currDataType(""), domainType(0),
    xExists(false), yExists(false), zExists(false), tExists(false), isParam(false), isAxis(false),
    canConvertToCovJson(false), axisCount(0), parameterCount(0)
{
    if (!_dds) throw BESInternalError("File out COVJSON, null DDS passed to constructor", __FILE__, __LINE__);
}

void FoDapCovJsonTransform::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FoDapCovJsonTransform::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if(_dds != 0) {
        _dds->print(strm);
    }
    BESIndent::UnIndent();
}

void FoDapCovJsonTransform::transform(ostream &ostrm, bool sendData, bool testOverride)
{
    transform(&ostrm, _dds, "", sendData, testOverride);
}

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

void FoDapCovJsonTransform::transformNodeWorker(ostream *strm, vector<libdap::BaseType *> leaves,
    vector<libdap::BaseType *> nodes, string indent, bool sendData)
{
    // Get this node's leaves
    for(vector<libdap::BaseType *>::size_type l = 0; l < leaves.size(); l++) {
        libdap::BaseType *v = leaves[l];
        BESDEBUG(FoDapCovJsonTransform_debug_key, "Processing LEAF: " << v->name() << endl);

        // FOR TESTING AND DEBUGGING PURPOSES
        // *strm << "Processing LEAF: " <<  v->name() << endl;

        transform(strm, v, indent + _indent_increment, sendData);
    }

    // Get this node's child nodes
    for(vector<libdap::BaseType *>::size_type n = 0; n < nodes.size(); n++) {
        libdap::BaseType *v = nodes[n];
        BESDEBUG(FoDapCovJsonTransform_debug_key, "Processing NODE: " << v->name() << endl);

        // FOR TESTING AND DEBUGGING PURPOSES
        // *strm << "Processing NODE: " <<  v->name() << endl;

        transform(strm, v, indent + _indent_increment, sendData);
    }
}

void FoDapCovJsonTransform::printCoverageJSON(ostream *strm, string indent, bool testOverride)
{
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
        // Prints the entire Coverage to stream
        printCoverage(strm, indent);
    }
    else {
        // If this file can't be converted, then its failing spatial/temporal requirements
        throw BESInternalError("File cannot be converted to CovJSON format due to missing or incompatible spatial dimensions", __FILE__, __LINE__);
    }
}

void FoDapCovJsonTransform::printCoverage(ostream *strm, string indent)
{
    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;

    BESDEBUG(FoDapCovJsonTransform_debug_key, "Printing COVERAGE" << endl);

    *strm << indent << "{" << endl;
    *strm << child_indent1 << "\"type\": \"Coverage\"," << endl;

    printDomain(strm, child_indent1);

    printParameters(strm, child_indent1);

    printRanges(strm, child_indent1);

    *strm << indent << "}" << endl;
}

void FoDapCovJsonTransform::printDomain(ostream *strm, string indent)
{
    string child_indent1 = indent + _indent_increment;

    BESDEBUG(FoDapCovJsonTransform_debug_key, "Printing DOMAIN" << endl);

    *strm << indent << "\"domain\": {" << endl;
    *strm << child_indent1 << "\"type\" : \"Domain\"," << endl;

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

    // Prints the axes metadata and range values
    printAxes(strm, child_indent1);

    // Prints the references for the given Axes
    printReference(strm, child_indent1);

    *strm << indent << "}," << endl;
}

void FoDapCovJsonTransform::printAxes(ostream *strm, string indent)
{
    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;

    BESDEBUG(FoDapCovJsonTransform_debug_key, "Printing AXES" << endl);

    // FOR TESTING AND DEBUGGING PURPOSES
    // *strm << "\"type_name\": \"" << a->var()->type_name() << "\"" << endl;

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
    *strm << indent << "}," << endl;
}

void FoDapCovJsonTransform::printReference(ostream *strm, string indent)
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
    *strm << indent << "}]" << endl;
}

void FoDapCovJsonTransform::printParameters(ostream *strm, string indent)
{
    string child_indent1 = indent + _indent_increment;
    string child_indent2 = child_indent1 + _indent_increment;
    string child_indent3 = child_indent2 + _indent_increment;
    string child_indent4 = child_indent3 + _indent_increment;

    BESDEBUG(FoDapCovJsonTransform_debug_key, "Printing PARAMETERS" << endl);

    // @TODO NEEDS REFACTORING FOR BES ISSUE #244
    // https://github.com/OPENDAP/bes/issues/244
    // Write down the parameter metadata
    *strm << indent << "\"parameters\": {" << endl;
    for(unsigned int i = 0; i < parameterCount; i++) {
        *strm << child_indent1 << "\"" << parameters[i]->name << "\": {" << endl;
        *strm << child_indent2 << "\"type\": \"Parameter\"," << endl;
        *strm << child_indent2 << "\"description\": {" << endl;

        if(parameters[i]->longName.compare("") != 0) {
            *strm << child_indent3 << "\"en\": \"" << parameters[i]->longName << "\"" << endl;
        }
        else if(parameters[i]->standardName.compare("") != 0) {
            *strm << child_indent3 << "\"en\": \"" << parameters[i]->standardName << "\"" << endl;
        }
        else {
            *strm << child_indent3 << "\"en\": \"" << parameters[i]->name << "\"" << endl;
        }

        *strm << child_indent2 << "}," << endl;
        *strm << child_indent2 << "\"unit\": {" << endl;
        *strm << child_indent3 << "\"label\": {" << endl;
        *strm << child_indent4 << "\"en\": \"" << parameters[i]->unit << "\"" << endl;
        *strm << child_indent3 << "}," << endl;
        *strm << child_indent3 << "\"symbol\": {" << endl;
        *strm << child_indent4 << "\"value\": \"" << parameters[i]->unit << "\"," << endl;
        *strm << child_indent4 << "\"type\": \"http://www.opengis.net/def/uom/UCUM/\"" << endl;
        *strm << child_indent3 << "}" << endl;
        *strm << child_indent2 << "}," << endl;
        *strm << child_indent2 << "\"observedProperty\": {" << endl;

        // Per Jon Blower:
        // observedProperty->id comes from the CF standard_name,
        // mapped to a URI like this: http://vocab.nerc.ac.uk/standard_name/<standard_name>.
        // If the standard_name is not present, omit the id.
        if(parameters[i]->standardName.compare("") != 0) {
            *strm << child_indent3 << "\"id\": \"http://vocab.nerc.ac.uk/standard_name/" << parameters[i]->standardName << "/\"" << endl;
        }

        // Per Jon Blower:
        // observedProperty->label comes from:
        //    - The CF long_name, if it exists
        //    - If not, the CF standard_name, perhaps with underscores removed
        //    - If the standard_name doesn’t exist, use the variable ID
        *strm << child_indent3 << "\"label\": {" << endl;

        if(parameters[i]->longName.compare("") != 0) {
            *strm << child_indent4 << "\"en\": \"" << parameters[i]->longName << "\"" << endl;
        }
        else if(parameters[i]->standardName.compare("") != 0) {
            *strm << child_indent4 << "\"en\": \"" << parameters[i]->standardName << "\"" << endl;
        }
        else {
            *strm << child_indent4 << "\"en\": \"" << parameters[i]->name << "\"" << endl;
        }

        *strm << child_indent3 << "}" << endl;
        *strm << child_indent2 << "}" << endl;

        if(i == parameterCount - 1) {
            *strm << child_indent1 << "}" << endl;
        }
        else {
            *strm << child_indent1 << "}," << endl;
        }
    }

    *strm << indent << "}," << endl;
}

void FoDapCovJsonTransform::printRanges(ostream *strm, string indent)
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
            dataType = "string";
        }

        // @TODO NEEDS REFACTORING FOR BES ISSUE #244
        // https://github.com/OPENDAP/bes/issues/244
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

    *strm << indent << "}" << endl;
}

void FoDapCovJsonTransform::transform(ostream *strm, libdap::DDS *dds, string indent, bool sendData, bool testOverride)
{
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
    transformNodeWorker(strm, leaves, nodes, indent + _indent_increment + _indent_increment, sendData);

    // Print the Coverage data to stream as CoverageJSON
    printCoverageJSON(strm, indent, testOverride);
}

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
        string s = (string) "File out COVJSON, DAP4 types not yet supported.";
        throw BESInternalError(s, __FILE__, __LINE__);
        break;
    }

    default: {
        string s = (string) "File out COVJSON, Unrecognized type.";
        throw BESInternalError(s, __FILE__, __LINE__);
        break;
    }
    }
}

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

    case libdap::dods_structure_c:
        throw BESInternalError("File out COVJSON, Arrays of Structure objects not a supported return type.", __FILE__, __LINE__);

    case libdap::dods_grid_c:
        throw BESInternalError("File out COVJSON, Arrays of Grid objects not a supported return type.", __FILE__, __LINE__);

    case libdap::dods_sequence_c:
        throw BESInternalError("File out COVJSON, Arrays of Sequence objects not a supported return type.", __FILE__, __LINE__);

    case libdap::dods_array_c:
        throw BESInternalError("File out COVJSON, Arrays of Array objects not a supported return type.", __FILE__, __LINE__);

    case libdap::dods_int8_c:
    case libdap::dods_uint8_c:
    case libdap::dods_int64_c:
    case libdap::dods_uint64_c:
    case libdap::dods_enum_c:
    case libdap::dods_group_c:
        throw BESInternalError("File out COVJSON, DAP4 types not yet supported.", __FILE__, __LINE__);

    default:
        throw BESInternalError("File out COVJSON, Unrecognized type.", __FILE__, __LINE__);
    }
}
