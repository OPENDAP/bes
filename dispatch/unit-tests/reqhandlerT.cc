// reqhandlerT.C

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

using namespace CppUnit;

#include <iostream>
#include <sys/stat.h>
#include <string.h>
#include <util.h>
#include <fcntl.h>

using std::cerr;
using std::cout;
using std::endl;

#include "TestRequestHandler.h"
#include "BESRequestHandlerList.h"
#include "BESRequestHandler.h"
#include "BESFileContainer.h"
#include "BESNotFoundError.h"
#include "BESContextManager.h"
#include <GetOpt.h>

#include "test_config.h"

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class reqhandlerT: public TestFixture {
private:

public:
    reqhandlerT()
    {
    }
    ~reqhandlerT()
    {
    }

    void setUp()
    {
    }

    void tearDown()
    {
    }

    void get_lmt_test(){

    	string relative_file = "/input-files/temp_01.dmr";
		string real_name = string(TEST_SRC_DIR) + relative_file;
		BESFileContainer cont("cont", real_name, "test_handler");
		cont.set_relative_name(relative_file);

    	BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
    	CPPUNIT_ASSERT(besRH != 0);

    	try{
    		int fd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
    		CPPUNIT_ASSERT(fd != -1);

			struct stat statbuf;
			if (stat(real_name.c_str(), &statbuf) == -1){
				throw BESNotFoundError(strerror(errno), __FILE__, __LINE__);
			}//end if

			time_t ctime = statbuf.st_ctime;
			DBG(cerr << "ctime: " << ctime << endl);
			time_t mtime = besRH->get_lmt(real_name);
			DBG(cerr << "mtime: " << mtime << endl);

			bool test = ((ctime - mtime) <= 2);
			CPPUNIT_ASSERT(test);
    	}
    	catch (BESError &e) {
    		ostringstream oss;
    		oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
    		CPPUNIT_FAIL(oss.str());
    	}
    	catch (...) {
    		throw;
    	}


    }//get_lmt_test()

CPPUNIT_TEST_SUITE( reqhandlerT );

	CPPUNIT_TEST(get_lmt_test);

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END()
    ;

    void do_test()
    {
        cout << "*****************************************" << endl;
        cout << "Entered reqhandlerT::run" << endl;

        TestRequestHandler trh("test");
        int retVal = trh.test();

        cout << "*****************************************" << endl;
        cout << "Returning from reqhandlerT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( reqhandlerT );

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dh");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: reqhandlerT has the following tests:" << endl;
            const std::vector<Test*> &tests = reqhandlerT::suite()->getTests();
            unsigned int prefix_len = reqhandlerT::suite()->getName().append("::").length();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = reqhandlerT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

