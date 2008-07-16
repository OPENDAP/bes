// SampleRequestHandler.cc

#include "config.h"

#include "SampleRequestHandler.h"
#include "BESResponseHandler.h"
#include "BESResponseNames.h"
#include "SampleResponseNames.h"
#include "BESVersionInfo.h"
#include "BESTextInfo.h"
#include "BESConstraintFuncs.h"
#include "BESInternalError.h"

SampleRequestHandler::SampleRequestHandler( const string &name )
    : BESRequestHandler( name )
{
    add_handler( VERS_RESPONSE, SampleRequestHandler::sample_build_vers ) ;
    add_handler( HELP_RESPONSE, SampleRequestHandler::sample_build_help ) ;
}

SampleRequestHandler::~SampleRequestHandler()
{
}

bool
SampleRequestHandler::sample_build_vers( BESDataHandlerInterface &dhi )
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
SampleRequestHandler::sample_build_help( BESDataHandlerInterface &dhi )
{
    bool ret = true ;

    BESResponseObject *response =
        dhi.response_handler->get_response_object() ;
    BESInfo *info = dynamic_cast< BESInfo * >( response ) ;
    if( !info )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    info->begin_tag("Handler");
    info->add_tag("name", PACKAGE_NAME);
    info->add_tag("version", PACKAGE_STRING);
    info->begin_tag("info");
    info->add_data_from_file( "Sample.Help", "Sample Help" ) ;
    info->end_tag("info");
    info->end_tag("Handler");

    return ret ;
}

void
SampleRequestHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "SampleRequestHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESRequestHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

