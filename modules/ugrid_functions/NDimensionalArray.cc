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

#include "config.h"

#include <sstream>      // std::stringstream
#include <string.h>

#include "NDimensionalArray.h"
#include "util.h"

#include "Byte.h"
#include "Int16.h"
#include "UInt16.h"
#include "Int32.h"
#include "UInt32.h"
#include "Float32.h"
#include "Float64.h"

#include "BESDebug.h"

#ifdef NDEBUG
#undef BESDEBUG
#define BESDEBUG( x, y )
#endif

namespace libdap {

string NDimensionalArray::vectorToIndices(vector<unsigned int> *v)
{
    stringstream s;
    for (unsigned int i = 0; i < v->size(); i++) {
        s << "[" << (*v)[i] << "]";
    }
    return s.str();
}

#if 0
NDimensionalArray::NDimensionalArray()
:_dapType(dods_null_c),_shape(0),_currentLastDimensionSlabIndex(0),_totalValueCount(0),_sizeOfValue(0),_storage(0) {

    string msg = "NDimArray::NDimArray() - INTERNAL_ERROR: This is the private constructor and should never be used";
    BESDEBUG(NDimensionalArray_debug_key, msg << endl);
    throw libdap::InternalErr(__FILE__, __LINE__, msg);
}
#endif

NDimensionalArray::NDimensionalArray(libdap::Array *a) :
    _dapType(dods_null_c), _shape(0), _currentLastDimensionSlabIndex(0), _totalValueCount(0), _sizeOfValue(0), _storage(
        0)
{
    BESDEBUG(NDimensionalArray_debug_key, "NDimensionalArray::NDimensionalArray(libdap::Array *) - BEGIN"<< endl);

    _shape = new vector<unsigned int>(a->dimensions(true), (unsigned int) 1);
    _totalValueCount = computeConstrainedShape(a, _shape);
    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::NDimensionalArray() - _shape" <<vectorToIndices(_shape) << endl);
    _dapType = a->var()->type();
    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::NDimensionalArray() - Total Value Count: " << _totalValueCount << " element(s) of type '"<< libdap::type_name(_dapType) << "'" << endl);

    allocateStorage(_totalValueCount, _dapType);
    BESDEBUG(NDimensionalArray_debug_key, "NDimensionalArray::NDimensionalArray(libdap::Array *) - END"<< endl);
}

NDimensionalArray::NDimensionalArray(std::vector<unsigned int> *shape, libdap::Type dapType) :
    _dapType(dods_null_c), _shape(0), _currentLastDimensionSlabIndex(0), _totalValueCount(0), _sizeOfValue(0), _storage(
        0)
{
    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::NDimensionalArray(std::vector<unsigned int> *, libdap::Type) - BEGIN"<< endl);

    _shape = new vector<unsigned int>(*shape);
    _totalValueCount = computeArraySizeFromShapeVector(_shape);
    _dapType = dapType;
    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::NDimensionalArray() - _shape" <<vectorToIndices(_shape) << endl);
    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::NDimensionalArray() - Total Value Count: " << _totalValueCount << " element(s) of type '"<< libdap::type_name(_dapType) << "'" << endl);
    allocateStorage(_totalValueCount, _dapType);
    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::NDimensionalArray(std::vector<unsigned int> *, libdap::Type) - END"<< endl);

}

NDimensionalArray::~NDimensionalArray()
{
    delete[] (char *) _storage;
    delete _shape;
}

/**
 * Returns a pointer to the underlying storage for the NDimensionalArray. Calling this function is
 * effectively telling the instance of NDimensionalArray to 'release' it's reference to the storage. While
 * the memory will not be deleted by this call, the instance of NDimensionalArray will remove it's internal reference to
 * the storage and thus when the NDimensionalArray goes out of scope, or is otherwise deleted the storage WILL NOT BE DELETED.
 * CALLING THIS METHOD MEANS THAT YOU ARE NOW RESPONSIBLE FOR FREEING THE MEMORY REFERENCED BY THE RETURNED POINTER.
 */
void *NDimensionalArray::relinquishStorage()
{
    void *s = _storage;
    _storage = 0;
    return s;
}

/**
 * Computes and returns (via the returned value parameter 'shape') the constrained shape of the libdap::Array 'a'.
 * Returns the total number of elements in constrained shape.
 */
long NDimensionalArray::computeConstrainedShape(libdap::Array *a, vector<unsigned int> *shape)
{
    BESDEBUG(NDimensionalArray_debug_key, "NDimensionalArray::computeConstrainedShape() - BEGIN." << endl);

    libdap::Array::Dim_iter dIt;
    unsigned int start;
    unsigned int stride;
    unsigned int stop;

    unsigned int dimSize = 1;
    int dimNum = 0;
    long totalSize = 1;

    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::computeConstrainedShape() - Array has " << a->dimensions(true) << " dimensions."<< endl);

    stringstream msg;

    for (dIt = a->dim_begin(); dIt != a->dim_end(); dIt++) {
        BESDEBUG(NDimensionalArray_debug_key,
            "NDimensionalArray::computeConstrainedShape() - Processing dimension '" << a->dimension_name(dIt)<< "'. (dim# "<< dimNum << ")"<< endl);
        start = a->dimension_start(dIt, true);
        stride = a->dimension_stride(dIt, true);
        stop = a->dimension_stop(dIt, true);
        BESDEBUG(NDimensionalArray_debug_key,
            "NDimensionalArray::computeConstrainedShape() - start: " << start << "  stride: " << stride << "  stop: "<<stop<< endl);

        dimSize = 1 + ((stop - start) / stride);
        BESDEBUG(NDimensionalArray_debug_key,
            "NDimensionalArray::computeConstrainedShape() - dimSize: " << dimSize << endl);

        (*shape)[dimNum++] = dimSize;
        totalSize *= dimSize;
    }
    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::computeConstrainedShape() - totalSize: " << totalSize << endl);
    BESDEBUG(NDimensionalArray_debug_key, "NDimensionalArray::computeConstrainedShape() - END." << endl);

    return totalSize;
}

void NDimensionalArray::retrieveLastDimHyperSlabLocationFromConstrainedArrray(libdap::Array *a,
    vector<unsigned int> *location)
{
    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::retrieveLastDimHyperSlabLocationFromConstrainedArrray() - BEGIN." << endl);

    libdap::Array::Dim_iter dIt;
    libdap::Array::Dim_iter next_dIt;
    unsigned int start;
    unsigned int stride;
    unsigned int stop;

    int dimNum = 0;

    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::retrieveLastDimHyperSlabLocationFromConstrainedArrray() - Array has " << a->dimensions(true) << " dimensions."<< endl);

    stringstream msg;

    for (dIt = a->dim_begin(); dIt != a->dim_end(); dIt++) {
        next_dIt = dIt;
        next_dIt++;

        BESDEBUG(NDimensionalArray_debug_key,
            "NDimensionalArray::retrieveLastDimHyperSlabLocationFromConstrainedArrray() - Processing dimension '" << a->dimension_name(dIt)<< "'. (dim# "<< dimNum << ")"<< endl);
        start = a->dimension_start(dIt, true);
        stride = a->dimension_stride(dIt, true);
        stop = a->dimension_stop(dIt, true);
        BESDEBUG(NDimensionalArray_debug_key,
            "NDimensionalArray::retrieveLastDimHyperSlabLocationFromConstrainedArrray() - start: " << start << "  stride: " << stride << "  stop: "<<stop<< endl);

        if (next_dIt != a->dim_end() && start != stop && stride != 1) {
            msg << "retrieveLastDimHyperSlabLocationFromConstrainedArrray() - The array '" << a->name()
                << "' has not been constrained to a last dimension hyperslab.";
            BESDEBUG(NDimensionalArray_debug_key, msg.str() << endl);
            throw Error(msg.str());

        }
        if (next_dIt == a->dim_end()) {
            if (start != 0 || stride != 1 || stop != ((unsigned int) a->dimension_size(dIt) - 1)) {
                msg << "retrieveLastDimHyperSlabLocationFromConstrainedArrray() - The array '" << a->name()
                    << "' has not been constrained to a last dimension hyperslab.";
                BESDEBUG(NDimensionalArray_debug_key, msg.str() << endl);
                throw Error(msg.str());

            }

            BESDEBUG(NDimensionalArray_debug_key,
                "NDimensionalArray::retrieveLastDimHyperSlabLocationFromConstrainedArrray() - location"<< vectorToIndices(location) << endl);
            BESDEBUG(NDimensionalArray_debug_key,
                "NDimensionalArray::retrieveLastDimHyperSlabLocationFromConstrainedArrray() - END." << endl);
            return;
        }

        BESDEBUG(NDimensionalArray_debug_key,
            "NDimensionalArray::retrieveLastDimHyperSlabLocationFromConstrainedArrray() - Adding location "<< start << " to dimension " << location->size() << endl);

        location->push_back(start);
    }

    msg
        << "retrieveLastDimHyperSlabLocationFromConstrainedArrray() - Method Failure - this line should never be reached.";
    BESDEBUG(NDimensionalArray_debug_key, msg.str() << endl);
    throw Error(msg.str());

}

/**
 * Computes the total number of elements of the n-dimensional array described by the shape vector.
 */
long NDimensionalArray::computeArraySizeFromShapeVector(vector<unsigned int> *shape)
{
    long totalSize = 1;

    for (unsigned int i = 0; i < shape->size(); i++) {
        totalSize *= (*shape)[i];
    }

    return totalSize;
}

/**
 * Allocates internal storage for the NDimensionalArray
 */
void NDimensionalArray::allocateStorage(long numValues, Type dapType)
{

    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::allocateStorage() - Allocating memory for " << numValues << " element(s) of type '"<< libdap::type_name(dapType) << "'" << endl);

    switch (dapType) {
    case dods_byte_c:
        _sizeOfValue = sizeof(dods_byte);
        break;
    case dods_int16_c:
        _sizeOfValue = sizeof(dods_int16);
        break;
    case dods_uint16_c:
        _sizeOfValue = sizeof(dods_uint16);
        break;
    case dods_int32_c:
        _sizeOfValue = sizeof(dods_int32);
        break;
    case dods_uint32_c:
        _sizeOfValue = sizeof(dods_uint32);
        break;
    case dods_float32_c:
        _sizeOfValue = sizeof(dods_float32);
        break;
    case dods_float64_c:
        _sizeOfValue = sizeof(dods_float64);
        break;
    default:
        throw InternalErr(__FILE__, __LINE__, "Unknown DAP type encountered when constructing NDimensionalArray");
    }

    _storage = new char[numValues * _sizeOfValue];

}

/**
 * Verifies that the allocated storage for the NDimensioalArray has not been previously surrendered.
 */
void NDimensionalArray::confirmStorage()
{
    if (_storage == 0) {
        string msg =
            "ERROR - NDimensionalArray storage has been relinquished. Instance is no longer viable for set/get operations.";
        BESDEBUG(NDimensionalArray_debug_key, msg << endl);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
}

/**
 * Verifies that the passed TypedapTypen is the same as the underlying type of the NDimensionalArray. If not, and Error is thrown.
 */
void NDimensionalArray::confirmType(Type dapType)
{
    if (_dapType != dapType) {
        string msg = "NDimensionalArray::setValue() - Passed value does not match template array type. Expected "
            + libdap::type_name(_dapType) + " received " + libdap::type_name(dapType);
        BESDEBUG(NDimensionalArray_debug_key, msg << endl);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
}

/**
 * Verifies that the passed value n is the same as the size of the last dimension. If not, and Error is thrown.
 */
void NDimensionalArray::confirmLastDimSize(unsigned int n)
{
    unsigned long elementCount = getLastDimensionElementCount();
    if (elementCount != n) {
        string msg =
            "NDimensionalArray::setLastDimensionHyperSlab() - Passed valueCount does not match size of last dimension hyper-slab. ";
        msg += "Last dimension hyper-slab has " + libdap::long_to_string(elementCount) + " elements. ";
        msg += "Received a valueCount of  " + libdap::long_to_string(n);
        BESDEBUG(NDimensionalArray_debug_key, msg << endl);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

}

/**
 * Sets the value of the array at the location specified by the passed location vector. The number
 * of elements in the location vector must be the the same as the number of dimensions in the NDimensionalArray
 * and the type of the passed value must match the underlying type of the NDimensionalArray
 */
dods_byte NDimensionalArray::setValue(std::vector<unsigned int> *location, dods_byte value)
{

    confirmStorage();
    confirmType(dods_byte_c);

    unsigned int storageIndex = getStorageIndex(_shape, location);
    dods_byte *_store = static_cast<dods_byte*>(_storage);
    dods_byte oldValue = _store[storageIndex];
    _store[storageIndex] = value;
    return oldValue;
}

/**
 * Sets the value of the array at the location specified by the passed location vector. The number
 * of elements in the location vector must be the the same as the number of dimensions in the NDimensionalArray
 * and the type of the passed value must match the underlying type of the NDimensionalArray
 */
dods_int16 NDimensionalArray::setValue(std::vector<unsigned int> *location, dods_int16 value)
{

    confirmStorage();
    confirmType(dods_int16_c);

    unsigned int storageIndex = getStorageIndex(_shape, location);
    dods_int16 *_store = static_cast<dods_int16 *>(_storage);
    dods_int16 oldValue = _store[storageIndex];
    _store[storageIndex] = value;
    return oldValue;
}

/**
 * Sets the value of the array at the location specified by the passed location vector. The number
 * of elements in the location vector must be the the same as the number of dimensions in the NDimensionalArray
 * and the type of the passed value must match the underlying type of the NDimensionalArray
 */
dods_uint16 NDimensionalArray::setValue(std::vector<unsigned int> *location, dods_uint16 value)
{
    confirmStorage();
    confirmType(dods_uint16_c);

    unsigned int storageIndex = getStorageIndex(_shape, location);
    dods_uint16 *_store = static_cast<dods_uint16 *>(_storage);
    dods_uint16 oldValue = _store[storageIndex];
    _store[storageIndex] = value;
    return oldValue;
}

/**
 * Sets the value of the array at the location specified by the passed location vector. The number
 * of elements in the location vector must be the the same as the number of dimensions in the NDimensionalArray
 * and the type of the passed value must match the underlying type of the NDimensionalArray
 */
dods_int32 NDimensionalArray::setValue(std::vector<unsigned int> *location, dods_int32 value)
{
    confirmStorage();
    confirmType(dods_int32_c);

    unsigned int storageIndex = getStorageIndex(_shape, location);
    dods_int32 *_store = static_cast<dods_int32 *>(_storage);
    dods_int32 oldValue = _store[storageIndex];
    _store[storageIndex] = value;
    return oldValue;
}

/**
 * Sets the value of the array at the location specified by the passed location vector. The number
 * of elements in the location vector must be the the same as the number of dimensions in the NDimensionalArray
 * and the type of the passed value must match the underlying type of the NDimensionalArray
 */
dods_uint32 NDimensionalArray::setValue(std::vector<unsigned int> *location, dods_uint32 value)
{
    confirmStorage();
    confirmType(dods_uint32_c);

    unsigned int storageIndex = getStorageIndex(_shape, location);
    dods_uint32 *_store = static_cast<dods_uint32 *>(_storage);
    dods_uint32 oldValue = _store[storageIndex];
    _store[storageIndex] = value;
    return oldValue;
}

/**
 * Sets the value of the array at the location specified by the passed location vector. The number
 * of elements in the location vector must be the the same as the number of dimensions in the NDimensionalArray
 * and the type of the passed value must match the underlying type of the NDimensionalArray
 */
dods_float32 NDimensionalArray::setValue(std::vector<unsigned int> *location, dods_float32 value)
{
    confirmStorage();
    confirmType(dods_float32_c);

    unsigned int storageIndex = getStorageIndex(_shape, location);
    dods_float32 *_store = static_cast<dods_float32 *>(_storage);
    dods_float32 oldValue = _store[storageIndex];
    _store[storageIndex] = value;
    return oldValue;
}

/**
 * Sets the value of the array at the location specified by the passed location vector. The number
 * of elements in the location vector must be the the same as the number of dimensions in the NDimensionalArray
 * and the type of the passed value must match the underlying type of the NDimensionalArray
 */
dods_float64 NDimensionalArray::setValue(std::vector<unsigned int> *location, dods_float64 value)
{
    confirmStorage();
    confirmType(dods_float64_c);

    unsigned int storageIndex = getStorageIndex(_shape, location);
    dods_float64 *_store = static_cast<dods_float64 *>(_storage);
    dods_float64 oldValue = _store[storageIndex];
    _store[storageIndex] = value;
    return oldValue;
}

/**
 * The return value parameters slab and elementCount are used to return a pointer to the first element of the last dimension hyper-slab
 * and the number of elements in the hyper-slab. The passed the location vector, identifies the requested slab.The location vector must
 * have N-1 elements where N is the number of dimensions in the NDimensionalArray.
 */
void NDimensionalArray::getLastDimensionHyperSlab(std::vector<unsigned int> *location, void **slab,
    unsigned int *elementCount)
{
    BESDEBUG(NDimensionalArray_debug_key, endl<< endl <<"NDimensionalArray::getLastDimensionHyperSlab() - BEGIN"<<endl);
    confirmStorage();
    if (location->size() != _shape->size() - 1) {
        string msg =
            "NDimensionalArray::getLastDimensionHyperSlab() - Passed location vector doesn't match array shape.";
        BESDEBUG(NDimensionalArray_debug_key, msg << endl);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::getLastDimensionHyperSlab() - location" <<vectorToIndices(location) << endl);

    vector<unsigned int> slabLocation(*location);

    slabLocation.push_back(0);
    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::getLastDimensionHyperSlab() - slabLocation" <<vectorToIndices(&slabLocation) << endl);

    unsigned int storageIndex = getStorageIndex(_shape, &slabLocation);

    *slab = &((char *) _storage)[storageIndex * _sizeOfValue];
    *elementCount = *(_shape->rbegin());
    BESDEBUG(NDimensionalArray_debug_key, "NDimensionalArray::getLastDimensionHyperSlab() - END"<<endl<<endl);

}

void NDimensionalArray::getNextLastDimensionHyperSlab(void **slab)
{

    unsigned int storageIndex = _shape->back() * _currentLastDimensionSlabIndex++;
    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::getNextLastDimensionHyperSlab() - Storage Index:"<< libdap::long_to_string(storageIndex) << endl);
    *slab = &((char *) _storage)[storageIndex * _sizeOfValue];

}

/**
 * Computes the element index in the underlying one dimensional array for the passed location based on an
 * n-dimensional array described by the shape vector.
 */
long NDimensionalArray::getStorageIndex(vector<unsigned int> *shape, vector<unsigned int> *location)
{
    BESDEBUG(NDimensionalArray_debug_key, "NDimensionalArray::getStorageIndex() - BEGIN." << endl);
    long storageIndex = 0;

    if (location->size() != shape->size()) {
        string msg = "getStorageIndex() - The supplied location vector does not match array shape.";
        BESDEBUG(NDimensionalArray_debug_key, msg << endl);
        throw Error(msg);
    }

    BESDEBUG(NDimensionalArray_debug_key,
        "NDimensionalArray::getStorageIndex() - Shape and location have the same number of elements." << endl);

    long dimIndex = 0;
    long chunkSize = 1;

    for (dimIndex = shape->size() - 1; dimIndex >= 0; dimIndex--) {
        BESDEBUG(NDimensionalArray_debug_key,
            "NDimensionalArray::getStorageIndex() - dimIndex=" << libdap::long_to_string(dimIndex) << endl);

        if ((*location)[dimIndex] >= (*shape)[dimIndex]) {
            string msg =
                "NDimensionalArray::getStorageIndex() - The location vector references a value that does not match the array shape. ";
            msg += "location[" + libdap::long_to_string(dimIndex) + "]=";
            msg += libdap::long_to_string((*location)[dimIndex]) + " ";
            msg += "shape[" + libdap::long_to_string(dimIndex) + "]=";
            msg += libdap::long_to_string((*shape)[dimIndex]) + " ";
            BESDEBUG(NDimensionalArray_debug_key, msg << endl);
            throw Error(msg);
        }
        storageIndex += chunkSize * ((*location)[dimIndex]);
        chunkSize *= ((*shape)[dimIndex]);
    }

    BESDEBUG(NDimensionalArray_debug_key, "NDimensionalArray::getStorageIndex() - END." << endl);
    return storageIndex;
}

/**
 * Sets all of the values in the last dimension hyper-hyper slab identified by the N-1 element location vector (where N is the
 * number of dimensions in the NDimensionalArray). The number of values passed in must match the size of the last dimension
 * hyper-slab, and the type of the values must match the underlying type of the NDimensionalArray.
 */
void NDimensionalArray::setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_byte *values,
    unsigned int valueCount)
{
    confirmType(dods_byte_c);
    confirmLastDimSize(valueCount);
    setLastDimensionHyperSlab(location, (void *) values, valueCount * sizeof(dods_byte));
}

/**
 * Sets all of the values in the last dimension hyper-hyper slab identified by the N-1 element location vector (where N is the
 * number of dimensions in the NDimensionalArray). The number of values passed in must match the size of the last dimension
 * hyper-slab, and the type of the values must match the underlying type of the NDimensionalArray.
 */
void NDimensionalArray::setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_int16 *values,
    unsigned int valueCount)
{
    confirmType(dods_int16_c);
    confirmLastDimSize(valueCount);
    setLastDimensionHyperSlab(location, (void *) values, valueCount * sizeof(dods_int16));
}

/**
 * Sets all of the values in the last dimension hyper-hyper slab identified by the N-1 element location vector (where N is the
 * number of dimensions in the NDimensionalArray). The number of values passed in must match the size of the last dimension
 * hyper-slab, and the type of the values must match the underlying type of the NDimensionalArray.
 */
void NDimensionalArray::setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_uint16 *values,
    unsigned int valueCount)
{
    confirmType(dods_uint16_c);
    confirmLastDimSize(valueCount);
    setLastDimensionHyperSlab(location, (void *) values, valueCount * sizeof(dods_uint16));
}

/**
 * Sets all of the values in the last dimension hyper-hyper slab identified by the N-1 element location vector (where N is the
 * number of dimensions in the NDimensionalArray). The number of values passed in must match the size of the last dimension
 * hyper-slab, and the type of the values must match the underlying type of the NDimensionalArray.
 */
void NDimensionalArray::setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_int32 *values,
    unsigned int valueCount)
{
    confirmType(dods_int32_c);
    confirmLastDimSize(valueCount);
    setLastDimensionHyperSlab(location, (void *) values, valueCount * sizeof(dods_int32));
}

/**
 * Sets all of the values in the last dimension hyper-hyper slab identified by the N-1 element location vector (where N is the
 * number of dimensions in the NDimensionalArray). The number of values passed in must match the size of the last dimension
 * hyper-slab, and the type of the values must match the underlying type of the NDimensionalArray.
 */
void NDimensionalArray::setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_uint32 *values,
    unsigned int valueCount)
{
    confirmType(dods_uint32_c);
    confirmLastDimSize(valueCount);
    setLastDimensionHyperSlab(location, (void *) values, valueCount * sizeof(dods_uint32));
}

