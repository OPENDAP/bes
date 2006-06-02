// DODSConstraintFuncs.cc

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

#include "DODSConstraintFuncs.h"
#include "OPeNDAPDataNames.h"

string
DODSConstraintFuncs::pre_to_post_constraint( const string &name,
					     const string &pre_constraint )
{
    string str = pre_constraint ;
    string new_name = name ;
    new_name.append( "." ) ;
    if( str != "" )
    {
	str.insert( 0, new_name ) ;
	int pos = 0 ;
	pos = str.find( ',', pos ) ;
	while( pos != -1 )
	{
	    pos++ ;
	    str.insert( pos, new_name ) ;
	    pos = str.find( ',', pos ) ;
	}
    }

    return str ;
}

void
DODSConstraintFuncs::post_append( DODSDataHandlerInterface &dhi )
{
    if( dhi.container && dhi.container->get_constraint() != "" )
    {
	string to_append =
	    pre_to_post_constraint( dhi.container->get_symbolic_name(),
				    dhi.container->get_constraint() ) ;
	string constraint = dhi.data[POST_CONSTRAINT] ;
	if( constraint != "" )
	    constraint += "," ;
	constraint.append( to_append ) ;
	dhi.data[POST_CONSTRAINT] = constraint ;
    }
}

// $Log: DODSConstraintFuncs.cc,v $
// Revision 1.3  2005/02/02 20:12:18  pwest
// bug appending constraints to post_constraint
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
