
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of xml_data_handler, software which can return an XML data
// representation of the data read from a DAP server.

// Copyright (c) 2010 OPeNDAP, Inc.
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

// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// Interface for XDUrl type. See XDByte.h
//
// 3/12/98 jhrg

#ifndef _xdurl_h
#define _xdurl_h 1

#include <Url.h>

#include "XDOutput.h"

#if 0
// This should work, but linking fails because of the XDOutputFactory which
// returns a pointer to this class using Url*
class XDUrl: public XDStr {
public:
    XDUrl(const string &n) : XDStr( n ) {}
    XDUrl( Url *bt ) : XDStr( bt ) {
        set_send_p(bt->send_p());
    }
    virtual ~XDUrl() {}

    virtual BaseType *ptr_duplicate();
};
#endif

class XDUrl: public libdap::Url, public XDOutput {
public:
    XDUrl(const string &n) : Url( n ) {}
    XDUrl( Url *bt ) : Url( bt->name() ), XDOutput( bt ) {
        set_send_p(bt->send_p());
    }
    virtual ~XDUrl() {}

    virtual BaseType *ptr_duplicate();

    void print_xml_data(libdap::XMLWriter *writer, bool show_type);
};

#endif

