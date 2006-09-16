// infoT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "infoT.h"
#include "TheBESKeys.h"
#include "BESTextInfo.h"
#include "BESHTMLInfo.h"
#include "BESXMLInfo.h"
#include "BESInfoList.h"
#include "BESInfoNames.h"

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
    t_info->begin_response( "testTextResponse" ) ;
    t_info->add_tag( "tag1", "tag1 data" ) ;
    t_info->begin_tag( "tag2" ) ;
    t_info->add_tag( "tag3", "tag3 data", &attrs ) ;
    t_info->end_tag( "tag2" ) ;
    t_info->end_response() ;
    t_info->print( stdout ) ;

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
    h_info->begin_response( "testHTMLResponse" ) ;
    h_info->add_tag( "tag1", "tag1 data" ) ;
    h_info->begin_tag( "tag2" ) ;
    h_info->add_tag( "tag3", "tag3 data", &attrs ) ;
    h_info->end_tag( "tag2" ) ;
    h_info->end_response() ;
    h_info->print( stdout ) ;

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
    x_info->begin_response( "testXMLResponse" ) ;
    x_info->add_tag( "tag1", "tag1 data" ) ;
    x_info->begin_tag( "tag2" ) ;
    x_info->add_tag( "tag3", "tag3 data", &attrs ) ;
    x_info->end_tag( "tag2" ) ;
    x_info->end_response() ;
    x_info->print( stdout ) ;

    cout << endl << "*****************************************" << endl;
    cout << "Returning from infoT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    putenv( "BES_CONF=./info_test.ini" ) ;
    Application *app = new infoT();
    return app->main(argC, argV);
}

