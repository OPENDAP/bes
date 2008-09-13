// BESDataHandlerInterface.cc

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

#include "BESDataHandlerInterface.h"
#include "BESContainer.h"
#include "BESResponseHandler.h"
#include "BESInfo.h"
#include "BESIndent.h"

BESResponseObject *
BESDataHandlerInterface::get_response_object()
{
    BESResponseObject *response = 0 ;

    if( response_handler )
    {
	response = response_handler->get_response_object() ;
    }
    return response ;
}

void
BESDataHandlerInterface::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDataHandlerInterface::dump" << endl ;
    BESIndent::Indent() ;
    if( response_handler )
    {
	strm << BESIndent::LMarg << "response handler:" << endl ;
	BESIndent::Indent() ;
	response_handler->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "response handler: not set" << endl ;
    }

    if( container )
    {
	strm << BESIndent::LMarg << "current container:" << endl ;
	BESIndent::Indent() ;
	container->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "current container: not set" << endl ;
    }

    if( containers.size() )
    {
	strm << BESIndent::LMarg << "container list:" << endl ;
	BESIndent::Indent() ;
	list<BESContainer *>::const_iterator i = containers.begin() ;
	list<BESContainer *>::const_iterator ie = containers.end() ;
	for( ; i != ie; i++ )
	{
	    (*i)->dump( strm ) ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "container list: empty" << endl ;
    }

    strm << BESIndent::LMarg << "action: " << action << endl ;
    strm << BESIndent::LMarg << "action name: " << action_name << endl ;
    strm << BESIndent::LMarg << "transmit protocol: " << transmit_protocol << endl ;
    if( data.size() )
    {
	strm << BESIndent::LMarg << "data:" << endl ;
	BESIndent::Indent() ;
	data_citer i = data.begin() ;
	data_citer ie = data.end() ;
	for( ; i != ie; i++ )
	{
	    strm << BESIndent::LMarg << (*i).first << ": "
				     << (*i).second << endl ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "data: none" << endl ;
    }

    if( error_info )
    {
	strm << BESIndent::LMarg << "error info:" << endl ;
	BESIndent::Indent() ;
	error_info->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "error info: null" << endl ;
    }
    BESIndent::UnIndent() ;
}

