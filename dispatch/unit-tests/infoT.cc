// infoT.C

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
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ostringstream ;

#include "infoT.h"
#include "TheBESKeys.h"
#include "BESTextInfo.h"
#include "BESHTMLInfo.h"
#include "BESXMLInfo.h"
#include "BESInfoList.h"
#include "BESInfoNames.h"
#include "BESDataHandlerInterface.h"
#include <test_config.h>

string txt_baseline = "tag1: tag1 data\n\
tag2\n\
    tag3: tag3 data\n\
        attr_name: \"attr_val\"\n" ;

string html_baseline = "<HTML>\n\
    <HEAD>\n\
        <TITLE>testHTMLResponse</TITLE>\n\
    </HEAD>\n\
    <BODY>\n\
        tag1: tag1 data<BR />\n\
        tag2<BR />\n\
            tag3: tag3 data<BR />\n\
                attr_name: \"attr_val\"<BR />\n\
    </BODY>\n\
</HTML>\n" ;

string xml_baseline = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\
<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\"><testXMLResponse><tag1>tag1 data</tag1><tag2><tag3 attr_name=\"&quot;attr_val&quot;\">tag3 data</tag3></tag2></testXMLResponse></response>\n" ;

int infoT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered infoT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "add info builders to info list ... " ;
    BESInfoList::TheList()->add_info_builder( BES_TEXT_INFO,
					      BESTextInfo::BuildTextInfo ) ;
    BESInfoList::TheList()->add_info_builder( BES_HTML_INFO,
					      BESHTMLInfo::BuildHTMLInfo ) ;
    BESInfoList::TheList()->add_info_builder( BES_XML_INFO,
					      BESXMLInfo::BuildXMLInfo ) ;
    cout << "OK" << endl ;

    map<string,string> attrs ;
    attrs["attr_name"] = "\"attr_val\"" ;

    cout << endl << "*****************************************" << endl;
    cout << "Set Info type to txt ... " ;
    TheBESKeys::TheKeys()->set_key( "BES.Info.Type", "txt" ) ;
    BESInfo *info = BESInfoList::TheList()->build_info() ;
    BESTextInfo *t_info = dynamic_cast<BESTextInfo *>(info) ;
    if( t_info )
    {
	cout << "OK" << endl ;
    }
    else
    {
	cout << "FAIL" << endl ;
	return 1 ;
    }
    BESDataHandlerInterface dhi ;
    t_info->begin_response( "testTextResponse", dhi ) ;
    t_info->add_tag( "tag1", "tag1 data" ) ;
    t_info->begin_tag( "tag2" ) ;
    t_info->add_tag( "tag3", "tag3 data", &attrs ) ;
    t_info->end_tag( "tag2" ) ;
    t_info->end_response() ;
    ostringstream tstrm ;
    t_info->print( tstrm ) ;
    if( tstrm.str() != txt_baseline )
    {
	cout << "Failed text" << endl ;
	cout << tstrm.str() ;
	cout << txt_baseline ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Set Info type to html ... " << endl ;
    TheBESKeys::TheKeys()->set_key( "BES.Info.Type", "html" ) ;
    info = BESInfoList::TheList()->build_info() ;
    BESHTMLInfo *h_info = dynamic_cast<BESHTMLInfo *>(info) ;
    if( h_info )
    {
	cout << "OK" << endl ;
    }
    else
    {
	cout << "FAIL" << endl ;
	return 1 ;
    }
    h_info->begin_response( "testHTMLResponse", dhi ) ;
    h_info->add_tag( "tag1", "tag1 data" ) ;
    h_info->begin_tag( "tag2" ) ;
    h_info->add_tag( "tag3", "tag3 data", &attrs ) ;
    h_info->end_tag( "tag2" ) ;
    h_info->end_response() ;
    ostringstream hstrm ;
    h_info->print( hstrm ) ;
    if( hstrm.str() != html_baseline )
    {
	cout << "Failed html" << endl ;
	cout << hstrm.str() ;
	cout << html_baseline ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Set Info type to xml ... " << endl ;
    TheBESKeys::TheKeys()->set_key( "BES.Info.Type", "xml" ) ;
    info = BESInfoList::TheList()->build_info() ;
    BESXMLInfo *x_info = dynamic_cast<BESXMLInfo *>(info) ;
    if( x_info )
    {
	cout << "OK" << endl ;
    }
    else
    {
	cout << "FAIL" << endl ;
	return 1 ;
    }
    x_info->begin_response( "testXMLResponse", dhi ) ;
    x_info->add_tag( "tag1", "tag1 data" ) ;
    x_info->begin_tag( "tag2" ) ;
    x_info->add_tag( "tag3", "tag3 data", &attrs ) ;
    x_info->end_tag( "tag2" ) ;
    x_info->end_response() ;
    ostringstream xstrm ;
    x_info->print( xstrm ) ;
    if( xstrm.str() != xml_baseline )
    {
	cout << "Failed xml" << endl ;
	cout << xstrm.str() ;
	cout << xml_baseline ;
	return 1 ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from infoT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR
                     + "/info_test.ini" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new infoT();
    return app->main(argC, argV);
}

