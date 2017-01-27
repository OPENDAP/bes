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

#ifndef __NCML_MODULE__SHAPE_H__
#define __NCML_MODULE__SHAPE_H__

#include <iostream>
#include <iterator>
#include <Array.h>
#include "NCMLDebug.h"
#include <string>
#include <vector>

using libdap::Array;

namespace ncml_module {
/** @brief A wrapper class for a vector of Array::dimension structs.
 *
 * We add functionality for equality, getting space sizes for
 * constrained and unconstrained cases, and a way to get
 * row major absolute indices into a linear array rep of a multi-dimensional array
 * (as C stores them).
 *
 * Shape::IndexIterator: We also add an iterator for generating the full set of index tuples
 * for all the points in a given Shape (constrained or not) using a row major order memory traversal
 * (i.e. for the case of no constraints, we should advance linearly one element at a time through the memory
 * when looking up an index tuple). Leftmost component of the tuple varies fastest.  Again,
 * these iterator work if there is a constraint on the dataset as well, generating one index tuple
 * for each entry in the defined hyperslab, also in row major order so that the constrained
 * elements can be shoved into a Vector _buf for shipping off the hyperslab.
 */
class Shape {
public:
    // Indices into the space of the shape.
    typedef std::vector<unsigned int> IndexTuple;

public:
    // Inner Classes

    /** A custom iterator that enumerates all the points in the space
     * defined by a Shape in row major order.  It ALSO handles constraints
     * on the Shape and will return the enumeration of only the points
     * in the constraint hyperslab, starting with the start index on
     * all dimensions and incrementing the rightmost dimensions fastest */
    class IndexIterator: public std::iterator<std::forward_iterator_tag, IndexTuple> {
    public:
        /** isEnd is only set by Shape for creating an end() iterator... */
        IndexIterator(); // for uninitialized.  Don't use it!
        IndexIterator(const Shape& shape, bool isEnd = false);
        IndexIterator(const IndexIterator& proto);
        ~IndexIterator();
        IndexIterator& operator=(const IndexIterator& rhs);
        bool operator==(const IndexIterator& rhs) const;

        inline bool operator!=(const IndexIterator& rhs) const
        {
            return (!((*this) == rhs));
        }

        inline IndexIterator& operator++() //prefix
        {
            advanceCurrent();
            return *this;
        }

        inline IndexIterator operator++(int) // postfix...
        {
            // Copy it, this is why prefix increment is preferred in STL...
            Shape::IndexIterator tmp(*this);
            ++(*this);
            return tmp;
        }

        // don't mutate the return, we use it to compute next element!
        inline const Shape::IndexTuple& operator*()
        {
            NCML_ASSERT_MSG(!_end, "Can't reference end iterator!");
            return _current;
        }

    private:
        /** If !_end,  Advance the iterator by one point.  If there are no more points, it sets _end to true. */
        void advanceCurrent();

        /** Set the _current tuple to be the start value for all dimensions */
        void setCurrentToStart();

    private:
        const Shape* _shape; // the shape of the space we are iterating on.  It cannot change during an iteration!
        IndexTuple _current; // the current point.
        bool _end; // set to true when we reach the end since there's no other way to tell if we're at start or end.
    }; // class IndexIterator

public:
    friend class IndexIterator;

public:
    /** The empty shape, with no dimensions.  Not valid for anything useful except IndexIterator */
    Shape();
    Shape(const Shape& proto);
    /** Create this Shape from the shape of the given Array. */
    Shape(const Array& copyDimsFrom);
    ~Shape();
    Shape& operator=(const Shape& rhs);

    /** Return if this contains the same shape as rhs.
     * They must match ALL fields of ALL dimensions to be equal!!
     */
    bool operator==(const Shape& rhs) const;

    /** @return !(*this == rhs) */
    bool operator!=(const Shape& rhs) const
    {
        return !(*this == rhs);
    }

    /** Are there constraints on the dimension? */
    bool isConstrained() const;

    /** Go through and set all the values to be the unconstrained values */
    void setToUnconstrained();

    inline unsigned int getNumDimensions() const
    {
        return _dims.size();
    }

    /** Helper:
     * @returns whether all the fields of the two args are equal */
    static bool areDimensionsEqual(const Array::dimension& lhs, const Array::dimension& rhs);

    /** Get the product of all the dimension sizes */
    inline unsigned int getUnconstrainedSpaceSize() const
    {
        unsigned int size = 1;
        for (unsigned int i = 0; i < _dims.size(); ++i) {
            size *= _dims[i].size;
        }
        return size;
    }

    /** Get the production of all dimension c_sizes. */
    inline unsigned int getConstrainedSpaceSize() const
    {
        unsigned int c_size = 1;
        for (unsigned int i = 0; i < _dims.size(); ++i) {
            c_size *= _dims[i].c_size;
        }
        return c_size;
    }

    /** Return the row major linear index into a slowest varying dimension first
     * flattening of the given UNCONSTRAINED space (ie use dim.size for each dimension's size)
     *
     * @param indices an array with an index into each dimension of the space.
     * @param validate whether to do bounds checks or not.  true means slightly slower, but safer.
     *                 exceptions are only thrown if validate, unless a memory fault occurs from bad indices.
     *
     * ASSUME: indices.size() is the dimensionality of the space, which MUST match _dims.size() or we throw.
     *
     * Given there are d dimensions of size n_i for i in {0, d-1}
     * and indices is the d-tuple (i_0, i_1, ..., 1_{d-1}) where each i_j is in [0, n_j - 1]
     * the index is: i_{d-1} + n_{d-1} * (i_{d-2} + n_{d-2} * ( ... + n_1 * i_0) )
     *
     * @exception If the dimensionality of indices does not match the dimensionality of _dims.
     * @exception If any index in indices is out of bounds for the matching dimension in _dims.
     */
    unsigned int getRowMajorIndex(const IndexTuple& indices, bool validate = true) const;

    /**
     * Create a forward iterator that returns IndexTuple's in a row major order
     * (leftmost dimension slowest varying) enumeration of all the points
     * in this Shape.  This uses constraints, so should return
     * getConstrainedSpaceSize() values.  If constraints are not set,
     * the getConstrainedSpaceSize() == getUnconstrainedSpaceSize().
     *
     * ASSUMES: this Shape CANNOT be mutated during the life of the return value!!
     * Only const functions should be used for the life of the returned iterator.
     */
    Shape::IndexIterator beginSpaceEnumeration() const;

    /** The end of the enumeration for testing end conditions.
     * @see beginSpaceEnumeration() */
    Shape::IndexIterator endSpaceEnumeration() const;

    /** Make a string that prints the contents of this */
    std::string toString() const;

    /** Print the contents of this to the stream */
    void print(std::ostream& strm) const;

    /** Helper to print the dimension to a stream. */
    static void printDimension(std::ostream& strm, const Array::dimension& dim);

    /** @return whether all the indices are in the correct ranges for current dims
     * and that the indices.size() matches the _dims.size().
     */
    bool validateIndices(const IndexTuple& indices) const;

private:
    // Methods

private:
    std::vector<Array::dimension> _dims;

};
// class Shape

}// namespace ncml_module

inline std::ostream &
operator<<(std::ostream &strm, const ncml_module::Shape& shape)
{
    shape.print(strm);
    return strm;
}
#endif /* __NCML_MODULE__SHAPE_H__ */
