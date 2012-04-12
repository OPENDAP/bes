// BESCatalogEntry.h

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

#ifndef I_BESCatalogEntry_h
#define I_BESCatalogEntry_h 1

#include <string>
#include <list>
#include <map>

using std::string ;
using std::list ;
using std::map ;

#include "BESObj.h"

class BESCatalogEntry : public BESObj
{
private:
    string			_name ;
    string			_catalog ;
    string			_size ;
    string			_mod_date ;
    string			_mod_time ;
    list<string>		_services ;
    map<string,BESCatalogEntry *>_entry_list ;
    map<string,string>		_metadata ;

    				BESCatalogEntry() {}
public:
    				BESCatalogEntry( const string &name,
						 const string &catalog ) ;
    virtual			~BESCatalogEntry( void ) ;

    virtual void		add_entry( BESCatalogEntry *entry )
				{
				    if( entry )
				    {
					_entry_list[entry->get_name()] = entry ;
				    }
				}

    virtual string		get_name() { return _name ; }
    virtual string		get_catalog() { return _catalog ; }
    virtual bool		is_collection() { return (get_count() > 0) ; }

    virtual string		get_size() { return _size ; }
    virtual void		set_size( off_t size ) ;

    virtual string		get_mod_date() { return _mod_date ; }
    virtual void		set_mod_date( const string &mod_date )
				{ _mod_date = mod_date ; }

    virtual string		get_mod_time() { return _mod_time ; }
    virtual void		set_mod_time( const string &mod_time )
				{ _mod_time = mod_time ; }

    virtual list<string>	get_service_list() { return _services ; }
    virtual void		set_service_list( list<string> &slist )
				{ _services = slist ; }

    virtual unsigned int	get_count() { return _entry_list.size() ; }

    virtual map<string,string>	get_info() { return _metadata ; }
    virtual void		add_info( const string &name,
					  const string &value )
				{
				    _metadata[name] = value ;
				}

    typedef map<string,BESCatalogEntry *>::const_iterator catalog_citer ;
    virtual catalog_citer	get_beginning_entry()
				{ return _entry_list.begin() ; }
    virtual catalog_citer	get_ending_entry()
				{ return _entry_list.end() ; }

    virtual void		dump( ostream &strm ) const ;
};

#endif // I_BESCatalogEntry_h

