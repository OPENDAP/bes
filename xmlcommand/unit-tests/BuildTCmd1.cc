// BuildTCmd1.cc

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
using std::ostream;

#include "BuildTCmd1.h"
#include "BESXMLUtils.h"

extern int what_test ;

/** @brief parse a test command element
 *
    <cmd1.1 prop1=\"prop1val\"> \
	<element1>element1val</element1> \
	<element2 prop2.1=\"prop2.1val\">element2val</element2> \
    </cmd1.1> \
 *
 * @param node xml2 element node pointer
 */
void
BuildTCmd1::parse_request( xmlNode *node )
{
    // make sure that we actually entered this method
    what_test++ ;

    string name ;
    string value ;
    map<string, string> props ;
    BESXMLUtils::GetNodeInfo( node, name, value, props ) ;
    CPPUNIT_ASSERT( name == "cmd1.1" ) ;
    CPPUNIT_ASSERT( value == "" ) ;
    string prop_val = props["prop1"] ;
    CPPUNIT_ASSERT( prop_val == "prop1val" ) ;

    props.clear() ;
    xmlNode *child_node =
	BESXMLUtils::GetFirstChild( node, name, value, props ) ;
    CPPUNIT_ASSERT( child_node ) ;
    CPPUNIT_ASSERT( name == "element1" ) ;
    CPPUNIT_ASSERT( value == "element1val" ) ;

    props.clear() ;
    child_node = BESXMLUtils::GetNextChild( child_node, name, value, props ) ;
    CPPUNIT_ASSERT( child_node ) ;
    CPPUNIT_ASSERT( name == "element2" ) ;
    CPPUNIT_ASSERT( value == "element2val" ) ;
    prop_val = props["prop2.1"] ;
    CPPUNIT_ASSERT( prop_val == "prop2.1val" ) ;

    props.clear() ;
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
BuildTCmd1::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BuildTCmd1::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESXMLCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESXMLCommand *
BuildTCmd1::Cmd1Builder( const BESDataHandlerInterface &dhi )
{
    return new BuildTCmd1( dhi ) ;
}

