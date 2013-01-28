// BESServerFunctionsList.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include <algorithm>

#include <ConstraintEvaluator.h>
#include <expr.h>

#include "BESServerFunctionsList.h"
#include "BESReporter.h"

using namespace std;
using namespace libdap;

BESServerFunctionsList *BESServerFunctionsList::d_instance = 0 ;

BESServerFunctionsList::BESServerFunctionsList()
{
}

BESServerFunctionsList::~BESServerFunctionsList()
{
}

bool
BESServerFunctionsList::add_function( string name, btp_func func )
{
    if (d_btp_func_list[name] == 0) {
        d_btp_func_list[name] = func;
        return true;
    }

    return false;
}

bool
BESServerFunctionsList::add_function( string name, bool_func func )
{
    if (d_bool_func_list[name] == 0) {
        d_bool_func_list[name] = func;
        return true;
    }

    return false;
}

bool
BESServerFunctionsList::add_function( string name, proj_func func )
{
    if (d_proj_func_list[name] == 0) {
        d_proj_func_list[name] = func;
        return true;
    }

    return false;
}

void BESServerFunctionsList::store_functions(ConstraintEvaluator &ce)
{
    if (d_btp_func_list.size() > 0) {
        map<string, btp_func>::iterator i = d_btp_func_list.begin();
        map<string, btp_func>::iterator e = d_btp_func_list.end();
        while (i != e) {
            ce.add_function((*i).first, (*i).second);
            ++i;
        }
    }

    if (d_bool_func_list.size() > 0) {
        map<string, bool_func>::iterator i = d_bool_func_list.begin();
        map<string, bool_func>::iterator e = d_bool_func_list.end();
        while (i != e) {
            ce.add_function((*i).first, (*i).second);
            ++i;
        }
    }

    if (d_proj_func_list.size() > 0) {
        map<string, proj_func>::iterator i = d_proj_func_list.begin();
        map<string, proj_func>::iterator e = d_proj_func_list.end();
        while (i != e) {
            ce.add_function((*i).first, (*i).second);
            ++i;
        }
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this catalog directory.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESServerFunctionsList::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESServerFunctionsList::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();

    if (d_btp_func_list.size() > 0) {
        strm << BESIndent::LMarg << "registered btp functions:" << endl;
        BESIndent::Indent();
        map<string, btp_func>::const_iterator i = d_btp_func_list.begin();
        map<string, btp_func>::const_iterator e = d_btp_func_list.end();
        while (i != e) {
            strm << (*i).first << endl;
            ++i;
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "registered btp functions: none" << endl;
    }

    if (d_bool_func_list.size() > 0) {
        strm << BESIndent::LMarg << "registered bool functions:" << endl;
        BESIndent::Indent();
        map<string, bool_func>::const_iterator i = d_bool_func_list.begin();
        map<string, bool_func>::const_iterator e = d_bool_func_list.end();
        while (i != e) {
            strm << (*i).first << endl;
            ++i;
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "registered bool functions: none" << endl;
    }

    if (d_proj_func_list.size() > 0) {
        strm << BESIndent::LMarg << "registered projection functions:" << endl;
        BESIndent::Indent();
        map<string, proj_func>::const_iterator i = d_proj_func_list.begin();
        map<string, proj_func>::const_iterator e = d_proj_func_list.end();
        while (i != e) {
            strm << (*i).first << endl;
            ++i;
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "registered projection functions: none" << endl;
    }

    BESIndent::UnIndent();
}

BESServerFunctionsList *
BESServerFunctionsList::TheList()
{
    if (d_instance == 0) {
        d_instance = new BESServerFunctionsList;
    }
    return d_instance;
}

