// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

//#include <cstdio>

#include <pthread.h>
#include <vector>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include "TheBESKeys.h"
#include "BESCatalogList.h"
#include <GetOpt.h>

static bool d_debug = false;

#undef DBG
#define DBG(x) do { if (d_debug) (x); } while(false);

using namespace std;
using namespace CppUnit ;

class BESCatalogListUnitTest: public CppUnit::TestFixture {

public:

    // Called once before everything gets tested
    BESCatalogListUnitTest() {}

    // Called at the end of the test
    ~BESCatalogListUnitTest() {}

    // Called before each test
    void setup() {}

    // Called after each test
    void tearDown() {}

    CPPUNIT_TEST_SUITE( BESCatalogListUnitTest );

    CPPUNIT_TEST(bclut_test);
    //CPPUNIT_TEST(always_pass);

    CPPUNIT_TEST_SUITE_END();

    void printCatalogNames(){
        vector<string> *names = new vector<string>();
        printCatalogNames(names);
        delete names;
    }

    void printCatalogNames(vector<string> *names){
        DBG(cerr << "Server_Function_List_Unit_Test::printFunctionNames() - ServerFunctionList::getFunctionNames(): " << endl);
        if(names->empty()){
            DBG(cerr << "     Function list is empty." << endl);
            return;
        }

        for(unsigned int i=0; i<names->size() ;i++){
            DBG(cerr <<  "   name["<< i << "]: "<< (*names)[i] << endl);
        }
    }
    void always_pass(){
        CPPUNIT_ASSERT(true);
    }


    void bclut_test(){

        try {
            DBG(cerr << endl);
            DBG(cerr << "bclut_test() - BEGIN." << endl);

            string defcat = BESCatalogList::TheCatalogList()->default_catalog() ;
            DBG(cerr << "bclut_test() - Default catalog is '" << defcat << "'" << endl);
            CPPUNIT_ASSERT( defcat == "catalog" ) ;


            int numCat = BESCatalogList::TheCatalogList()->num_catalogs();
            DBG(cerr << "bclut_test() - TheCatalogList()->num_catalogs(): " << numCat << endl);
            CPPUNIT_ASSERT( numCat == 0);

            DBG(cerr << "bclut_test() - Calling  BESCatalogList::delete_instance()"  << endl);
            BESCatalogList::delete_instance();
            DBG(cerr << "bclut_test() - Calling  BESCatalogList::initialize_instance()"  << endl);
            BESCatalogList::initialize_instance();

            defcat = BESCatalogList::TheCatalogList()->default_catalog() ;
            DBG(cerr << "bclut_test() - Default catalog is '" << defcat << "'" << endl);
            CPPUNIT_ASSERT( defcat == "catalog" ) ;


            numCat = BESCatalogList::TheCatalogList()->num_catalogs();
            DBG(cerr << "bclut_test() - TheCatalogList()->num_catalogs(): " << numCat << endl);
            CPPUNIT_ASSERT( numCat == 0);

            DBG(cerr << "bclut_test() - END." << endl);
            CPPUNIT_ASSERT(true);
        }
        catch( BESError &e )
        {
            cerr << "bclut_test() - ERROR: " << e.get_message() << endl ;
            CPPUNIT_ASSERT(false);
        }

    }

};

// BindTest

CPPUNIT_TEST_SUITE_REGISTRATION(BESCatalogListUnitTest);


int main(int argc, char*argv[]) {

    int start = 0;
    GetOpt getopt(argc, argv, "dh");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd': {
            d_debug = 1;  // debug is a static global
            start = 1;
            break;
        }
        case 'h': {     // help - show test names
            cerr << "Usage: BESCatalogListUnitTest has the following tests:" << endl;
            const std::vector<Test*> &tests = BESCatalogListUnitTest::suite()->getTests();
            unsigned int prefix_len = BESCatalogListUnitTest::suite()->getName().append("::").length();
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

//    int start = 0;
//    if(argc>1) {
//        string first(argv[1]);
//        cerr << "first=" << first << endl;
//
//        if(first.compare("-d")==0){
//            d_debug = true;  // debug is a static global
//            start = 1;
//            DBG(cerr << "Debug Enabled" << endl);
//        }
//        if()
//    }


    bool wasSuccessful = true;
    string test = "";
    if (start==0 || (start==1 && d_debug) ) {
        DBG(cerr << "Running All Tests" << endl);
        wasSuccessful = runner.run("");
    }
    else {
        DBG(cerr << "Running Selected Tests" << endl);

        while (start < argc) {
            test = string("BESCatalogListUnitTest::") + argv[start++];
            DBG(cerr << " Running Test: " << test << endl);

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