/**
 * Sets all of the values in the last dimension hyper-hyper slab identified by the N-1 element location vector (where N is the
 * number of dimensions in the NDimensionalArray). The number of values passed in must match the size of the last dimension
 * hyper-slab, and the type of the values must match the underlying type of the NDimensionalArray.
 */
void NDimensionalArray::setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_float32 *values,
    unsigned int valueCount)
{
    confirmType(dods_float32_c);
    confirmLastDimSize(valueCount);
    setLastDimensionHyperSlab(location, (void *) values, valueCount * sizeof(dods_float32));
}

/**
 * Sets all of the values in the last dimension hyper-hyper slab identified by the N-1 element location vector (where N is the
 * number of dimensions in the NDimensionalArray). The number of values passed in must match the size of the last dimension
 * hyper-slab, and the type of the values must match the underlying type of the NDimensionalArray.
 */
void NDimensionalArray::setLastDimensionHyperSlab(std::vector<unsigned int> *location, dods_float64 *values,
    unsigned int valueCount)
{
    confirmType(dods_float64_c);
    confirmLastDimSize(valueCount);
    setLastDimensionHyperSlab(location, (void *) values, valueCount * sizeof(dods_float64));
}

/**
 * This private method uses 'memcopy' to perform a byte by byte copy of the passed values array onto the last dimension
 * hyper-slab referenced by the N-1 element vector location.
 */
