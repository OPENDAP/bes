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
#include "FoDapCovJsonValidation.h"

namespace libdap {
class BaseType;
class DDS;
class Array;
}

class BESDataHandlerInterface;

/**
 * Used to transform a DDS into a w10n CovJSON metadata or w10n CovJSON data document.
 * The output is written to a local file whose name is passed as a parameter
 * to the constructor.
 */
class FoDapCovJsonTransform: public BESObj {
private:
    libdap::DDS *_dds;
    std::string _returnAs;
    std::string _indent_increment;
    std::string currAxis;
    std::string currUnit;
    std::string currLongName;

    /*
    * if:
    * 0 NdArray
    * 1 TiledNdArray
    * 2 Coverage
    */
    std::string paramType;
    std::string paramName;
    std::string paramUnits;
    std::string paramLongName;
    std::string paramHistory;

    enum domains { Grid = 0, VerticalProfile = 1, PointSeries = 2, Point = 3 };

    void writeAxesMetadata(std::ostream *strm, libdap::BaseType *bt, std::string indent);
    void writeParameterMetadata(std::ostream *strm, libdap::BaseType *bt, std::string indent);
    void writeDatasetMetadata(std::ostream *strm, libdap::DDS *dds, std::string indent);

    void getAxisAttributes(std::ostream *strm, libdap::AttrTable &attr_table);
    void getParameterAttributes(std::ostream *strm, libdap::AttrTable &attr_table);

    void transformAtomic(std::ostream *strm, libdap::BaseType *bt, std::string indent, bool sendData);

    void transform(std::ostream *strm, libdap::DDS *dds, std::string indent, bool sendData, FoDapCovJsonValidation fv);
    void transform(std::ostream *strm, libdap::BaseType *bt, std::string indent, bool sendData, bool isAxes);
    void transform(std::ostream *strm, libdap::Constructor *cnstrctr, std::string indent, bool sendData);
    void transform(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData, bool isAxes);
    void transform(std::ostream *strm, libdap::AttrTable &attr_table, std::string indent);

    void transformAxesWorker(std::ostream *strm, std::vector<libdap::BaseType *> leaves, std::string indent, bool sendData);
    void transformReferenceWorker(std::ostream *strm, std::string indent, FoDapCovJsonValidation fv);
    void transformParametersWorker(std::ostream *strm, std::vector<libdap::BaseType *> nodes, std::string indent,
        bool sendData);
    void transformRangesWorker(std::ostream *strm, std::vector<libdap::BaseType *> leaves,
            std::string indent, bool sendData);

    template<typename T>
    void covjsonSimpleTypeArray(std::ostream *strm, libdap::Array *a, std::string indent,
        bool sendData, bool isAxes);
    void covjsonStringArray(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);

    template<typename T>
    unsigned int covjsonSimpleTypeArrayWorker(std::ostream *strm, T *values, unsigned int indx,
        std::vector<unsigned int> *shape, unsigned int currentDim);

public:
    FoDapCovJsonTransform(libdap::DDS *dds);

    virtual ~FoDapCovJsonTransform() { }

    virtual void transform(std::ostream &ostrm, bool sendData, FoDapCovJsonValidation fv);

    virtual void dump(std::ostream &strm) const;
};

#endif /* FODAPCOVJSONTRANSFORM_H_ */
