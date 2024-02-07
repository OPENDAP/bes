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
// Author: Kent Yang <myang6@hdfgroup.org> 2022-10
// Note from KY: Make the module correctly generate simple grid,point,
//               point series and vertical profile coverage. The DAP2 
//               grid also correctly maps to coverage.
//               Also the original testsuite is completely replaced.
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
#include <time.h>

using std::ostringstream;
using std::istringstream;

#define MODULE "covj"
#define prolog string("FoDapCovJsonTransform::").append(__func__).append("() - ")

#include <libdap/DDS.h>
#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/D4Attributes.h>
#include <libdap/Structure.h>
#include <libdap/Constructor.h>
#include <libdap/Array.h>
#include <libdap/Grid.h>
#include <libdap/Sequence.h>
#include <libdap/Byte.h>
#include <libdap/UInt16.h>
#include <libdap/Int16.h>
#include <libdap/UInt32.h>
#include <libdap/Int32.h>
#include <libdap/Float32.h>
#include <libdap/Float64.h>
#include <libdap/Str.h>
#include <libdap/Url.h>

#include <BESDebug.h>
#include <BESInternalError.h>
#include <DapFunctionUtils.h>
#include <RequestServiceTimer.h>
#include "FoDapCovJsonTransform.h"
#include "focovjson_utils.h"
#include "FoCovJsonRequestHandler.h"

using std::map;
#define FoDapCovJsonTransform_debug_key "focovjson"


bool FoDapCovJsonTransform::canConvert()
{
    // If x, y, z, and t all exist
    // We are assuming the following is true:
    //    - shapeVals[0] = x axis
    //    - shapeVals[1] = y axis
    //    - shapeVals[2] = z axis
    //    - shapeVals[3] = t axis
#if 0
cerr<<"Before X and Y and Z and T"<<endl;
cerr<<"Number of parameters is  "<<this->parameters.size() <<endl;
cerr<<"shapeVals is "<<shapeVals.size() <<endl;
cerr<<"Number of Axis is "<<this->axes.size() <<endl;
for (int i = 0; i <this->axes.size(); i++) {
cerr<<"Axis name is "<<this->axes[i]->name << endl;
cerr<<"Axis value is "<<this->axes[i]->values << endl;

}
    
#endif

    bool ret_value = false;
    if(true == is_simple_cf_geographic || true == is_dap2_grid) {
        domainType = "Grid";   
        ret_value = true;
    }
    else {
        
        ret_value = true;
        switch (dsg_type) {
        case SPOINT: {
            domainType = "Point";
            break;
        }
        case POINTS: {
            domainType = "PointSeries";
            break;
        }
        case PROFILE: {
            domainType = "VerticalProfile";
            break;
        }
        default:
            ret_value = false;
        }
    }

    return ret_value;
    
    // The following code is commented out for the time being. 
    // We only support the simple CF geographic projection, point, pointSeries, vertical profile for the time being.  
    // We will enhance this module to support the other cases in the new tickets.
    // KY 2022-06-10
#if 0
    if(xExists && yExists && zExists && tExists) {

        if (shapeVals.size() < 4)
            return false;

        // A domain with Grid domain type MUST have the axes "x" and "y"
        // and MAY have the axes "z" and "t".
        if((shapeVals[0] > 1) && (shapeVals[1] > 1) && (shapeVals[2] >= 1) && (shapeVals[3] >= 0)) {
            domainType = "Grid";
            return true;
        }

        // A domain with VerticalProfile domain type MUST have the axes "x",
        // "y", and "z", where "x" and "y" MUST have a single coordinate only.
        else if((shapeVals[0] == 1) && (shapeVals[1] == 1) && (shapeVals[2] >= 1) && ((shapeVals[3] <= 1) && (shapeVals[3] >= 0))) {
            domainType = "Vertical Profile";
            return true;
        }

        // A domain with PointSeries domain type MUST have the axes "x", "y",
        // and "t" where "x" and "y" MUST have a single coordinate only. A
        // domain with PointSeries domain type MAY have the axis "z" which
        // MUST have a single coordinate only.
        else if((shapeVals[0] == 1) && (shapeVals[1] == 1) && (shapeVals[2] == 1) && (shapeVals[3] >= 0)) {
            domainType = "Point Series";
            return true;
        }

        // A domain with Point domain type MUST have the axes "x" and "y" and MAY
        // have the axes "z" and "t" where all MUST have a single coordinate only.
        else if((shapeVals[0] == 1) && (shapeVals[1] == 1) && (shapeVals[2] == 1) && (shapeVals[3] == 1)) {
            domainType = "Point";
            return true;
        }
//cerr<<"Before X and Y and T"<<endl;
    }

    // If just x, y, and t exist
    // We are assuming the following is true:
    //    - shapeVals[0] = x axis
    //    - shapeVals[1] = y axis
    //    - shapeVals[2] = t axis
    else if(xExists && yExists && !zExists && tExists) {

        if (shapeVals.size() < 3)
            return false;

#if 0
//cerr <<"shapeVals[0] is "<< shapeVals[0] <<endl;
//cerr <<"shapeVals[1] is "<< shapeVals[1] <<endl;
//cerr <<"shapeVals[2] is "<< shapeVals[2] <<endl;
#endif

        // A domain with Grid domain type MUST have the axes "x" and "y"
        // and MAY have the axes "z" and "t".
        // The issue here is that shapeVals[0], shapeVals[1],shapeVals[2] may not be exactly x,y,z/t. 
        if((shapeVals[0] >= 1) && (shapeVals[1] >= 1) && (shapeVals[2] >= 0)) {
            domainType = "Grid";
            return true;
        }

        // A domain with PointSeries domain type MUST have the axes "x", "y",
        // and "t" where "x" and "y" MUST have a single coordinate only. A
        // domain with PointSeries domain type MAY have the axis "z" which
        // MUST have a single coordinate only.
        else if((shapeVals[0] == 1) && (shapeVals[1] == 1) && (shapeVals[2] >= 0)) {
            domainType = "Point Series";
            return true;
        }

        // A domain with Point domain type MUST have the axes "x" and "y" and MAY
        // have the axes "z" and "t" where all MUST have a single coordinate only.
        else if((shapeVals[0] == 1) && (shapeVals[1] == 1) && (shapeVals[2] == 1)) {
            domainType = "Point";
            return true;
        }
//cerr<<"Before X and Y "<<endl;
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
            domainType = "Grid";
            return true;
        }

        // A domain with Point domain type MUST have the axes "x" and "y" and MAY
        // have the axes "z" and "t" where all MUST have a single coordinate only.
        else if((shapeVals[0] == 1) && (shapeVals[1] == 1)) {
            domainType = "Point";
            return true;
        }
    }
//cerr<<"Coming to the last step."<<endl;

    return false; // This source DDS is not valid as CovJSON

#endif
}

