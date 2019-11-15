// BuildTCmd2.cc

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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <iostream>

using std::cout ;
using std::endl ;
using std::string;
using std::map;
using std::ostream;

#include "BuildTCmd2.h"
#include "BESXMLUtils.h"
#include "BESUtil.h"

extern int what_test ;

/** @brief parse a test command element
 *
    <cmd2.1 prop2=\"prop2val\"> \
	<element3> \
	    <element3.1 prop3.1.1=\"prop3.1.1val\" /> \
	</element3> \
	<element4> \
	    element4val \
	</element4> \
    </cmd2.1> \
 *
 * @param node xml2 element node pointer
 */
void
BuildTCmd2::parse_request( xmlNode *node )
{
    // make sure that we actually entered this method
    what_test++ ;

    string name ;
    string value ;
    map<string,string> props ;
    BESXMLUtils::GetNodeInfo( node, name, value, props ) ;
    CPPUNIT_ASSERT( name == "cmd2.1" ) ;
    CPPUNIT_ASSERT( value == "" ) ;
    string prop_val = props["prop2"] ;
    CPPUNIT_ASSERT( prop_val == "prop2val" ) ;

    props.clear() ;
    xmlNode *child_node =
	BESXMLUtils::GetFirstChild( node, name, value, props ) ;
    CPPUNIT_ASSERT( child_node ) ;
    CPPUNIT_ASSERT( name == "element3" ) ;
    CPPUNIT_ASSERT( value == "" ) ;
    CPPUNIT_ASSERT( props.size() == 0 ) ;

    props.clear() ;
    xmlNode *child_child =
	BESXMLUtils::GetFirstChild( child_node, name, value, props ) ;
    CPPUNIT_ASSERT( child_child ) ;
    CPPUNIT_ASSERT( name == "element3.1" ) ;
    CPPUNIT_ASSERT( value == "" ) ;
    prop_val = props["prop3.1.1"] ;
    CPPUNIT_ASSERT( prop_val == "prop3.1.1val" ) ;

    // make sure no more children
    child_child = BESXMLUtils::GetNextChild( child_child, name, value, props ) ;
    CPPUNIT_ASSERT( !child_child ) ;

    props.clear() ;
    child_node = BESXMLUtils::GetNextChild( child_node, name, value, props ) ;
    CPPUNIT_ASSERT( child_node ) ;
    CPPUNIT_ASSERT( name == "element4" ) ;
    CPPUNIT_ASSERT( value == "element4val" ) ;

    value = "" ;
    props.clear() ;
    child_node = BESXMLUtils::GetChild( node, "element4", value, props ) ;
    CPPUNIT_ASSERT( child_node ) ;
    CPPUNIT_ASSERT( value == "element4val" ) ;

    child_node = BESXMLUtils::GetNextChild( child_node, name, value, props ) ;
    CPPUNIT_ASSERT( !child_node ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BuildTCmd2::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BuildTCmd2::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESXMLCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESXMLCommand *
BuildTCmd2::Cmd2Builder( const BESDataHandlerInterface &dhi )
{
    return new BuildTCmd2( dhi ) ;
}

