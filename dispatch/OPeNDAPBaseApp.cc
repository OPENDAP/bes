// OPeNDAPBaseApp.C

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

#include <iostream>

using std::cerr ;
using std::endl ;

#include "OPeNDAPBaseApp.h"
#include "DODSGlobalIQ.h"
#include "DODSBasicException.h"
#include "OPeNDAPPluginFactory.h"
#include "OPeNDAPAbstractModule.h"
#include "OPeNDAPPluginException.h"
#include "TheDODSKeys.h"

OPeNDAPApp *OPeNDAPApp::_theApplication = 0;

/** @brief Default constructor
 *
 * Initialized the static _the Applicatioon to point to this application
 * object
 */
OPeNDAPBaseApp::
OPeNDAPBaseApp(void)
{
    OPeNDAPApp::_theApplication = this;
}

/** @brief Default destructor
 *
 * sets the static _theApplicaiton to null. Does not call terminate. It is up
 * to the main method to call the terminate method.
 */
OPeNDAPBaseApp::
~OPeNDAPBaseApp(void)
{
    OPeNDAPApp::_theApplication = 0;
}

/** @brief main method of the BES application
 *
 * sets the appName to argv[0], then calls initialize, run, and terminate in
 * that order. Exceptions should be caught in the individual methods
 * initialize, run and terminate and handled there.
 *
 * @return 0 if successful and not 0 otherwise
 * @param argC argc value passed to the main function
 * @param argV argv value passed to the main function
 */
int OPeNDAPBaseApp::
main(int argC, char **argV)
{
    _appName = argV[0] ;
    int retVal = initialize( argC, argV ) ;
    if( retVal != 0 )
    {
	cerr << "OPeNDAPBaseApp::initialize - failed" << endl;
    }
    else
    {
	retVal = run() ;
	retVal = terminate( retVal ) ;
    }
    return retVal ;
}

/** @brief initializes the OPeNDAP BES application
 *
 * uses the DODSGlobalIQ static method DODSGlobalInit to initialize any global
 * variables needed by this application
 *
 * @return 0 if successful and not 0 otherwise
 * @param argC argc value passed to the main function
 * @param argV argv value passed to the main function
 * @throws DODSBasicException if any exceptions or errors are encountered
 * @see DODSGlobalIQ
 */
int OPeNDAPBaseApp::
initialize(int argC, char **argV)
{
    int retVal = 0;

    // initialize application information
    try
    {
	if( !_isInitialized )
	    DODSGlobalIQ::DODSGlobalInit( argC, argV ) ;
	_isInitialized = true ;
    }
    catch( DODSException &e )
    {
	string newerr = "Error initializing application: " ;
	newerr += e.get_error_description() ;
	throw DODSBasicException( newerr ) ;
    }
    catch( ... )
    {
	string newerr = "Error initializing application: " ;
	newerr += "caught unknown exception" ;
	throw DODSBasicException( newerr ) ;
    }

    return loadModules() ;
}

/** @brief load data handler modules using the initialization file
 */
int
OPeNDAPBaseApp::loadModules()
{
    int retVal = 0 ;

    bool found = false ;
    string mods = TheDODSKeys::TheKeys()->get_key( "OPeNDAP.modules", found ) ;
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
	    string so = TheDODSKeys::TheKeys()->get_key( key, found ) ;
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

	try
	{
	    for( i = _module_list.begin(); i != e; i++ )
	    {
		OPeNDAPAbstractModule *o = _moduleFactory.get( (*i).first ) ;
		o->initialize() ;
	    }
	}
	catch( OPeNDAPPluginException &e )
	{
	    cerr << "Caught exception during initialize: "
	         << e.get_error_description() << endl ;
	    retVal = 1 ;
	}
	catch( ... )
	{
	    cerr << "Caught unknown exception during initialize" << endl ;
	    retVal = 1 ;
	}
    }

    return retVal ;
}

/** @brief the applications functionality is implemented in the run method
 *
 * It is up to the derived class to implement this method.
 *
 * @return 0 if successful and not 0 otherwise
 * @throws DODSBasicException if the derived class does not implement this
 * method
 */
int OPeNDAPBaseApp::
run(void)
{
    throw DODSBasicException( "OPeNDAPBaseApp::run - overload run operation" ) ;
    return 0;
}

/** @brief clean up after the application
 *
 * Cleans up any global variables registered with DODSGlobalIQ
 *
 * @return 0 if successful and not 0 otherwise
 * @param sig if the application is terminating due to a signal, otherwise 0
 * is passed.
 * @see DODSGlobalIQ
 */
int OPeNDAPBaseApp::
terminate( int sig )
{
    if( sig )
    {
	cerr << "OPeNDAPBaseApp::terminating with signal " << sig << endl ;
    }

    map< string, string >::iterator i = _module_list.begin() ;
    map< string, string >::iterator e = _module_list.end() ;
    try
    {
	for( i = _module_list.begin(); i != e; i++ )
	{
	    OPeNDAPAbstractModule *o = _moduleFactory.get( (*i).first ) ;
	    o->terminate() ;
	}
    }
    catch( OPeNDAPPluginException &e )
    {
	cerr << "Caught exception during terminate: "
	     << e.get_error_description() << endl ;
    }
    catch( ... )
    {
	cerr << "Caught unknown exception during terminate" << endl ;
    }

    DODSGlobalIQ::DODSGlobalQuit() ;
    _isInitialized = false ;
    return sig ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this class along with the name of the
 * application, whether the application is initialized or not and whether the
 * application debugging is turned on.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void OPeNDAPBaseApp::
dump( ostream &strm ) const
{
    strm << "OPeNDAPBaseApp::dump - (" << (void *)this << ")" << endl ;
    strm << "    appName = " << appName() << endl ;
    strm << "    application " ;
    if( _isInitialized )
	strm << "is" ;
    else
	strm << "is not" ;
    strm << " initialized" << endl ;
    strm << "    debug is turned " ;
    if( debug() )
	strm << "on" ;
    else
	strm << "off" ;
    strm << endl ;
}

