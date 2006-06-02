// DODSCgi.cc

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

#include "DODSCgi.h"
#include "DODSFilter.h"
#include "DODSFilterTransmitter.h"
#include "DODSHandlerException.h"
#include "DODSResponseHandlerList.h"
#include "cgi_util.h"
#include "OPeNDAPDataNames.h"

/** @brief Instantiate an instance of the DODSCgi interface

    Creates a DODSFilterTransmitter to transmit the response back to the
    caller. Only the build_data_request_plan method is implemented in this
    class. All other methods are inherited from DODS.
    
    @param type data type handled by this OpenDAP CGI server
    @param df DODSFilter object built from command line arguments
    @see DODSFilter
    @see DODSFilterTransmitter
 */
DODSCgi::DODSCgi( const string &type, DODSFilter &df )
    : _type( type ),
      _df( &df )
{
    _dhi.transmit_protocol = "HTTP" ;
    _transmitter = new DODSFilterTransmitter( df ) ;
}

DODSCgi::~DODSCgi()
{
    clean() ;
    if( _transmitter ) delete _transmitter ;
    _transmitter = 0 ;
}

/** @brief Build the data request plan using the given DODSFilter

    The DODSDataHandlerInterace is built using information from the
    DODSFilter object passed to the constructor of this object. The
    constraint, data type, dataset, and action are retrieved from the
    DODSFilter to build the request plan.

    @see _DODSDataHandlerInterface
    @see DODSContainer
    @see DODSFilter
 */
void
DODSCgi::build_data_request_plan()
{
    string symbolic_name = name_path( _df->get_dataset_name() ) ;
    DODSContainer d( symbolic_name ) ;
    d.set_constraint( _df->get_ce() ) ;
    d.set_real_name ( _df->get_dataset_name() ) ;
    d.set_container_type( _type ) ;
    d.set_valid_flag( true ) ;

    _dhi.containers.push_back( d ) ;
    _dhi.action = _df->get_action() ;
    _dhi.response_handler =
	DODSResponseHandlerList::TheList()->find_handler( _df->get_action() ) ;
    if( !_dhi.response_handler )
    {
	DODSHandlerException he ;
	he.set_error_description( (string)"Improper command " + _df->get_action() ) ;
	throw he ;
    }
}

// $Log: DODSCgi.cc,v $
// Revision 1.6  2005/04/19 17:57:27  pwest
// was not setting the symbolic name on the container, which translated into missing structure name in resulting DAP object
//
// Revision 1.5  2005/04/06 20:06:24  pwest
// was not looking up response handler to handle the request type
//
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
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
