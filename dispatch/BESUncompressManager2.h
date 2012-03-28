// BESUncompressManager2.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2012 OPeNDAP, Inc
// Author: James Gallagher <jgallagher@opendap.org>
// Based in code by Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#ifndef I_BESUncompressManager2_h
#define I_BESUncompressManager2_h 1

#include <map>
#include <string>

using std::map;
using std::string;

#include "BESObj.h"

class BESCache2;

typedef void (*p_bes_uncompress)(const string &src, const string &target);

/** @brief List of all registered decompression methods
 *
 * The BESUncompressManager2 allows the developer to add or remove named
 * decompression methods from the list for this server. By default a gz and
 * bz2 and Z function is provided.
 *
 * What is actually added to the list are static decompression functions.
 * Each of these functions is responsible for decompressing a specific type
 * of compressed file. The manager knows which type to decompress by the
 * file extension.
 *
 * @see BESUncompressGZ
 * @see BESUncompressBZ2
 * @see BESUncompressZ
 * @see BESCache
 */
class BESUncompressManager2: public BESObj {
private:
    static BESUncompressManager2 * _instance;
    map<string, p_bes_uncompress> _uncompress_list;
    typedef map<string, p_bes_uncompress>::const_iterator UCIter;
#if 0
    typedef map<string, p_bes_uncompress>::iterator UIter;
#endif

    BESUncompressManager2(void);

public:
    virtual ~BESUncompressManager2(void)
    {
    }

    virtual bool add_method(const string &name, p_bes_uncompress method);
    virtual p_bes_uncompress find_method(const string &name);

    virtual bool uncompress(const string &src, string &target, BESCache2 *cache);

    virtual void dump(ostream &strm) const ;

    static BESUncompressManager2 * TheManager();
};

#endif // I_BESUncompressManager2_h
