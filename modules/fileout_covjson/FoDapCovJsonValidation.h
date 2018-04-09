// -*- mode: c++; c-basic-offset:4 -*-
//
// FoDapCovJsonValidation.h
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

#ifndef A_FoDapCovJsonValidation_h
#define A_FoDapCovJsonValidation_h 1

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

/*
* This class is for validating a dds to see if it is possible to convert the dataset to the coverageJson format
*/

class FoDapCovJsonValidation: public BESObj {
private:
    libdap::DDS *_dds;

    void checkAttribute(std::string name, std::string value);

    void validateDataset(libdap::DDS *dds);

    void validateDataset(libdap::AttrTable &attr_table);

    void transform_node_worker(std::vector<libdap::BaseType *> leaves, std::vector<libdap::BaseType *> nodes);

    void validateDataset(libdap::BaseType *bt);

    void validateDataset(libdap::Array *a);

    void validateDataset(libdap::Constructor *cnstrctr);

    void covjson_string_array(libdap::Array *a);

    void writeLeafMetadata(libdap::BaseType *bt);

    template<typename T>
    void covjson_simple_type_array(libdap::Array *a);

    template<typename T>
    unsigned int covjson_simple_type_array_worker(T *values, unsigned int indx,
        std::vector<unsigned int> *shape, unsigned int currentDim);



public:
    bool hasX;
    bool hasY;
    bool hasT;

    long int shapeX;
    long int shapeY;
    long int shapeT;
    long int shapeOrig;

    /*
    * if:
    * 0 grid
    * 1 vertical profile
    * 2 pointseries
    * 3 point
    */
    int domaintype;

    FoDapCovJsonValidation(libdap::DDS *dds);

    virtual ~FoDapCovJsonValidation(){ }

    virtual void validateDataset();

    virtual void dump(std::ostream &strm) const;

    bool canConvert();
};

#endif // A_FoDapCovJsonValidation_h
