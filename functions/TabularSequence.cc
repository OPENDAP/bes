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

#include "BESIndent.h"
#include "TabularSequence.h"

using namespace std;
using namespace libdap;

namespace functions {

// static constants and functions copied from the parent class. These
// should never have been static... hindsight

static const unsigned char end_of_sequence = 0xA5; // binary pattern 1010 0101
static const unsigned char start_of_instance = 0x5A; // binary pattern 0101 1010

static void
write_end_of_sequence(Marshaller &m)
{
    m.put_opaque( (char *)&end_of_sequence, 1 ) ;
}

static void
write_start_of_instance(Marshaller &m)
{
    m.put_opaque( (char *)&start_of_instance, 1 ) ;
}

void TabularSequence::load_prototypes_with_values(BaseTypeRow &btr, bool safe)
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
            throw InternalErr(__FILE__, __LINE__, "Expected a scalar type when loading values for selection expression evaluation.");
        }
    }
}

// Public member functions

/**
 * Specialized version of Sequence::serialize() for tables that already
 * hold their data. This will not work for nested Sequences.
 *
 * @note The ce_eval parameter was being set to false in BESDapResponseBuilder
 * when the code was processing a response from a function. I changed that to
 * 'true' in that code to avoid the special case here. Our tests for the functions
 * are pretty thin at this point, however, so the change should be reviewed
 * when those tests are improved.
 *
 * @param eval
 * @param dds
 * @param m
 * @param ce_eval
 * @return
 */
bool
TabularSequence::serialize(ConstraintEvaluator &eval, DDS &dds, Marshaller &m, bool ce_eval /* true */)
{
    DBG(cerr << "Entering TabularSequence::serialize for " << name() << endl);

    SequenceValues &values = value_ref();
    //ce_eval = true; Commented out here and changed in BESDapResponseBuilder. jhrg 3/10/15

    for (SequenceValues::iterator i = values.begin(), e = values.end(); i != e; ++i) {

        BaseTypeRow &btr = **i;

        // Transfer values of the current row into the Seq's prototypes so the CE
        // evaluator will find the values.
#if 1
        load_prototypes_with_values(btr, false);
#else
        int j = 0;
        for (BaseTypeRow::iterator vi = btr.begin(), ve = btr.end(); vi != ve; ++vi) {
            void *val = 0;
            (*vi)->buf2val(&val);
            d_vars.at(j++)->val2buf(val);
        }
#endif
        DBG(cerr << __func__ << ": Sequence element: " << hex << *btr.begin() << dec << endl);
        // Evaluate the CE against this row; continue (skipping this row) if it fails
        if (ce_eval && !eval.eval_selection(dds, dataset()))
            continue;

        // Write out this row of values
        write_start_of_instance(m);

        // In this loop serialize will signal an error with an exception.
        for (BaseTypeRow::iterator vi = btr.begin(), ve = btr.end(); vi != ve; ++vi) {
            if ((*vi)->send_p()) {
                (*vi)->serialize(eval, dds, m, false);
            }
        }
    }

    write_end_of_sequence(m);

    return true;  // Signal errors with exceptions.
}

/**
 * Specialized intern_data(). This version copies data from the TabularSequence's
 * local store and filters it. Because callers of intern_data() expect that the
 * object will, after calling this method, hold only data to be sent, this version
 * performs both projection and selection operations.
 *
 * @param eval
 * @param dds
 */
void TabularSequence::intern_data(ConstraintEvaluator &eval, DDS &dds)
{
    DBG(cerr << "Entering TabularSequence::intern_data" << endl);

    // TODO Special case when there are no selection clauses
    // TODO Use a destructive copy to move values from 'values' to
    // result? Or pop values - find a way to not copy all the values
    // after doing some profiling to see if this code can be meaningfully
    // optimized
    SequenceValues result;      // These values satisfy the CE
    SequenceValues &values = value_ref();

    for (SequenceValues::iterator i = values.begin(), e = values.end(); i != e; ++i) {

        BaseTypeRow &btr = **i;

        // Transfer values of the current row into the Seq's prototypes so the CE
        // evaluator will find the values.
        load_prototypes_with_values(btr, false /* safe */);
#if 0
        int j = 0;
        for (BaseTypeRow::iterator vi = btr.begin(), ve = btr.end(); vi != ve; ++vi) {
            // TODO check this for efficiency - is the switch-based version (load_prototypes_with_values) faster?
            void *val = 0;
            (*vi)->buf2val(&val);
            d_vars.at(j++)->val2buf(val);
        }
#endif
        // Evaluate the CE against this row; continue (skipping this row) if it fails
        if (!eval.eval_selection(dds, dataset()))
            continue;

        BaseTypeRow *result_row = new BaseTypeRow();
        for (BaseTypeRow::iterator vi = btr.begin(), ve = btr.end(); vi != ve; ++vi) {
            if ((*vi)->send_p()) {
                result_row->push_back(*vi);
            }
        }

        result.push_back(result_row);
    }

    set_value(result);

    DBG(cerr << "Leaving TabularSequence::intern_data" << endl);
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
TabularSequence::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "TabularSequence::dump - (" << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    Sequence::dump(strm) ;
    BESIndent::UnIndent() ;
}

} // namespace functions

