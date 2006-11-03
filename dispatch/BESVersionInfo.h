// BESVersionInfo.h

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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef BESVersionInfo_h_
#define BESVersionInfo_h_ 1

#include "BESInfo.h"

/** brief represents simple text information in a response object, such as
 * version and help inforamtion.
 *
 * Uses the default add_data and print methods, where the print method, if the
 * response is going to a browser, sets the mime type to text.
 *
 * @see BESXMLInfo
 * @see BESResponseObject
 */
class BESVersionInfo : public BESInfo {
private:
    bool		_indap ;
    bool		_inbes ;
    bool		_inhandler ;
    BESInfo *		_info ;
public:
  			BESVersionInfo() ;
    virtual 		~BESVersionInfo() ;

    virtual void	beginBESVersion( ) ;
    virtual void	addBESVersion( const string &n, const string &v ) ;
    virtual void	endBESVersion( ) ;

    virtual void	beginHandlerVersion( ) ;
    virtual void	addHandlerVersion( const string &n, const string &v ) ;
    virtual void	endHandlerVersion( ) ;

    virtual void	begin_response( const string &response_name )
			{
			    _info->begin_response( response_name ) ;
			}
    virtual void	end_response( )
			{
			    _info->end_response( ) ;
			}

    virtual void	add_tag( const string &tag_name,
                                 const string &tag_data,
				 map<string,string> *attrs = 0 )
			{
			    _info->add_tag( tag_name, tag_data, attrs ) ;
			}
    virtual void	begin_tag( const string &tag_name,
                                   map<string,string> *attrs = 0 )
			{
			    _info->begin_tag( tag_name, attrs ) ;
			}
    virtual void	end_tag( const string &tag_name )
			{
			    _info->end_tag( tag_name ) ;
			}

    virtual void 	add_data( const string &s )
			{
			    _info->add_data( s ) ;
			}
    virtual void	add_space( unsigned long num_spaces )
			{
			    _info->add_space( num_spaces ) ;
			}
    virtual void	add_break( unsigned long num_breaks )
			{
			    _info->add_break( num_breaks ) ;
			}
    virtual void 	add_data_from_file( const string &key,
                                            const string &name )
			{
			    _info->add_data_from_file( key, name ) ;
			}
    virtual void	add_exception( const string &type, BESException &e )
			{
			    _info->add_exception( type, e ) ;
			}
    virtual void	transmit( BESTransmitter *transmitter,
				  BESDataHandlerInterface &dhi )
			{
			    _info->transmit( transmitter, dhi ) ;
			}
    virtual void 	print( FILE *out )
			{
			    _info->print( out ) ;
			}
};

#endif // BESVersionInfo_h_

