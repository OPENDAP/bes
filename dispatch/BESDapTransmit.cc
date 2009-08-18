// BESDapTransmit.cc

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

#include <sstream>

using std::ostringstream;

#include "BESDapTransmit.h"
#include "DODSFilter.h"
#include "BESContainer.h"
#include "BESDapNames.h"
#include "BESDataNames.h"
#include "BESResponseNames.h"
#include "cgi_util.h"
#include "BESDASResponse.h"
#include "BESDDSResponse.h"
#include "BESDataDDSResponse.h"
#include "BESContextManager.h"
#include "BESDapError.h"
#include "BESInternalFatalError.h"
#include "Error.h"

BESDapTransmit::BESDapTransmit()
    : BESBasicTransmitter()
{
    add_method( DAS_SERVICE, BESDapTransmit::send_basic_das ) ;
    add_method( DDS_SERVICE, BESDapTransmit::send_basic_dds ) ;
    add_method( DDX_SERVICE, BESDapTransmit::send_basic_ddx ) ;
    add_method( DATA_SERVICE, BESDapTransmit::send_basic_data ) ;
    add_method( DATADDX_SERVICE, BESDapTransmit::send_basic_dataddx ) ;
}

void
BESDapTransmit::send_basic_das(BESResponseObject * obj,
                               BESDataHandlerInterface & dhi)
{
    BESDASResponse *bdas = dynamic_cast < BESDASResponse * >(obj);
    if( !bdas )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
    DAS *das = bdas->get_das();
    dhi.first_container();

    bool found = false ;
    string context = "transmit_protocol" ;
    string protocol = BESContextManager::TheManager()->get_context( context,
								    found ) ;
    bool print_mime = false ;
    if( protocol == "HTTP" ) print_mime = true ;

    try
    {
        DODSFilter df ;
        df.set_dataset_name( dhi.container->get_real_name() ) ;
        df.send_das( dhi.get_output_stream(), *das, "", print_mime ) ;
    }
    catch( InternalErr &e )
    {
        string err = "libdap error transmitting DAS: "
            + e.get_error_message() ;
        throw BESDapError( err, true, e.get_error_code(), __FILE__, __LINE__ ) ;
    }
    catch( Error &e )
    {
        string err = "libdap error transmitting DAS: "
            + e.get_error_message() ;
        throw BESDapError( err, false, e.get_error_code(), __FILE__, __LINE__ );
    }
    catch(...)
    {
        string s = "unknown error caught transmitting DAS" ;
        BESInternalFatalError ex( s, __FILE__, __LINE__ ) ;
        throw ex;
    }
}

void BESDapTransmit::send_basic_dds(BESResponseObject * obj,
                                    BESDataHandlerInterface & dhi)
{
    BESDDSResponse *bdds = dynamic_cast < BESDDSResponse * >(obj);
    if( !bdds )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
    DDS *dds = bdds->get_dds();
    ConstraintEvaluator & ce = bdds->get_ce();
    dhi.first_container();

    bool found = false ;
    string context = "transmit_protocol" ;
    string protocol = BESContextManager::TheManager()->get_context( context,
								    found ) ;
    bool print_mime = false ;
    if( protocol == "HTTP" ) print_mime = true ;

    try {
        DODSFilter df;
        df.set_dataset_name(dhi.container->get_real_name());
        df.set_ce(dhi.data[POST_CONSTRAINT]);
        df.send_dds(dhi.get_output_stream(), *dds, ce, true, "", print_mime);
    }
    catch( InternalErr &e )
    {
        string err = "libdap error transmitting DDS: "
            + e.get_error_message() ;
        throw BESDapError( err, true, e.get_error_code(), __FILE__, __LINE__ ) ;
    }
    catch( Error &e )
    {
        string err = "libdap error transmitting DDS: "
            + e.get_error_message() ;
        throw BESDapError( err, false, e.get_error_code(), __FILE__, __LINE__ );
    }
    catch(...)
    {
        string s = "unknown error caught transmitting DDS" ;
        BESInternalFatalError ex( s, __FILE__, __LINE__ ) ;
        throw ex;
    }
}

