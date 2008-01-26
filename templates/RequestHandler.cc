// OPENDAP_CLASSRequestHandler.cc

#include "config.h"

#include "OPENDAP_CLASSRequestHandler.h"
#include "BESResponseHandler.h"
#include "BESResponseNames.h"
#include "OPENDAP_CLASSResponseNames.h"
#include "BESVersionInfo.h"
#include "BESTextInfo.h"
#include "BESConstraintFuncs.h"

OPENDAP_CLASSRequestHandler::OPENDAP_CLASSRequestHandler( const string &name )
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

    info->begin_tag("Handler");
    info->add_tag("name", PACKAGE_NAME);
    info->add_tag("version", PACKAGE_STRING);
    info->begin_tag("info");
    info->add_data_from_file( "OPENDAP_CLASS.Help", "OPENDAP_CLASS Help" ) ;
    info->end_tag("info");
    info->end_tag("Handler");

    return ret ;
}

void
OPENDAP_CLASSRequestHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "OPENDAP_CLASSRequestHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESRequestHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

