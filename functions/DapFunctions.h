// DapFunctions.h

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

#ifndef A_DapFunctions_H
#define A_DapFunctions_H 1

#include "BESAbstractModule.h"

using namespace libdap;

class DapFunctions: public BESAbstractModule {
public:
    DapFunctions()
    {
    }
    virtual ~DapFunctions()
    {
    }
    virtual void initialize(const string &modname);
    virtual void terminate(const string &modname);

    virtual void dump(ostream &strm) const;
};



#endif // A_DapFunctions_H
