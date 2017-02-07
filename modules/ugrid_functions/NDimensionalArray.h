// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003,2011,2012 OPeNDAP, Inc.
// Authors: Nathan Potter <ndp@opendap.org>
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

#ifndef NDIMENSIONALARRAY_H_
#define NDIMENSIONALARRAY_H_

#include "BESDebug.h"

#include "Array.h"

namespace libdap {

static string NDimensionalArray_debug_key = "ugrid";

/**
 *
 */
class NDimensionalArray {
private:

    NDimensionalArray();

    libdap::Type _dapType;

    std::vector<unsigned int> *_shape;
    unsigned int _currentLastDimensionSlabIndex;

    long _totalValueCount; // Number of elements
    unsigned int _sizeOfValue;
    void *_storage;

    void allocateStorage(long numValues, libdap::Type dapType);
    void confirmStorage();
    void confirmType(Type dapType);
    void confirmLastDimSize(unsigned int n);
    void setLastDimensionHyperSlab(std::vector<unsigned int> *location, void *values, unsigned int byteCount);

    string toString_worker(vector<unsigned int> *index);

public:

    NDimensionalArray(libdap::Array *arrayTemplate);
    NDimensionalArray(std::vector<unsigned int> *shape, libdap::Type dapType);

    virtual ~NDimensionalArray();

    dods_byte setValue(std::vector<unsigned int> *location, dods_byte value);
    dods_int16 setValue(std::vector<unsigned int> *location, dods_int16 value);
    dods_uint16 setValue(std::vector<unsigned int> *location, dods_uint16 value);
    dods_int32 setValue(std::vector<unsigned int> *location, dods_int32 value);
    dods_uint32 setValue(std::vector<unsigned int> *location, dods_uint32 value);
    dods_float32 setValue(std::vector<unsigned int> *location, dods_float32 value);
    dods_float64 setValue(std::vector<unsigned int> *location, dods_float64 value);

    static void retrieveLastDimHyperSlabLocationFromConstrainedArrray(libdap::Array *a, vector<unsigned int> *location);
    static long computeConstrainedShape(libdap::Array *a, vector<unsigned int> *shape);
    static long computeArraySizeFromShapeVector(vector<unsigned int> *shape);
    static long getStorageIndex(vector<unsigned int> *shape, vector<unsigned int> *location);

    long elementCount()
    {
        return _totalValueCount;
    }
    unsigned int sizeOfElement()
    {
        return _sizeOfValue;
    }

    void *relinquishStorage();

    void *getStorage()
    {
        return _storage;
    }
    void setAll(char val);

    long getLastDimensionElementCount();

    Type getTypeTemplate()
    {
        return _dapType;
    }

    void getLastDimensionHyperSlab(std::vector<unsigned int> *location, void **slab, unsigned int *elementCount);
    void getNextLastDimensionHyperSlab(void **slab);
    void resetSlabIndex()
    {
        _currentLastDimensionSlabIndex = 0;
    }
    unsigned int getCurrentLastDimensionHyperSlab()
    {
        return _currentLastDimensionSlabIndex;
    }
    void setCurrentLastDimensionHyperSlab(unsigned int newIndex)
    {
        _currentLastDimensionSlabIndex = newIndex;
    }

    void setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_byte *values, unsigned int numVal);
    void setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_int16 *values, unsigned int numVal);
    void setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_uint16 *values, unsigned int numVal);
    void setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_int32 *values, unsigned int numVal);
    void setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_uint32 *values, unsigned int numVal);
    void setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_float32 *values, unsigned int numVal);
    void setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_float64 *values, unsigned int numVal);

    libdap::Array *getArray(libdap::Array *templateArray);

    string toString();
    static string vectorToIndices(vector<unsigned int> *v);

};
//NdimensionalArray

} /* namespace libdap */
#endif /* NDIMENSIONALARRAY_H_ */
