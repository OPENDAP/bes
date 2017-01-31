//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////

#include "Shape.h"
#include "NCMLDebug.h"
#include <sstream> // for std::stringstream

namespace ncml_module {
typedef vector<Array::dimension>::const_iterator DimItC;

/** Empty shape */
Shape::Shape() :
    _dims()
{
}

Shape::Shape(const Array& copyDimsFrom)
{
    Array& from = const_cast<Array&>(copyDimsFrom);
    Array::Dim_iter endIt = from.dim_end();
    Array::Dim_iter it;
    for (it = from.dim_begin(); it != endIt; ++it) {
        const Array::dimension& dim = (*it);
        _dims.push_back(dim);
    }
}

Shape::Shape(const Shape& proto)
{
    _dims = proto._dims;
}

Shape::~Shape()
{
    _dims.resize(0);
}

Shape&
Shape::operator=(const Shape& rhs)
{
    if (&rhs == this) {
        return *this;
    }

    _dims = rhs._dims;
    return *this;
}

bool Shape::operator==(const Shape& rhs) const
{
    bool ret = true;

    // Simple dimensionality check must pass first
    if (rhs._dims.size() != _dims.size()) {
        ret = false;
    }
    else // then compare all the dimensions
    {
        for (unsigned int i = 0; i < _dims.size(); ++i) {
            ret = areDimensionsEqual(_dims[i], rhs._dims[i]);
            if (!ret) // just give up...
            {
                break;
            }
        }
    }
    return ret;
}

bool Shape::isConstrained() const
{
    bool ret = false;
    for (unsigned int i = 0; i < _dims.size(); ++i) {
        const Array::dimension& dim = _dims[i];
        ret |= (dim.c_size != dim.size);
        if (ret) {
            break;
        }
    }
    return ret;
}

void Shape::setToUnconstrained()
{
    for (unsigned int i = 0; i < _dims.size(); ++i) {
        Array::dimension& dim = _dims[i];
        dim.c_size = dim.size;
        dim.start = 0;
        dim.stop = dim.size - 1;
        dim.stride = 1;
    }
}

bool Shape::areDimensionsEqual(const Array::dimension& lhs, const Array::dimension& rhs)
{
    // Stolen from Array::dimension....
    /*
     int size;  ///< The unconstrained dimension size.
     string name;    ///< The name of this dimension.
     int start;  ///< The constraint start index
     int stop;  ///< The constraint end index
     int stride;  ///< The constraint stride
     int c_size;  ///< Size of dimension once constrained
     */
    // Must all match
    bool equal = true;
    equal &= (lhs.size == rhs.size);
    equal &= (lhs.name == rhs.name);
    equal &= (lhs.start == rhs.start);
    equal &= (lhs.stride == rhs.stride);
    equal &= (lhs.c_size == rhs.c_size);
    return equal;
}

unsigned int Shape::getRowMajorIndex(const IndexTuple& indices, bool validate /* = true */) const
{
    if (validate && !validateIndices(indices)) {
        THROW_NCML_INTERNAL_ERROR(
            "Shape::getRowMajorIndex got indices that were out of range for the given space dimensions!");
    }

    NCML_ASSERT(indices.size() >= 1);
    unsigned int index = indices[0];
    for (unsigned int i = 1; i < indices.size(); ++i) {
        index = indices[i] + (_dims[i].size * index);
    }
    return index;
}

Shape::IndexIterator Shape::beginSpaceEnumeration() const
{
    return IndexIterator(*this, false);
}

Shape::IndexIterator Shape::endSpaceEnumeration() const
{
    return IndexIterator(*this, true);
}

std::string Shape::toString() const
{
    std::stringstream sos;
    print(sos);
    return sos.str();
}

void Shape::print(std::ostream& strm) const
{
    strm << "Shape = { ";
    for (unsigned int i = 0; i < _dims.size(); ++i) {
        const Array::dimension& dim = _dims[i];
        printDimension(strm, dim);
    }
    strm << " }\n";
}

void Shape::printDimension(std::ostream& strm, const Array::dimension& dim)
{
    strm << "\tDim = { \n";
    strm << "\t\tname=" << dim.name << "\n";
    strm << "\t\tsize=" << dim.size << "\n";
    strm << "\t\tc_size=" << dim.c_size << "\n";
    strm << "\t\tstart=" << dim.start << "\n";
    strm << "\t\tstop=" << dim.stop << "\n";
    strm << "\t\tstride=" << dim.stride << "\n";
}

bool Shape::validateIndices(const IndexTuple& indices) const
{
    if (indices.size() != _dims.size()) {
        return false;
    }

    for (unsigned int i = 0; i < _dims.size(); ++i) {
        // Must be > 0 and <= size-1
        if (indices[i] >= static_cast<unsigned int>(_dims[i].size)) {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////// Shape::IndexIterator

Shape::IndexIterator::IndexIterator() :
    _shape(0), _current(), _end(true)
{
}

Shape::IndexIterator::IndexIterator(const Shape& shape, bool isEnd/*=false*/) :
    _shape(&shape), _current(shape.getNumDimensions()) // start with default of proper size, we'll fix it later.
        , _end(isEnd)
{
    setCurrentToStart();
}

Shape::IndexIterator::IndexIterator(const Shape::IndexIterator& proto) :
    _shape(proto._shape), _current(proto._current), _end(proto._end)
{
}

Shape::IndexIterator::~IndexIterator()
{
    _shape = 0;
    _current.resize(0);
    _end = true;
}

Shape::IndexIterator&
Shape::IndexIterator::operator=(const Shape::IndexIterator& rhs)
{
    if (this == &rhs) {
        return *this;
    }

    _shape = rhs._shape;
    _current = rhs._current;
    _end = rhs._end;
    return *this;
}

bool Shape::IndexIterator::operator==(const Shape::IndexIterator& rhs) const
{
    // The ptr's must be the same...  It has to come from the same Shape object.
    // We could use Shape::operator== but this is too slow for every increment.
    if (_shape != rhs._shape) {
        return false;
    }

    // If one has end set, the other needs to as well to be equal
    if (_end != rhs._end) {
        return false;
    }

    // Finally, make sure the arrays have the same values.
    return (_current == rhs._current);
}

/** Algorithm:
 *
 *  We want to enumerate all points in the space defined by the Shape and its constraints (if any).
 *  This involves generating all constrained N-tuples, where N is the number of dimensions in
 *  the Shape we are iterating, in row major order (leftmost tuple components vary SLOWEST).
 *
 * The algorithm is:
 *
 *  In order from rightmost dimension to leftmost, for dimension i:
 *    Increment the index using the stride.
 *    If it goes past the stop and wraps back to start, set a carry bit to propagate the increment left.
 *  If the carry bit is set, recurse to the i-1 dimension, else stop.
 *
 *  If the leftmost dimension wraps, we have reached the end, so set _end and be done.
 *
 *  This will enumerate all elements in the space defined by Shape (essentially a product of discrete
 *  cyclic groups) in a "row major order" (leftmost varies slowest).
 */
void Shape::IndexIterator::advanceCurrent()
{
    // We're done already, let the caller know they goofed.  This works for uninitialize with null _shape as well.
    if (_end) {
        THROW_NCML_INTERNAL_ERROR("Shape::IndexIterator::advanceCurrent(): tried to advance beyond end()!");
    }

    bool carry = true; // if a dimension's index wraps back to start on increment, this is set.  Starts true to initiate loop.
    unsigned int dimInd = _shape->getNumDimensions();
    while (carry && dimInd-- > 0) // first pass will decrement this so it starts at N-1m as we desire.
    {
        const Array::dimension& dim = _shape->_dims[dimInd];
        // grab the reference so we mutate the elt itself
        unsigned int& currentDimIndex = _current[dimInd];
        currentDimIndex += dim.stride;
        carry = false; // assume no carry to start
        // if we wrap, then set the carry bit to propagate the increment to the dimension to our left in the tuple
        if (currentDimIndex > static_cast<unsigned int>(dim.stop)) {
            currentDimIndex = dim.start;
            carry = true;
        }
    }

    // If we end the loop with carry still set, we've wrapped the slowest varying dimension
    // and we're back to the starting group element.  Set _end!
    if (carry) {
        _end = true;
    }
}

void Shape::IndexIterator::setCurrentToStart()
{
    VALID_PTR(_shape);
    for (unsigned int i = 0; i < _shape->getNumDimensions(); ++i) {
        _current[i] = _shape->_dims[i].start;
    }
}

} // namespace ncml_module
