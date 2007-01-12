// BESFilterTransmitter.cc

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

#include "BESFilterTransmitter.h"
#include "DODSFilter.h"
#include "BESInfo.h"
#include "BESDASResponse.h"
#include "BESDDSResponse.h"
#include "BESDataDDSResponse.h"
#include "BESDataNames.h"
#include "BESTransmitter.h"
#include "BESReturnManager.h"
#include "BESTransmitterNames.h"
#include "cgi_util.h"

#define DAS_TRANSMITTER "das"
#define DDS_TRANSMITTER "dds"
#define DDX_TRANSMITTER "ddx"
#define DATA_TRANSMITTER "data"

BESFilterTransmitter *BESFilterTransmitter::Transmitter = 0 ;

BESFilterTransmitter::BESFilterTransmitter( DODSFilter &df )
    : _df( &df )
{
    BESFilterTransmitter::Transmitter = this ;

    add_method( DAS_TRANSMITTER, BESFilterTransmitter::send_basic_das ) ;
    add_method( DDS_TRANSMITTER, BESFilterTransmitter::send_basic_dds ) ;
    add_method( DDX_TRANSMITTER, BESFilterTransmitter::send_basic_ddx ) ;
    add_method( DATA_TRANSMITTER, BESFilterTransmitter::send_basic_data);
}

void
BESFilterTransmitter::send_text( BESInfo &info,
                                 BESDataHandlerInterface & )
{
    if( info.is_buffered() )
    {
	set_mime_text( stdout, unknown_type ) ;
	info.print( stdout ) ;
    }
}

void
BESFilterTransmitter::send_html( BESInfo &info,
                                 BESDataHandlerInterface & )
{
    if( info.is_buffered() )
    {
	set_mime_html( stdout, unknown_type ) ;
	info.print( stdout ) ;
    }
}

void
BESFilterTransmitter::send_basic_das( BESResponseObject *obj,
				      BESDataHandlerInterface &dhi )
{
    BESDASResponse *bdas = dynamic_cast < BESDASResponse * >(obj);
    DAS *das = bdas->get_das();
    BESFilterTransmitter::Transmitter->get_filter()->send_das( stdout, *das ) ;
}

void
BESFilterTransmitter::send_basic_dds( BESResponseObject *obj,
				      BESDataHandlerInterface &dhi )
{
    BESDDSResponse *bdds = dynamic_cast < BESDDSResponse * >(obj);
    DDS *dds = bdds->get_dds();
    ConstraintEvaluator & ce = bdds->get_ce();
    dhi.first_container();
    BESFilterTransmitter::Transmitter->get_filter()->set_ce( dhi.data[POST_CONSTRAINT] ) ;
    BESFilterTransmitter::Transmitter->get_filter()->send_dds( stdout, *dds, ce, true ) ;
}

void
BESFilterTransmitter::send_basic_data( BESResponseObject *obj,
				       BESDataHandlerInterface &dhi )
{
    BESDataDDSResponse *bdds = dynamic_cast < BESDataDDSResponse * >(obj);
    DataDDS *dds = bdds->get_dds();
    ConstraintEvaluator & ce = bdds->get_ce();
    dhi.first_container();
    BESFilterTransmitter::Transmitter->get_filter()->set_ce( dhi.data[POST_CONSTRAINT] ) ;
    BESFilterTransmitter::Transmitter->get_filter()->send_data( *dds, ce, stdout ) ;
}

void
BESFilterTransmitter::send_basic_ddx( BESResponseObject *obj,
				      BESDataHandlerInterface &dhi )
{
    BESDDSResponse *bdds = dynamic_cast < BESDDSResponse * >(obj);
    DDS *dds = bdds->get_dds();
    ConstraintEvaluator &ce = bdds->get_ce();
    dhi.first_container();
    BESFilterTransmitter::Transmitter->get_filter()->set_ce( dhi.data[POST_CONSTRAINT] ) ;
    BESFilterTransmitter::Transmitter->get_filter()->send_ddx( *dds, ce, stdout ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance and calls dump on parent
 * class
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESFilterTransmitter::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESFilterTransmitter::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESTransmitter::dump( strm ) ;
    BESIndent::UnIndent() ;
}

