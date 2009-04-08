// servicesT.C

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <iostream>
#include <sstream>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ostringstream ;

#include "servicesT.h"
#include "BESServiceRegistry.h"
#include "BESError.h"
#include "BESXMLInfo.h"
#include "BESDataNames.h"

string dump1 = "BESServiceRegistry::dump - (X)\n\
    registered services\n\
        cedar\n\
            flat\n\
                description: CEDAR flat format data response\n\
                formats:\n\
                    cedar\n\
            stream\n\
                description: CEDAR stream .cbf data file\n\
                formats:\n\
                    cedar\n\
            tab\n\
                description: CEDAR tab separated data format response\n\
                formats:\n\
                    cedar\n\
        dap\n\
            ascii\n\
                description: OPeNDAP ASCII data\n\
                formats:\n\
                    dap2\n\
            das\n\
                description: OPeNDAP Data Attributes\n\
                formats:\n\
                    dap2\n\
            dds\n\
                description: OPeNDAP Data Description\n\
                formats:\n\
                    dap2\n\
            dods\n\
                description: OPeNDAP Data Object\n\
                formats:\n\
                    dap2\n\
                    netcdf\n\
            html_form\n\
                description: OPeNDAP Form for access\n\
                formats:\n\
                    dap2\n\
            info_page\n\
                description: OPeNDAP Data Information\n\
                formats:\n\
                    dap2\n\
    services provided by handler\n\
        cedar: cedar, dap\n\
        nc: dap\n\
" ;

string dump2 = "BESServiceRegistry::dump - (X)\n\
    registered services\n\
        cedar\n\
            flat\n\
                description: CEDAR flat format data response\n\
                formats:\n\
                    cedar\n\
            stream\n\
                description: CEDAR stream .cbf data file\n\
                formats:\n\
                    cedar\n\
            tab\n\
                description: CEDAR tab separated data format response\n\
                formats:\n\
                    cedar\n\
    services provided by handler\n\
        cedar: cedar\n\
        nc\n\
" ;

string show1 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\" reqID=\"123456789\"><showServices><serviceDescription name=\"cedar\"><command name=\"flat\"><description>CEDAR flat format data response</description><format name=\"cedar\"/></command><command name=\"stream\"><description>CEDAR stream .cbf data file</description><format name=\"cedar\"/></command><command name=\"tab\"><description>CEDAR tab separated data format response</description><format name=\"cedar\"/></command></serviceDescription><serviceDescription name=\"dap\"><command name=\"ascii\"><description>OPeNDAP ASCII data</description><format name=\"dap2\"/></command><command name=\"das\"><description>OPeNDAP Data Attributes</description><format name=\"dap2\"/></command><command name=\"dds\"><description>OPeNDAP Data Description</description><format name=\"dap2\"/></command><command name=\"dods\"><description>OPeNDAP Data Object</description><format name=\"dap2\"/><format name=\"netcdf\"/></command><command name=\"html_form\"><description>OPeNDAP Form for access</description><format name=\"dap2\"/></command><command name=\"info_page\"><description>OPeNDAP Data Information</description><format name=\"dap2\"/></command></serviceDescription></showServices></response>\n\
" ;

string show2 = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\" reqID=\"123456789\"><showServices><serviceDescription name=\"cedar\"><command name=\"flat\"><description>CEDAR flat format data response</description><format name=\"cedar\"/></command><command name=\"stream\"><description>CEDAR stream .cbf data file</description><format name=\"cedar\"/></command><command name=\"tab\"><description>CEDAR tab separated data format response</description><format name=\"cedar\"/></command></serviceDescription></showServices></response>\n\
" ;

