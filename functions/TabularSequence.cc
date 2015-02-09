// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 20153 OPeNDAP, Inc.
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

// #define DODS_DEBUG

#include <BaseType.h>
#include <DDS.h>
#include <ConstraintEvaluator.h>
#include <Marshaller.h>
#include <UnMarshaller.h>
#include <debug.h>

#include "TabularSequence.h"

using namespace std;

namespace libdap {

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

// Public member functions

/** The Sequence constructor requires only the name of the variable
    to be created.  The name may be omitted, which will create a
    nameless variable.  This may be adequate for some applications.

    @param n A string containing the name of the variable to be
    created.

    @brief The Sequence constructor. */
TabularSequence::TabularSequence(const string &n) : Sequence(n)
{
}

/** The Sequence server-side constructor requires the name of the variable
    to be created and the dataset name from which this variable is being
    created.

    @param n A string containing the name of the variable to be
    created.
    @param d A string containing the name of the dataset from which this
    variable is being created.

    @brief The Sequence server-side constructor. */
TabularSequence::TabularSequence(const string &n, const string &d)
    : Sequence(n, d)
{}

/** @brief The Sequence copy constructor. */
TabularSequence::TabularSequence(const TabularSequence &rhs) : Sequence(rhs)
{
}

BaseType *
TabularSequence::ptr_duplicate()
{
    return new TabularSequence(*this);
}

TabularSequence::~TabularSequence()
{

}

TabularSequence &
TabularSequence::operator=(const TabularSequence &rhs)
{
    if (this == &rhs)
        return *this;

    dynamic_cast<Sequence &>(*this) = rhs; // run Sequence=

    return *this;
}

// This version ignores any constraint. jhrg 2/6/15
bool
TabularSequence::serialize(ConstraintEvaluator &eval, DDS &dds, Marshaller &m, bool ce_eval /* true */)
{
    DBG(cerr << "Entering TabularSequence::serialize for " << name() << endl);

    SequenceValues values = value();
    ce_eval = true;

    for (SequenceValues::iterator i = values.begin(), e = values.end(); i != e; ++i) {

        BaseTypeRow &btr = **i;

        // Transfer values of the current row into the Seq's prototypes so the CE
        // evaluator will find the values.
        int j = 0;
        for (BaseTypeRow::iterator vi = btr.begin(), ve = btr.end(); vi != ve; ++vi) {
            void *val = 0;
            (*vi)->buf2val(&val);
            d_vars.at(j++)->val2buf(val);
        }

        // Evaluate the CE against this row; continue (skipping this row) if it fails
        if (ce_eval && !eval.eval_selection(dds, dataset()))
            continue;

        // Write out this row of values
        write_start_of_instance(m);

        // In this loop serialize will signal an error with an exception.
        for (BaseTypeRow::iterator vi = btr.begin(), ve = btr.end(); vi != ve; ++vi) {
            DBG(cerr << "TabularSequence::serialize(), serializing " << (*vi)->name() << endl);
            if ((*vi)->send_p()) {
                DBG(cerr << "Send P is true, sending " << (*vi)->name() << endl);
                (*vi)->serialize(eval, dds, m, false);
            }
        }
    }

    write_end_of_sequence(m);

    return true;  // Signal errors with exceptions.
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
    strm << DapIndent::LMarg << "TabularSequence::dump - (" << (void *)this << ")" << endl ;
    DapIndent::Indent() ;
    Sequence::dump(strm) ;
    DapIndent::UnIndent() ;
}

} // namespace libdap

