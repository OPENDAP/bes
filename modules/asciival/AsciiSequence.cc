// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of asciival, software which can return an ASCII
// representation of the data read from a DAP server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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

// (c) COPYRIGHT URI/MIT 1998,2000
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// Implementation for the class AsciiStructure. See AsciiByte.cc
//
// 3/12/98 jhrg

#include "config.h"

#include <iostream>
#include <string>

#include <BESDebug.h>

#include "InternalErr.h"
#include "AsciiSequence.h"
#include "AsciiStructure.h"
#include "get_ascii.h"
#include "debug.h"

using std::endl;
using namespace dap_asciival;

BaseType *
AsciiSequence::ptr_duplicate()
{
    return new AsciiSequence(*this);
}

AsciiSequence::AsciiSequence(const string &n) :
    Sequence(n)
{
}

AsciiSequence::AsciiSequence(Sequence * bt) :
    Sequence(bt->name()), AsciiOutput(bt)
{
    // Let's make the alternative structure of Ascii types now so that we
    // don't have to do it on the fly.
    Vars_iter p = bt->var_begin();
    while (p != bt->var_end()) {
        BaseType *new_bt = basetype_to_asciitype(*p);
        add_var(new_bt);
        delete new_bt;
        p++;
    }

    BaseType::set_send_p(bt->send_p());
}

AsciiSequence::~AsciiSequence()
{
}

int AsciiSequence::length() const
{
    return -1;
}

// This specialization is different from the Sequence version only in that
// it tests '(*iter)->send_p()' before incrementing 'i' by
// '(*iter)->element_count(true)'.
int AsciiSequence::element_count(bool leaves)
{
    if (!leaves)
        return d_vars.size();
    else {
        int i = 0;
        for (Vars_iter iter = d_vars.begin(); iter != d_vars.end(); iter++) {
            if ((*iter)->send_p()) i += (*iter)->element_count(true);
        }
        return i;
    }
}

void AsciiSequence::print_ascii_row(ostream &strm, int row, BaseTypeRow outer_vars)
{
    BESDEBUG("ascii", "    In AsciiSequence::print_ascii_row" << endl);

    Sequence *seq = dynamic_cast<Sequence *>(_redirect);
    if (!seq) seq = this;

    // Print the values from this sequence.
    // AsciiSequence::element_count() returns only vars with send_p() set.
    const int elements = element_count();
    bool first_var = true;     // used to control printing the comma separator
    int j = 0;
    do {
        BaseType *bt_ptr = seq->var_value(row, j);
        if (bt_ptr) {           // Check for data.
            BaseType *abt_ptr = basetype_to_asciitype(bt_ptr);
            if (abt_ptr->type() == dods_sequence_c) {
                if (abt_ptr->send_p()) {
                    if (!first_var)
                        strm << ", ";
                    else
                        first_var = false;

                    dynamic_cast<AsciiSequence&>(*abt_ptr).print_ascii_rows(strm, outer_vars);
                }
            }
            else {
                // push the real base type pointer instead of the ascii one.
                // We can cast it again later from the outer_vars vector.
                outer_vars.push_back(bt_ptr);
                if (abt_ptr->send_p()) {
                    if (!first_var)
                        strm << ", ";
                    else
                        first_var = false;

                    dynamic_cast<AsciiOutput&>(*abt_ptr).print_ascii(strm, false);
                }
            }

            // we only need the ascii type here, so delete it
            delete abt_ptr;
        }

        ++j;
    } while (j < elements);
}

void AsciiSequence::print_leading_vars(ostream &strm, BaseTypeRow & outer_vars)
{
    BESDEBUG("ascii", "    In AsciiSequence::print_leading_vars" << endl);

    bool first_var = true;
    BaseTypeRow::iterator BTR_iter = outer_vars.begin();
    while (BTR_iter != outer_vars.end()) {
        BaseType *abt_ptr = basetype_to_asciitype(*BTR_iter);
        if (!first_var)
            strm << ", ";
        else
            first_var = false;
        dynamic_cast<AsciiOutput&>(*abt_ptr).print_ascii(strm, false);
        delete abt_ptr;

        ++BTR_iter;
    }

    BESDEBUG("ascii", "    Out AsciiSequence::print_leading_vars" << endl);
}

void AsciiSequence::print_ascii_rows(ostream &strm, BaseTypeRow outer_vars)
{
    Sequence *seq = dynamic_cast<Sequence *>(_redirect);
    if (!seq) seq = this;

    const int rows = seq->number_of_rows() - 1;
    int i = 0;
    bool done = false;
    do {
        if (i > 0 && !outer_vars.empty()) print_leading_vars(strm, outer_vars);

        print_ascii_row(strm, i++, outer_vars);

        if (i > rows)
            done = true;
        else
            strm << "\n";
    } while (!done);

    BESDEBUG("ascii", "    Out AsciiSequence::print_ascii_rows" << endl);
}

void AsciiSequence::print_header(ostream &strm)
{
    bool first_var = true;    // Print commas as separators
    Vars_iter p = var_begin();
    while (p != var_end()) {
        if ((*p)->send_p()) {
            if (!first_var)
                strm << ", ";
            else
                first_var = false;

            if ((*p)->is_simple_type())
                strm << dynamic_cast<AsciiOutput *>(*p)->get_full_name();
            else if ((*p)->type() == dods_sequence_c)
                dynamic_cast<AsciiSequence *>(*p)->print_header(strm);
            else if ((*p)->type() == dods_structure_c)
                dynamic_cast<AsciiStructure *>(*p)->print_header(strm);
            else
                throw InternalErr(
                __FILE__,
                __LINE__,
                    "This method should only be called by instances for which `is_simple_sequence' returns true.");
        }

        ++p;
    }
}

void AsciiSequence::print_ascii(ostream &strm, bool print_name) throw (InternalErr)
{
    BESDEBUG("ascii", "In AsciiSequence::print_ascii" << endl);
    Sequence *seq = dynamic_cast<Sequence *>(_redirect);
    if (!seq) seq = this;

    if (seq->is_linear()) {
        if (print_name) {
            print_header(strm);
            strm << "\n";
        }

        BaseTypeRow outer_vars(0);
        print_ascii_rows(strm, outer_vars);
    }
    else {
        const int rows = seq->number_of_rows() - 1;
        const int elements = seq->element_count() - 1;

        // For each row of the Sequence...
        bool rows_done = false;
        int i = 0;
        do {
            // For each variable of the row...
            bool vars_done = false;
            int j = 0;
            do {
                BaseType *bt_ptr = seq->var_value(i, j++);
                BaseType *abt_ptr = basetype_to_asciitype(bt_ptr);
                dynamic_cast<AsciiOutput&>(*abt_ptr).print_ascii(strm, true);
                // abt_ptr is not stored for future use, so delete it
                delete abt_ptr;

                if (j > elements)
                    vars_done = true;
                else
                    strm << "\n";
            } while (!vars_done);

            i++;
            if (i > rows)
                rows_done = true;
            else
                strm << "\n";
        } while (!rows_done);
    }
}
