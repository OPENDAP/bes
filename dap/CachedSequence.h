// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
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

#ifndef _cached_sequence_h
#define _cached_sequence_h 1

#include <libdap/Sequence.h>

namespace libdap {
class ConstraintEvaluator;
class DDS;
class Marshaller;
}

// namespace bes {

/** @brief Specialization of Sequence for cached responses
 *
 * Assumes that the data are loaded into the Sequence using set_value()
 */
class CachedSequence: public libdap::Sequence
{
private:
protected:
    unsigned int d_value_index;

    void load_prototypes_with_values(libdap::BaseTypeRow &btr, bool safe = true);

public:
    /** The Sequence constructor requires only the name of the variable
        to be created.  The name may be omitted, which will create a
        nameless variable.  This may be adequate for some applications.

        @param n A string containing the name of the variable to be
        created.

        @brief The Sequence constructor. */
    CachedSequence(const string &n) : Sequence(n), d_value_index(0) { }

    /** The Sequence server-side constructor requires the name of the variable
        to be created and the dataset name from which this variable is being
        created.

        @param n A string containing the name of the variable to be
        created.
        @param d A string containing the name of the dataset from which this
        variable is being created.

        @brief The Sequence server-side constructor. */
    CachedSequence(const string &n, const string &d) : Sequence(n, d), d_value_index(0) { }

    /** @brief The Sequence copy constructor. */
    CachedSequence(const CachedSequence &rhs) : Sequence(rhs), d_value_index(0) { }

    virtual ~CachedSequence() { }

    virtual BaseType *ptr_duplicate() { return new CachedSequence(*this); }

    CachedSequence &operator=(const CachedSequence &rhs) {
        if (this == &rhs)
            return *this;

        static_cast<Sequence &>(*this) = rhs; // run Sequence=

        return *this;
    }

    virtual bool read_row(int row, libdap::DDS &dds, libdap::ConstraintEvaluator &eval, bool ce_eval);

    virtual void intern_data(libdap::ConstraintEvaluator &eval, libdap::DDS &dds);
    virtual bool serialize(libdap::ConstraintEvaluator &eval, libdap::DDS &dds, libdap::Marshaller &m, bool ce_eval = true);

    void dump(ostream &strm) const override;
};

// } // namespace bes

#endif //_cached_sequence_h
