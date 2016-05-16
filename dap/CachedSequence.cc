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

#include "config.h"

//#define DODS_DEBUG

#include <algorithm>
#include <string>
#include <sstream>
#include <cassert>

#include <BaseType.h>
#include <Byte.h>
#include <Int16.h>
#include <Int32.h>
#include <UInt16.h>
#include <UInt32.h>
#include <Float32.h>
#include <Float64.h>
#include <Str.h>
#include <Url.h>

#include <DDS.h>
#include <ConstraintEvaluator.h>
#include <Marshaller.h>
//#include <UnMarshaller.h>
#include <debug.h>

#include "CachedSequence.h"

using namespace std;
using namespace libdap;

// namespace bes {

void CachedSequence::load_prototypes_with_values(BaseTypeRow &btr, bool safe)
{
    // For each of the prototype variables in the Sequence, load it
    // with a values from the BaseType* vector. The order should match.
    // Test the type, but assume if that matches, the value is correct
    // for the variable.
    Vars_iter i = d_vars.begin(), e = d_vars.end();
    for (BaseTypeRow::iterator vi = btr.begin(), ve = btr.end(); vi != ve; ++vi) {

        if (safe && (i == e || ((*i)->type() != (*vi)->type())))
            throw InternalErr(__FILE__, __LINE__, "Expected number and types to match when loading values.");

        // Ugly... but faster than the generic code that allocates storage for each scalar?
        switch ((*i)->type()) {
        case dods_byte_c:
            static_cast<Byte*>(*i++)->set_value(static_cast<Byte*>(*vi)->value());
            break;
        case dods_int16_c:
            static_cast<Int16*>(*i++)->set_value(static_cast<Int16*>((*vi))->value());
            break;
        case dods_int32_c:
            static_cast<Int32*>(*i++)->set_value(static_cast<Int32*>((*vi))->value());
            break;
        case dods_uint16_c:
            static_cast<UInt16*>(*i++)->set_value(static_cast<UInt16*>((*vi))->value());
            break;
        case dods_uint32_c:
            static_cast<UInt32*>(*i++)->set_value(static_cast<UInt32*>((*vi))->value());
            break;
        case dods_float32_c:
            static_cast<Float32*>(*i++)->set_value(static_cast<Float32*>((*vi))->value());
            break;
        case dods_float64_c:
            static_cast<Float64*>(*i++)->set_value(static_cast<Float64*>((*vi))->value());
            break;
        case dods_str_c:
            static_cast<Str*>(*i++)->set_value(static_cast<Str*>((*vi))->value());
            break;
        case dods_url_c:
            static_cast<Url*>(*i++)->set_value(static_cast<Url*>((*vi))->value());
            break;

        case dods_sequence_c:
            if (vi + 1 != ve)
                throw InternalErr(__FILE__, __LINE__, "Expected nested sequence to be the last variable.");
            break;

        default:
            throw InternalErr(__FILE__, __LINE__, "Expected a scalar (or nested sequence) when loading values.");
        }
    }
}

// Public member functions

/**
 * @brief Read row number <i>row</i> of the Sequence.
 *
 * This version of read_row() gets the next row of data from the internal
 * 'values' of the Sequence (d_values field) and loads them into the prototype
 * variables so that the stock serialize() code can be used to write them
 * out.
 *
 * @note This code can likely replace the class TabularSequence in bes/functions.
 * It is a more general version of that because it should work for nested sequences
 * too.
 *
 * @note The first row is row number zero. A Sequence with 100 rows will
 * have row numbers 0 to 99. This method makes use of the get_row_number()
 * method (added in libdap 3.17.1) that wraps the d_row_number field. The
 * field uses -1 as a sentinel meaning 'before the start' so the value
 * must be incremented to '0' before it can be used to read a row.
 *
 * @return A boolean value, with TRUE indicating that read_row
 * should be called again because there's more data to be read.
 * FALSE indicates the end of the Sequence.
 *
 * @param row The row number to read.
 * @param dds A reference to the DDS for this dataset.
 * @param eval Use this as the constraint expression evaluator.
 * @param ce_eval If True, evaluate any CE, otherwise do not.
 */

bool CachedSequence::read_row(int row, DDS &dds, ConstraintEvaluator &eval, bool ce_eval)
{
    DBGN(cerr << __PRETTY_FUNCTION__ << " name: " << name() << ", row number " << row << ", current row " << get_row_number() << endl);

    // get_row_number() returns the current row number for the sequence. This
    // means the number of the current row that satisfies the selection constraint.
    // Thus, if 20 rows have been res (d_value_index == 19 then) but only 5
    // satisfy the selection, get_row_number() will return 4 (the number of
    // the current row). We alwasy have to be asking for a row greater then
    // the current row - it is not possible to back-up when reading a Sequences's
    // values.
    assert(row > get_row_number());

    while (row > get_row_number()) {
        // Read the next row of values. d_value_index is reset when the code
        // first runs serialize() or intern_data(). This enables the code here
        // to mimic the 'read the next set of values' behavior of the parent
        // class.
        BaseTypeRow *btr_ptr = row_value(d_value_index++);

        // This corresponds to the 'return false on EOF' behavior of Sequence:read_row.
        // When the Sequence::read() method returns 'true' at EOF (the last value has
        // been read). This works because the row_value() method above returns 0 (NULL)
        // when the code tries to read past the end of the vector or BaseTypeRow*s.
        if (!btr_ptr) return false;

        // Load the values into the variables
#ifdef NDEBUG
        load_prototypes_with_values(*btr_ptr, false);
#else
        load_prototypes_with_values(*btr_ptr, true);
#endif
        if (!ce_eval) {
            increment_row_number(1);
            return true;
        }
        else if (ce_eval && eval.eval_selection(dds, dataset())) {
            increment_row_number(1);
            return true;
        }
    }

    return false;
}

/**
 * @brief Specialization that resets CachedSequence's 'value index' state variable
 *
 * This specialization resets the index into the 'value' field and calls the
 * parent class' method.
 *
 * @param eval
 * @param dds
 * @param m
 * @param ce_eval
 * @return The value of the parent class' serialize() method.
 */
bool CachedSequence::serialize(ConstraintEvaluator &eval, DDS &dds, Marshaller &m, bool ce_eval)
{
    // Reset the index to the parent's value field's index
    d_value_index = 0;

    return Sequence::serialize(eval, dds, m, ce_eval);
}

/**
 * @brief Specialization that resets CachedSequence's 'value index' state variable
 *
 * This specialization resets the index into the 'value' field and calls the
 * parent class' method.
 *
 * @param eval
 * @param dds
 */
void CachedSequence::intern_data(ConstraintEvaluator &eval, DDS &dds)
{
    d_value_index = 0;

    Sequence::intern_data(eval, dds);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance and information about this
 * instance.
 *
 * @param strm C++ i/o stream to dump the information to
 * @return void
 */
void
CachedSequence::dump(ostream &strm) const
{
    strm << DapIndent::LMarg << "CachedSequence::dump - (" << (void *)this << ")" << endl ;
    DapIndent::Indent() ;
    Sequence::dump(strm) ;
    DapIndent::UnIndent() ;
}

// } // namespace bes

