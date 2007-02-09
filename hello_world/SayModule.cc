// SayModule.cc

#include <iostream>

using std::endl ;

#include "SayModule.h"
#include "BESRequestHandlerList.h"
#include "SayRequestHandler.h"
#include "BESLog.h"
#include "BESResponseHandlerList.h"
#include "BESResponseNames.h"
#include "BESCommand.h"
#include "SayResponseNames.h"
#include "SayResponseHandler.h"
#include "SayCommand.h"
#include "BESDebug.h"

void
SayModule::initialize( const string &modname )
{
    BESDEBUG( "Initializing Say Handler:" << endl )

    BESDEBUG( "    adding " << modname << " request handler" << endl )
    BESRequestHandlerList::TheList()->add_handler( modname, new SayRequestHandler( modname ) ) ;

    BESDEBUG( "    adding " << say_RESPONSE << " response handler" << endl )
    BESResponseHandlerList::TheList()->add_handler( say_RESPONSE, SayResponseHandler::SayResponseBuilder ) ;

    BESDEBUG( "    adding " << say_RESPONSE << " command" << endl )
    BESCommand *cmd = new SayCommand( say_RESPONSE ) ;
    BESCommand::add_command( say_RESPONSE, cmd ) ;


    // INIT_END
}

void
SayModule::terminate( const string &modname )
{
    BESDEBUG( "Removing Say Handlers" << endl )
    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ;

    BESDEBUG( "    removing " << say_RESPONSE << " response handler" << endl )
    BESResponseHandlerList::TheList()->remove_handler( say_RESPONSE ) ;

    BESDEBUG( "    removing " << say_RESPONSE << " command" << endl )
    BESCommand::del_command( say_RESPONSE ) ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new SayModule ;
    }
}

void
SayModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "SayModule::dump - ("
			     << (void *)this << ")" << endl ;
}

