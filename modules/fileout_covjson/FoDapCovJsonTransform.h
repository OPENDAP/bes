// -*- mode: c++; c-basic-offset:4 -*-
//
// FoDapJsonTransform.cc
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
    int domainType;
    bool xExists;
    bool yExists;
    bool zExists;
    bool tExists;
    bool isParam;
    bool isAxis;

    struct Axis {
        std::string name;
        std::string values;
    };

    struct Parameter {
        std::string name;
        std::string type;
        std::string unit;
        std::string longName;
        std::string shape;
        std::string values;
    };

    unsigned int axisCount;
    std::vector<Axis *> axes;
    unsigned int parameterCount;
    std::vector<Parameter *> parameters;
    unsigned int shapeValsCount;
    std::vector<int> shapeVals;

    enum domains { Grid = 0, VerticalProfile = 1, PointSeries = 2, Point = 3, CoverageCollection = 4 };

    bool canConvert();

    void getAttributes(std::ostream *strm, libdap::AttrTable &attr_table, std::string name,
        bool *axisRetrieved, bool *parameterRetrieved);

    void transform(std::ostream *strm, libdap::DDS *dds, std::string indent, bool sendData);
    void transform(std::ostream *strm, libdap::BaseType *bt, std::string indent, bool sendData);
    void transform(std::ostream *strm, libdap::Constructor *cnstrctr, std::string indent, bool sendData);
    void transform(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);

    void transformAtomic(std::ostream *strm, libdap::BaseType *bt, std::string indent, bool sendData);
    void transformNodeWorker(std::ostream *strm, vector<libdap::BaseType *> leaves, vector<libdap::BaseType *> nodes,
        string indent, bool sendData);

    void printCoverageHeaderWorker(std::ostream *strm, std::string indent, bool isCoverageCollection);
    void printAxesWorker(std::ostream *strm, std::string indent);
    void printReferenceWorker(std::ostream *strm, std::string indent);
    void printParametersWorker(std::ostream *strm, std::string indent);
    void printRangesWorker(std::ostream *strm, std::string indent);
    void printCoverageFooterWorker(std::ostream *strm, std::string indent);

    template<typename T>
    void covjsonSimpleTypeArray(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);
    void covjsonStringArray(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);

    template<typename T>
    unsigned int covjsonSimpleTypeArrayWorker(std::ostream *strm, T *values, unsigned int indx,
        std::vector<unsigned int> *shape, unsigned int currentDim, struct Axis *a);

    template<typename T>
    unsigned int covjsonSimpleTypeArrayWorker(std::ostream *strm, T *values, unsigned int indx,
        std::vector<unsigned int> *shape, unsigned int currentDim, struct Parameter *p);

public:
    FoDapCovJsonTransform(libdap::DDS *dds);

    virtual ~FoDapCovJsonTransform() { }

    virtual void transform(std::ostream &ostrm, bool sendData);

    virtual void dump(std::ostream &strm) const;

    virtual void printCoverageHeaderWorker(std::ostream &ostrm, std::string indent, bool isCoverageCollection) {
        printCoverageHeaderWorker(&ostrm, indent, isCoverageCollection);
    }

    virtual void printAxesWorker(std::ostream &ostrm, std::string indent) {
        printAxesWorker(&ostrm, indent);
    }

    virtual void printReferenceWorker(std::ostream &ostrm, std::string indent) {
        printReferenceWorker(&ostrm, indent);
    }

    virtual void printParametersWorker(std::ostream &ostrm, std::string indent) {
        printParametersWorker(&ostrm, indent);
    }

    virtual void printCoverageFooterWorker(std::ostream &ostrm, std::string indent) {
        printCoverageFooterWorker(&ostrm, indent);
    }
};

#endif /* FODAPCOVJSONTRANSFORM_H_ */
