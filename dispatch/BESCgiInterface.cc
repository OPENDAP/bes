// BESCgiInterface.cc

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

#include "BESCgiInterface.h"
#include "DODSFilter.h"
#include "BESFilterTransmitter.h"
#include "BESHandlerException.h"
#include "BESResponseHandlerList.h"
#include "cgi_util.h"
#include "BESDataNames.h"

/** @brief Instantiate an instance of the BESCgiInterface interface

    Creates a BESFilterTransmitter to transmit the response back to the
    caller. Only the build_data_request_plan method is implemented in this
    class. All other methods are inherited from BESInterface.
    
    @param type data type handled by this OPeNDAP CGI server
    @param df DODSFilter object built from command line arguments
    @see DODSFilter
    @see BESFilterTransmitter
 */
BESCgiInterface::BESCgiInterface( const string &type, DODSFilter &df )
    : _type( type ),
      _df( &df )
{
    _dhi.transmit_protocol = "HTTP" ;
    _transmitter = new BESFilterTransmitter( df ) ;
}

BESCgiInterface::~BESCgiInterface()
{
    clean() ;
    if( _transmitter ) delete _transmitter ;
    _transmitter = 0 ;
}

/** @brief Build the data request plan using the given DODSFilter

    The BESDSDataHandlerInterace is built using information from the
    DODSFilter object passed to the constructor of this object. The
    constraint, data type, dataset, and action are retrieved from the
    DODSFilter to build the request plan.

    @see _BESDataHandlerInterface
    @see BESContainer
    @see DODSFilter
 */
void
BESCgiInterface::build_data_request_plan()
{
    string symbolic_name = name_path( _df->get_dataset_name() ) ;
    BESContainer d( symbolic_name ) ;
    d.set_constraint( _df->get_ce() ) ;
    d.set_real_name ( _df->get_dataset_name() ) ;
    d.set_container_type( _type ) ;
    d.set_valid_flag( true ) ;

    _dhi.containers.push_back( d ) ;
    string myaction = (string)"get." + _df->get_action() ; 
    _dhi.action = myaction ;
    _dhi.response_handler =
	BESResponseHandlerList::TheList()->find_handler( myaction ) ;
    if( !_dhi.response_handler )
    {
	string s = (string)"Improper command " + myaction ;
	throw BESHandlerException( s, __FILE__, __LINE__ ) ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about the
 * DODSFilter and the type of data handled by this interface.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESCgiInterface::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESCgiInterface::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "data type: " << _type << endl ;
    strm << BESIndent::LMarg << "action: get." << _df->get_action() << endl ;
    strm << BESIndent::LMarg << "dataset name: "
			     << _df->get_dataset_name() << endl ;
    strm << BESIndent::LMarg << "constraint expression: "
			     << _df->get_ce() << endl ;
    BESIndent::UnIndent() ;
}

