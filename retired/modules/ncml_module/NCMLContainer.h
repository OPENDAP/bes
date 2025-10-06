// NCMLContainer.h

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of wcs_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle wcs requests.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: Patrick West <pwest@ucar.edu>
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

// (c) COPYRIGHT URI/MIT 1994-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      pcw       Patrick West <pwest@ucar.edu>

#ifndef NCMLContainer_h_
#define NCMLContainer_h_ 1

#include <string>

#include "BESContainer.h"

/** @brief Container representing a NCML request
 *
 * The real name of a NCMLContainer is the actual NCML request. When the
 * access method is called the NCML request is made, the response cached if
 * Successful, and the target response returned as the real container that
 * a data handler would then open.
 *
 * @note This code is broken - but it was never actually used. The code closes
 * the open stream that is returned using a value-result parameter in NCMLContainer::access
 * but it should not be doing that. Instead is should keep the stream as a field and
 * close it in the dtor. jhrg 8/12/15
 *
 * @see NCMLContainerStorage
 */
class NCMLContainer: public BESContainer {
private:
    string _xml_doc;
    bool _accessed;
    string _tmp_file_name;

    NCMLContainer() :
        BESContainer(), _accessed(false)
    {
    }
protected:
    void _duplicate(NCMLContainer &copy_to);
public:
    NCMLContainer(const string &sym_name, const string &real_name);

    NCMLContainer(const NCMLContainer &copy_from);

    virtual ~NCMLContainer();

    virtual BESContainer * ptr_duplicate();

    virtual string access();

    virtual bool release();

    virtual void dump(ostream &strm) const;
};

#endif // NCMLContainer_h_

