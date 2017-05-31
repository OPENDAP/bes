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


#include "config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <pthread.h>

#include <iostream>
#include <iterator>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include "debug.h"
#include "util.h"

#include "GetOpt.h"
#include "BaseType.h"
#include "Str.h"
#include "DDS.h"
#include "ServerFunction.h"

static pthread_once_t instance_control = PTHREAD_ONCE_INIT;
static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class SingletonList {
private:
    static SingletonList * d_instance;

    std::multimap<std::string, libdap::ServerFunction *> d_func_list;

    static void initialize_instance()
    {
        if (d_instance == 0) {
            d_instance = new SingletonList;
#if HAVE_ATEXIT
            atexit(delete_instance);
#endif
        }
    }

    static void delete_instance()
    {
        delete d_instance;
        d_instance = 0;
    }

    virtual ~SingletonList()
    {
        std::multimap<string, libdap::ServerFunction *>::iterator fit;
        for (fit = d_func_list.begin(); fit != d_func_list.end(); fit++) {
            libdap::ServerFunction *func = fit->second;
            DBG(
                cerr << "SingletonList::~SingletonList() - Deleting ServerFunction " << func->getName()
                    << " from SingletonList." << endl);
            delete func;
        }
        d_func_list.clear();
    }

    friend class PossiblyLost;

protected:
    SingletonList()
    {
    }

public:

    static SingletonList * TheList()
    {
        pthread_once(&instance_control, initialize_instance);

        return d_instance;
    }

    virtual void add_function(libdap::ServerFunction *func)
    {
        d_func_list.insert(std::make_pair(func->getName(), func));
    }

    void getFunctionNames(vector<string> *names)
    {
        if (d_func_list.empty()) {
            DBG(cerr << "SingletonList::getFunctionNames() - Function list is empty." << endl);
            return;
        }
        std::multimap<string, libdap::ServerFunction *>::iterator fit;
        for (fit = d_func_list.begin(); fit != d_func_list.end(); fit++) {
            libdap::ServerFunction *func = fit->second;
            DBG(
                cerr << "SingletonList::getFunctionNames() - Adding function '" << func->getName() << "' to names list."
                    << endl);
            names->push_back(func->getName());
        }
    }

};

SingletonList *SingletonList::d_instance = 0;

void possibly_lost_function(int /*argc*/, libdap::BaseType */*argv*/[], libdap::DDS &/*dds*/, libdap::BaseType **btpp)
{
    string info = string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") + "<function name=\"ugr4\" version=\"0.1\">\n"
        + "Valgrind Possibly Lost Error Test.\n" + "usage: possibly_lost_test()" + "\n" + "</function>";

    libdap::Str *response = new libdap::Str("info");
    response->set_value(info);
    *btpp = response;
    return;

}

class PLTF: public libdap::ServerFunction {
public:
    PLTF()
    {
        setName("pltf");
        setDescriptionString(
            "This is a unit test to determine why valgrind is returning possibly lost errors on this code pattern.");
        setUsageString("pltf()");
        setRole("http://services.opendap.org/dap4/unit-tests/possibly_lost_test");
        setDocUrl("http://docs.opendap.org/index.php/unit-tests");
        setFunction(possibly_lost_function);
        setVersion("1.0");

    }
};

class PossiblyLost: public CppUnit::TestFixture {

public:

    // Called once before everything gets tested
    PossiblyLost()
    {
        //    DBG(cerr << " BindTest - Constructor" << endl);

    }

    // Called at the end of the test
    ~PossiblyLost()
    {
        //    DBG(cerr << " BindTest - Destructor" << endl);
    }

    // Called before each test
    void setup()
    {
        //    DBG(cerr << " BindTest - setup()" << endl);
    }

    // Called after each test
    void tearDown()
    {
        //    DBG(cerr << " tearDown()" << endl);
    }

CPPUNIT_TEST_SUITE( PossiblyLost );

    CPPUNIT_TEST(possibly_lost_fail);
    CPPUNIT_TEST(possibly_lost_solution);

    CPPUNIT_TEST_SUITE_END()
    ;

    void possibly_lost_fail()
    {
        DBG(cerr << endl);

        PLTF *ssf = new PLTF();
        ssf->setName("Possibly_Lost_FAIL");

        //printFunctionNames();

        DBG(
            cerr << "PossiblyLost::possibly_lost_solution() - Adding function(): " << ssf->getDescriptionString()
                << endl);
        SingletonList::TheList()->add_function(ssf);

        printFunctionNames();

    }

    void printFunctionNames()
    {
        vector<string> names;
        SingletonList::TheList()->getFunctionNames(&names);
        DBG(cerr << "PossiblyLost::possibly_lost_solution() - SingletonList::getFunctionNames(): " << endl);
        DBG(copy(names.begin(), names.end(), ostream_iterator<string>(cerr, ", ")));
    }

    void possibly_lost_solution()
    {
        DBG(cerr << endl);

        PLTF *ssf = new PLTF();
        ssf->setName("Possibly_Lost_Solution");

        printFunctionNames();

        DBG(
            cerr << "PossiblyLost::possibly_lost_solution() - Adding function(): " << ssf->getDescriptionString()
                << endl);
        SingletonList::TheList()->add_function(ssf);

        printFunctionNames();

        DBG(cerr << "PossiblyLost::possibly_lost_solution() - Deleting the List." << endl);
        SingletonList::delete_instance();

        // This is needed because we used pthread_once to ensure that
        // initialize_instance() is called at most once. We manually call
        // the delete method, so the object must be remade. This would never
        // be done by non-test code. jhrg 5/2/13
        SingletonList::initialize_instance();

        printFunctionNames();
    }

};
// BindTest

CPPUNIT_TEST_SUITE_REGISTRATION(PossiblyLost);

int main(int argc, char*argv[])
{
    GetOpt getopt(argc, argv, "dh");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            std::cerr << "Usage: PossiblyLost has the following tests:" << std::endl;
            const std::vector<CppUnit::Test*> &tests = PossiblyLost::suite()->getTests();
            unsigned int prefix_len = PossiblyLost::suite()->getName().append("::").length();
            for (std::vector<CppUnit::Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                std::cerr << (*i)->getName().replace(0, prefix_len, "") << std::endl;
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
            test = PossiblyLost::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

