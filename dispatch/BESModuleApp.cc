// BESModuleApp.C

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <iostream>

using std::cerr ;
using std::endl ;

#include "BESModuleApp.h"
#include "BESException.h"
#include "BESPluginFactory.h"
#include "BESAbstractModule.h"
#include "TheBESKeys.h"

/** @brief Default constructor
 *
 * Initialized the static _the Applicatioon to point to this application
 * object
 */
BESModuleApp::
BESModuleApp(void) : BESBaseApp()
{
}

/** @brief Default destructor
 *
 * sets the static _theApplicaiton to null. Does not call terminate. It is up
 * to the main method to call the terminate method.
 */
BESModuleApp::
~BESModuleApp(void)
{
}

/** @brief initializes the OPeNDAP BES application
 *
 * Loads and initializes any OPeNDAP modules
 *
 * @return 0 if successful and not 0 otherwise
 * @param argC argc value passed to the main function
 * @param argV argv value passed to the main function
 */
int BESModuleApp::
initialize(int argC, char **argV)
{
    int retVal = BESBaseApp::initialize( argC, argV ) ;
    if( !retVal )
    {
	try
	{
	    retVal = loadModules() ;
	}
	catch( BESException &e )
	{
	    string newerr = "Error during module initialization: " ;
	    newerr += e.get_message() ;
	    cerr << newerr << endl ;
	    retVal = 1 ;
	}
	catch( ... )
	{
	    string newerr = "Error during module initialization: " ;
	    newerr += "caught unknown exception" ;
	    cerr << newerr << endl ;
	    retVal = 1 ;
	}
    }

    return retVal ;
}

/** @brief load data handler modules using the initialization file
 */
int
BESModuleApp::loadModules()
{
    int retVal = 0 ;

    bool found = false ;
    string mods = TheBESKeys::TheKeys()->get_key( "OPeNDAP.modules", found ) ;
    if( mods != "" )
    {
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
	    string so = TheBESKeys::TheKeys()->get_key( key, found ) ;
	    if( so == "" )
	    {
		cerr << "couldn't find the module for " << mod << endl ;
		return 1 ;
	    }
	    _module_list[mod] = so ;

	    start = comma + 1 ;
	}

	map< string, string >::iterator i = _module_list.begin() ;
	map< string, string >::iterator e = _module_list.end() ;
	for( ; i != e; i++ )
	{
	    _moduleFactory.add_mapping( (*i).first, (*i).second ) ;
	}

	for( i = _module_list.begin(); i != e; i++ )
	{
	    try
	    {
		BESAbstractModule *o = _moduleFactory.get( (*i).first ) ;
		o->initialize() ;
	    }
	    catch( BESException &e )
	    {
		cerr << "Caught plugin exception during initialization of "
		     << (*i).first << " module:" << endl << "    "
		     << e.get_message() << endl ;
		retVal = 1 ;
		break ;
	    }
	    catch( ... )
	    {
		cerr << "Caught unknown exception during initialization of "
		     << (*i).first << " module" << endl ;
		retVal = 1 ;
		break ;
	    }
	}
    }

    return retVal ;
}

/** @brief clean up after the application
 *
 * Calls terminate on each of the loaded modules
 *
 * @return 0 if successful and not 0 otherwise
 * @param sig if the application is terminating due to a signal, otherwise 0
 * is passed.
 */
int BESModuleApp::
terminate( int sig )
{
    map< string, string >::iterator i = _module_list.begin() ;
    map< string, string >::iterator e = _module_list.end() ;
    try
    {
	for( i = _module_list.begin(); i != e; i++ )
	{
	    BESAbstractModule *o = _moduleFactory.get( (*i).first ) ;
	    o->terminate() ;
	}
    }
    catch( BESException &e )
    {
	cerr << "Caught exception during module termination: "
	     << e.get_message() << endl ;
    }
    catch( ... )
    {
	cerr << "Caught unknown exception during terminate" << endl ;
    }

    return BESBaseApp::terminate( sig ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this class along with the name of the
 * application, whether the application is initialized or not and whether the
 * application debugging is turned on.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESModuleApp::
dump( ostream &strm ) const
{
    strm << "BESModuleApp::dump - (" << (void *)this << ")" << endl ;
    strm << "    loaded modules = " << endl ;
    map< string, string >::const_iterator i = _module_list.begin() ;
    map< string, string >::const_iterator e = _module_list.end() ;
    bool any_loaded = false ;
    for( ; i != e; i++ )
    {
	strm << "        " << (*i).first << ": " << (*i).second << endl ;
	any_loaded = true ;
    }
    if( !any_loaded )
    {
	strm << "        no modules loaded" << endl ;
    }

    strm << endl ;
}

