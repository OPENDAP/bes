// OPENDAP_CLASSRequestHandler.cc

#include "OPENDAP_CLASSRequestHandler.h"
#include "DODSResponseHandler.h"
#include "DODSResponseException.h"
#include "DODSResponseNames.h"
#include "OPENDAP_CLASSResponseNames.h"
#include "DODSVersionInfo.h"
#include "config_OPENDAP_TYPE.h"
#include "DODSConstraintFuncs.h"

OPENDAP_CLASSRequestHandler::OPENDAP_CLASSRequestHandler( string name )
    : DODSRequestHandler( name )
{
    add_handler( VERS_RESPONSE, OPENDAP_CLASSRequestHandler::OPENDAP_TYPE_build_vers ) ;
    add_handler( HELP_RESPONSE, OPENDAP_CLASSRequestHandler::OPENDAP_TYPE_build_help ) ;
}

OPENDAP_CLASSRequestHandler::~OPENDAP_CLASSRequestHandler()
{
}

bool
OPENDAP_CLASSRequestHandler::OPENDAP_TYPE_build_vers( DODSDataHandlerInterface &dhi )
{
    bool ret = true ;
    DODSVersionInfo *info = dynamic_cast<DODSVersionInfo *>(dhi.response_handler->get_response_object() ) ;
    info->addHandlerVersion( PACKAGE_NAME, PACKAGE_VERSION ) ;
    return ret ;
}

bool
OPENDAP_CLASSRequestHandler::OPENDAP_TYPE_build_help( DODSDataHandlerInterface &dhi )
{
    bool ret = true ;
    DODSInfo *info = dynamic_cast<DODSInfo *>(dhi.response_handler->get_response_object());

    info->add_data( (string)PACKAGE_NAME + " help: " + PACKAGE_VERSION + "\n" );

    string key ;
    if( dhi.transmit_protocol == "HTTP" )
	key = (string)"OPENDAP_CLASS.Help." + dhi.transmit_protocol ;
    else
	key = "OPENDAP_CLASS.Help.TXT" ;
    info->add_data_from_file( key, OPENDAP_CLASS_NAME ) ;

    return ret ;
}

