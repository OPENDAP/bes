// DODSInfo.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

#ifndef DODSInfo_h_
#define DODSInfo_h_ 1

#include <string>

using std::string ;

#include "DODSResponseObject.h"
#include "cgi_util.h"

/** @brief informational response object
 *
 * This class provides a means to store information about a DODS dataset, such
 * as help information and version information. The retrieval of this
 * information can be buffered until all information is retrieved, or can be
 * directly output thereby not using memory resources.
 *
 * Information is added to this response object through the add_data method
 * and then output using the print method. If the information is not buffered
 * then the information is output during the add_data processing, otherwise
 * the print method performs the output.
 *
 * This class is can not be directly created but simply provides a base class
 * implementation of DODSResponseObject for simple informational responses.
 *
 * @see DODSResponseObject
 */
class DODSInfo :public DODSResponseObject
{
private:
			DODSInfo() {}
protected:
    ostream		*_strm ;
    bool		_buffered ;
    bool		_header ;
    bool		_is_http ;
    ObjectType		_otype ;

    			DODSInfo( ObjectType type ) ;
    			DODSInfo( bool is_http,
				  ObjectType type = unknown_type ) ;

    virtual void	initialize( string key ) ;
public:
    virtual		~DODSInfo() ;

    virtual void 	add_data( const string &s ) ;
    virtual void 	add_data_from_file( const string &key,
                                            const string &name ) ;
    virtual void	add_exception( const string &type,
                                       const string &msg,
				       const string &file,
				       int line ) ;
    virtual void 	print( FILE *out ) ;
    /** @brief return whether the information is to be buffered or not.
     *
     * @return true if information is buffered, false if not
     */
    virtual bool	is_buffered() { return _buffered ; }
    /** @brief returns the type of data
     *
     * @return type of data
     */
    virtual ObjectType	type() { return _otype ; }
};

#endif // DODSInfo_h_

// $Log: DODSInfo.h,v $
// Revision 1.4  2005/04/07 19:55:17  pwest
// added add_data_from_file method to allow for information to be added from a file, for example when adding help information from a file
//
// Revision 1.3  2004/12/15 17:39:03  pwest
// Added doxygen comments
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
