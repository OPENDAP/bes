
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of www_int, software which returns an HTML form which
// can be used to build a URL to access data from a DAP data server.

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

// (c) COPYRIGHT URI/MIT 1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// implementation for WWWGrid. See WWWByte.
//
// 4/7/99 jhrg

#include "config.h"

static char rcsid[] not_used =
    { "$Id$" };

#include <assert.h>
#include <iostream>
#include <sstream>
#include <string>

#include <libdap/Array.h>
#include <libdap/escaping.h>
#include <libdap/InternalErr.h>

#include "WWWGrid.h"
#include "WWWOutput.h"
#include "get_html_form.h"

using namespace dap_html_form;

BaseType *WWWGrid::ptr_duplicate()
{
    return new WWWGrid(*this);
}

WWWGrid::WWWGrid(const string & n) : Grid(n)
{
}

WWWGrid::WWWGrid(Grid * grid): Grid(grid->name())
{
    BaseType *bt = basetype_to_wwwtype(grid->array_var());
    bt->set_attr_table(grid->array_var()->get_attr_table());
    add_var(bt, libdap::array);
    delete bt;

    Grid::Map_iter i = grid->map_begin();
    Grid::Map_iter e = grid->map_end();
    while ( i != e ) {
        Array *at = dynamic_cast<Array *>(basetype_to_wwwtype(*i));
        at->set_attr_table((*i)->get_attr_table());
        add_var(at, maps);
        delete at;
        ++i;
    }
}


WWWGrid::~WWWGrid()
{
}

void
WWWGrid::do_print_val(ostream &ss)
{
    const string fqn = get_fqn(this);
    ss << "<script type=\"text/javascript\">\n"
        << "<!--\n"
        << name_for_js_code(fqn) << " = new dods_var(\""
        << id2www_ce(fqn)
        << "\", \"" << name_for_js_code(fqn) << "\", 1);\n"
        << "DODS_URL.add_dods_var(" << name_for_js_code(fqn) << ");\n"
        << "// -->\n" << "</script>\n";

    ss << "<b>"
        << "<input type=\"checkbox\" name=\"get_" <<
        name_for_js_code(fqn)
        << "\"\n" << "onclick=\"" << name_for_js_code(fqn)
        << ".handle_projection_change(get_"
        << name_for_js_code(fqn) << ") \"  onfocus=\"describe_projection()\">\n"
        << "<font size=\"+1\">" << name() << "</font></b>"
        << ": " << fancy_typename(this) << "<br>\n\n";

    Array *a = dynamic_cast < Array * >(array_var());
    if (!a) throw InternalErr(__FILE__, __LINE__, "Expected an Array");

    Array::Dim_iter p = a->dim_begin();
    for (int i = 0; p != a->dim_end(); ++i, ++p) {
        const int size = a->dimension_size(p, true);
        const string n = a->dimension_name(p);
        if (n != "")
            ss << n << ":";
        ss << "<input type=\"text\" name=\"" << name_for_js_code(fqn)
            << "_" << i
            << "\" size=8 onfocus=\"describe_index()\""
            << "onChange=\"DODS_URL.update_url()\">\n";
        ss << "<script type=\"text/javascript\">\n"
            << "<!--\n"
            << name_for_js_code(fqn) << ".add_dim(" << size << ");\n"
            << "// -->\n" << "</script>\n";
    }

    ss << "<br>\n";
}

void
WWWGrid::print_val(FILE * os, string /*space*/, bool /*print_decl_p*/)
{
    ostringstream ss ;
    do_print_val( ss ) ;
    fprintf(os, "%s", ss.str().c_str());
}

void
WWWGrid::print_val(ostream &strm, string /*space*/, bool /*print_decl_p*/)
{
    do_print_val(strm);
}

