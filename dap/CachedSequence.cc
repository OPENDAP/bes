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

#include <algorithm>
#include <string>
#include <sstream>

//#define DODS_DEBUG

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
#include <UnMarshaller.h>
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

        if (safe && (i == e || ((*i)->type() != (*vi)->var()->type())))
            throw InternalErr(__FILE__, __LINE__, "Expected number and types to match when loading values for selection expression evaluation.");

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
        default:
            // FIXME: only throw when there's a sequence not in the last position or a
            // structure or grid. 5/15/16
            throw InternalErr(__FILE__, __LINE__, "Expected a scalar type when loading values for selection expression evaluation.");
        }
    }
}

// Public member functions

/**
 * @brief Read row number <i>row</i> of the Sequence.
 *
 * @todo Describe how this specialization works
 *
 * @note The first row is row number zero. A Sequence with 100 rows will
 * have row numbers 0 to 99.
 *
 * @return A boolean value, with TRUE indicating that read_row
 * should be called again because there's more data to be read.
 * FALSE indicates the end of the Sequence.
 * @param row The row number to read.
 * @param dds A reference to the DDS for this dataset.
 * @param eval Use this as the constraint expression evaluator.
 * @param ce_eval If True, evaluate any CE, otherwise do not.
 */

bool CachedSequence::read_row(int row, DDS &dds, ConstraintEvaluator &eval, bool ce_eval)
{
    DBGN(cerr << __PRETTY_FUNCTION__ << " name: " << name() << ", row number " << row << ", current row " << d_row_number << endl);
    if (row < get_row_number())
        throw InternalErr("Trying to back up inside a sequence!");

    if (!((unsigned)row < value_ref().size()))
        throw InternalErr(__FILE__, __LINE__, "Trying to read beyond the end of internal data");

    if (row == get_row_number()) {
        DBGN(cerr << __PRETTY_FUNCTION__ << "Exit, name: " << name() << endl);
        return false;
    }

    while (row < get_row_number()) {
        // Read the row.
        BaseTypeRow *btr_ptr = row_value(get_row_number());
        increment_row_number(1);    // add 1 to the internal row counter

        // This corresponds to the 'return false on EOF' behavior of Sequence:read_row.
        // When the Sequence::read() method returns 'true' at EOF (the last value has
        // been read).
        if (!btr_ptr) return false;

        // Load the values into the variables
        load_prototypes_with_values(*btr_ptr, true);

        if (ce_eval && eval.eval_selection(dds, dataset())) {
            set_read_p(true);
            return true;
        }
    }
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