void NDimensionalArray::setLastDimensionHyperSlab(std::vector<unsigned int> *location, void *values,
    unsigned int byteCount)
{
    confirmStorage();
    void *slab;
    unsigned int slabElementCount;

    getLastDimensionHyperSlab(location, &slab, &slabElementCount);
    memcpy(slab, values, byteCount);

}

/**
 * Uses 'memset' to set ALL of the values in the NDimensionalArray to the passed char value.
 */
void NDimensionalArray::setAll(char val)
{
    confirmStorage();
    memset(_storage, val, _totalValueCount * _sizeOfValue);

}

/**
 * Returns the size, in elements, of the last dimension.
 */
long NDimensionalArray::getLastDimensionElementCount()
{
    return *(_shape->rbegin());
}

libdap::Array *NDimensionalArray::getArray(libdap::Array *templateArray)
{

    if (_shape->size() != templateArray->dimensions(true))
        throw Error("Template Array has different number of dimensions than NDimensional Array!!");

    libdap::Array *resultDapArray;

    switch (_dapType) {
    case dods_byte_c: {
        libdap::Byte tt(templateArray->name());
        resultDapArray = new libdap::Array(templateArray->name(), &tt);
        break;
    }
    case dods_uint16_c: {
        libdap::Int16 tt(templateArray->name());
        resultDapArray = new libdap::Array(templateArray->name(), &tt);
        break;
    }
    case dods_int16_c: {
        libdap::UInt16 tt(templateArray->name());
        resultDapArray = new libdap::Array(templateArray->name(), &tt);
        break;
    }
    case dods_int32_c: {
        libdap::Int32 tt(templateArray->name());
        resultDapArray = new libdap::Array(templateArray->name(), &tt);
        break;
    }
    case dods_uint32_c: {
        libdap::UInt32 tt(templateArray->name());
        resultDapArray = new libdap::Array(templateArray->name(), &tt);
        break;
    }
    case dods_float32_c: {
        libdap::Float32 tt(templateArray->name());
        resultDapArray = new libdap::Array(templateArray->name(), &tt);
        break;
    }
    case dods_float64_c: {
        libdap::Float64 tt(templateArray->name());
        resultDapArray = new libdap::Array(templateArray->name(), &tt);
        break;
    }
    default:
        throw InternalErr(__FILE__, __LINE__,
            "Unknown DAP type encountered when converting to gridfields internal type.");
    }

    libdap::Array::Dim_iter dimIt;
    int s = 0;
    for (dimIt = templateArray->dim_begin(); dimIt != templateArray->dim_end(); dimIt++, s++) {
        resultDapArray->append_dim((*_shape)[s], (*dimIt).name);
    }

    // Copy the source objects attributes.
    BESDEBUG(NDimensionalArray_debug_key,
        "TwoDMeshTopology::getGFAttributeAsDapArray() - Copying libdap::Attribute's from template array " << templateArray->name() << endl);
    resultDapArray->set_attr_table(templateArray->get_attr_table());

    switch (_dapType) {
    case dods_byte_c: {
        resultDapArray->set_value((dods_byte *) _storage, _totalValueCount);
        break;
    }
    case dods_uint16_c: {
        resultDapArray->set_value((dods_uint16 *) _storage, _totalValueCount);
        break;
    }
    case dods_int16_c: {
        resultDapArray->set_value((dods_int16 *) _storage, _totalValueCount);
        break;
    }
    case dods_uint32_c: {
        resultDapArray->set_value((dods_uint32 *) _storage, _totalValueCount);
        break;
    }
    case dods_int32_c: {
        resultDapArray->set_value((dods_int32 *) _storage, _totalValueCount);
        break;
    }
    case dods_float32_c: {
        resultDapArray->set_value((dods_float32 *) _storage, _totalValueCount);
        break;
    }
    case dods_float64_c: {
        resultDapArray->set_value((dods_float64 *) _storage, _totalValueCount);
        break;
    }
    default:
        throw InternalErr(__FILE__, __LINE__,
            "Unknown DAP type encountered when converting to gridfields internal type.");
    }

    return resultDapArray;
}

