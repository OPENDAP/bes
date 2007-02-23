// CSVRequestHandler.cc

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
#include "BESResponseException.h"
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
#include "BESHandlerException.h"
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
  string error;
  bool ret = true ;
    BESResponseObject *response =
        dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast < BESDASResponse * >(response);
    DAS *das = bdas->get_das();
  
  try {
    csv_read_attributes(*das, dhi.container->access());
    return ret;
  } catch(Error &e) {
    BESHandlerException ex("unknown exception caught building DataDDS", __FILE__, __LINE__);
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
    DDS *dds = bdds->get_dds();
    BaseTypeFactory *factory = new BaseTypeFactory ;
    dds->set_factory(factory);
    
    try {
      csv_read_descriptors(*dds, dhi.container->access());
      BESDEBUG( "dds = " << endl << *dds << endl )
      dhi.data[POST_CONSTRAINT] = dhi.container->get_constraint();
      return ret;
    } catch(Error &e) {
      BESHandlerException ex("unknown exception caught building DDS", __FILE__, __LINE__);
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
    DataDDS *dds = bdds->get_dds();
    BaseTypeFactory *factory = new BaseTypeFactory ;
    dds->set_factory(factory);

    try {
      csv_read_descriptors(*dds, dhi.container->access());
      dhi.data[POST_CONSTRAINT] = dhi.container->get_constraint();
      return ret;
    } catch(Error &e) {
      BESHandlerException ex("unknown exception caught building DataDDS", __FILE__, __LINE__);
      throw ex;
    }
}

bool
CSVRequestHandler::csv_build_vers( BESDataHandlerInterface &dhi )
{
    bool ret = true ;
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object() ) ;
    info->addHandlerVersion( PACKAGE_NAME, PACKAGE_VERSION ) ;
    return ret ;
}

bool
CSVRequestHandler::csv_build_help( BESDataHandlerInterface &dhi )
{
    bool ret = true ;
    BESInfo *info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());

    info->add_data( (string)PACKAGE_NAME + " help: " + PACKAGE_VERSION + "\n" );

    string key ;
    if( dhi.transmit_protocol == "HTTP" )
	key = (string)"CSV.Help." + dhi.transmit_protocol ;
    else
	key = "CSV.Help.TXT" ;
    info->add_data_from_file( key, CSV_NAME ) ;

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

