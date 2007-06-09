// BESUncompressManager.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef I_BESUncompressManager_h
#define I_BESUncompressManager_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "BESObj.h"

class BESCache ;

typedef string (*p_bes_uncompress)( const string &src, const string &target ) ;

/** @brief List of all registered uncompress methods
 *
 * The BESUncompressManager allows the developer to add or remove named
 * uncompression methods from the list for this server. By default a gz and
 * bz2 and Z function is provided.
 *
 * What is actually added to the list are static uncompression functions.
 * Each of these functions is responsible for uncompressing a specific type
 * of compressed file. The manager knows which type to uncompress by the
 * file extension. All extensions are converted to lower case in order to
 * make a match, so, for example, all .Z compressed files will be matched
 * with the static function registered with the name z.
 *
 * @see BESUncompressGZ
 * @see BESUncompressBZ2
 * @see BESUncompressZ
 * @see BESCache
 */
class BESUncompressManager : public BESObj
{
private:
    static BESUncompressManager *	_instance ;
    map< string, p_bes_uncompress >	_uncompress_list ;
    unsigned int			_retry ; // milleseconds
    unsigned int			_num_tries ;

    typedef map< string, p_bes_uncompress >::const_iterator UCIter ;
    typedef map< string, p_bes_uncompress >::iterator UIter ;
protected:
					BESUncompressManager(void) ;
public:
    virtual				~BESUncompressManager(void) {}

    virtual bool			add_method( const string &name,
						    p_bes_uncompress method ) ;
    virtual bool			remove_method( const string &name ) ;
    virtual p_bes_uncompress		find_method( const string &name ) ;

    virtual string			get_method_names() ;

    virtual string			uncompress( const string &src,
						    BESCache &cache ) ;

    virtual void			dump( ostream &strm ) const ;

    static BESUncompressManager *	TheManager() ;
};

#endif // I_BESUncompressManager_h

