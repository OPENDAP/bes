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

#include <string>

#include <BESDebug.h>

#include "InternalErr.h"
#include "D4Group.h"
#include "AsciiStructure.h"
#include "AsciiSequence.h"
//#include "name_map.h"
#include "get_ascii.h"

using namespace dap_asciival;

BaseType *
AsciiStructure::ptr_duplicate()
{
    return new AsciiStructure(*this);
}

AsciiStructure::AsciiStructure(const string &n) :
    Structure(n)
{
}

AsciiStructure::AsciiStructure(Structure *bt) :
    Structure(bt->name()), AsciiOutput(bt)
{
    // Let's make the alternative structure of Ascii types now so that we
    // don't have to do it on the fly. This will also set the parents of
    // each of the underlying vars of the structure.
    Vars_iter p = bt->var_begin();
    while (p != bt->var_end()) {
        BaseType *new_bt = basetype_to_asciitype(*p);
        add_var(new_bt);
        // add_var makes a copy of the base type passed to it, so delete
        // it here
        delete new_bt;
        p++;
    }

    BaseType::set_send_p(bt->send_p());
}

AsciiStructure::~AsciiStructure()
{
}

void AsciiStructure::transform_to_dap4(D4Group *root, Constructor *container){
    cerr << __func__ << "() -BEGIN " <<
        "('"<< root->name() << "':"<< (void *)root << ")" <<
        "('"<< container->name() << "':"<< (void *)container << ")" <<
        endl;
    AsciiStructure *dest =  new AsciiStructure(name());
    Constructor::transform_to_dap4(root,dest);
    container->add_var_nocopy(dest);
    cerr << __func__ << "() - END" << endl;
}

void AsciiStructure::print_header(ostream &strm)
{
    Vars_iter p = var_begin();
    while (p != var_end()) {
        if ((*p)->is_simple_type())
            strm << dynamic_cast<AsciiOutput&>(**p).get_full_name();
        else if ((*p)->type() == dods_structure_c)
            dynamic_cast<AsciiStructure&>(**p).print_header(strm);
        // May need a case here for Sequence 2/18/2002 jhrg
        // Yes, we do, and for Grid as well. 04/04/03 jhrg
        else
            throw InternalErr(__FILE__, __LINE__,
                "Support for ASCII output of datasets with structures which contain Sequences or Grids has not been completed.");
        if (++p != var_end()) strm << ", ";
    }
}

void AsciiStructure::print_ascii(ostream &strm, bool print_name) throw (InternalErr)
{
    BESDEBUG("ascii", "In 'AsciiStructure::print_ascii'" << endl);

    if (is_linear()) {
        if (print_name) {
            print_header(strm);
            strm << "\n";
        }

        Vars_iter p = var_begin();
        while (p != var_end()) {
            if ((*p)->send_p()) dynamic_cast<AsciiOutput &>(**p).print_ascii(strm, false);

            if (++p != var_end()) strm << ", ";
        }
    }
    else {
        for (Vars_iter p = var_begin(); p != var_end(); ++p) {
            if ((*p)->send_p()) {
                dynamic_cast<AsciiOutput&>(**p).print_ascii(strm, true);
                // This line outputs an extra endl when print_ascii is called for
                // nested structures because an endl is written for each member
                // and then once for the structure itself. 9/14/2001 jhrg
                strm << "\n";
            }
        }
    }
}

