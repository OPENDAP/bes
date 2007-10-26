// SampleModule.cc

#include <iostream>

using std::endl ;

#include "SampleModule.h"
#include "BESRequestHandlerList.h"
#include "SampleRequestHandler.h"
#include "BESDebug.h"
#include "BESResponseHandlerList.h"
#include "BESResponseNames.h"
#include "BESCommand.h"
#include "SampleResponseNames.h"
#include "SampleSayResponseHandler.h"
#include "SampleSayCommand.h"
#include "BESReporterList.h"
#include "SayReporter.h"

void
SampleModule::initialize( const string &modname )
{
    BESDEBUG( modname, "Initializing Sample Module " << modname << endl )

    BESDEBUG( modname, "    adding " << modname << " request handler" << endl )
    BESRequestHandlerList::TheList()->add_handler( modname, new SampleRequestHandler( modname ) ) ;

    // If new commands are needed, then let's declare this once here. If
    // not, then you can remove this line.
    string cmd_name ;

    BESDEBUG( modname, "    adding " << SAY_RESPONSE << " response handler" << endl )
    BESResponseHandlerList::TheList()->add_handler( SAY_RESPONSE, SampleSayResponseHandler::SampleSayResponseBuilder ) ;

    cmd_name = SAY_RESPONSE ;
    BESDEBUG( modname, "    adding " << cmd_name << " command" << endl )
    BESCommand *say_cmd = new SampleSayCommand( SAY_RESPONSE ) ;
    BESCommand::add_command( cmd_name, say_cmd ) ;

    BESDEBUG( "say", "    adding Say reporter" << endl )
    BESReporterList::TheList()->add_reporter( modname, new SayReporter ) ;

    // INIT_END

    BESDEBUG( modname, "    adding Sample debug context" << endl )
    BESDebug::Register( modname ) ;

    BESDEBUG( modname, "Done Initializing Sample Module " << modname << endl )
}

void
SampleModule::terminate( const string &modname )
{
    BESDEBUG( modname, "Cleaning Sample module " << modname << endl )

    BESDEBUG( modname, "    removing " << modname << " request handler" << endl )
    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ;

    // If new commands are needed, then let's declare this once here. If
    // not, then you can remove this line.
    string cmd_name ;

    BESDEBUG( modname, "    removing " << SAY_RESPONSE << " response handler" << endl )
    BESResponseHandlerList::TheList()->remove_handler( SAY_RESPONSE ) ;

    cmd_name = SAY_RESPONSE ;
    BESDEBUG( modname, "    removing " << cmd_name << " command" << endl )
    BESCommand::del_command( cmd_name ) ;

    // TERM_END
    BESDEBUG( modname, "Done Cleaning Sample module " << modname << endl )
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new SampleModule ;
    }
}

void
SampleModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "SampleModule::dump - ("
			     << (void *)this << ")" << endl ;
}

