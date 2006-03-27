#include <iostream>
#include <string>
#include <map>

using std::cout ;
using std::endl ;
using std::map ;
using std::string ;

#include "OPeNDAPPluginFactory.h"
#include "OPeNDAPAbstractModule.h"
#include "OPeNDAPPluginException.h"
#include "TheDODSKeys.h"
#include "DODSResponseHandlerList.h"
#include "DODSResponseHandler.h"
#include "DODSDataHandlerInterface.h"
#include "DODSException.h"

int
main( int argc, char **argv )
{
    OPeNDAPPluginFactory<OPeNDAPAbstractModule> moduleFactory ;

    map< string, string > module_list ;
    bool found = false ;
    string mods = TheDODSKeys::TheKeys()->get_key( "OPeNDAP.modules", found ) ;
    if( mods != "" )
    {
	cout << "mods to load = " << mods << endl ;
	std::string::size_type start = 0 ;
	std::string::size_type comma = 0 ;
	bool done = false ;
	while( !done )
	{
	    string mod ;
	    comma = mods.find( ',', start ) ;
	    if( comma == string::npos )
	    {
		mod = mods.substr( start, mods.length() - start ) ;
		done = true ;
	    }
	    else
	    {
		mod = mods.substr( start, comma - start ) ;
	    }
	    string key = "OPeNDAP.module." + mod ;
	    string so = TheDODSKeys::TheKeys()->get_key( key, found ) ;
	    if( so == "" )
	    {
		cerr << "couldn't find the module for " << mod << endl ;
		return 1 ;
	    }
	    module_list[mod] = so ;

	    start = comma + 1 ;
	}

	map< string, string >::iterator i = module_list.begin() ;
	map< string, string >::iterator e = module_list.end() ;
	for( ; i != e; i++ )
	{
	    moduleFactory.add_mapping( (*i).first, (*i).second ) ;
	}

	try
	{
	    for( i = module_list.begin(); i != e; i++ )
	    {
		OPeNDAPAbstractModule *o = moduleFactory.get( (*i).first ) ;
		o->initialize() ;
	    }
	}
	catch( OPeNDAPPluginException &e )
	{
	    cout << "Caught exception during initialize: "
	         << e.get_error_description() << endl ;
	}
	catch( ... )
	{
	    cout << "Caught unknown exception during initialize" << endl ;
	}

	try
	{
	    cout << "registered handlers = " << DODSResponseHandlerList::TheList()->get_handler_names() << endl ;
	}
	catch( DODSException &e )
	{
	    cout << "Caught exception during response test: " << e.get_error_description() << endl ;
	}
	catch( ... )
	{
	    cout << "Caught unknown exception during response test" << endl ;
	}

	try
	{
	    for( i = module_list.begin(); i != e; i++ )
	    {
		OPeNDAPAbstractModule *o = moduleFactory.get( (*i).first ) ;
		o->terminate() ;
	    }
	}
	catch( OPeNDAPPluginException &e )
	{
	    cout << "Caught exception during terminate: "
	         << e.get_error_description() << endl ;
	}
	catch( ... )
	{
	    cout << "Caught unknown exception during terminate" << endl ;
	}
    }
    else
    {
	cout << "no modules to load" << endl ;
    }

    return 0 ;
}

