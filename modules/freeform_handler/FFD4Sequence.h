
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of freeform_handler; a FreeForm API handler for the OPeNDAP
// data server.

// Copyright (c) 2014 OPeNDAP, Inc.
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

#ifndef _ffd4sequence_h
#define _ffd4sequence_h 1

#include <D4Sequence.h>

using namespace libdap ;

class FFD4Sequence: public D4Sequence {
private:
    string d_input_format_file;

public:
    FFD4Sequence(const string &name, const string &dataset, const string &iff)
		: D4Sequence(name, dataset), d_input_format_file(iff) { }

    virtual ~FFD4Sequence() { }

    virtual BaseType *ptr_duplicate() {
    	return new FFD4Sequence(*this);
    }

    virtual bool read();

    virtual void transfer_attributes(AttrTable *at);
};

#endif
