// OPeNDAPAggFactory.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org> and Jose Garcia <jgarcia@ucar.org>
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

#ifndef I_OPeNDAPAggFactory_h
#define I_OPeNDAPAggFactory_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

class DODSAggregationServer ;

typedef DODSAggregationServer * (*p_agg_handler)( string name ) ;

/** @brief List of all registered aggregation handlers for this server
 *
 * A OPeNDAPAggFactory allows the developer to add or remove aggregation
 * handlers from the list of handlers available for this server.
 *
 * @see 
 */
class OPeNDAPAggFactory {
private:
    static OPeNDAPAggFactory *	_instance ;

    map< string, p_agg_handler > _handler_list ;
protected:
				OPeNDAPAggFactory(void) {}
public:
    virtual			~OPeNDAPAggFactory(void) {}

    typedef map< string, p_agg_handler >::const_iterator Handler_citer ;
    typedef map< string, p_agg_handler >::iterator Handler_iter ;

    virtual bool		add_handler( string handler_name,
					   p_agg_handler handler_method ) ;
    virtual bool		remove_handler( string handler_name ) ;
    virtual DODSAggregationServer *find_handler( string handler_name ) ;

    virtual string		get_handler_names() ;

    static OPeNDAPAggFactory *	TheFactory() ;
};

#endif // I_OPeNDAPAggFactory_h

// $Log: OPeNDAPAggFactory.h,v $
