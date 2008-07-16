// CSVRequestHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Stephan Zednik <zednik@ucar.edu> and Patrick West <pwest@ucar.edu>
// and Jose Garcia <jgarcia@ucar.edu>
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
//	zednik      Stephan Zednik <zednik@ucar.edu>
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include "BESDASResponse.h"
#include "BESDDSResponse.h"
#include "BESDataDDSResponse.h"
#include "BESInfo.h"
#include "BESContainer.h"
#include "BESVersionInfo.h"
#include "BESDataNames.h"
#include "CSVRequestHandler.h"
#include "BESResponseHandler.h"
#include "BESResponseNames.h"
#include "CSVResponseNames.h"
#include "BESVersionInfo.h"
#include "BESTextInfo.h"
#include "BESDASResponse.h"
#include "BESDDSResponse.h"
#include "BESDataDDSResponse.h"
#include "DDS.h"
#include "DDS.h"
#include "DAS.h"
#include "BaseTypeFactory.h"
#include "BESConstraintFuncs.h"
#include "InternalErr.h"
#include "BESDapError.h"
#include "BESDebug.h"

#include "CSVDDS.h"
#include "CSVDAS.h"

CSVRequestHandler::CSVRequestHandler( string name )
    : BESRequestHandler( name )
{
    add_handler( DAS_RESPONSE, CSVRequestHandler::csv_build_das ) ;
    add_handler( DDS_RESPONSE, CSVRequestHandler::csv_build_dds ) ;
    add_handler( DATA_RESPONSE, CSVRequestHandler::csv_build_data ) ;
    add_handler( VERS_RESPONSE, CSVRequestHandler::csv_build_vers ) ;
    add_handler( HELP_RESPONSE, CSVRequestHandler::csv_build_help ) ;
}

CSVRequestHandler::~CSVRequestHandler()
{
}

bool
CSVRequestHandler::csv_build_das( BESDataHandlerInterface &dhi )
{
    string error ;
    bool ret = true ;
    BESResponseObject *response =
        dhi.response_handler->get_response_object() ;
    BESDASResponse *bdas = dynamic_cast < BESDASResponse * >(response) ;
    DAS *das = 0 ;
    if (bdas)
	das = bdas->get_das() ;
    else
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
  
    try
    {
	csv_read_attributes(*das, dhi.container->access());
	return ret;
    }
    catch(InternalErr &e)
    {
	BESDapError ex( e.get_error_message(), true,
		        e.get_error_code(), __FILE__, __LINE__ ) ;
	throw ex ;
    }
    catch(Error &e)
    {
	BESDapError ex( e.get_error_message(), false,
			e.get_error_code(), __FILE__, __LINE__);
	throw ex;
    }
    catch(...)
    {
	BESDapError ex( "Caught unknown error build CSV DAS response", true,
			unknown_error, __FILE__, __LINE__);
	throw ex;
    }
}

bool
CSVRequestHandler::csv_build_dds( BESDataHandlerInterface &dhi )
{
    bool ret = true ;
    BESResponseObject *response =
        dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast < BESDDSResponse * >(response);
    DDS *dds = 0 ;
    if (bdds)
	dds = bdds->get_dds();
    else
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
  
    BaseTypeFactory *factory = new BaseTypeFactory ;
    dds->set_factory(factory);
    
    try
    {
	string accessed = dhi.container->access() ;
	dds->filename( accessed ) ;
	csv_read_descriptors( *dds, accessed ) ;

	DAS das;
	csv_read_attributes(das, accessed);
	dds->transfer_attributes( &das ) ;

	BESDEBUG( "csv", "dds = " << endl << *dds << endl )
	dhi.data[POST_CONSTRAINT] = dhi.container->get_constraint();

	return ret;
  }
    catch(InternalErr &e)
    {
	BESDapError ex( e.get_error_message(), true,
			e.get_error_code(), __FILE__, __LINE__);
	throw ex;
    }
    catch(Error &e)
    {
	BESDapError ex( e.get_error_message(), false,
			e.get_error_code(), __FILE__, __LINE__);
	throw ex;
    }
    catch(...)
    {
	BESDapError ex( "Caught unknown error build CSV DDS response", true,
			unknown_error, __FILE__, __LINE__);
	throw ex;
    }
}

bool
CSVRequestHandler::csv_build_data( BESDataHandlerInterface &dhi )
{
    bool ret = true ;
    BESResponseObject *response =
        dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds =
        dynamic_cast < BESDataDDSResponse * >(response);
    DataDDS *dds = 0 ;
    if (bdds)
	dds = bdds->get_dds();
    else
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
  
    BaseTypeFactory *factory = new BaseTypeFactory ;
    dds->set_factory(factory);

    try
    {
	string accessed = dhi.container->access() ;
	dds->filename( accessed ) ;
	csv_read_descriptors(*dds, accessed);

	DAS das;
	csv_read_attributes(das, accessed);
	dds->transfer_attributes( &das ) ;

	BESDEBUG( "csv", "dds = " << endl << *dds << endl )
	dhi.data[POST_CONSTRAINT] = dhi.container->get_constraint();
	return ret;
    }
    catch(InternalErr &e)
    {
	BESDapError ex( e.get_error_message(), true,
			e.get_error_code(), __FILE__, __LINE__);
	throw ex;
    }
    catch(Error &e)
    {
	BESDapError ex( e.get_error_message(), false,
			e.get_error_code(), __FILE__, __LINE__);
	throw ex;
    }
    catch(...)
    {
	BESDapError ex( "Caught unknown error build CSV DataDDS response", true,
			unknown_error, __FILE__, __LINE__);
	throw ex;
    }
}

bool
CSVRequestHandler::csv_build_vers( BESDataHandlerInterface &dhi )
{
    bool ret = true ;

    BESResponseObject *response =
        dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast < BESVersionInfo * >(response);
    if( !info )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
  
    info->addHandlerVersion( PACKAGE_NAME, PACKAGE_VERSION ) ;
    return ret ;
}

bool
CSVRequestHandler::csv_build_help( BESDataHandlerInterface &dhi )
{
    bool ret = true ;
    BESInfo *info =
	dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());
    if( !info )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    info->begin_tag("Handler");
    info->add_tag("name", PACKAGE_NAME);
    string handles = (string) DAS_RESPONSE
        + "," + DDS_RESPONSE
        + "," + DATA_RESPONSE + "," + HELP_RESPONSE + "," + VERS_RESPONSE;
    info->add_tag("handles", handles);
    info->add_tag("version", PACKAGE_STRING);
    info->end_tag("Handler");

    return ret ;
}

void
CSVRequestHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "CSVRequestHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESRequestHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

