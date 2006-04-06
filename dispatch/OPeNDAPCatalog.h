// OPeNDAPCatalog.h

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

#ifndef I_OPeNDAPCatalog_h
#define I_OPeNDAPCatalog_h 1

#include <string>

using std::string ;

class DODSInfo ;

/** @brief abstract base class catalog object. Derived classes know how to
 * show nodes and leaves in a catalog.
 */
class OPeNDAPCatalog {
protected:
				OPeNDAPCatalog( ) {} ;
public:
    virtual			~OPeNDAPCatalog( void ) {} ;

    virtual bool		show_catalog( const string &container,
					      const string &catalog_or_info,
					      DODSInfo *info ) = 0 ;
};

#endif // I_OPeNDAPCatalog_h

// $Log: OPeNDAPCatalog.h,v $
