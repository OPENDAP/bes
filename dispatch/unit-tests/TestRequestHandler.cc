// TestRequestHandler.cc

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

#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit;

#include <iostream>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

#include "TestRequestHandler.h"

TestRequestHandler *trh = nullptr;

TestRequestHandler::TestRequestHandler(string name) : BESRequestHandler(name), _resp_num(0) {
    trh = this;
    CPPUNIT_ASSERT(add_method("resp1", TestRequestHandler::test_build_resp1));
    CPPUNIT_ASSERT(add_method("resp2", TestRequestHandler::test_build_resp2));
    CPPUNIT_ASSERT(add_method("resp3", TestRequestHandler::test_build_resp3));
    CPPUNIT_ASSERT(add_method("resp4", TestRequestHandler::test_build_resp4));
}

TestRequestHandler::~TestRequestHandler() = default;

bool TestRequestHandler::test_build_resp1(BESDataHandlerInterface &r) {
    trh->_resp_num = 1;
    return true;
}

bool TestRequestHandler::test_build_resp2(BESDataHandlerInterface &r) {
    trh->_resp_num = 2;
    return true;
}

bool TestRequestHandler::test_build_resp3(BESDataHandlerInterface &r) {
    trh->_resp_num = 3;
    return true;
}

bool TestRequestHandler::test_build_resp4(BESDataHandlerInterface &r) {
    trh->_resp_num = 4;
    return true;
}

int TestRequestHandler::test() {
    cout << "*****************************************" << endl;
    cout << "finding the handlers" << endl;
    BESDataHandlerInterface dhi;

    cout << "    finding resp1" << endl;
    p_request_handler_method p = find_method("resp1");
    CPPUNIT_ASSERT(p);
    p(dhi);
    CPPUNIT_ASSERT(_resp_num == 1);

    cout << "    finding resp2" << endl;
    p = find_method("resp2");
    CPPUNIT_ASSERT(p);
    p(dhi);
    CPPUNIT_ASSERT(_resp_num == 2);

    cout << "    finding resp3" << endl;
    p = find_method("resp3");
    CPPUNIT_ASSERT(p);
    p(dhi);
    CPPUNIT_ASSERT(_resp_num == 3);

    cout << "    finding resp4" << endl;
    p = find_method("resp4");
    CPPUNIT_ASSERT(p);
    p(dhi);
    CPPUNIT_ASSERT(_resp_num == 4);

    cout << "    finding not_there" << endl;
    p = find_method("not_there");
    CPPUNIT_ASSERT(!p);

    cout << "*****************************************" << endl;
    cout << "try to add resp3 again" << endl;
    bool ret = add_method("resp3", TestRequestHandler::test_build_resp3);
    CPPUNIT_ASSERT(ret == false);

    cout << "*****************************************" << endl;
    cout << "removing resp2" << endl;
    CPPUNIT_ASSERT(remove_method("resp2"));
    p = find_method("resp2");
    CPPUNIT_ASSERT(!p);

    cout << "*****************************************" << endl;
    cout << "add resp2 back" << endl;
    ret = add_method("resp2", TestRequestHandler::test_build_resp2);
    CPPUNIT_ASSERT(ret == true);

    p = find_method("resp2");
    CPPUNIT_ASSERT(p);
    p(dhi);
    CPPUNIT_ASSERT(_resp_num == 2);

    return 0;
}
