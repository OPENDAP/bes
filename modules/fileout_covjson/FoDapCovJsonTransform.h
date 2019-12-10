// -*- mode: c++; c-basic-offset:4 -*-
//
// FoDapCovJsonTransform.h
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

#ifndef FODAPNJSONTRANSFORM_H_
#define FODAPNJSONTRANSFORM_H_

#include <string>
#include <vector>
#include <map>

#include <BESObj.h>

namespace libdap {
class BaseType;
class DDS;
class Array;
}

class BESDataHandlerInterface;

/**
 * Used to transform a DDS into a w10n CovJSON metadata or w10n CovJSON data document.
 * The output is written to a local file whose name is passed as a parameter to the
 * constructor.
 */
class FoDapCovJsonTransform: public BESObj {
private:
    libdap::DDS *_dds;
    std::string _returnAs;
    std::string _indent_increment;
    std::string atomicVals;
    std::string currDataType;
    int domainType;
    bool xExists;
    bool yExists;
    bool zExists;
    bool tExists;
    bool isParam;
    bool isAxis;
    bool canConvertToCovJson;

    struct Axis {
        std::string name;
        std::string values;
    };

    struct Parameter {
        std::string id;
        std::string name;
        std::string type;
        std::string dataType;
        std::string unit;
        std::string longName;
        std::string standardName;
        std::string shape;
        std::string values;
    };

    unsigned int axisCount;
    std::vector<Axis *> axes;
    unsigned int parameterCount;
    std::vector<Parameter *> parameters;
#if 0
    // not used
    unsigned int shapeValsCount;
#endif

    std::vector<int> shapeVals;

    enum domains { Grid = 0, VerticalProfile = 1, PointSeries = 2, Point = 3, CoverageCollection = 4 };

    bool canConvert();

    void getAttributes(std::ostream *strm, libdap::AttrTable &attr_table, std::string name,
        bool *axisRetrieved, bool *parameterRetrieved);

    void transform(std::ostream *strm, libdap::DDS *dds, std::string indent, bool sendData, bool testOverride);
    void transform(std::ostream *strm, libdap::BaseType *bt, std::string indent, bool sendData);
    void transform(std::ostream *strm, libdap::Constructor *cnstrctr, std::string indent, bool sendData);
    void transform(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);

    void transformAtomic(libdap::BaseType *bt, std::string indent, bool sendData);
    void transformNodeWorker(std::ostream *strm, vector<libdap::BaseType *> leaves, vector<libdap::BaseType *> nodes,
        string indent, bool sendData);

    void printCoverage(std::ostream *strm, std::string indent);
    void printDomain(std::ostream *strm, std::string indent);
    void printAxes(std::ostream *strm, std::string indent);
    void printReference(std::ostream *strm, std::string indent);
    void printParameters(std::ostream *strm, std::string indent);
    void printRanges(std::ostream *strm, std::string indent);
    void printCoverageJSON(std::ostream *strm, string indent, bool testOverride);

    template<typename T>
    void covjsonSimpleTypeArray(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);
    void covjsonStringArray(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);

    template<typename T>
    unsigned int covjsonSimpleTypeArrayWorker(std::ostream *strm, T *values, unsigned int indx,
        std::vector<unsigned int> *shape, unsigned int currentDim);

    // FOR TESTING PURPOSES ------------------------------------------------------------------------------------
    void addAxis(std::string name, std::string values) {
        struct Axis *newAxis = new Axis;

        newAxis->name = name;
        newAxis->values = values;

        this->axes.push_back(newAxis);
        this->axisCount++;
    }

    void addParameter(std::string id, std::string name, std::string type, std::string dataType, std::string unit,
            std::string longName, std::string standardName, std::string shape, std::string values) {
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

    void setAxesExistence(bool x, bool y, bool z, bool t) {
        this->xExists = x;
        this->yExists = y;
        this->zExists = z;
        this->tExists = t;
    }

    void setDomainType(int domainType) {
        this->domainType = domainType;
    }
    // ---------------------------------------------------------------------------------------------------------

public:
    // FOR TESTING PURPOSES ------------------------------------------------------------------------------------
    FoDapCovJsonTransform(libdap::DDS *dds);

    virtual ~FoDapCovJsonTransform()
    {
        for (std::vector<Axis*>::const_iterator i = axes.begin(); i != axes.end(); ++i)
            delete (*i);

        for (std::vector<Parameter *>::const_iterator i = parameters.begin(); i != parameters.end(); ++i)
            delete (*i);
    }

    virtual void transform(std::ostream &ostrm, bool sendData, bool testOverride);

    virtual void dump(std::ostream &strm) const;

    virtual void addTestAxis(std::string name, std::string values) {
        addAxis(name, values);
    }

    virtual void addTestParameter(std::string id, std::string name, std::string type, std::string dataType, std::string unit,
            std::string longName, std::string standardName, std::string shape, std::string values) {
        addParameter(id, name, type, dataType, unit, longName, standardName, shape, values);
    }

    virtual void setTestAxesExistence(bool x, bool y, bool z, bool t) {
        setAxesExistence(x, y, z, t);
    }

    virtual void setTestDomainType(int domainType) {
        setDomainType(domainType);
    }

    virtual void printCoverage(std::ostream &ostrm, std::string indent) {
        printCoverage(&ostrm, indent);
    }

    virtual void printDomain(std::ostream &ostrm, std::string indent) {
        printDomain(&ostrm, indent);
    }

    virtual void printAxes(std::ostream &ostrm, std::string indent) {
        printAxes(&ostrm, indent);
    }

    virtual void printReference(std::ostream &ostrm, std::string indent) {
        printReference(&ostrm, indent);
    }

    virtual void printParameters(std::ostream &ostrm, std::string indent) {
        printParameters(&ostrm, indent);
    }

    virtual void printRanges(std::ostream &ostrm, std::string indent) {
        printRanges(&ostrm, indent);
    }
    // ---------------------------------------------------------------------------------------------------------
};

#endif /* FODAPCOVJSONTRANSFORM_H_ */
