// OPENDAP_CLASSRequestHandler.cc

#include "config.h"

#include "OPENDAP_CLASSRequestHandler.h"
#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include "OPENDAP_CLASSResponseNames.h"
#include <BESVersionInfo.h>
#include <BESTextInfo.h>
#include <BESConstraintFuncs.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>

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
    info->add_module( PACKAGE_NAME, PACKAGE_VERSION ) ;
    return ret ;
}

bool
OPENDAP_CLASSRequestHandler::OPENDAP_TYPE_build_help( BESDataHandlerInterface &dhi )
{
    bool ret = true ;
    BESInfo *info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map<string,string> attrs ;
    attrs["name"] = PACKAGE_NAME ;
    attrs["version"] = PACKAGE_VERSION ;
    list<string> services ;
    BESServiceRegistry::TheRegistry()->services_handled( OPENDAP_CLASS_NAME, services );
    if( services.size() > 0 )
    {
	string handles = BESUtil::implode( services, ',' ) ;
	attrs["handles"] = handles ;
    }
    info->begin_tag( "module", &attrs ) ;
    //info->add_data_from_file( "OPENDAP_CLASS.Help", "OPENDAP_CLASS Help" ) ;
    info->end_tag( "module" ) ;

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