int
servicesT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered servicesT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "create the registry" << endl;
    BESServiceRegistry *registry = BESServiceRegistry::TheRegistry() ;
    if( !registry )
    {
	cerr << "failed to create/access the registry object" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "add dap service with das, dds, dods" << endl;
	registry->add_service( "dap" ) ;
	map<string,string> cmds ;
	registry->add_to_service( "dap", "das",
				  "OPeNDAP Data Attributes", "dap2" ) ;
	registry->add_to_service( "dap", "dds",
				  "OPeNDAP Data Description", "dap2" ) ;
	registry->add_to_service( "dap", "dods",
				  "OPeNDAP Data Object", "dap2" ) ;
    }
    catch( BESError &e )
    {
	cerr << "failed to add service dap with das, dds, dods cmds" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "try to add duplicate dap service with das, dds, dods" << endl;
	registry->add_to_service( "dap", "das",
				  "OPeNDAP Data Attributes", "dap2" ) ;
	cerr << "succeeded to add duplicate service ... bad" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "failed to add duplicate service ... good" << endl ;
	cout << e.get_message() << endl ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "add to the dap service" << endl;
	registry->add_to_service( "dap", "ascii",
				  "OPeNDAP ASCII data", "dap2" ) ;
	registry->add_to_service( "dap", "info_page",
				  "OPeNDAP Data Information", "dap2" ) ;
	registry->add_to_service( "dap", "html_form",
				  "OPeNDAP Form for access", "dap2" ) ;
    }
    catch( BESError &e )
    {
	cerr << "failed to add to the dap service" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "try to add duplicate cmd to dap service" << endl;
	registry->add_to_service( "dap", "ascii",
				  "OPeNDAP ASCII data", "dap2" ) ;
	cerr << "succeeded to add duplicate cmd ... bad" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "failed to add duplicate cmd to dap service ... good" << endl ;
	cout << e.get_message() << endl ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "try to add cedar services" << endl;
	registry->add_service( "cedar" ) ;
	registry->add_to_service( "cedar", "flat",
				  "CEDAR flat format data response", "cedar" ) ;
	registry->add_to_service( "cedar", "tab",
				  "CEDAR tab separated data format response",
				  "cedar" ) ;
	registry->add_to_service( "cedar", "stream",
				  "CEDAR stream .cbf data file", "cedar" ) ;
    }
    catch( BESError &e )
    {
	cerr << "failed to add cedar services" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "try to add a format to the dap data response" << endl;
	registry->add_format( "dap", "dods", "netcdf" ) ;
    }
    catch( BESError &e )
    {
	cerr << "failed to add format" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "try to re-add a format to the dap data response" << endl;
	registry->add_format( "dap", "dods", "netcdf" ) ;
	cerr << "successfully added duplicate format ... bad" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "failed to add duplicate format ... good" << endl ;
	cout << e.get_message() << endl ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "add format to non-existant service" << endl;
	registry->add_format( "nogood", "dods", "netcdf" ) ;
	cerr << "successfully added format to non-existent service" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "failed to add format to non-existent service ... good" << endl;
	cout << e.get_message() << endl ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "add format to non-existant cmd" << endl;
	registry->add_format( "dap", "nocmd", "netcdf" ) ;
	cerr << "successfully added format to non-existent cmd" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "failed to add format to non-existent cmd ... good" << endl;
	cout << e.get_message() << endl ;
    }

    if( !registry->service_available( "dap" ) )
    {
	cerr << "failed to find the dap service" << endl ;
	return 1 ;
    }
    if( !registry->service_available( "dap", "ascii" ) )
    {
	cerr << "failed to find the dap service cmd ascii" << endl ;
	return 1 ;
    }
    if( registry->service_available( "not" ) )
    {
	cerr << "found the not service ... bad" << endl ;
	return 1 ;
    }
    if( registry->service_available( "not", "ascii" ) )
    {
	cerr << "found the not service cmd ascii ... bad" << endl ;
	return 1 ;
    }
    if( registry->service_available( "not", "nono" ) )
    {
	cerr << "found the not service cmd nono ... bad" << endl ;
	return 1 ;
    }
    if( registry->service_available( "dap", "nono" ) )
    {
	cerr << "found the dap service nono ... bad" << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "handler services" << endl;
	registry->handles_service( "nc", "dap" ) ;
	registry->handles_service( "cedar", "dap" ) ;
	registry->handles_service( "cedar", "cedar" ) ;
    }
    catch( BESError &e )
    {
	cerr << "handles_service calls failed" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "dump the registry" << endl;
	ostringstream strm ;
	strm << *registry ;
	string res = strm.str() ;
	string::size_type spos = res.find( "(0x" ) ;
	string::size_type epos = res.find( ")", spos ) ;
	if( spos == string::npos || epos == string::npos )
	{
	    cerr << "dump of registry incorrect" << endl ;
	    cerr << "result = " << res << endl ;
	    cerr << "expected = " << dump1 << endl ;
	    return 1 ;
	}
	res.replace( spos+1, epos - spos - 1, "X" ) ;
	if( res != dump1 )
	{
	    cerr << "dump of registry incorrect" << endl ;
	    cerr << "result = " << res << endl ;
	    cerr << "expected = " << dump1 << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "failed to dump the registry" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "try to add command to service that doesn't exist" << endl;
	registry->add_to_service( "notexist", "something",
				"something description", "something format" ) ;
	cerr << "succeeded to add command to non-existent service ... bad" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "failed to add command to non-existent service ... good" << endl ;
	cout << e.get_message() << endl ;
    }

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "handle service that does not exist" << endl;
	registry->handles_service( "csv", "nonexist" ) ;
	cerr << "succeeded to handle non-existent service ... bad" << endl ;
	return 1 ;
    }
    catch( BESError &e )
    {
	cout << "failed to handle non-existent service ... good" << endl ;
	cout << e.get_message() << endl ;
    }

    bool does_it = false ;
    cout << endl << "*****************************************" << endl;
    cout << "does handle?" << endl;
    does_it = registry->does_handle_service( "nc", "dap" ) ;
    if( does_it == false )
    {
	cerr << "nc does, failed" << endl ;
    }
    does_it = registry->does_handle_service( "cedar", "cedar" ) ;
    if( does_it == false )
    {
	cerr << "cedar does, failed" << endl ;
    }
    does_it = registry->does_handle_service( "nc", "ascii" ) ;
    if( does_it == true )
    {
	cerr << "nc doesn't, failed" << endl ;
    }
    does_it = registry->does_handle_service( "nonexist", "dap" ) ;
    if( does_it == true )
    {
	cerr << "nonexist doesn't, failed" << endl ;
    }
    does_it = registry->does_handle_service( "nc", "nonexist" ) ;
    if( does_it == true )
    {
	cerr << "nc doesn't, failed" << endl ;
    }
    does_it = registry->does_handle_service( "nonexist", "nonexist" ) ;
    if( does_it == true )
    {
	cerr << "nonexist doesn't, failed" << endl ;
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "services handled by cedar" << endl;
	list<string> services ;
	registry->services_handled( "cedar", services ) ;
	map<string,string> baseline ;
	baseline["dap"] = "dap" ;
	baseline["cedar"] = "cedar" ;
	if( services.size() != baseline.size() )
	{
	    cerr << "returned " << services.size()
		 << " services, should have returned " << baseline.size()
		 << endl ;
	    return 1 ;
	}
	list<string>::const_iterator si = services.begin() ;
	list<string>::const_iterator se = services.end() ;
	for( ; si != se; si++ )
	{
	    cerr << (*si) << endl ;
	    map<string,string>::iterator fi = baseline.find( (*si) ) ;
	    if( fi == baseline.end() )
	    {
		cerr << "didn't find " << (*si) << endl ;
		return 1 ;
	    }
	}
    }

    {
	cout << endl << "*****************************************" << endl;
	cout << "services handled by nc" << endl;
	list<string> services ;
	registry->services_handled( "nc", services ) ;
	map<string,string> baseline ;
	baseline["dap"] = "dap" ;
	if( services.size() != baseline.size() )
	{
	    cerr << "returned " << services.size()
		 << " services, should have returned " << baseline.size()
		 << endl ;
	    return 1 ;
	}
	list<string>::const_iterator si = services.begin() ;
	list<string>::const_iterator se = services.end() ;
	for( ; si != se; si++ )
	{
	    cerr << (*si) << endl ;
	    map<string,string>::iterator fi = baseline.find( (*si) ) ;
	    if( fi == baseline.end() )
	    {
		cerr << "didn't find " << (*si) << endl ;
		return 1 ;
	    }
	}
    }

    /*
    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "show services" << endl;
	BESXMLInfo info ;
	BESDataHandlerInterface dhi ;
	dhi.data[REQUEST_ID] = "123456789" ;
	info.begin_response( "showServices", dhi ) ;
	registry->show_services( info ) ;
	info.end_response( ) ;
	ostringstream strm ;
	info.print( strm ) ;
	cout << "strm = " << endl << strm.str() << endl ;
	cout << "show1 = " << endl << show1 << endl ;
	if( strm.str() != show1 )
	{
	    cerr << "show services incorrect format" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "failed to show services" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    */

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "remove service" << endl;
	registry->remove_service( "dap" ) ;
    }
    catch( BESError &e )
    {
	cerr << "failed to remove the service" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    /*
    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "show services" << endl;
	BESXMLInfo info ;
	BESDataHandlerInterface dhi ;
	dhi.data[REQUEST_ID] = "123456789" ;
	info.begin_response( "showServices", dhi ) ;
	registry->show_services( info ) ;
	info.end_response( ) ;
	ostringstream strm ;
	info.print( strm ) ;
	cout << "strm = " << endl << strm.str() << endl ;
	cout << "show2 = " << endl << show2 << endl ;
	if( strm.str() != show2 )
	{
	    cerr << "show services incorrect format" << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "failed to show services" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }
    */

    try
    {
	cout << endl << "*****************************************" << endl;
	cout << "dump the registry" << endl;
	ostringstream strm ;
	strm << *registry ;
	string res = strm.str() ;
	string::size_type spos = res.find( "(0x" ) ;
	string::size_type epos = res.find( ")", spos ) ;
	if( spos == string::npos || epos == string::npos )
	{
	    cerr << "dump of registry incorrect" << endl ;
	    cerr << "result = " << res << endl ;
	    cerr << "expected = " << dump2 << endl ;
	    return 1 ;
	}
	res.replace( spos+1, epos - spos - 1, "X" ) ;
	cout << "res = " << endl << res << endl ;
	cout << "dump2 = " << endl << dump2 << endl ;
	if( res != dump2 )
	{
	    cerr << "dump of registry incorrect" << endl ;
	    cerr << "result = " << res << endl ;
	    cerr << "expected = " << dump2 << endl ;
	    return 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "failed to dump the registry" << endl ;
	cerr << e.get_message() << endl ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from servicesT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new servicesT();
    return app->main(argC, argV);
}

