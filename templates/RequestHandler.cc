// OPENDAP_CLASSRequestHandler.cc

#include "OPENDAP_CLASSRequestHandler.h"
#include "BESResponseHandler.h"
#include "BESResponseException.h"
#include "BESResponseNames.h"
#include "OPENDAP_CLASSResponseNames.h"
#include "BESVersionInfo.h"
#include "BESTextInfo.h"
#include "config_OPENDAP_TYPE.h"
#include "BESConstraintFuncs.h"

OPENDAP_CLASSRequestHandler::OPENDAP_CLASSRequestHandler( string name )
    : BESRequestHandler( name )
{
    add_handler( VERS_RESPONSE, OPENDAP_CLASSRequestHandler::OPENDAP_TYPE_build_vers ) ;
    add_handler( HELP_RESPONSE, OPENDAP_CLASSRequestHandler::OPENDAP_TYPE_build_help ) ;
}

OPENDAP_CLASSRequestHandler::~OPENDAP_CLASSRequestHandler()
{
}

bool
OPENDAP_CLASSRequestHandler::OPENDAP_TYPE_build_vers( BESDataHandlerInterface &dhi )
{
    bool ret = true ;
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object() ) ;
    info->addHandlerVersion( PACKAGE_NAME, PACKAGE_VERSION ) ;
    return ret ;
}

bool
OPENDAP_CLASSRequestHandler::OPENDAP_TYPE_build_help( BESDataHandlerInterface &dhi )
{
    bool ret = true ;
    BESInfo *info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());

    info->add_data( (string)PACKAGE_NAME + " help: " + PACKAGE_VERSION + "\n" );

    string key ;
    if( dhi.transmit_protocol == "HTTP" )
	key = (string)"OPENDAP_CLASS.Help." + dhi.transmit_protocol ;
    else
	key = "OPENDAP_CLASS.Help.TXT" ;
    info->add_data_from_file( key, OPENDAP_CLASS_NAME ) ;

    return ret ;
}