template<typename T>
unsigned int FoDapCovJsonTransform::covjsonSimpleTypeArrayWorker(ostream *strm, T *values, unsigned int indx,
    vector<unsigned int> *shape, unsigned int currentDim, bool is_axis_t_sgeo,libdap::Type a_type)
{
    unsigned int currentDimSize = (*shape)[currentDim];

    // FOR TESTING AND DEBUGGING PURPOSES
    // *strm << "\"currentDim\": \"" << currentDim << "\"" << endl;
    // *strm << "\"currentDimSize\": \"" << currentDimSize << "\"" << endl;

    for(unsigned int i = 0; i < currentDimSize; i++) {
        if(currentDim < shape->size() - 1) {
            BESDEBUG(FoDapCovJsonTransform_debug_key,
                "covjsonSimpleTypeArrayWorker() - Recursing! indx:  " << indx << " currentDim: " << currentDim << " currentDimSize: " << currentDimSize << endl);
            indx = covjsonSimpleTypeArrayWorker<T>(strm, values, indx, shape, currentDim + 1,is_axis_t_sgeo,a_type);
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
                // We need to convert CF time to greg time.
                if(is_axis_t_sgeo) { 
                    string axis_t_value;
                    std::ostringstream tmp_stream; 
                    long long tmp_value = 0;
                    switch (a_type) {
                    case libdap::dods_byte_c: {
                        unsigned char tmp_byte_value = reinterpret_cast<unsigned char *>(values)[indx++];                       
                        tmp_value = (long long) tmp_byte_value;
                        break;
                    }

                    case libdap::dods_uint16_c:  {
                        unsigned short tmp_uint16_value = reinterpret_cast<unsigned short *>(values)[indx++];                       
                        tmp_value = (long long) tmp_uint16_value;
                        break;
                    }

                    case libdap::dods_int16_c: {
                        short tmp_int16_value = reinterpret_cast<short *>(values)[indx++];                       
                        tmp_value = (long long) tmp_int16_value;
                        break;
                    }

                    case libdap::dods_uint32_c: {
                        unsigned int tmp_uint_value = reinterpret_cast<unsigned int *>(values)[indx++];                       
                        tmp_value = (long long) tmp_uint_value;
                        break;
                    }

                    case libdap::dods_int32_c: {
                        int tmp_int_value = reinterpret_cast<int *>(values)[indx++];                       
                        tmp_value = (long long) tmp_int_value;
                        break;
                    }

                    case libdap::dods_float32_c: {
                        float tmp_float_value = reinterpret_cast<float *>(values)[indx++];                       
                        // In theory, it may cause overflow. In reality, the time in seconds will never be that large.
                        tmp_value = (long long) tmp_float_value;
                        break;
                    }

                    case libdap::dods_float64_c: {
                        double tmp_double_value = reinterpret_cast<double *>(values)[indx++];                       
                        // In theory, it may cause overflow. In reality, the time in seconds will never be that large.
                        tmp_value = (long long) tmp_double_value;
                        break;
                    }

                    default:
                        throw BESInternalError("Attempt to extract CF time information from an invalid source", __FILE__, __LINE__);
                    }


                    axis_t_value = cf_time_to_greg(tmp_value);
#if 0
                    cerr<<"time value is " <<axis_t_value <<endl;
                    cerr<<"CF time unit is "<<axis_t_units <<endl;
#endif
                    *strm << "\"" << focovjson::escape_for_covjson(axis_t_value) << "\"";
                }
                else 
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
    //*strm << "\"type_name\": \"" << a->var()->type_name() << "\"" << endl;
    //*strm << "\"name\": \"" << a->var()->name() << "\"" << endl;

    getAttributes(strm, a->get_attr_table(), a->name(), &axisRetrieved, &parameterRetrieved);

    //a->print_val(*strm, "\n", true); // For testing purposes

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

        bool handle_axis_t_here = true;
        if(is_simple_cf_geographic==false && dsg_type == UNSUPPORTED_DSG  && currAxis->name.compare("t") == 0)
            handle_axis_t_here = false;
        if (handle_axis_t_here) {
            if (sendData) {
                currAxis->values += "\"values\": [";
                unsigned int indx = 0;
                vector<T> src(length);
                a->value(src.data());

                ostringstream astrm;
                bool is_time_axis_for_sgeo = false;
                if((is_simple_cf_geographic || dsg_type != UNSUPPORTED_DSG) && currAxis->name.compare("t") == 0)
                    is_time_axis_for_sgeo = true;


                indx = covjsonSimpleTypeArrayWorker(&astrm, src.data(), 0, &shape, 0,is_time_axis_for_sgeo,a->var()->type());
                currAxis->values += astrm.str();

                currAxis->values += "]";
#if 0
//cerr<<"currAxis value at covjsonSimpleTypeArray is "<<currAxis->values <<endl;
#endif
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

#if 0
cerr<<"Parameter name is "<< currParameter->name<<endl;
#endif
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
            if((i == 0) && tExists && is_simple_cf_geographic == false && dsg_type == UNSUPPORTED_DSG) {
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
            a->value(src.data());

            ostringstream pstrm;
            indx = covjsonSimpleTypeArrayWorker(&pstrm, src.data(), 0, &shape, 0,false,a->var()->type());
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

        bool handle_axis_t_here = true;
        if(is_simple_cf_geographic==false && dsg_type == UNSUPPORTED_DSG  && currAxis->name.compare("t") == 0)
            handle_axis_t_here = false;
        if (handle_axis_t_here) {
            if (sendData) {
                currAxis->values += "\"values\": ";
                unsigned int indx = 0;
                // The string type utilizes a specialized version of libdap:Array.value()
                vector<string> sourceValues;
                a->value(sourceValues);

                ostringstream astrm;
                indx = covjsonSimpleTypeArrayWorker(&astrm, (string *) (sourceValues.data()), 0, &shape, 0,false,a->var()->type());
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
            if((i == 0) && tExists && is_simple_cf_geographic == false && dsg_type == UNSUPPORTED_DSG) {
            //if((i == 0) && tExists) {
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
            indx = covjsonSimpleTypeArrayWorker(&pstrm, (string *) (sourceValues.data()), 0, &shape, 0,false,a->var()->type());
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
#if 0
cerr << "axis name is "<< name <<endl;
cerr << "axis value is "<< values <<endl;
#endif
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
    //*strm << "\"attr_tableName\": \"" << name << "\"" << endl;

    
    if (is_simple_cf_geographic || dsg_type!=UNSUPPORTED_DSG) 
        getAttributes_simple_cf_geographic_dsg(strm,attr_table,name,axisRetrieved,parameterRetrieved);
    else {
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
            //*strm << "\"Found Y-Axis\": \"" << name << "\"" << endl;
    
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
                        //*strm << "\"currName\": \"" << currName << "\", \"currValue\": \"" << currValue << "\"" << endl;
    
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
                            currUnit = currValue;
                            if(isAxis) {
                                if(currAxisName.compare("t") == 0) {
                                    currAxisTimeOrigin = currValue;
                                }
                            }
                        }
    
                        // Per Jon Blower:
                        // observedProperty->label comes from:
                        //    - The CF long_name, if it exists
                        //    - If not, the CF standard_name, perhaps with underscores removed
                        //    - If the standard_name doesn’t exist, use the variable ID
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
            // If we're dealing with the time axis, capture the time origin 
            // timestamp value with the appropriate formatting for printing.
            // @TODO See https://covjson.org/spec/#temporal-reference-systems
            if(currAxisName.compare("t") == 0) {
                addAxis(currAxisName, "\"values\": [\"" + sanitizeTimeOriginString(currAxisTimeOrigin) + "\"]");
            }
            else {
                addAxis(currAxisName, "");
            }
#if 0
    //cerr<<"currAxisName is "<<currAxisName <<endl;
    //cerr<<"currUnit is "<<currUnit <<endl;
#endif
    
            // See https://covjson.org/spec/#projected-coordinate-reference-systems
            // KENT: The below "if block" is wrong. If the units of lat/lon includes east, north, it may be geographic projection.
            // The ProjectedCRS may imply the 2-D lat/lon. If the variable name is the same as the axis name, and the lat/lon
            // are 1-D, this is a geographic system.
            if ((is_geo_dap2_grid == false) && 
                ((currUnit.find("east") != string::npos) || (currUnit.find("East") != string::npos) || 
                   (currUnit.find("north") != string::npos) || (currUnit.find("North") != string::npos))) 
                    coordRefType = "ProjectedCRS";
                
    
            *axisRetrieved = true;
        }
        else if(isParam) {
            addParameter("", name, "", currDataType, currUnit, currLongName, currStandardName, "", "");
            *parameterRetrieved = true;
        }
        else {
            // Do nothing
        }
    }
}

void FoDapCovJsonTransform::
    getAttributes_simple_cf_geographic_dsg(ostream *strm, libdap::AttrTable &attr_table, const string& name,
    bool *axisRetrieved, bool *parameterRetrieved)
{
    string currAxisName;
    string currUnit;
    string currLongName;
    string currStandardName;

    isAxis = false;
    isParam = false;

    *axisRetrieved = false;
    *parameterRetrieved = false;

    // FOR TESTING AND DEBUGGING PURPOSES
    //*strm << "\"attr_tableName\": \"" << name << "\"" << endl;
    
    if(axisVar_x.name == name) {
        // FOR TESTING AND DEBUGGING PURPOSES
        //*strm << "\"Found X-Axis\": \"" << name << "\"" << endl;

        if(!xExists) {
            xExists = true;
            isAxis = true;
            currAxisName = "x";
        }
    }
    else if(axisVar_y.name == name) {
        // FOR TESTING AND DEBUGGING PURPOSES
        //*strm << "\"Found Y-Axis\": \"" << name << "\"" << endl;

        if(!yExists) {
            yExists = true;
            isAxis = true;
            currAxisName = "y";
        }
    }
    else if(axisVar_z.name == name) {
        // FOR TESTING AND DEBUGGING PURPOSES
        //*strm << "\"Found Z-Axis\": \"" << name << "\"" << endl;

        if(!zExists) {
            zExists = true;
            isAxis = true;
            currAxisName = "z";
        }
    }
    else if(axisVar_t.name == name) {
        // FOR TESTING AND DEBUGGING PURPOSES
        //*strm << "\"Found T-Axis\": \"" << name << "\"" << endl;

        if(!tExists) {
            tExists = true;
            isAxis = true;
            currAxisName = "t";
        }
    }
    else {
        // TODO: We should manage to remove this loop when improving the whole module.
        for (unsigned int i = 0; i <par_vars.size(); i++) {
            if(par_vars[i] == name){
                isParam = true;
                break;
            }
        }
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
                getAttributes_simple_cf_geographic_dsg(strm, *atbl, name, axisRetrieved, parameterRetrieved);
                break;
            }
            default: {
                vector<string> *values = attr_table.get_attr_vector(at_iter);

                for(vector<string>::size_type i = 0; i < values->size(); i++) {
                    string currName = attr_table.get_name(at_iter);
                    string currValue = (*values)[i];

                    // FOR TESTING AND DEBUGGING PURPOSES
                    //*strm << "\"currName\": \"" << currName << "\", \"currValue\": \"" << currValue << "\"" << endl;

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
                    if(currName.compare("units") == 0) 
                        currUnit = currValue;

                    // Per Jon Blower:
                    // observedProperty->label comes from:
                    //    - The CF long_name, if it exists
                    //    - If not, the CF standard_name, perhaps with underscores removed
                    //    - If the standard_name doesn’t exist, use the variable ID
                    // See http://cfconventions.org/Data/cf-conventions/cf-conventions-1.7/cf-conventions.html#long-name
                    else if(currName.compare("long_name") == 0) {
                        currLongName = currValue;
                    }
                    // See http://cfconventions.org/Data/cf-conventions/cf-conventions-1.7/cf-conventions.html#standard-name
                    else if(currName.compare("standard_name") == 0) {
                        currStandardName = currValue;
                    }
                    
                }

                if (axisVar_z.name == name) {// We need to check if the attribute "positive" exists.

                    for(vector<string>::size_type i = 0; i < values->size(); i++) {
                        string currName = attr_table.get_name(at_iter);
                        if (currName == "positive") {
                            string currValue = (*values)[i];
                            axis_z_direction = currValue;
                            break;
                        }
                    }

                }

                break;
            }
            }
        }
    }

    if(isAxis) {
        addAxis(currAxisName, "");
#if 0
//cerr<<"currAxisName is "<<currAxisName <<endl;
//cerr<<"currUnit is "<<currUnit <<endl;
#endif
        if (currAxisName.compare("t") == 0)
            axis_t_units = currUnit;
        if (currAxisName.compare("z") == 0) {
             axis_z_standardName=currStandardName;
             axis_z_units = currUnit;
        }
        *axisRetrieved = true;
    }
    else if(isParam) {
        addParameter("", name, "", currDataType, currUnit, currLongName, currStandardName, "", "");
        *parameterRetrieved = true;
    }
    else {
        // Do nothing
    }
  
}


string FoDapCovJsonTransform::sanitizeTimeOriginString(string timeOrigin) 
{
    // If the calendar is based on years, months, days,
    // then the referenced values SHOULD use one of the
    // following ISO8601-based lexical representations:

    //  YYYY
    //  ±XYYYY (where X stands for extra year digits)
    //  YYYY-MM
    //  YYYY-MM-DD
    //  YYYY-MM-DDTHH:MM:SS[.F]Z where Z is either “Z”
    //      or a time scale offset + -HH:MM
    //      ex: "2018-01-01T00:12:20Z"

    // If calendar dates with reduced precision are
    // used in a lexical representation (e.g. "2016"),
    // then a client SHOULD interpret those dates in
    // that reduced precision.

    // Remove any commonly found words from the origin timestamp
    vector<string> subStrs = { "hours", "hour", "minutes", "minute", 
                        "seconds", "second", "since", "  " };

    string cleanTimeOrigin = timeOrigin;

    // If base time, use an arbitrary base time string
    if(timeOrigin.find("base_time") != string::npos) {
        cleanTimeOrigin = "2020-01-01T12:00:00Z";
    }
    else {
        for(unsigned int i = 0; i < subStrs.size(); i++)
            focovjson::removeSubstring(cleanTimeOrigin, subStrs[i]);
    }

    return cleanTimeOrigin;
}

FoDapCovJsonTransform::FoDapCovJsonTransform(libdap::DDS *dds) : _dds(dds)
{
    if (!_dds) throw BESInternalError("File out COVJSON, null DDS passed to constructor", __FILE__, __LINE__);
}

FoDapCovJsonTransform::FoDapCovJsonTransform(libdap::DMR *dmr) : _dmr(dmr)
{
    if (!_dmr) throw BESInternalError("File out COVJSON, null DMR passed to constructor", __FILE__, __LINE__);
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

void FoDapCovJsonTransform::transform_dap4(ostream &ostrm, bool sendData, bool testOverride)
{
    transform(&ostrm, _dmr, "", sendData, testOverride);
}

void FoDapCovJsonTransform::transform(ostream *strm, libdap::Constructor *cnstrctr, string indent, bool sendData)
{
    vector<libdap::BaseType *> leaves;
    vector<libdap::BaseType *> nodes;
    // Sort the variables into two sets
    libdap::DDS::Vars_iter vi = cnstrctr->var_begin();
    libdap::DDS::Vars_iter ve = cnstrctr->var_end();

//cerr<<"coming to the loop before " <<endl;

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
        //*strm << "Processing LEAF: " <<  v->name() << endl;

        transform(strm, v, indent + _indent_increment, sendData);
    }

    // Get this node's child nodes
    for(vector<libdap::BaseType *>::size_type n = 0; n < nodes.size(); n++) {
        libdap::BaseType *v = nodes[n];
        BESDEBUG(FoDapCovJsonTransform_debug_key, "Processing NODE: " << v->name() << endl);

        // FOR TESTING AND DEBUGGING PURPOSES
        //*strm << "Processing NODE: " <<  v->name() << endl;

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
//cerr<< "Can convert to CovJSON "<<endl;
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
    *strm << child_indent1 << "\"domainType\": \"" + domainType + "\"," << endl;

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

    // Obtain bound values if having them
    // First handle the t-axis because of GLADS
    std::vector<std::string> t_bnd_val;
    if(axisVar_t_bnd_val.size() >0) {
        t_bnd_val.resize(axisVar_t_bnd_val.size());
        for (unsigned i = 0; i < axisVar_t_bnd_val.size();i++) {
           t_bnd_val[i] = cf_time_to_greg((long long)(axisVar_t_bnd_val[i]));
#if 0
//cerr<<"time bound value is "<<t_bnd_val[i] <<endl;
#endif
        }
    }

    // bound for  x-axis
    std::vector<std::string> x_bnd_val;
    if(axisVar_x_bnd_val.empty() == false) {
        x_bnd_val.resize(axisVar_x_bnd_val.size());
        for (unsigned i = 0; i < axisVar_x_bnd_val.size();i++) {
           ostringstream temp_strm;
           temp_strm<<axisVar_x_bnd_val[i];
           x_bnd_val[i] = temp_strm.str();
#if 0
//cerr<<"X bound value is "<<x_bnd_val[i] <<endl;
#endif
        }
    }

    // bound for  y-axis
    std::vector<std::string> y_bnd_val;
    if(axisVar_y_bnd_val.empty() == false) {
        y_bnd_val.resize(axisVar_y_bnd_val.size());
        for (unsigned i = 0; i < axisVar_y_bnd_val.size();i++) {
           ostringstream temp_strm;
           temp_strm<<axisVar_y_bnd_val[i];
           y_bnd_val[i] = temp_strm.str();
#if 0
//cerr<<"Y bound value is "<<y_bnd_val[i] <<endl;
#endif
        }
    }

    // bound for  z-axis
    std::vector<std::string> z_bnd_val;
    if(axisVar_z_bnd_val.empty() == false) {
        z_bnd_val.resize(axisVar_z_bnd_val.size());
        for (unsigned i = 0; i < axisVar_z_bnd_val.size();i++) {
           ostringstream temp_strm;
           temp_strm<<axisVar_z_bnd_val[i];
           z_bnd_val[i] = temp_strm.str();
#if 0
//cerr<<"Z bound value is "<<z_bnd_val[i] <<endl;
#endif
        }
    }  
    
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
                    print_bound(strm, x_bnd_val,child_indent2,false);
                }
                else if((i == 1) && (axes[j]->name.compare("y") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                    print_bound(strm, y_bnd_val,child_indent2,false);
                }
                else if((i == 2) && (axes[j]->name.compare("z") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                    print_bound(strm, z_bnd_val,child_indent2,false);
                }
                else if((i == 3) && (axes[j]->name.compare("t") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                    print_bound(strm, t_bnd_val,child_indent2,true);
                }
            }
            // If just x, y, and t exist (x, y, t)
            else if(xExists && yExists && !zExists && tExists) {
                if((i == 0) && (axes[j]->name.compare("x") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                    print_bound(strm, x_bnd_val,child_indent2,false);
                }
                else if((i == 1) && (axes[j]->name.compare("y") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                    print_bound(strm, y_bnd_val,child_indent2,false);
                }
                else if((i == 2) && (axes[j]->name.compare("t") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                    print_bound(strm, t_bnd_val,child_indent2,true);
                }
            }
            // If just x and y exist (x, y)
            else if(xExists && yExists && !zExists && !tExists) {
                if((i == 0) && (axes[j]->name.compare("x") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                    print_bound(strm, x_bnd_val,child_indent2,false);
                }
                else if((i == 1) && (axes[j]->name.compare("y") == 0)) {
                    *strm << child_indent1 << "\"" << axes[j]->name << "\": {" << endl;
                    *strm << child_indent2 << axes[j]->values << endl;
                    print_bound(strm, y_bnd_val,child_indent2,false);
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
    string child_indent3 = child_indent2 + _indent_increment;
    string child_indent4 = child_indent3 + _indent_increment;
    string child_indent5 = child_indent4 + _indent_increment;
    string coordVars;

    BESDEBUG(FoDapCovJsonTransform_debug_key, "Printing REFERENCES" << endl);

    if(xExists) {
        coordVars += "\"x\"";
    }

    if(yExists) {
        if(!coordVars.empty()) {
            coordVars += ", ";
        }
        coordVars += "\"y\"";
    }

    if(zExists && !is_simple_cf_geographic && (dsg_type == UNSUPPORTED_DSG)) {
        if(!coordVars.empty()) {
            coordVars += ", ";
        }
        coordVars += "\"z\"";
    }

    *strm << indent << "\"referencing\": [{" << endl;

    // See https://covjson.org/spec/#temporal-reference-systems
    if(tExists) {
        *strm << child_indent1 << "\"coordinates\": [\"t\"]," << endl;
        *strm << child_indent1 << "\"system\": {" << endl;
        *strm << child_indent2 << "\"type\": \"TemporalRS\"," << endl;
        *strm << child_indent2 << "\"calendar\": \"Gregorian\"" << endl;
        *strm << child_indent1 << "}" << endl;
        *strm << indent << "}," << endl;
        *strm << indent << "{" << endl;
    }

    // See https://covjson.org/spec/#geospatial-coordinate-reference-systems
    *strm << child_indent1 << "\"coordinates\": [" << coordVars << "]," << endl;
    *strm << child_indent1 << "\"system\": {" << endl;
    *strm << child_indent2 << "\"type\": \"" + coordRefType + "\"," << endl;

    // Most of the datasets I've seen do not contain a link to a coordinate
    // reference system, so I've set some defaults here - CRH 1/2020
    if(coordRefType.compare("ProjectedCRS") == 0) {
        // Projected Coordinate Reference System (north/east): http://www.opengis.net/def/crs/EPSG/0/27700
        *strm << child_indent2 << "\"id\": \"http://www.opengis.net/def/crs/EPSG/0/27700\"" << endl;
    }
    else {
        if(xExists && yExists && zExists) {
            // 3-Dimensional Geographic Coordinate Reference System (lat/lon/height): http://www.opengis.net/def/crs/EPSG/0/4979
            if(!is_simple_cf_geographic &&(dsg_type == UNSUPPORTED_DSG))
               *strm << child_indent2 << "\"id\": \"http://www.opengis.net/def/crs/EPSG/0/4979\"" << endl;
        }
        else {
            // 2-Dimensional Geographic Coordinate Reference System (lat/lon): http://www.opengis.net/def/crs/OGC/1.3/CRS84
            if(!is_simple_cf_geographic && (dsg_type==UNSUPPORTED_DSG) && (!is_geo_dap2_grid))
               *strm << child_indent2 << "\"id\": \"http://www.opengis.net/def/crs/OGC/1.3/CRS84\"" << endl;
        }
    }

    *strm << child_indent1 << "}" << endl;

    if (zExists && (is_simple_cf_geographic || dsg_type != UNSUPPORTED_DSG)) {
        *strm << indent << "}," << endl;
        *strm << indent << "{" << endl;
        *strm << child_indent1 << "\"coordinates\": [" << "\"z\"]," << endl;
        *strm << child_indent1 << "\"system\": {" << endl;
        *strm << child_indent2 << "\"type\": \"" <<"VerticalCRS" << "\"," << endl;
        *strm << child_indent2 << "\"cs\": {" << endl;
        *strm << child_indent3 << "\"csAxes\": [{" << endl;
        *strm << child_indent4 << "\"name\": {" << endl;
        if (axis_z_standardName =="") 
            *strm << child_indent5 << "\"en\": \"" << axisVar_z.name << "\"" << endl;
        else 
            *strm << child_indent5 << "\"en\": \"" << axis_z_standardName << "\"" << endl;
        *strm << child_indent4 << "}," << endl;
        if (axis_z_direction !="") 
            *strm << child_indent4 << "\"direction\": \"" << axis_z_direction << "\","<<endl;
        if (axis_z_units !="") {
            *strm << child_indent4 << "\"units\": {" << endl;
            *strm << child_indent5 << "\"symbol\": \"" << axis_z_units << "\"" << endl;
            *strm << child_indent4 << "}" << endl;
        }
        *strm << child_indent3 <<"}]"<<endl;
        *strm << child_indent2 <<"}"<<endl;
        *strm << child_indent1 <<"}"<<endl;
        *strm << indent <<"}]"<<endl;
    }
    else 
        *strm << indent << "}]" << endl;
    
}

void FoDapCovJsonTransform::printParameters(ostream *strm, string indent)
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
            *strm << child_indent3 << "\"id\": \"http://vocab.nerc.ac.uk/standard_name/" << parameters[i]->standardName << "/\"," << endl;
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
        if(!axisNames.empty()) {
            axisNames += ", ";
        }
        axisNames += "\"z\"";
    }

    if(yExists) {
        if(!axisNames.empty()) {
            axisNames += ", ";
        }
        axisNames += "\"y\"";
    }

    if(xExists) {
        if(!axisNames.empty()) {
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

        // For discrete examples axisNames and shape may not need to be printed.
        // For point, no axis and shape,
        // For pointseries, axisNames: t
        // For vertical Profile, axisNames: z
        // For parameters that don't have shape, don't print shape.
        if (dsg_type !=SPOINT) {
            if (dsg_type == POINTS) 
                axisNames = "\"t\"";
            else if (dsg_type == PROFILE)
                axisNames = "\"z\"";
            *strm << child_indent2 << "\"axisNames\": [" << axisNames << "]," << endl;
            if (parameters[i]->shape!="")
                *strm << child_indent2 << parameters[i]->shape << endl;
        }
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

    // We need to support DAP2 grid. If a DDS contains DAP2 grids, since only DAP2 grids can map to the coverage Grid object and 
    // other objects are most likely not objects that can be mapped to the coverage, 
    // the other objects need to be ignored; otherwise, wrong information will be generated.  
    
    vector<string> dap2_grid_map_names;
    for (const auto &var:dds->variables()) {
        if(var->send_p()) {
            libdap::Type type = var->type();
            if (type == libdap::dods_grid_c) {
                is_dap2_grid = true;
                auto vgrid = dynamic_cast<libdap::Grid*>(var);
                for (libdap::Grid::Map_iter i = vgrid->map_begin(); i != vgrid->map_end();  ++i)  {
                    dap2_grid_map_names.emplace_back((*i)->name());
#if 0
cout <<"grid map name: "<<(*i)->name() <<endl;
#endif
                }
                break;
            }
        }
    }
 
    if (is_dap2_grid)
        is_geo_dap2_grid = check_geo_dap2_grid(dds,dap2_grid_map_names);

    // Sort the variables into two sets
    vector<libdap::BaseType *> leaves;
    vector<libdap::BaseType *> nodes;


    libdap::DDS::Vars_iter vi = dds->var_begin();
    libdap::DDS::Vars_iter ve = dds->var_end();


    // If we find this file contains DAP2 grids, ignore other variables.
    if (is_dap2_grid == true) {

        for(; vi != ve; vi++) {
            if((*vi)->send_p()) {
                libdap::BaseType *v = *vi;
                libdap::Type type = v->type();
                if (type == libdap::dods_grid_c) {
                    if(v->is_constructor_type() || (v->is_vector_type() && v->var()->is_constructor_type())) {
                        nodes.push_back(v);
                    }
                    else {
                        leaves.push_back(v);
                    }
                }
            }
        }
    }
    else {
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

    // Check if CF discrete Sample Geometries
    bool is_simple_discrete = check_update_simple_dsg(dds);
   
    // Check simple grid
    if (is_simple_discrete == false && FoCovJsonRequestHandler::get_simple_geo()) 
        check_update_simple_geo(dds, sendData);

    }
    // Read through the source DDS leaves and nodes, extract all axes and
    // parameter data, and store that data as Axis and Parameters
    transformNodeWorker(strm, leaves, nodes, indent + _indent_increment + _indent_increment, sendData);

    // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed
    RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog + "ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
    BESUtil::conditional_timeout_cancel();

    // Print the Coverage data to stream as CoverageJSON
    printCoverageJSON(strm, indent, testOverride);
}

void FoDapCovJsonTransform::transform(ostream *strm, libdap::DMR *dmr, const string& indent, bool sendData, bool testOverride)
{

    // Now we only consider the variables under the groups.
    libdap::D4Group *root_grp = dmr->root();

    // Sort the variables into two sets
    vector<libdap::BaseType *> leaves;
    vector<libdap::BaseType *> nodes;

    for (auto i = root_grp->var_begin(), e = root_grp->var_end(); i != e; ++i) {

        if ((*i)->send_p()) {
            libdap::BaseType *v = *i;
            if(v->is_constructor_type() || (v->is_vector_type() && v->var()->is_constructor_type())) 
                nodes.push_back(v);
            else 
                leaves.push_back(v);
        }
    }

#if 0
    // Check if CF discrete Sample Geometries
    bool is_simple_discrete = check_update_simple_dsg(dds);
   
    // Check simple grid
    if (is_simple_discrete == false && FoCovJsonRequestHandler::get_simple_geo()) 
        check_update_simple_geo(dds, sendData);

    }
#endif

    // We currently only consider simple grids.
    if (FoCovJsonRequestHandler::get_simple_geo()) 
        check_update_simple_geo_dap4(root_grp);


    // Read through the source DDS leaves and nodes, extract all axes and
    // parameter data, and store that data as Axis and Parameters
    transformNodeWorker(strm, leaves, nodes, indent + _indent_increment + _indent_increment, sendData);

    // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed
    RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog + "ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
    BESUtil::conditional_timeout_cancel();

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
    case libdap::dods_int8_c:
    case libdap::dods_uint8_c:
    case libdap::dods_int64_c:
    case libdap::dods_uint64_c:
 
        transformAtomic(strm, bt, indent, sendData);
        break;

    case libdap::dods_structure_c:
        transform(strm, (libdap::Structure *) bt, indent, sendData);
        break;

    case libdap::dods_grid_c:
        is_dap2_grid = true;
        transform(strm, (libdap::Grid *) bt, indent, sendData);
        break;

    case libdap::dods_sequence_c:
        transform(strm, (libdap::Sequence *) bt, indent, sendData);
        break;

    case libdap::dods_array_c:
        transform(strm, (libdap::Array *) bt, indent, sendData);
        break;

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

#if 0
void FoDapCovJsonTransform::transformAtomic(libdap::BaseType *b, const string& indent, bool sendData)
{
    string childindent = indent + _indent_increment;
    struct Axis *newAxis = new Axis;
cerr<<"b name is "<<b->name() <<endl;

    // Why assigning the name as "test" and why assigning the string type value? KY 2022-01-18
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
#endif

void FoDapCovJsonTransform::transformAtomic(ostream *strm, libdap::BaseType *b, const string& indent, bool sendData) {

    string childindent = indent + _indent_increment;
    bool axisRetrieved = false;
    bool parameterRetrieved = false;
 
    getAttributes(strm, b->get_attr_table(), b->name(), &axisRetrieved, &parameterRetrieved);

    if((axisRetrieved == true) && (parameterRetrieved == false)) {
        struct Axis *currAxis;
        currAxis = axes[axisCount - 1];

        if (sendData) {
            currAxis->values += "\"values\": [";

            bool is_cf_time_axis = false;
            if ((is_simple_cf_geographic || dsg_type != UNSUPPORTED_DSG) && currAxis->name.compare("t") == 0)
                is_cf_time_axis = true;

            if (b->type() == libdap::dods_str_c || b->type() == libdap::dods_url_c) {
                // Strings need to be escaped to be included in a CovJSON object.
                const auto s = dynamic_cast<libdap::Str *>(b);
                string val = s->value();
                currAxis->values +="\"" + focovjson::escape_for_covjson(val) + "\"";

            }
            else if (is_cf_time_axis) {

                // We need to convert CF time to greg time.
                string axis_t_value;
                std::ostringstream tmp_stream; 
                long long tmp_value = 0;
                switch (b->type()) {
                case libdap::dods_byte_c: {
                    const auto tmp_byte = dynamic_cast<libdap::Byte *>(b);
                    tmp_value = (long long) (tmp_byte->value());
                    break;
                }

                case libdap::dods_uint16_c:  {
                    const auto tmp_uint16 = dynamic_cast<libdap::UInt16 *>(b);
                    tmp_value = (long long) (tmp_uint16->value());
                    break;
                }

                case libdap::dods_int16_c: {
                    const auto tmp_int16 = dynamic_cast<libdap::Int16 *>(b);
                    tmp_value = (long long) (tmp_int16->value());
                    break;
                }

                case libdap::dods_uint32_c: {
                    const auto tmp_uint32 = dynamic_cast<libdap::UInt32 *>(b);
                    tmp_value = (long long) (tmp_uint32->value());
                    break;
                }

                case libdap::dods_int32_c: {
                    const auto tmp_int32 = dynamic_cast<libdap::Int32 *>(b);
                    tmp_value = (long long) (tmp_int32->value());
                    break;
                }

                case libdap::dods_float32_c: {
                    // In theory, it may cause overflow. In reality, the time in seconds will never be that large.
                    const auto tmp_float32 = dynamic_cast<libdap::Float32 *>(b);
                    tmp_value = (long long) (tmp_float32->value());
                    break;
                }

                case libdap::dods_float64_c: {
                    // In theory, it may cause overflow. In reality, the time in seconds will never be that large.
                    const auto tmp_float64 = dynamic_cast<libdap::Float64 *>(b);
                    tmp_value = (long long) (tmp_float64->value());
                    break;
                }

                default:
                    throw BESInternalError("Attempt to extract CF time information from an invalid source", __FILE__, __LINE__);
                }

                axis_t_value = cf_time_to_greg(tmp_value);
   
                currAxis->values += "\"" + focovjson::escape_for_covjson(axis_t_value) + "\"";
            }
            else {
                ostringstream otemp;
                b->print_val(otemp, "", false);
                currAxis->values += otemp.str();
            }
            currAxis->values += "]";
#if 0
//cerr<<"currAxis value at covjsonSimpleTypeArray is "<<currAxis->values <<endl;
#endif
        }
        else {
                currAxis->values += "\"values\": []";
        }
    }

    // If we are dealing with a Parameter
    else if(axisRetrieved == false && parameterRetrieved == true) {
        struct Parameter *currParameter;
        currParameter = parameters[parameterCount - 1];
        if (sendData) {
            currParameter->values += "\"values\": [";
            if (b->type() == libdap::dods_str_c || b->type() == libdap::dods_url_c) {
                // Strings need to be escaped to be included in a CovJSON object.
                const auto s = dynamic_cast<libdap::Str *>(b);
                string val = s->value();
                currParameter->values +="\"" + focovjson::escape_for_covjson(val) + "\"";

            }
            else {
                ostringstream otemp;
                b->print_val(otemp, "", false);
                currParameter->values += otemp.str();
            }
            currParameter->values += "]";

        }
        else {
            currParameter->values += "\"values\": []";
        }
    }

}

void FoDapCovJsonTransform::transform(ostream *strm, libdap::Array *a, string indent, bool sendData)
{
    BESDEBUG(FoDapCovJsonTransform_debug_key,
        "FoCovJsonTransform::transform() - Processing Array. " << " a->type(): " << a->type() << " a->var()->type(): " << a->var()->type() << endl);
#if 0
//cerr<<"transform a name is "<<a->name() <<endl;
#endif
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

bool FoDapCovJsonTransform::check_update_simple_dsg(libdap::DDS *dds) {

    bool ret_value = false;
    DSGType sDC_candidate = UNSUPPORTED_DSG;

    libdap::AttrTable &globals = dds->get_attr_table();
    auto i = globals.attr_begin();
    auto e = globals.attr_end();
    for (; i != e; i++) {
        if (globals.get_attr_type(i) == libdap::Attr_container) {
            string attr_name = globals.get_name(i);
            if (BESUtil::endsWith(attr_name, "_GLOBAL")) {
                libdap::AttrTable *container = globals.get_attr_table(i);
                auto ci = container->attr_begin();
                auto ce = container->attr_end();
                bool find_featureType = false;
                for (; ci != ce; ci++) {
                    if (container->get_attr_type(ci) == libdap::Attr_string) {
                        string c_attr_name = container->get_name(ci);
                        if (c_attr_name == "featureType") {
                            string attr_val = container->get_attr(ci,0);
                            if (attr_val == "point") 
                                sDC_candidate = SPOINT;
                            else if (attr_val == "timeSeries")
                                sDC_candidate = POINTS;
                            else if (attr_val == "profile")
                                sDC_candidate = PROFILE;
                            find_featureType = true;
                            break;
                        } 
                    }
                }
                if (find_featureType)
                    break;
                
            }
        }
        else if (globals.get_attr_type(i) == libdap::Attr_string) {
            string attr_name = globals.get_name(i);
            // If following CF conventions
            if (attr_name == "featureType") {
                string attr_val = globals.get_attr(i,0);
                if (attr_val == "point") 
                    sDC_candidate = SPOINT;
                else if (attr_val == "timeSeries")
                    sDC_candidate = POINTS;
                else if (attr_val == "profile")
                    sDC_candidate = PROFILE;
                break;
            } 
        }
    }

#if 0
if (cf_point)
    cerr<<"point "<<endl;
else if(cf_timeSeries)
    cerr<<"timeSeries "<<endl;
else if(cf_profile)
    cerr<<"Profile "<<endl;
#endif


    if (sDC_candidate != UNSUPPORTED_DSG) {

        libdap::DDS::Vars_iter vi = dds->var_begin();
        libdap::DDS::Vars_iter ve = dds->var_end();
        for(; vi != ve; vi++) {
            if ((*vi)->send_p()) {
                libdap::BaseType *v = *vi;
                if (is_supported_vars_by_type(v) == false)
                    continue;
                libdap::AttrTable &attrs = v->get_attr_table();
                unsigned int num_attrs = attrs.get_size();
#if 0
//cerr<<"var name is "<<v->name() <<endl;
//cerr<<"num_attrs is "<<num_attrs <<endl;
#endif
                if (num_attrs) {
                    libdap::AttrTable::Attr_iter di = attrs.attr_begin();
                    libdap::AttrTable::Attr_iter de = attrs.attr_end();
                    for (; di != de; di++) {
                        string cf_attr_name = "axis";
                        string attr_name = attrs.get_name(di);
                        unsigned int num_vals = attrs.get_attr_num(di);
                        if (num_vals == 1 && cf_attr_name == attr_name) {
                            string val = attrs.get_attr(di,0);
                            set_axisVar(v,val);
#if 0
//cerr<<"val is "<<val << endl;
#endif
                        }
                    }
                }
            }
        }

        if (is_simple_dsg(sDC_candidate)) {
#if 0
//cerr<<"simple dsg "<<endl;
#endif
            ret_value = obtain_valid_dsg_par_vars(dds);
        }
    }

#if 0
cerr<<"axisVar_x.name is "<<axisVar_x.name <<endl;
cerr<<"axisVar_x.dim_name is "<<axisVar_x.dim_name <<endl;
cerr<<"axisVar_x.dim_size is "<<axisVar_x.dim_size <<endl;
cerr<<"axisVar_x.bound_name is "<<axisVar_x.bound_name <<endl;

cerr<<"axisVar_y.name is "<<axisVar_y.name <<endl;
cerr<<"axisVar_y.dim_name is "<<axisVar_y.dim_name <<endl;
cerr<<"axisVar_y.dim_size is "<<axisVar_y.dim_size <<endl;
cerr<<"axisVar_y.bound_name is "<<axisVar_y.bound_name <<endl;

cerr<<"axisVar_z.name is "<<axisVar_z.name <<endl;
cerr<<"axisVar_z.dim_name is "<<axisVar_z.dim_name <<endl;
cerr<<"axisVar_z.dim_size is "<<axisVar_z.dim_size <<endl;
cerr<<"axisVar_z.bound_name is "<<axisVar_z.bound_name <<endl;

cerr<<"axisVar_t.name is "<<axisVar_t.name <<endl;
cerr<<"axisVar_t.dim_name is "<<axisVar_t.dim_name <<endl;
cerr<<"axisVar_t.dim_size is "<<axisVar_t.dim_size <<endl;
cerr<<"axisVar_t.bound_name is "<<axisVar_t.bound_name <<endl;
#endif

    return ret_value;
}

bool FoDapCovJsonTransform::is_simple_dsg(DSGType dsg) {

    if (dsg == SPOINT) 
       dsg_type = is_single_point();
    else if (dsg == POINTS)
       dsg_type = is_point_series(); 
    else if (dsg == PROFILE)
       dsg_type = is_single_profile();
    return (dsg_type != UNSUPPORTED_DSG);
}

DSGType FoDapCovJsonTransform::is_single_point() const {

    DSGType ret_dsg = SPOINT;
    if ((axisVar_z.name !="" && axisVar_z.dim_size >1) ||
        (axisVar_t.name !="" && axisVar_t.dim_size >1)) 
        ret_dsg = UNSUPPORTED_DSG;
    return ret_dsg;

}

DSGType FoDapCovJsonTransform::is_point_series() const {

    DSGType ret_dsg = POINTS;
    if ((axisVar_z.name !="" && axisVar_z.dim_size >1) ||
        (axisVar_t.name =="" )) 
        ret_dsg = UNSUPPORTED_DSG;
    return ret_dsg;

}


DSGType FoDapCovJsonTransform::is_single_profile() const {

    DSGType ret_dsg = PROFILE;
    if ((axisVar_t.name !="" && axisVar_t.dim_size >1) ||
        (axisVar_z.name =="" )) 
        ret_dsg = UNSUPPORTED_DSG;
    return ret_dsg;

}

bool FoDapCovJsonTransform::is_simple_dsg_common() const {

    bool ret_value = true;
    if (axisVar_x.name =="" || axisVar_y.name == "") 
        ret_value = false;
    else if (axisVar_x.dim_size >1 || axisVar_y.dim_size >1)
        ret_value = false;
     
    return ret_value;

}


// Only support array and atomic scalar datatypes.
bool FoDapCovJsonTransform::is_supported_vars_by_type(libdap::BaseType*v) const{

    bool ret_value = false;
    libdap::Type type = v->type();
    if (type  == libdap::dods_array_c) 
        type = v->var()->type();
    switch (type) {
    case libdap::dods_byte_c:
    case libdap::dods_int16_c:
    case libdap::dods_uint16_c:
    case libdap::dods_int32_c:
    case libdap::dods_uint32_c:
    case libdap::dods_float32_c:
    case libdap::dods_float64_c:
    case libdap::dods_str_c:
    case libdap::dods_url_c:
        ret_value = true;
        break;
    default:
        break;
    }
    return ret_value;
}

void FoDapCovJsonTransform::handle_axisVars_array(libdap::BaseType*v,axisVar & this_axisVar) {

    auto d_a = dynamic_cast<libdap::Array *>(v);
    if (d_a !=nullptr) {

        int d_ndims = d_a->dimensions();

        if (d_ndims == 1) {

            libdap::Array::Dim_iter di = d_a->dim_begin();
            this_axisVar.dim_size = d_a->dimension_size(di, true);
            this_axisVar.name = d_a->name();
            this_axisVar.dim_name = d_a->dimension_name(di);

        }
    }

}
void FoDapCovJsonTransform::set_axisVar(libdap::BaseType*v,const string &val) {

    // Here we assume the libdap datatype is either array of atomic datatypes
    // or just atomic datatypes. is_supported_vars_by_type() should be called 
    // beforehand. 
    if (val == "X") {
        if (v->type() == libdap::dods_array_c) 
            handle_axisVars_array(v,axisVar_x);
        else {
            axisVar_x.name = v->name();
            axisVar_x.dim_size = 0;
        }
    }
    else if (val == "Y") {
        if (v->type() == libdap::dods_array_c) 
            handle_axisVars_array(v,axisVar_y);
        else {
            axisVar_y.name = v->name();
            axisVar_y.dim_size = 0;
        }
    }
    else if (val == "Z") {
        if (v->type() == libdap::dods_array_c) 
            handle_axisVars_array(v,axisVar_z);
        else {
            axisVar_z.name = v->name();
            axisVar_z.dim_size = 0;
        }
    }
    else if (val == "T") {
        if (v->type() == libdap::dods_array_c) 
            handle_axisVars_array(v,axisVar_t);
        else {
            axisVar_t.name = v->name();
            axisVar_t.dim_size = 0;
        }
    }
}

bool FoDapCovJsonTransform::is_cf_grid_mapping_var(libdap::BaseType *v) const {

    bool ret_value = false;
    libdap::AttrTable attr_table = v->get_attr_table();

    if (attr_table.get_size() != 0) {

        libdap::AttrTable::Attr_iter begin = attr_table.attr_begin();
        libdap::AttrTable::Attr_iter end = attr_table.attr_end();

        for (libdap::AttrTable::Attr_iter at_iter = begin; at_iter != end; at_iter++) {

            if (attr_table.get_attr_type(at_iter) == libdap::Attr_string && 
                attr_table.get_name(at_iter) == "grid_mapping_name") {
                ret_value = true;
                break;
            }
        }
    }

    return ret_value;
}

bool FoDapCovJsonTransform::is_fake_coor_vars(libdap::Array *d_a) const{

    bool ret_value = false;

    if (d_a->dimensions() == 1) {
        libdap::Array::Dim_iter i = d_a->dim_begin();
        string dim_name = d_a->dimension_name(i);
        if (dim_name == d_a->name()) {
            libdap::AttrTable attr_table = d_a->get_attr_table();
            if (attr_table.get_size() == 0)
                ret_value = true;
        }
    }
    return ret_value;

}

bool FoDapCovJsonTransform::is_valid_single_point_par_var(libdap::BaseType *v) const {

    bool ret_value = true;
    if (v->name() == axisVar_x.name || v->name() == axisVar_y.name ||
        v->name() == axisVar_z.name || v->name() == axisVar_t.name ||
        is_cf_grid_mapping_var(v))
        ret_value = false;

    return ret_value;
}

bool FoDapCovJsonTransform::is_valid_array_dsg_par_var(libdap::Array *d_a) const {

    bool ret_value = false;

    if (d_a->dimensions() == 1) {

        libdap::Array::Dim_iter i = d_a->dim_begin();
        int dim_size = d_a->dimension_size(i);
        string dim_name = d_a->dimension_name(i);
        bool fake_coor = is_fake_coor_vars(d_a);
        bool real_coor = false;
        if (d_a->name() == axisVar_t.name || d_a->name() == axisVar_z.name || d_a->name() == axisVar_x.name || d_a->name() == axisVar_y.name)
            real_coor = true;
        if (real_coor == false && fake_coor == false) {
            if ((dsg_type == SPOINT) 
                ||(dsg_type == POINTS && dim_name == axisVar_t.dim_name && dim_size == axisVar_t.dim_size) 
                ||(dsg_type == PROFILE && dim_name == axisVar_z.dim_name && dim_size == axisVar_z.dim_size)) 
                ret_value = true;
        }
    }
    return ret_value;
}

bool FoDapCovJsonTransform::is_valid_dsg_par_var(libdap::BaseType *v) {

    bool ret_value = true;
    if (v->type() != libdap::dods_array_c) { // Scalar
        // Parameter variables for timeseries and profile should have dimensions
        if (dsg_type != SPOINT) 
            ret_value = false;
        else  
            ret_value = is_valid_single_point_par_var(v);
    }
    else {// Array
        // Single point parameter vars can be 1 element array. 
        auto d_a = dynamic_cast<libdap::Array *>(v);
        ret_value = is_valid_array_dsg_par_var(d_a);
    }

    return ret_value;
}

bool FoDapCovJsonTransform::obtain_valid_dsg_par_vars(libdap::DDS *dds) {

    libdap::DDS::Vars_iter vi = dds->var_begin();
    libdap::DDS::Vars_iter ve = dds->var_end();
    for(; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            libdap::BaseType *v = *vi;
            if (is_supported_vars_by_type(v) == false)
                continue;
            else if (is_valid_dsg_par_var(v) == true) 
                par_vars.push_back(v->name());
        }
    }
    return (par_vars.empty()==false);
}



void FoDapCovJsonTransform::check_update_simple_geo(libdap::DDS *dds,bool sendData) {

    libdap::DDS::Vars_iter vi = dds->var_begin();
    libdap::DDS::Vars_iter ve = dds->var_end();
 
    // First search CF units from 1-D array. 
    bool has_axis_var_x = false;
    short axis_var_x_count = 0;
    bool has_axis_var_y = false;
    short axis_var_y_count = 0;
    bool has_axis_var_z = false;
    short axis_var_z_count = 0;
    bool has_axis_var_t = false;
    short axis_var_t_count = 0;

    string units_name ="units";
    for (; vi != ve; vi++) {
#if 0
//cerr<<"coming to the loop  " <<endl;
#endif 
        if((*vi)->send_p()) {
            libdap::BaseType *v = *vi;
            libdap::Type type = v->type();

            // Check if this qualifies a simple geographic grid coverage
            if(type == libdap::dods_array_c) {
                libdap::Array * d_a = dynamic_cast<libdap::Array *>(v);
                int d_ndims = d_a->dimensions();
#if 0
//cerr<<"d_ndims is "<< d_ndims <<endl;
#endif
                if(d_ndims == 1) {
#if 0
//cerr<<"d_a name is "<<d_a->name() <<endl;
#endif
                    libdap::AttrTable &attrs = d_a->get_attr_table();
                    unsigned int num_attrs = attrs.get_size();
                    if (num_attrs) {
                        libdap::AttrTable::Attr_iter i = attrs.attr_begin();
                        libdap::AttrTable::Attr_iter e = attrs.attr_end();
                        for (; i != e; i++) {
                            string attr_name = attrs.get_name(i);
#if 0
//cerr<<"attr_name is "<<attr_name <<endl;
#endif
                            unsigned int num_vals = attrs.get_attr_num(i);
                            if (num_vals == 1) {
                                // Check if the attr_name is units. 
                                bool is_attr_units = false;
                                if((attr_name.size() == units_name.size()) 
                                   && (attr_name.compare(units_name) == 0))
                                    is_attr_units = true;
                                if(is_attr_units == false)
                                    if(attr_name.size() == (units_name.size()+1) &&
                                       attr_name[units_name.size()] == '\0' &&
                                       attr_name.compare(0,units_name.size(),units_name) ==0)
                                        is_attr_units = true;

                                if (is_attr_units) {
                                    string val = attrs.get_attr(i,0);
                                    vector<string> unit_candidates;

                                    // Here we need to check if there are 2 latitudes or longitudes. 
                                    // If we find this issue, we should mark it. The coverage json won't support this case.
                                    // longitude axis x
                                    unit_candidates.push_back("degrees_east");
                                    has_axis_var_x = check_add_axis(d_a,val,unit_candidates,axisVar_x,false);
                                    if (true == has_axis_var_x) {
                                        axis_var_x_count++;
                                        if (axis_var_x_count ==2)
                                            break;
                                    }
                                    unit_candidates.clear();
 
                                    // latitude axis y
                                    unit_candidates.push_back("degrees_north");
                                    has_axis_var_y = check_add_axis(d_a,val,unit_candidates,axisVar_y,false);
                                    if (true == has_axis_var_y) {
                                        axis_var_y_count++;
                                        if (axis_var_y_count == 2)
                                            break;
                                    }
                                    unit_candidates.clear();

                                    // height/pressure
                                    unit_candidates.push_back("hpa");
                                    unit_candidates.push_back("hPa");
                                    unit_candidates.push_back("meter");
                                    unit_candidates.push_back("m");
                                    unit_candidates.push_back("km");
                                    has_axis_var_z = check_add_axis(d_a,val,unit_candidates,axisVar_z,false);
                                    if (true == has_axis_var_z) {
                                        axis_var_z_count++;
                                        if (axis_var_z_count == 2)
                                            break;
                                    }
                                    unit_candidates.clear();
#if 0
for(int i = 0; i <unit_candidates.size(); i++)
    cerr<<"unit_candidates[i] is "<<unit_candidates[i] <<endl;
#endif

                                    // time: CF units only
                                    unit_candidates.push_back("seconds since ");
                                    unit_candidates.push_back("minutes since ");
                                    unit_candidates.push_back("hours since ");
                                    unit_candidates.push_back("days since ");
#if 0
for(int i = 0; i <unit_candidates.size(); i++)
cerr<<"unit_candidates[i] again is "<<unit_candidates[i] <<endl;
#endif

                                    has_axis_var_t = check_add_axis(d_a,val,unit_candidates,axisVar_t,true);
                                    if (true == has_axis_var_t) {
                                        axis_var_t_count++;
                                        if (axis_var_t_count == 2)
                                            break;
                                    }
                                    unit_candidates.clear();

                                }

                            }
                        }
                    }
                }
            }
        }
    }
#if 0
cerr<<"axis_var_x_count is "<< axis_var_x_count <<endl;
cerr<<"axis_var_y_count is "<< axis_var_y_count <<endl;
cerr<<"axis_var_z_count is "<< axis_var_z_count <<endl;
cerr<<"axis_var_t_count is "<< axis_var_t_count <<endl;
#endif
    bool is_simple_geo_candidate = true;
    if(axis_var_x_count !=1 || axis_var_y_count !=1) 
        is_simple_geo_candidate = false;
    // Single coverage for the time being
    // make z axis and t axis be empty if multiple z or t.
    if(axis_var_z_count > 1) {
        axisVar_z.name="";
        axisVar_z.dim_name = "";
        axisVar_z.bound_name = "";
    }
    if(axis_var_t_count > 1) {
        axisVar_t.name="";
        axisVar_t.dim_name = "";
        axisVar_t.bound_name = "";
    }
    if(is_simple_geo_candidate == true) {

        // Check bound variables
        // Check if any 1-D variable has the "bounds" attribute;  
        // we need to remember the attribute value and match the variable that
        // holds the bound values later. KY 2022-1-21
        map<string, string> vname_bname;
        
        check_bounds(dds,vname_bname);

        map<string, string>::iterator it;
#if 0
for(it = vname_bname.begin(); it != vname_bname.end(); it++) {
cerr<<it->first <<endl;
cerr<<it->second <<endl;
}
#endif

        for(it = vname_bname.begin(); it != vname_bname.end(); it++) {
//            cerr<<it->first <<endl;
//            cerr<<it->second <<endl;
            if(axisVar_x.name == it->first)
                axisVar_x.bound_name = it->second;
            else if(axisVar_y.name == it->first)
                axisVar_y.bound_name = it->second;
            else if(axisVar_z.name == it->first)
                axisVar_z.bound_name = it->second;
            else if(axisVar_t.name == it->first)
                axisVar_t.bound_name = it->second;
        }
#if 0
cerr<<"axisVar_x.name is "<<axisVar_x.name <<endl;
cerr<<"axisVar_x.dim_name is "<<axisVar_x.dim_name <<endl;
cerr<<"axisVar_x.dim_size is "<<axisVar_x.dim_size <<endl;
cerr<<"axisVar_x.bound_name is "<<axisVar_x.bound_name <<endl;

cerr<<"axisVar_y.name is "<<axisVar_y.name <<endl;
cerr<<"axisVar_y.dim_name is "<<axisVar_y.dim_name <<endl;
cerr<<"axisVar_y.dim_size is "<<axisVar_y.dim_size <<endl;
cerr<<"axisVar_y.bound_name is "<<axisVar_y.bound_name <<endl;

cerr<<"axisVar_z.name is "<<axisVar_z.name <<endl;
cerr<<"axisVar_z.dim_name is "<<axisVar_z.dim_name <<endl;
cerr<<"axisVar_z.dim_size is "<<axisVar_z.dim_size <<endl;
cerr<<"axisVar_z.bound_name is "<<axisVar_z.bound_name <<endl;

cerr<<"axisVar_t.name is "<<axisVar_t.name <<endl;
cerr<<"axisVar_t.dim_name is "<<axisVar_t.dim_name <<endl;
cerr<<"axisVar_t.dim_size is "<<axisVar_t.dim_size <<endl;
cerr<<"axisVar_t.bound_name is "<<axisVar_t.bound_name <<endl;
#endif


        is_simple_cf_geographic = obtain_valid_vars(dds,axis_var_z_count,axis_var_t_count);

        if(true == is_simple_cf_geographic) {
#if 0
//cerr<<"this is a simple CF geographic grid we can handle" <<endl;
#endif
            // We should handle the bound value
            // ignore 1-D bound dimension variable, 
            // Retrieve the values of the 2-D bound variable, 
            // Will save as the bound value in the coverage            

            string x_bnd_dim_name;
            string y_bnd_dim_name;
            string z_bnd_dim_name;
            string t_bnd_dim_name;

            obtain_bound_values(dds,axisVar_x,axisVar_x_bnd_val, x_bnd_dim_name,sendData);
            obtain_bound_values(dds,axisVar_y,axisVar_y_bnd_val, y_bnd_dim_name,sendData);
            obtain_bound_values(dds,axisVar_z,axisVar_z_bnd_val, z_bnd_dim_name,sendData);
            obtain_bound_values(dds,axisVar_t,axisVar_t_bnd_val, t_bnd_dim_name,sendData);
            
            if(x_bnd_dim_name!="")
                bnd_dim_names.push_back(x_bnd_dim_name);
            else if(y_bnd_dim_name!="")
                bnd_dim_names.push_back(y_bnd_dim_name);
            else if(z_bnd_dim_name!="")
                bnd_dim_names.push_back(z_bnd_dim_name);
            else if(t_bnd_dim_name!="")
                bnd_dim_names.push_back(t_bnd_dim_name);
        }

    }
}

void FoDapCovJsonTransform::check_update_simple_geo_dap4(libdap::D4Group *d4g) {

 
    // First search CF units from 1-D array. 
    bool has_axis_var_x = false;
    short axis_var_x_count = 0;
    bool has_axis_var_y = false;
    short axis_var_y_count = 0;
    bool has_axis_var_z = false;
    short axis_var_z_count = 0;
    bool has_axis_var_t = false;
    short axis_var_t_count = 0;

    string units_name ="units";
    for (auto vi = d4g->var_begin(), ve = d4g->var_end(); vi != ve; ++vi) {
#if 0
//cerr<<"coming to the loop  " <<endl;
#endif 
        if((*vi)->send_p()) {

            libdap::BaseType *v = *vi;
            libdap::Type type = v->type();

            // Check if this qualifies a simple geographic grid coverage
            // TODO: here we still use dap2's way to find dimensions. This will be changed later.
            if(type == libdap::dods_array_c) {
                auto d_a = dynamic_cast<libdap::Array *>(v);
                int d_ndims = d_a->dimensions();
#if 0
//cerr<<"d_ndims is "<< d_ndims <<endl;
#endif
                if (d_ndims == 1) {
#if 0
//cerr<<"d_a name is "<<d_a->name() <<endl;
#endif
                    libdap::D4Attributes *d4_attrs = d_a->attributes();
                    for (libdap::D4Attributes::D4AttributesIter ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end();
                         ii != ee; ++ii) {

                        string attr_name = (*ii)->name();
                        unsigned int num_vals = (*ii)->num_values();

                        if (num_vals == 1) {

                            // Check if the attr_name is units. 
                            bool is_attr_units = false;
                            if ((attr_name.size() == units_name.size())
                               && (attr_name.compare(units_name) == 0))
                                is_attr_units = true;
                            if (is_attr_units == false &&
                                (attr_name.size() == (units_name.size()+1) &&
                                   attr_name[units_name.size()] == '\0' &&
                                   attr_name.compare(0,units_name.size(),units_name) ==0))
                                    is_attr_units = true;

                            if (is_attr_units) {
                                string val = (*ii)->value(0);
                                vector<string> unit_candidates;

                                // Here we need to check if there are 2 latitudes or longitudes. 
                                // If we find this issue, we should mark it. The coverage json won't support this case.
                                // longitude axis x
                                unit_candidates.emplace_back("degrees_east");
                                has_axis_var_x = check_add_axis(d_a,val,unit_candidates,axisVar_x,false);
                                if (true == has_axis_var_x) {
                                    axis_var_x_count++;
                                    if (axis_var_x_count == 2)
                                        break;
                                }
                                unit_candidates.clear();

                                // latitude axis y
                                unit_candidates.emplace_back("degrees_north");
                                has_axis_var_y = check_add_axis(d_a,val,unit_candidates,axisVar_y,false);
                                if (true == has_axis_var_y) {
                                    axis_var_y_count++;
                                    if (axis_var_y_count == 2)
                                        break;
                                }
                                unit_candidates.clear();

                                // height/pressure
                                unit_candidates.emplace_back("hpa");
                                unit_candidates.emplace_back("hPa");
                                unit_candidates.emplace_back("meter");
                                unit_candidates.emplace_back("m");
                                unit_candidates.emplace_back("km");
                                has_axis_var_z = check_add_axis(d_a,val,unit_candidates,axisVar_z,false);
                                if (true == has_axis_var_z) {
                                    axis_var_z_count++;
                                    if (axis_var_z_count == 2)
                                        break;
                                }
                                unit_candidates.clear();
#if 0
for(int i = 0; i <unit_candidates.size(); i++)
    cerr<<"unit_candidates[i] is "<<unit_candidates[i] <<endl;
#endif

                                // time: CF units only
                                unit_candidates.emplace_back("seconds since ");
                                unit_candidates.emplace_back("minutes since ");
                                unit_candidates.emplace_back("hours since ");
                                unit_candidates.emplace_back("days since ");
#if 0
for(int i = 0; i <unit_candidates.size(); i++)
cerr<<"unit_candidates[i] again is "<<unit_candidates[i] <<endl;
#endif

                                has_axis_var_t = check_add_axis(d_a,val,unit_candidates,axisVar_t,true);
                                if (true == has_axis_var_t) {
                                    axis_var_t_count++;
                                    if (axis_var_t_count == 2)
                                        break;
                                }
                                unit_candidates.clear();
                            }
                        }
                    }
                }
            }
        } 
    }

#if 0
cerr<<"axis_var_x_count is "<< axis_var_x_count <<endl;
cerr<<"axis_var_y_count is "<< axis_var_y_count <<endl;
cerr<<"axis_var_z_count is "<< axis_var_z_count <<endl;
cerr<<"axis_var_t_count is "<< axis_var_t_count <<endl;
#endif

    bool is_simple_geo_candidate = true;
    if(axis_var_x_count != 1 || axis_var_y_count != 1) 
        is_simple_geo_candidate = false;

    // Single coverage for the time being
    // make z axis and t axis be empty if multiple z or t.
    if(axis_var_z_count > 1) {
        axisVar_z.name="";
        axisVar_z.dim_name = "";
        axisVar_z.bound_name = "";
    }
    if(axis_var_t_count > 1) {
        axisVar_t.name="";
        axisVar_t.dim_name = "";
        axisVar_t.bound_name = "";
    }
    if (is_simple_geo_candidate == true) {

#if 0
cerr<<"axisVar_x.name is "<<axisVar_x.name <<endl;
cerr<<"axisVar_x.dim_name is "<<axisVar_x.dim_name <<endl;
cerr<<"axisVar_x.dim_size is "<<axisVar_x.dim_size <<endl;
cerr<<"axisVar_x.bound_name is "<<axisVar_x.bound_name <<endl;

cerr<<"axisVar_y.name is "<<axisVar_y.name <<endl;
cerr<<"axisVar_y.dim_name is "<<axisVar_y.dim_name <<endl;
cerr<<"axisVar_y.dim_size is "<<axisVar_y.dim_size <<endl;
cerr<<"axisVar_y.bound_name is "<<axisVar_y.bound_name <<endl;

cerr<<"axisVar_z.name is "<<axisVar_z.name <<endl;
cerr<<"axisVar_z.dim_name is "<<axisVar_z.dim_name <<endl;
cerr<<"axisVar_z.dim_size is "<<axisVar_z.dim_size <<endl;
cerr<<"axisVar_z.bound_name is "<<axisVar_z.bound_name <<endl;

cerr<<"axisVar_t.name is "<<axisVar_t.name <<endl;
cerr<<"axisVar_t.dim_name is "<<axisVar_t.dim_name <<endl;
cerr<<"axisVar_t.dim_size is "<<axisVar_t.dim_size <<endl;
cerr<<"axisVar_t.bound_name is "<<axisVar_t.bound_name <<endl;
#endif

        is_simple_cf_geographic = obtain_valid_vars_dap4(d4g,axis_var_z_count,axis_var_t_count);
    }
}


bool FoDapCovJsonTransform::check_add_axis(libdap::Array *d_a,const string & unit_value, const vector<string> & CF_unit_values, axisVar & this_axisVar, bool is_t_axis) {

    bool ret_value = false;
    for (unsigned i = 0; i < CF_unit_values.size(); i++) {
#if 0
//cerr<<"CF_unit_values "<<CF_unit_values[i] << endl;
#endif
        bool is_cf_units = false;
        if(is_t_axis == false) {
            if((unit_value.size() == CF_unit_values[i].size() || unit_value.size() == (CF_unit_values[i].size() +1)) && unit_value.compare(0,CF_unit_values[i].size(),CF_unit_values[i])==0) 
                is_cf_units = true;    
        }
        else {
            if(unit_value.compare(0,CF_unit_values[i].size(),CF_unit_values[i])==0) 
                is_cf_units = true; 
        }
       
        if (is_cf_units) {
            libdap::Array::Dim_iter di = d_a->dim_begin();
            this_axisVar.dim_size = d_a->dimension_size(di, true);
            this_axisVar.name = d_a->name();
            this_axisVar.dim_name = d_a->dimension_name(di);
            this_axisVar.bound_name="";
            ret_value = true;
#if 0
cerr<<"axis size "<< this_axisVar.dim_size <<endl;
cerr<<"axis name "<< this_axisVar.name <<endl;
cerr<<"axis dim_name "<< this_axisVar.dim_name <<endl;
#endif
            break;
        }

    }
    return ret_value;

}


void FoDapCovJsonTransform::check_bounds(libdap::DDS *dds, map<string,string>& vname_bname) {

    string bound_name = "bounds";
    libdap::DDS::Vars_iter vi = dds->var_begin();
    libdap::DDS::Vars_iter ve = dds->var_end();
 
    for(; vi != ve; vi++) {
#if 0
//cerr<<"coming to the loop  " <<endl;
#endif
        if((*vi)->send_p()) {
            libdap::BaseType *v = *vi;
            libdap::Type type = v->type();

            // Check if this qualifies a simple geographic grid coverage
            if(type == libdap::dods_array_c) {
                libdap::Array * d_a = dynamic_cast<libdap::Array *>(v);
                int d_ndims = d_a->dimensions();
#if 0
//cerr<<"d_ndims is "<< d_ndims <<endl;
#endif
                if(d_ndims == 1) {
                    libdap::AttrTable &attrs = d_a->get_attr_table();
                    unsigned int num_attrs = attrs.get_size();
                    if (num_attrs) {
                        libdap::AttrTable::Attr_iter i = attrs.attr_begin();
                        libdap::AttrTable::Attr_iter e = attrs.attr_end();
                        for (; i != e; i++) {
                            string attr_name = attrs.get_name(i);
#if 0
//cerr<<"attr_name is "<<attr_name <<endl;
#endif
                            unsigned int num_vals = attrs.get_attr_num(i);
                            if (num_vals == 1) {
                                // Check if the attr_name is units. 
                                bool is_attr_bounds = false;
                                if((attr_name.size() == bound_name.size()) 
                                   && (attr_name.compare(bound_name) == 0))
                                    is_attr_bounds = true;
                                if(is_attr_bounds == false)
                                    if(attr_name.size() == (bound_name.size()+1) &&
                                       attr_name[bound_name.size()] == '\0' &&
                                       attr_name.compare(0,bound_name.size(),bound_name) ==0)
                                        is_attr_bounds = true;

                                if (is_attr_bounds) {
                                    string val = attrs.get_attr(i,0);
                                    vname_bname[d_a->name()] = val;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void FoDapCovJsonTransform::obtain_bound_values(libdap::DDS *dds, const axisVar & av, std::vector<float>& av_bnd_val, std::string& bnd_dim_name, bool sendData) {

//cerr<<"coming to the obtain_bound_values "<<endl;
    libdap::Array* d_a = obtain_bound_values_worker(dds, av.bound_name,bnd_dim_name); 
    if (d_a) {// float, now we just handle this way 
#if 0
//cerr<<"d_a->name in obtain_bound_values is "<<d_a->name() <<endl;
//cerr<<"in obtain_bound_values bnd_dim_name is "<<bnd_dim_name <<endl;
#endif
        if(d_a->var()->type_name() == "Float64") {
            if(sendData) {
                int num_lengths = d_a->length();
                vector<double>temp_val;
                temp_val.resize(num_lengths);
                d_a->value(temp_val.data());
                
                av_bnd_val.resize(num_lengths);
                for (unsigned i = 0; i <av_bnd_val.size();i++)
                    av_bnd_val[i] =(float)temp_val[i];
 
#if 0
for (int i = 0; i <av_bnd_val.size();i++)
cerr<<"av_bnd_val["<<i<<"] = "<<av_bnd_val[i] <<endl;
#endif
            }
        }
        else if(d_a->var()->type_name() == "Float32") {
            if(sendData) {
                int num_lengths = d_a->length();
                av_bnd_val.resize(num_lengths);
                d_a->value(av_bnd_val.data());
#if 0
for (int i = 0; i <av_bnd_val.size();i++)
cerr<<"av_bnd_val["<<i<<"] = "<<av_bnd_val[i] <<endl;
#endif
            }
        }

     
    }
}

void FoDapCovJsonTransform::obtain_bound_values(libdap::DDS *dds, const axisVar & av, std::vector<double>& av_bnd_val, std::string& bnd_dim_name, bool sendData) {

    libdap::Array* d_a = obtain_bound_values_worker(dds, av.bound_name,bnd_dim_name); 
    if(d_a) {
#if 0
cerr<<"d_a->name in obtain_bound_values is "<<d_a->name() <<endl;
cerr<<"in obtain_bound_values bnd_dim_name is "<<bnd_dim_name <<endl;
#endif
        if(d_a->var()->type_name() == "Float64") {
            if(sendData) {
                int num_lengths = d_a->length();
                av_bnd_val.resize(num_lengths);
                d_a->value(av_bnd_val.data());
#if 0
for (int i = 0; i <av_bnd_val.size();i++)
cerr<<"av_bnd_val["<<i<<"] = "<<av_bnd_val[i] <<endl;
#endif
            }
        }
        else if(d_a->var()->type_name() == "Float32") {
            if(sendData) {
                int num_lengths = d_a->length();
                vector<float>temp_val;
                temp_val.resize(num_lengths);
                d_a->value(temp_val.data());
                av_bnd_val.resize(num_lengths);
                for (unsigned i = 0; i <av_bnd_val.size();i++)
                    av_bnd_val[i] =(double)temp_val[i];
#if 0 
for (int i = 0; i <av_bnd_val.size();i++)
cerr<<"av_bnd_val["<<i<<"] = "<<av_bnd_val[i] <<endl;
#endif
            }
        }
   }
}

libdap::Array* FoDapCovJsonTransform::obtain_bound_values_worker(libdap::DDS *dds, const string& bnd_name, string & bnd_dim_name) {

    libdap::Array* d_a = nullptr;
    if(bnd_name!="") {
    
        libdap::DDS::Vars_iter vi = dds->var_begin();
        libdap::DDS::Vars_iter ve = dds->var_end();
     
        for(; vi != ve; vi++) {
#if 0
//    cerr<<"In worker: coming to the loop  " <<endl;
#endif
            if((*vi)->send_p()) {
                libdap::BaseType *v = *vi;
                libdap::Type type = v->type();
    
                // Check if this qualifies a simple geographic grid coverage
                if(type == libdap::dods_array_c) {
                    libdap::Array * td_a = dynamic_cast<libdap::Array *>(v);
                    int d_ndims = td_a->dimensions();
#if 0
//    cerr<<"In worker: d_ndims is "<< d_ndims <<endl;
#endif
                    if(d_ndims == 2) {
                        string tmp_bnd_dim_name;
                        int bound_dim_size = 0;
                        int dim_count = 0;
                        libdap::Array::Dim_iter di = td_a->dim_begin();
                        libdap::Array::Dim_iter de = td_a->dim_end();
                        for (; di != de; di++) {
                            if(dim_count == 1) {
                                bound_dim_size = td_a->dimension_size(di, true);
                                tmp_bnd_dim_name = td_a->dimension_name(di);
#if 0
//cerr<<"tmp_bnd_dim_name is "<<tmp_bnd_dim_name <<endl;
#endif
                            }
                            dim_count++;
                        }
                        
                        // For 1-D coordinate bound, the bound size should always be 2.
                        if((bound_dim_size == 2) && (td_a->name() == bnd_name)) {
                            d_a = td_a;
                            bnd_dim_name = tmp_bnd_dim_name;
#if 0
//cerr<<"bnd_dim_name is "<<bnd_dim_name <<endl;
#endif
                            break;
                        }
                    }
                }
            }
        }
        
    }
    return d_a;

}

bool FoDapCovJsonTransform::obtain_valid_vars(libdap::DDS *dds, short axis_var_z_count, short axis_var_t_count ) {

#if 0
//cerr<<"coming to obtain_valid_vars "<<endl;
#endif
    bool ret_value = true;
    std::vector<std::string> temp_x_y_vars;
    std::vector<std::string> temp_x_y_z_vars;
    std::vector<std::string> temp_x_y_t_vars;
    std::vector<std::string> temp_x_y_z_t_vars;

    libdap::DDS::Vars_iter vi = dds->var_begin();
    libdap::DDS::Vars_iter ve = dds->var_end();
    for(; vi != ve; vi++) {
        if((*vi)->send_p()) {
            libdap::BaseType *v = *vi;
            libdap::Type type = v->type();
            if(type == libdap::dods_array_c) {
                libdap::Array * d_a = dynamic_cast<libdap::Array *>(v);
                int d_ndims = d_a->dimensions();

                if(d_ndims >=2) {

                    short axis_x_count = 0;
                    short axis_y_count = 0;
                    short axis_z_count = 0;
                    short axis_t_count = 0;
                    bool  non_xyzt_dim = false;
                    bool  supported_var = true;

                    libdap::Array::Dim_iter di = d_a->dim_begin();
                    libdap::Array::Dim_iter de = d_a->dim_end();

                    for (; di != de; di++) {
                       // check x,y,z,t dimensions 
                       if((d_a->dimension_size(di,true) == axisVar_x.dim_size) && 
                           (d_a->dimension_name(di) == axisVar_x.dim_name))
                          axis_x_count++;
                       else if((d_a->dimension_size(di,true) == axisVar_y.dim_size) && 
                           (d_a->dimension_name(di) == axisVar_y.dim_name))
                          axis_y_count++;
                       else if((d_a->dimension_size(di,true) == axisVar_z.dim_size) && 
                           (d_a->dimension_name(di) == axisVar_z.dim_name))
                          axis_z_count++;
                       else if((d_a->dimension_size(di,true) == axisVar_t.dim_size) && 
                           (d_a->dimension_name(di) == axisVar_t.dim_name))
                          axis_t_count++;
                       else 
                          non_xyzt_dim = true;
                       
                       // Non-x,y,z,t dimension or duplicate x,y,z,t dimensions are not supported.
                       // Here for the "strict" case, I need to return false for the conversion to grid when
                       // a non-conform > 1D var appears except the "bound" variables.
                       if(non_xyzt_dim || axis_x_count >1 || axis_y_count >1 || axis_z_count >1 || axis_t_count >1) {
                          supported_var = false;
#if 0
cerr<<"Obtain: d_a->name() is "<<d_a->name() <<endl;
#endif
                          if (FoCovJsonRequestHandler::get_may_ignore_z_axis() == false) { 
                              if(d_a->name()!=axisVar_x.bound_name && d_a->name()!=axisVar_y.bound_name &&
                                 d_a->name()!=axisVar_z.bound_name && d_a->name()!=axisVar_t.bound_name)
                                 ret_value = false;
                          }
                          break;
                       }
                    }
                    
                    if(supported_var) {
                        // save the var names to the vars that hold (x,y),(x,y,z),(x,y,t),(x,y,z,t)
                        if(axis_x_count == 1 && axis_y_count == 1 && axis_z_count == 0 && axis_t_count == 0)
                            temp_x_y_vars.emplace_back(d_a->name());
                        else if(axis_x_count == 1 && axis_y_count == 1 && axis_z_count == 1 && axis_t_count == 0)
                            temp_x_y_z_vars.emplace_back(d_a->name());
                        else if(axis_x_count == 1 && axis_y_count == 1 && axis_z_count == 0 && axis_t_count == 1)
                            temp_x_y_t_vars.emplace_back(d_a->name());
                        else if(axis_x_count == 1 && axis_y_count == 1 && axis_z_count == 1 && axis_t_count == 1)
                            temp_x_y_z_t_vars.emplace_back(d_a->name());
                    }
                    else if(ret_value == false) 
                        break;
                }
            }
        }
    }
#if 0
//cerr<<"obtain: after loop "<<endl;
#endif

    if (ret_value == true) {
    if(FoCovJsonRequestHandler::get_may_ignore_z_axis()== true) { 

#if 0
cerr<<"coming to ignore mode "<<endl;
cerr<<"axis_var_z_count: "<<axis_var_z_count <<endl;
cerr<<"axis_var_t_count: "<<axis_var_t_count <<endl;
#endif

    // Select the common factor of (x,y),(x,y,z),(x,y,t),(x,y,z,t) among variables
    // If having vars that only holds x,y; these vars are only vars that will appear at the final coverage.
    if(axis_var_z_count <=1 && axis_var_t_count <=1) {

        for (unsigned i = 0; i <temp_x_y_vars.size(); i++)
            par_vars.emplace_back(temp_x_y_vars[i]);
        for (unsigned i = 0; i <temp_x_y_t_vars.size(); i++)
            par_vars.emplace_back(temp_x_y_t_vars[i]);
 
        if (temp_x_y_vars.empty())  {
            for (unsigned i = 0; i <temp_x_y_z_vars.size(); i++)
                par_vars.emplace_back(temp_x_y_z_vars[i]);
            for (unsigned i = 0; i <temp_x_y_z_t_vars.size(); i++)
                par_vars.emplace_back(temp_x_y_z_t_vars[i]);
            
        }
        else {
            // Ignore the (x,y,z) and (x,y,z,t) when (x,y) exists.
            // We also need to ignore the z-axis TODO,we may need to support multiple verical coordinates. !
            if (axis_var_z_count == 1) {
                axisVar_z.name="";
                axisVar_z.dim_name = "";
                axisVar_z.bound_name = "";
            }
        }
    }
    else if (axis_var_z_count >1 && axis_var_t_count <=1) {
        //Cover all variables that have (x,y) or (x,y,t) 
        for (unsigned i = 0; i <temp_x_y_vars.size(); i++)
            par_vars.emplace_back(temp_x_y_vars[i]);
        for (unsigned i = 0; i <temp_x_y_t_vars.size(); i++)
            par_vars.emplace_back(temp_x_y_t_vars[i]);
    }
    else if (axis_var_z_count <=1 && axis_var_t_count >1) {
        //Cover all variables that have (x,y) or (x,y,z) 
        for (unsigned i = 0; i <temp_x_y_vars.size(); i++)
            par_vars.emplace_back(temp_x_y_vars[i]);
        for (unsigned i = 0; i <temp_x_y_z_vars.size(); i++)
            par_vars.emplace_back(temp_x_y_z_vars[i]);
    }
    else {
        // Select the common factor of (x,y),(x,y,z),(x,y,t),(x,y,z,t) among variables
        // If having vars that only holds x,y; these vars are only vars that will appear at the final coverage.
        for (unsigned i = 0; i <temp_x_y_vars.size(); i++)
            par_vars.emplace_back(temp_x_y_vars[i]);
    }
    }
    else {
#if 0
cerr<<"coming to strict mode "<<endl;
#endif
        if(axis_var_z_count >1 || axis_var_t_count >1) 
            ret_value = false;
        else {
            //Cover all variables that have (x,y) or (x,y,z) or (x,y,t) or (x,y,z,t)
            for (unsigned i = 0; i <temp_x_y_vars.size(); i++)
                par_vars.emplace_back(temp_x_y_vars[i]);
            for (unsigned i = 0; i <temp_x_y_z_vars.size(); i++)
                par_vars.emplace_back(temp_x_y_z_vars[i]);
            for (unsigned i = 0; i <temp_x_y_t_vars.size(); i++)
                par_vars.emplace_back(temp_x_y_t_vars[i]);
            for (unsigned i = 0; i <temp_x_y_z_t_vars.size(); i++)
                par_vars.emplace_back(temp_x_y_z_t_vars[i]);
        }
    }

#if 0
cerr<<"Parameter Names: "<<endl;
for(unsigned i = 0; i <par_vars.size(); i++)
    cerr<<par_vars[i]<<endl;
#endif

    
    if(par_vars.size() == 0)
        ret_value = false;

    }
    return ret_value;

}

bool FoDapCovJsonTransform::obtain_valid_vars_dap4(libdap::D4Group *d4g, short axis_var_z_count, short axis_var_t_count ) {

#if 0
//cerr<<"coming to obtain_valid_vars "<<endl;
#endif
    bool ret_value = true;
    std::vector<std::string> temp_x_y_vars;
    std::vector<std::string> temp_x_y_z_vars;
    std::vector<std::string> temp_x_y_t_vars;
    std::vector<std::string> temp_x_y_z_t_vars;

    for (auto vi = d4g->var_begin(), ve = d4g->var_end(); vi != ve; ++vi) {

        if ((*vi)->send_p()) {

            libdap::BaseType *v = *vi;
            libdap::Type type = v->type();

            if (type == libdap::dods_array_c) {

                auto d_a = dynamic_cast<libdap::Array *>(v);
                int d_ndims = d_a->dimensions();

                if(d_ndims >=2) {

                    short axis_x_count = 0;
                    short axis_y_count = 0;
                    short axis_z_count = 0;
                    short axis_t_count = 0;
                    bool  non_xyzt_dim = false;
                    bool  supported_var = true;

                    libdap::Array::Dim_iter di = d_a->dim_begin();
                    libdap::Array::Dim_iter de = d_a->dim_end();

                    for (; di != de; di++) {
                       // check x,y,z,t dimensions 
                       if((d_a->dimension_size(di,true) == axisVar_x.dim_size) && 
                           (d_a->dimension_name(di) == axisVar_x.dim_name))
                          axis_x_count++;
                       else if((d_a->dimension_size(di,true) == axisVar_y.dim_size) && 
                           (d_a->dimension_name(di) == axisVar_y.dim_name))
                          axis_y_count++;
                       else if((d_a->dimension_size(di,true) == axisVar_z.dim_size) && 
                           (d_a->dimension_name(di) == axisVar_z.dim_name))
                          axis_z_count++;
                       else if((d_a->dimension_size(di,true) == axisVar_t.dim_size) && 
                           (d_a->dimension_name(di) == axisVar_t.dim_name))
                          axis_t_count++;
                       else 
                          non_xyzt_dim = true;
                       
                       // Non-x,y,z,t dimension or duplicate x,y,z,t dimensions are not supported.
                       // Here for the "strict" case, I need to return false for the conversion to grid when
                       // a non-conform > 1D var appears except the "bound" variables.
                       if(non_xyzt_dim || axis_x_count >1 || axis_y_count >1 || axis_z_count >1 || axis_t_count >1) {
                          supported_var = false;
#if 0
cerr<<"Obtain: d_a->name() is "<<d_a->name() <<endl;
#endif
                          if (FoCovJsonRequestHandler::get_may_ignore_z_axis() == false) { 
                              if(d_a->name()!=axisVar_x.bound_name && d_a->name()!=axisVar_y.bound_name &&
                                 d_a->name()!=axisVar_z.bound_name && d_a->name()!=axisVar_t.bound_name)
                                 ret_value = false;
                          }
                          break;
                       }
                    }
                    
                    if(supported_var) {
                        // save the var names to the vars that hold (x,y),(x,y,z),(x,y,t),(x,y,z,t)
                        if(axis_x_count == 1 && axis_y_count == 1 && axis_z_count == 0 && axis_t_count == 0)
                            temp_x_y_vars.emplace_back(d_a->name());
                        else if(axis_x_count == 1 && axis_y_count == 1 && axis_z_count == 1 && axis_t_count == 0)
                            temp_x_y_z_vars.emplace_back(d_a->name());
                        else if(axis_x_count == 1 && axis_y_count == 1 && axis_z_count == 0 && axis_t_count == 1)
                            temp_x_y_t_vars.emplace_back(d_a->name());
                        else if(axis_x_count == 1 && axis_y_count == 1 && axis_z_count == 1 && axis_t_count == 1)
                            temp_x_y_z_t_vars.emplace_back(d_a->name());
                    }
                    else if(ret_value == false) 
                        break;
                }
            }
        }
    }
#if 0
//cerr<<"obtain: after loop "<<endl;
#endif

    if (ret_value == true) {
    if(FoCovJsonRequestHandler::get_may_ignore_z_axis()== true) { 

#if 0
cerr<<"coming to ignore mode "<<endl;
cerr<<"axis_var_z_count: "<<axis_var_z_count <<endl;
cerr<<"axis_var_t_count: "<<axis_var_t_count <<endl;
#endif

    // Select the common factor of (x,y),(x,y,z),(x,y,t),(x,y,z,t) among variables
    // If having vars that only holds x,y; these vars are only vars that will be in the final output.
    if(axis_var_z_count <=1 && axis_var_t_count <=1) {

        for (const auto &txy_var:temp_x_y_vars)
            par_vars.emplace_back(txy_var);
        for (const auto &txyt_var:temp_x_y_t_vars)
            par_vars.emplace_back(txyt_var);
 
        if (temp_x_y_vars.empty())  {
            for (const auto &txyz_var:temp_x_y_z_vars)
                par_vars.emplace_back(txyz_var);
            for (const auto &txyzt_var:temp_x_y_z_t_vars)
                par_vars.emplace_back(txyzt_var);
            
        }
        else {
            // Ignore the (x,y,z) and (x,y,z,t) when (x,y) exists.
            // We also need to ignore the z-axis TODO,we may need to support multiple verical coordinates. !
            if (axis_var_z_count == 1) {
                axisVar_z.name="";
                axisVar_z.dim_name = "";
                axisVar_z.bound_name = "";
            }
        }
    }
    else if (axis_var_z_count >1 && axis_var_t_count <=1) {
        //Cover all variables that have (x,y) or (x,y,t) 
        for (const auto &txy_var:temp_x_y_vars)
            par_vars.emplace_back(txy_var);
        for (const auto &txyt_var:temp_x_y_t_vars)
            par_vars.emplace_back(txyt_var);
    }
    else if (axis_var_z_count <=1 && axis_var_t_count >1) {
        //Cover all variables that have (x,y) or (x,y,z) 
        for (const auto &txy_var:temp_x_y_vars)
            par_vars.emplace_back(txy_var);
        for (const auto &txyz_var:temp_x_y_z_vars)
            par_vars.emplace_back(txyz_var);
    }
    else {
        // Select the common factor of (x,y),(x,y,z),(x,y,t),(x,y,z,t) among variables
        // If having vars that only holds x,y; these vars are only vars that will appear at the final coverage.
        for (const auto &txy_var:temp_x_y_vars)
            par_vars.emplace_back(txy_var);
    }
    }
    else {
#if 0
cerr<<"coming to strict mode "<<endl;
#endif
        if(axis_var_z_count >1 || axis_var_t_count >1) 
            ret_value = false;
        else {
            //Cover all variables that have (x,y) or (x,y,z) or (x,y,t) or (x,y,z,t)
            for (const auto &txy_var:temp_x_y_vars)
                par_vars.emplace_back(txy_var);
            for (const auto &txyz_var:temp_x_y_z_vars)
                par_vars.emplace_back(txyz_var);
            for (const auto &txyt_var:temp_x_y_t_vars)
                par_vars.emplace_back(txyt_var);
            for (const auto &txyzt_var:temp_x_y_z_t_vars)
                par_vars.emplace_back(txyzt_var);
        }
    }

#if 0
cerr<<"Parameter Names: "<<endl;
for(unsigned i = 0; i <par_vars.size(); i++)
    cerr<<par_vars[i]<<endl;
#endif

    
    if (par_vars.empty() == true)
        ret_value = false;

    }
    return ret_value;

}


std::string FoDapCovJsonTransform::cf_time_to_greg(long long time_val) {

    tm ycf_1;

    // Here obtain the cf_time from the axis_t_units.
    string cf_time= axis_t_units ;

    // Check the time unit,day,hour, minute or second.
    short time_unit_length = -1;
    if(cf_time.compare(0,3,"day") == 0)
        time_unit_length = 0;
    else if(cf_time.compare(0,4,"hour") == 0)
        time_unit_length = 1;
    else if(cf_time.compare(0,6,"minute") == 0)
        time_unit_length = 2;
    else if(cf_time.compare(0,6,"second") == 0)
        time_unit_length = 3;

#if 0
//cerr<<"time_unit_length is "<<time_unit_length <<endl;        
#endif

    // Remove any commonly found words from the origin timestamp
    vector<string> subStrs = { "days", "day", "hours", "hour", "minutes", "minute",
                        "seconds", "second", "since", "  " };

    for(unsigned int i = 0; i < subStrs.size(); i++)
        focovjson::removeSubstring(cf_time, subStrs[i]);

#if 0
//cerr<<"cf_time stripped is "<<cf_time <<endl;
#endif

    // Separate the date from the hms.
    size_t cf_time_space_pos = cf_time.find(' ');
    string cf_date,cf_hms;

    if(cf_time_space_pos!=string::npos) { 
        cf_date= cf_time.substr(0,cf_time_space_pos);
        cf_hms = cf_time.substr(cf_time_space_pos+1);
    }
    // If without hours/minutes/seconds, we need to set them to 0.
    if(cf_hms==" " || cf_hms=="")
        cf_hms ="00:00:00";
   
#if 0
cerr<<"cf_date is "<<cf_date <<endl;
cerr<<"cf_hms is "<<cf_hms <<endl;
#endif

    // We need to obtain year,month,date,hour,minute and second 
    // of the time.

    string cf_y,cf_mo,cf_d;
    size_t cf_date_dash_pos = cf_date.find('-');
    if(cf_date_dash_pos !=string::npos) {
        string cf_md;
        cf_y = cf_date.substr(0,cf_date_dash_pos);
        cf_md = cf_date.substr(cf_date_dash_pos+1);
        size_t cf_md_dash_pos = cf_md.find("-");
        if(cf_md_dash_pos !=string::npos) {
            cf_mo = cf_md.substr(0,cf_md_dash_pos);
            cf_d = cf_md.substr(cf_md_dash_pos+1);
        }
    }

    string cf_h,cf_ms,cf_m,cf_s;
    size_t cf_hms_colon_pos = cf_hms.find(':');
    if(cf_hms_colon_pos !=string::npos) {
        cf_h = cf_hms.substr(0,cf_hms_colon_pos);
        cf_ms = cf_hms.substr(cf_hms_colon_pos+1);
        size_t cf_ms_colon_pos = cf_ms.find(":");
        if(cf_ms_colon_pos !=string::npos) {
            cf_m = cf_ms.substr(0,cf_ms_colon_pos);
            cf_s = cf_ms.substr(cf_ms_colon_pos+1);
        }
    }

    
#if 0
cerr<<"cf_y is "<<cf_y <<endl;
cerr<<"cf_mo is "<<cf_mo <<endl;
cerr<<"cf_d is "<<cf_d <<endl;

cerr<<"cf_h is "<<cf_h <<endl;
cerr<<"cf_m is "<<cf_m <<endl;
cerr<<"cf_s is "<<cf_s <<endl;
#endif

    // We need to convert the time from string to integer.
    int cf_y_i,cf_mo_i,cf_d_i,cf_h_i,cf_m_i,cf_s_i;
    cf_y_i = stoi(cf_y);
    cf_mo_i = stoi(cf_mo);
    cf_d_i = stoi(cf_d);
    cf_h_i = stoi(cf_h);
    cf_m_i = stoi(cf_m);
    cf_s_i = stoi(cf_s);

#if 0
cerr<<"cf_y_i " <<cf_y_i <<endl;
cerr<<"cf_mo_i " <<cf_mo_i <<endl;
cerr<<"cf_d_i " <<cf_d_i <<endl;
cerr<<"cf_h_i " <<cf_h_i <<endl;
cerr<<"cf_m_i " <<cf_m_i <<endl;
cerr<<"cf_s_i " <<cf_s_i <<endl;
#endif

    // Now we want to assign these time info to struct tm
    // Note: the mktime() and localtime() may only work for the date after 1970.
    // This should be sufficient for the data we serve now. 
    ycf_1.tm_hour = cf_h_i;   ycf_1.tm_min = cf_m_i; ycf_1.tm_sec = cf_s_i;
    ycf_1.tm_year = cf_y_i-1900; ycf_1.tm_mon = cf_mo_i; ycf_1.tm_mday = cf_d_i;
#if 0
    //time_t t_ycf_1 = mktime(&ycf_1);
#endif
    time_t t_ycf_1 = timegm(&ycf_1);

#if 0
cerr<<"t_ycf_1 is "<<t_ycf_1 <<endl;
cerr<<"time_val is "<<time_val <<endl;
#endif

  //time_val = 11046060;
  // Here is the value to calculate the new time. We need to convert them to seconds.
  //double val = 1.000000000001;
  time_t t_ycf_2 ;
  // Here we need to convert days, hours, minutes to seconds
  if(time_unit_length == 0)
        t_ycf_2 = t_ycf_1 + 86400*time_val;
  else if (time_unit_length == 1)
        t_ycf_2 = t_ycf_1 + 3600*time_val;
  else if (time_unit_length == 2)
        t_ycf_2 = t_ycf_1 + 60*time_val;
  else if (time_unit_length == 3)
        t_ycf_2 = t_ycf_1 + time_val;

#if 0
  //time_t t_ycf_2 = t_ycf_1 + 86340;
//cerr<<"t_ycf_2 is "<<t_ycf_2 <<endl;
#endif

  // jhrg 2/2/24 struct tm *t_new_ycf;
  struct tm temp_new_ycf{};
  // The use of localtime() is to calculate the time based on the CF time unit.
  // So the value actually represents the GMT time. 
  // Note: we didn't consider the use of local time in the CF. 
  // Our currently supported product uses GMT. Will consider the other cases later.
#if 0
  //t_new_ycf = localtime(&t_ycf_2);
  //t_new_ycf = gmtime(&t_ycf_2);
#endif
  auto t_new_ycf = gmtime_r(&t_ycf_2, &temp_new_ycf);

#if 0
cerr<< "t_new_ycf.tm_year is " <<t_new_ycf->tm_year <<endl;
cerr<< "t_new_ycf.tm_mon is " <<t_new_ycf->tm_mon <<endl;
cerr<< "t_new_ycf.tm_day is " <<t_new_ycf->tm_mday <<endl;
cerr<< "t_new_ycf.tm_hour is " <<t_new_ycf->tm_hour <<endl;
cerr<< "t_new_ycf.tm_min is " <<t_new_ycf->tm_min <<endl;
cerr<< "t_new_ycf.tm_sec is " <<t_new_ycf->tm_sec <<endl;
#endif 
    if(t_new_ycf->tm_mon == 0) {
        t_new_ycf->tm_year--;
        t_new_ycf->tm_mon = 12;
    }
    // Now, we need to change the time from int to string.
    string covjson_mon = (t_new_ycf->tm_mon<10)?
                         ("0"+to_string(t_new_ycf->tm_mon)):
                          to_string(t_new_ycf->tm_mon);
    string covjson_mday = (t_new_ycf->tm_mday<10)?
                         ("0"+to_string(t_new_ycf->tm_mday)):
                          to_string(t_new_ycf->tm_mday);

    string covjson_hour = (t_new_ycf->tm_hour<10)?
                         ("0"+to_string(t_new_ycf->tm_hour)):
                          to_string(t_new_ycf->tm_hour);

    string covjson_min = (t_new_ycf->tm_min<10)?
                         ("0"+to_string(t_new_ycf->tm_min)):
                          to_string(t_new_ycf->tm_min);

    string covjson_sec = (t_new_ycf->tm_sec<10)?
                         ("0"+to_string(t_new_ycf->tm_sec)):
                          to_string(t_new_ycf->tm_sec);


    // This is the final time. 
    string covjson_time = to_string(1900+t_new_ycf->tm_year)+"-"+
                          covjson_mon+"-"+covjson_mday+"T"+
                          covjson_hour+":"+covjson_min+":"+
                          covjson_sec+"Z";

    return covjson_time;
} 

void FoDapCovJsonTransform::print_bound(ostream *strm, const std::vector<std::string> & t_bnd_val, const std::string & indent, bool is_t_axis) const {

    if(axisVar_t.bound_name !="") {
        std::string print_values;
        if(t_bnd_val.size() >0) {
            print_values = "\"bounds\": [";
            for(unsigned i = 0; i <t_bnd_val.size(); i++) {   
                string tmpString = t_bnd_val[i];
                
                if (is_t_axis) {
                print_values +="\"";  
                print_values +=focovjson::escape_for_covjson(tmpString);
                print_values +="\"";  
                }
                else 
                    print_values +=tmpString;

                if(i !=(t_bnd_val.size()-1))
                    print_values +=", ";
                
 
            }
            print_values += "]";
        }
        else 
            print_values= "\"bounds\": []";
        *strm << indent << print_values <<endl;
   }

}

bool FoDapCovJsonTransform::check_geo_dap2_grid(libdap::DDS *dds, const vector<string> &dap2_grid_map_names) const {

    libdap::DDS::Vars_iter vi = dds->var_begin();
    libdap::DDS::Vars_iter ve = dds->var_end();

    bool has_lat = false;
    bool has_lon = false;
    bool ret_value = false;

    for (; vi != ve; vi++) {

        if ((*vi)->send_p()) {

            libdap::BaseType *v = *vi;
            libdap::Type type = v->type();

            if (type == libdap::dods_array_c) {
                
                for (const auto &map_name:dap2_grid_map_names) {
                    if (v->name() == map_name) {
                        auto d_a = dynamic_cast<libdap::Array *>(v);
                        short lat_or_lon = check_cf_unit_attr(d_a);
                        if (lat_or_lon == 1)
                            has_lat = true;
                        else if (lat_or_lon == 2)
                            has_lon = true;
                        break;
                    }
                }
            }

            if (has_lat && has_lon) {
                ret_value = true;
                break;
            }
        }
    }
    
    return ret_value;

}

short FoDapCovJsonTransform::check_cf_unit_attr(libdap::Array *d_a) const {

    short ret_value = 0;

    // The map must be 1-D array.
    if (d_a->dimensions() == 1) {

        libdap::AttrTable &attrs = d_a->get_attr_table();
        unsigned int num_attrs = attrs.get_size();

        if (num_attrs) {

            string lat_unit = "degrees_north";
            string lon_unit = "degrees_east";

            libdap::AttrTable::Attr_iter i = attrs.attr_begin();
            libdap::AttrTable::Attr_iter e = attrs.attr_end();
 
            for (; i != e; i++) {

                string attr_name = attrs.get_name(i);
#if 0
//cerr<<"attr_name is "<<attr_name <<endl;
#endif
                unsigned int num_vals = attrs.get_attr_num(i);

                if (num_vals == 1) {

                    string units_name ="units";
                    // Check if the attr_name is units. 
                    bool is_attr_units = false;
                    if ((attr_name.size() == units_name.size()) 
                         && (attr_name.compare(units_name) == 0))
                        is_attr_units = true;
                    if ((is_attr_units == false) && 
                        (attr_name.size() == (units_name.size()+1) &&
                         attr_name[units_name.size()] == '\0' &&
                         attr_name.compare(0,units_name.size(),units_name) ==0))
                        is_attr_units = true;

                    if (is_attr_units) {

                        string val = attrs.get_attr(i,0);
                        if (val.compare(0,lat_unit.size(),lat_unit) == 0) 
                            ret_value = 1;
                        else if (val.compare(0,lon_unit.size(),lon_unit) == 0) 
                            ret_value = 2;
                        if (ret_value !=0)
                            break;
                        
                    }
                }
            }
        }
    }
    return ret_value;

}