string NDimensionalArray::toString_worker(vector<unsigned int> *location)
{

    stringstream s;
    if (location->size() == _shape->size()) {
        s << "  storage";
        s << vectorToIndices(location);

        s << ": ";
        long storageIndex = getStorageIndex(_shape, location);
        switch (_dapType) {
        case dods_byte_c: {
            s << ((dods_byte *) _storage)[storageIndex];
            break;
        }
        case dods_uint16_c: {
            s << ((dods_uint16 *) _storage)[storageIndex];
            break;
        }
        case dods_int16_c: {
            s << ((dods_int16 *) _storage)[storageIndex];
            break;
        }
        case dods_uint32_c: {
            s << ((dods_uint32 *) _storage)[storageIndex];
            break;
        }
        case dods_int32_c: {
            s << ((dods_int32 *) _storage)[storageIndex];
            break;
        }
        case dods_float32_c: {
            s << ((dods_float32 *) _storage)[storageIndex];
            break;
        }
        case dods_float64_c: {
            s << ((dods_float64 *) _storage)[storageIndex];
            break;
        }
        default:
            throw InternalErr(__FILE__, __LINE__,
                "Unknown DAP type encountered when converting to gridfields internal type.");
        }
        s << endl;

    }
    else {
        int nextDimSize = (*_shape)[location->size()];
        for (int i = 0; i < nextDimSize; i++) {
            location->push_back(i);
            s << toString_worker(location);
            location->pop_back();
        }

    }
    return s.str();

}

string NDimensionalArray::toString()
{

    stringstream s;
    vector<unsigned int> location;

    s << endl << "NDimensionalArray: " << endl;
    s << toString_worker(&location);

    return s.str();

}

} /* namespace libdap */
