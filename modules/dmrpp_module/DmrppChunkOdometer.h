// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#ifndef DMRPP_CHUNK_ODOMETER_H_
#define DMRPP_CHUNK_ODOMETER_H_

#include <vector>

namespace dmrpp {

using shape = std::vector<unsigned long long>;

/**
 * Map the indices of a N-dimensional array to the offset into memory
 * (i.e., a vector) that matches those indices. This code can be used
 * to step through each element of an N-dim array without using
 * multiplication to compute the offset into the vector that holds
 * the array's data.
 *
 * @note The code does use multiplication, but only performs N-1 multiplies
 * for a N dimensional array in set_indices() (called once) and not in next()
 * which will likely be called many times.
 *
 * reset(): zero internal state
 *
 * bool next(): move to the next element, incrementing the shape information
 *          and returning an offset into a linear vector for that element.
 * 			Calling next() when the object is at the last element returns
 * 			false. Calling next() after that is probably a bad idea.
 *
 * vector<int> indices(): for the given state of the odometer, return the
 *          indices that match the offset.
 *
 */
class DmrppChunkOdometer
{
private:
    // The state set by the ctor
    shape d_shape;
    shape d_array_shape;

    // The varying state of the Odometer
    shape d_indices;

public:
    DmrppChunkOdometer() = default;
    ~DmrppChunkOdometer() = default;

    /**
     * Build an instance of Odometer using the given 'shape'. Each element
     * of the shape vector is the size of the corresponding dimension. E.G.,
     * a 10 by 20 by 30 array would be described by a vector of 10,20,30.
     *
     * Initially, the Odometer object is set to index 0, 0, ..., 0 that
     * matches the offset 0
     */
    DmrppChunkOdometer(shape chunk_shape, shape array_shape) :
        d_shape(std::move(chunk_shape)), d_array_shape(std::move(array_shape))
    {
        reset();
    }

    /**
     * Reset the internal state. The offset is reset to the 0th element
     * and the indices are reset to 0, 0, ..., 0.
     */
    void reset() noexcept
    {
        d_indices.resize(d_shape.size(), 0);
    }

    /**
     * Increment the Odometer to the next element and return the offset value.
     * This increments the internal state and returns
     * the offset to that element in a vector of values. Calling indices() after
     * calling this method will return a vector<unsigned int> of the current
     * index value.
     *
     * @return The offset into memory for the next element. Returns a value that
     * matches the one returned by end() when next has been called when the object
     * index is at the last element.
     */
    inline bool next()
    {
        bool status = false;    // false indicates the last 'odometer value' has been found
        auto si = d_shape.rbegin();
        auto ai = d_array_shape.rbegin();
        for (auto i = d_indices.rbegin(), e = d_indices.rend(); i != e; ++i, ++si, ++ai) {
            *i = *i + *ai;
            if (*i >= *si) {
                *i = 0;
            }
            else {
                status = true;
                break;
            }
        }

        return status;
    }

    /**
     * Return the current set of indices. These match the current offset.
     * Both the offset and indices are incremented by the next() method.
     *
     * To access the ith index, use [i] or .at(i)
     */
    inline const shape &indices() const noexcept
    {
        return d_indices;
    }
};

}	// namespace libdap

#endif // DMRPP_CHUNK_ODOMETER_H_