void BESDapTransmit::send_basic_data(BESResponseObject * obj,
                                     BESDataHandlerInterface & dhi)
{
    BESDataDDSResponse *bdds = dynamic_cast < BESDataDDSResponse * >(obj);
    if( !bdds )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
    DataDDS *dds = bdds->get_dds();
    ConstraintEvaluator & ce = bdds->get_ce();
    dhi.first_container();

    bool found = false ;
    string context = "transmit_protocol" ;
    string protocol = BESContextManager::TheManager()->get_context( context,
								    found ) ;
    bool print_mime = false ;
    if( protocol == "HTTP" ) print_mime = true ;

    try {
        DODSFilter df;
        df.set_dataset_name(dds->filename());
        df.set_ce(dhi.data[POST_CONSTRAINT]);
        df.send_data(*dds, ce, dhi.get_output_stream(), "", print_mime);
    }
    catch( InternalErr &e )
    {
        string err = "libdap error transmitting DataDDS: "
            + e.get_error_message() ;
        throw BESDapError( err, true, e.get_error_code(), __FILE__, __LINE__ ) ;
    }
    catch( Error &e )
    {
        string err = "libdap error transmitting DataDDS: "
            + e.get_error_message() ;
        throw BESDapError( err, false, e.get_error_code(), __FILE__, __LINE__ );
    }
    catch(...)
    {
        string s = "unknown error caught transmitting DataDDS" ;
        BESInternalFatalError ex( s, __FILE__, __LINE__ ) ;
        throw ex;
    }
}

void BESDapTransmit::send_basic_ddx(BESResponseObject * obj,
                                    BESDataHandlerInterface & dhi)
{
    BESDDSResponse *bdds = dynamic_cast < BESDDSResponse * >(obj);
    if( !bdds )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
    DDS *dds = bdds->get_dds();
    ConstraintEvaluator & ce = bdds->get_ce();
    dhi.first_container();

    bool found = false ;
    string context = "transmit_protocol" ;
    string protocol = BESContextManager::TheManager()->get_context( context,
								    found ) ;
    bool print_mime = false ;
    if( protocol == "HTTP" ) print_mime = true ;

    try {
        DODSFilter df;
        df.set_dataset_name(dhi.container->get_real_name());
        df.set_ce(dhi.data[POST_CONSTRAINT]);
        df.send_ddx(*dds, ce, dhi.get_output_stream(), print_mime);
    }
    catch( InternalErr &e )
    {
        string err = "libdap error transmitting DDX: "
            + e.get_error_message() ;
        throw BESDapError( err, true, e.get_error_code(), __FILE__, __LINE__ ) ;
    }
    catch( Error &e )
    {
        string err = "libdap error transmitting DDX: "
            + e.get_error_message() ;
        throw BESDapError( err, false, e.get_error_code(), __FILE__, __LINE__ );
    }
    catch(...)
    {
        string s = "unknown error caught transmitting DDX" ;
        BESInternalFatalError ex( s, __FILE__, __LINE__ ) ;
        throw ex;
    }
}

void BESDapTransmit::send_basic_dataddx(BESResponseObject * obj,
                                        BESDataHandlerInterface & dhi)
{
    BESDataDDSResponse *bdds = dynamic_cast < BESDataDDSResponse * >(obj);
    if( !bdds )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
    DataDDS *dds = bdds->get_dds();
    ConstraintEvaluator & ce = bdds->get_ce();
    dhi.first_container();

    bool found = false ;
    string context = "transmit_protocol" ;
    string protocol = BESContextManager::TheManager()->get_context( context,
								    found ) ;
    bool print_mime = false ;
    if( protocol == "HTTP" ) print_mime = true ;

    try {
        DODSFilter df;
        df.set_dataset_name(dds->filename());
        df.set_ce(dhi.data[POST_CONSTRAINT]);
	//FIXME: new DODSFilter function
        df.send_data(*dds, ce, dhi.get_output_stream(), "", print_mime);
    }
    catch( InternalErr &e )
    {
        string err = "libdap error transmitting DataDDS: "
            + e.get_error_message() ;
        throw BESDapError( err, true, e.get_error_code(), __FILE__, __LINE__ ) ;
    }
    catch( Error &e )
    {
        string err = "libdap error transmitting DataDDS: "
            + e.get_error_message() ;
        throw BESDapError( err, false, e.get_error_code(), __FILE__, __LINE__ );
    }
    catch(...)
    {
        string s = "unknown error caught transmitting DataDDS" ;
        BESInternalFatalError ex( s, __FILE__, __LINE__ ) ;
        throw ex;
    }
}

