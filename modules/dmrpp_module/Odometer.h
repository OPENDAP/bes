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

#ifndef ODOMETER_H_
#define ODOMETER_H_

#include <vector>

namespace dmrpp {

/**
 * Map the indices of a N-dimensional array to the offset into memory
 * (i.e., a vector) that matches those indices. This code can be used
 * to step through each element of an N-dim array without using
 * multiplication to compute the offset into the vector that holds
 * the array's data.
 *
 * @note The code does use multiplication, but only performs N-1 multiplies
 * for a N dimensions in set_indices() (called once) and not in next()
 * which will likely be called many times.
 */
class Odometer
{
public:
    typedef std::vector<unsigned int> shape;

private:
    // The state set by the ctor
    shape d_shape;
    unsigned int d_highest_offset;
    unsigned int d_rank;

    // The varying state of the Odometer
    shape d_indices;
    unsigned int d_offset;

public:
    /**
     * Build an instance of Odometer using the given 'shape'. Each element
     * of the shape vector is the size of the corresponding dimension. E.G.,
     * a 10 by 20 by 30 array would be described by a vector of 10,20,30.
     *
     * Initially, the Odometer object is set to index 0, 0, ..., 0 that
     * matches the offset 0
     *
     * 	reset(): zero internal state
     * 	next(): move to the next element, incrementing the shape information and returning an offset into a linear vector for that element.
     * 			Calling next() when the object is at the last element should return one past the last element. calling next() after that should throw an exception.
     * 	vector<int> indices(): for the given state of the odometer, return the indices that match the offset.
     * 	offset(): return the offset
     * 	end(): should return one past the last valid offset - the value returned by next() when it indicates all elements/indices have been visited.
     *
     */

    Odometer(shape shape) : d_shape(shape), d_offset(0)
    {
        d_rank = d_shape.size();

        // compute the highest offset value based on the array shape
        d_highest_offset = 1;
        for (unsigned int i = 0; i < d_rank; ++i) {
            d_highest_offset *= d_shape.at(i);
        }

        d_indices.resize(d_rank, 0);
    }

    const shape get_shape(){
    	return d_shape;
    }


    /**
     * Reset the internal state. The offset is reset to the 0th element
     * and the indices are reset to 0, 0, ..., 0.
     */
    void reset()
    {
        for (unsigned int i = 0; i < d_rank; ++i)
            d_indices.at(i) = 0;
        d_offset = 0;
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
    inline unsigned int next()
    {
        // About 2.4 seconds for 10^9 elements
        shape::reverse_iterator si = d_shape.rbegin();
        for (shape::reverse_iterator i = d_indices.rbegin(), e = d_indices.rend(); i != e; ++i, ++si) {
            if (++(*i) == *si) {
                *i = 0;
            }
            else {
                break;
            }
        }

        return ++d_offset;
    }

    // This version throws Error if offset() == end()
    unsigned int next_safe();

    /**
     * Given a set of indices, update offset to match the position
     * in the memory/vector they correspond to given the Odometer's
     * initial shape.
     * @param indices Indices of an element
     * @return The position in linear memory of that element
     */
    inline unsigned int set_indices(const shape &indices)
    {
        d_indices = indices;

        // I copied this algorithm from Nathan's code in NDimenensionalArray in the
        // ugrid function module. jhrg 5/22/15
#if 0
        shape::reverse_iterator si = d_shape.rbegin();
        for (shape::reverse_iterator i = d_indices.rbegin(), e = d_indices.rend(); i != e; ++i, ++si) {
            // The initial multiply is always 1 * N in both cases
            d_offset += chunk_size * *i;
            chunk_size *= *si;
        }
#endif

        shape::reverse_iterator shape_index = d_shape.rbegin();
        shape::reverse_iterator index = d_indices.rbegin(), index_end = d_indices.rend();
        d_offset = *index++;
        unsigned int chunk_size = *shape_index++;
        while (index != index_end) {
            d_offset += chunk_size * *index++;
            chunk_size *= *shape_index++;
        }

        return d_offset;
    }

    unsigned int set_indices(const std::vector<int> &indices)
    {
        shape temp;
        std::copy(indices.begin(), indices.end(), std::back_inserter(temp));

        return set_indices(temp);
    }

    /**
     * Return the current set of indices. These match the current offset.
     * Both the offset and indices are incremented by the next() method.
     *
     * To access the ith index, use [i] or .at(i)
     */
    inline void indices(shape &indices)
    {
        indices = d_indices;
    }

    /**
     * The offset into memory for the current element.
     */
    inline unsigned int offset()
    {
        return d_offset;
    }

    /**
     * Return the sentinel value that indicates that the offset (returned by
     * offset()) is at the end of the array. When offset() < end() the values
     * of offset() and indices() are valid elements of the array being indexed.
     * When offset() == end(), the values are no longer valid and the last
     * array element has been visited.
     */
    inline unsigned int end()
    {
        return d_highest_offset;
    }

};

}	// namespace libdap

#endif /* ODOMETER_H_ */
