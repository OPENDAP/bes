// BESXMLCommand.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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

#ifndef A_BESXMLCommand_h
#define A_BESXMLCommand_h 1

#include <string>
#include <map>

using std::string ;
using std::map ;

#include <libxml/encoding.h>

#include "BESObj.h"
#include "BESDataHandlerInterface.h"

class BESResponseHandler ;
class BESXMLCommand ;
typedef BESXMLCommand *(*p_xmlcmd_builder)( const BESDataHandlerInterface &dhi);

class BESXMLCommand : public BESObj
{
private:
    static map< string, p_xmlcmd_builder > cmd_list ;
    typedef map< string, p_xmlcmd_builder >::iterator cmd_iter ;
protected:
    BESDataHandlerInterface	_dhi ;
    virtual void		set_response() ;
    string			_str_cmd ;
    				BESXMLCommand( const BESDataHandlerInterface &base_dhi ) ;
public:
    virtual			~BESXMLCommand() {}

    virtual void		parse_request( xmlNode *node ) = 0 ;
    virtual bool		has_response() = 0 ;
    virtual void		prep_request() {}
    virtual BESDataHandlerInterface &get_dhi() { return _dhi ; }

    virtual void		dump( ostream &strm ) const ;

    static void			add_command( const string &cmd_str,
					     p_xmlcmd_builder cmd ) ;
    static bool			del_command( const string &cmd_str ) ;
    static p_xmlcmd_builder	find_command( const string &cmd_str ) ;
} ;

#endif // A_BESXMLCommand_h

