// DefineResponseHandler.cc

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

#include "DefineResponseHandler.h"
#include "DODSTextInfo.h"
#include "DODSParserException.h"
#include "DODSTokenizer.h"
#include "DODSDefine.h"
#include "DODSDefineList.h"
#include "OPeNDAPDataNames.h"

DefineResponseHandler::DefineResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

DefineResponseHandler::~DefineResponseHandler( )
{
}

/** @brief executes the command to create a new definition.
 *
 * A DODSDefine object is created and added to the list of definitions. If
 * a definition already exists with the given name then it is removed and
 * the new one added.
 *
 * The DODSDefine object is created using the containers, constraints,
 * attribute lists and aggregation command parsed in the parse method.
 *
 * The response object built for this command is a DODSTextInfo response
 * object. It will contain one of two possible responses:
 *
 * Successfully added definition &lt;def_name&gt;
 * <BR />
 * Successfully replaced definition &lt;def_name&gt;
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSTextInfo
 * @see DODSDefine
 * @see DODSDefineList
 */
void
DefineResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    
    string def_name = dhi.data[DEF_NAME] ;
    string def_type = "added" ;
    bool deleted = DODSDefineList::TheList()->remove_def( def_name ) ;
    if( deleted == true )
    {
	def_type = "replaced" ;
    }

    DODSDefine *dd = new DODSDefine ;
    dhi.first_container() ;
    while( dhi.container )
    {
	dd->containers.push_back( *dhi.container ) ;
	dhi.next_container() ;
    }
    dd->aggregation_command = dhi.data[AGG_CMD] ;
    dd->aggregation_handler = dhi.data[AGG_HANDLER] ;
    dhi.data[AGG_CMD] = "" ;
    dhi.data[AGG_HANDLER] = "" ;

    DODSDefineList::TheList()->add_def( def_name, dd ) ;
    string ret = (string)"Successfully " + def_type + " definition "
		 + def_name + "\n" ;
    info->add_data( ret ) ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * A DODSTextInfo response object was built in the exeucte command to inform
 * the user whether the definition was successfully added or replaced. The
 * method send_text is called on the DODSTransmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSTextInfo
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
DefineResponseHandler::transmit( DODSTransmitter *transmitter,
                                  DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *ti = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *ti, dhi ) ;
    }
}

DODSResponseHandler *
DefineResponseHandler::DefineResponseBuilder( string handler_name )
{
    return new DefineResponseHandler( handler_name ) ;
}

// $Log: DefineResponseHandler.cc,v $
// Revision 1.5  2005/03/26 00:33:33  pwest
// fixed aggregation server invoke problems. Other commands use the aggregation command but no aggregation is needed, so set aggregation to empty string when done
//
// Revision 1.4  2005/03/17 19:25:29  pwest
// string parameters changed to const references. remove_def now deletes the definition and returns true if removed or false otherwise. Added method remove_defs to remove all definitions
//
// Revision 1.3  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.2  2005/02/02 00:03:13  pwest
// ability to replace containers and definitions
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
