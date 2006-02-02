// CatalogResponseHandler.cc

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

#include "CatalogResponseHandler.h"
#include "DODSXMLInfo.h"
#include "cgi_util.h"
#include "DODSRequestHandlerList.h"
#include "DODSRequestHandler.h"
#include "OPeNDAPDataNames.h"
#include "DODSResponseNames.h"
#include "CatalogList.h"

CatalogResponseHandler::CatalogResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

CatalogResponseHandler::~CatalogResponseHandler( )
{
}

/** @brief executes the command 'show nodes|leaves [for &lt;node&gt;];' by
 * returning nodes or leaves at the top level or at the specified node.
 *
 * The response object DODSXMLInfo is created to store the information.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSXMLInfo
 * @see DODSRequestHandlerList
 */
void
CatalogResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSXMLInfo *info = new DODSXMLInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    string container = dhi.data[CONTAINER] ;
    string coi = dhi.data[CATALOG_OR_INFO] ;
    if( coi == CATALOG_RESPONSE )
    {
	info->add_data( "<showCatalog>\n" ) ;
    }
    else
    {
	info->add_data( "<showInfo>\n" ) ;
    }
    info->add_data( "    <response>\n" ) ;
    CatalogList::TheCatalogList()->show_catalog( container, coi, info ) ;
    info->add_data( "    </response>\n" ) ;
    if( coi == CATALOG_RESPONSE )
    {
	info->add_data( "</showCatalog>\n" ) ;
    }
    else
    {
	info->add_data( "</showInfo>\n" ) ;
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSXMLInfo
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
CatalogResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSXMLInfo *info = dynamic_cast<DODSXMLInfo *>(_response) ;
	transmitter->send_text( *info, dhi ) ;
    }
}

DODSResponseHandler *
CatalogResponseHandler::CatalogResponseBuilder( string handler_name )
{
    return new CatalogResponseHandler( handler_name ) ;
}

// $Log: CatalogResponseHandler.cc,v $
