// BESHTMLInfo.h

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

#ifndef BESHTMLInfo_h_
#define BESHTMLInfo_h_ 1

#include "BESInfo.h"

/** @brief represents an html formatted response object
 *
 * Uses the default add_data method, but overwrites print method in order to
 * set the mime type to html.
 *
 * @see BESInfo
 * @see DODSResponseObject
 */
class BESHTMLInfo : public BESInfo {
public:
  			BESHTMLInfo( const string &buffer_key = "",
			              ObjectType type = unknown_type ) ;
  			BESHTMLInfo( bool is_http,
			              const string &buffer_key = "",
				      ObjectType type = unknown_type ) ;
    virtual 		~BESHTMLInfo() ;

    virtual void 	add_data( const string &s ) ;
    virtual void	add_exception( const string &type,
                                       const string &msg,
				       const string &file,
				       int line ) ;
};

#endif // BESHTMLInfo_h_

// $Log: BESHTMLInfo.h,v $
// Revision 1.4  2005/04/19 17:58:52  pwest
// print of an html information object must include the header
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
