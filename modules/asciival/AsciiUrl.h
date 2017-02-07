
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

// Interface for AsciiUrl type. See AsciiByte.h
//
// 3/12/98 jhrg

#ifndef _asciiurl_h
#define _asciiurl_h 1

#include "Url.h"
#include "AsciiOutput.h"

class AsciiUrl: public Url, public AsciiOutput {
public:
    AsciiUrl(const string &n) : Url( n ) {}
    AsciiUrl( Url *bt ) : Url( bt->name() ), AsciiOutput( bt ) {
        set_send_p(bt->send_p());
    }
    virtual ~AsciiUrl() {}

    virtual BaseType *ptr_duplicate();
};

#endif

