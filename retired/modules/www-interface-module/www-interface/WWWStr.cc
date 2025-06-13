
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

// Implementation for WWWStr. See WWWByte.cc
//
// 4/7/99 jhrg

#include "config.h"

static char rcsid[] not_used = {"$Id$"};

#include <math.h>
#include <stdlib.h>

#include <iostream>
#include <string>

#include <libdap/InternalErr.h>

#include "WWWStr.h"
#include "WWWOutput.h"
#include "get_html_form.h"

using namespace dap_html_form;

WWWStr::WWWStr(const string &n) : Str(n)
{
}

BaseType *
WWWStr::ptr_duplicate()
{
    return new WWWStr(*this);
}

void
WWWStr::print_val(FILE *os, string, bool /*print_decl_p*/)
{
    write_simple_variable(os, this);
}

void
WWWStr::print_val(ostream &strm, string, bool /*print_decl_p*/)
{
    write_simple_variable(strm, this);
}

