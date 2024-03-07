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

#include <thread>

#include <iostream>
#include <iterator>

#include <libdap/BaseType.h>
#include <libdap/Str.h>
#include <libdap/DDS.h>
#include <libdap/ServerFunction.h>

#include <libdap/util.h>

#include "modules/common/run_tests_cppunit.h"

class SingletonList {

    std::multimap<std::string, libdap::ServerFunction *, less<>> d_func_list;

    friend class PossiblyLost;

public:
    SingletonList() = default;
    SingletonList(const SingletonList&) = delete;
    SingletonList& operator=(const SingletonList&) = delete;
    virtual ~SingletonList()
    {
        for (const auto& fit: d_func_list) {
            libdap::ServerFunction *func = fit.second;
            DBG(cerr << "SingletonList::~SingletonList() - Deleting ServerFunction " << func->getName()
                     << " from SingletonList." << endl);
            delete func;
        }

        d_func_list.clear();
    }

    static SingletonList *TheList()
    {
        static SingletonList instance;
        return &instance;
    }

    virtual void add_function(libdap::ServerFunction *func)
    {
        d_func_list.insert(std::make_pair(func->getName(), func));
    }

    void getFunctionNames(vector<string> *names) const
    {
        if (d_func_list.empty()) {
            DBG(cerr << "SingletonList::getFunctionNames() - Function list is empty." << endl);
            return;
        }

        for (const auto& fit: d_func_list) {
            libdap::ServerFunction *func = fit.second;
            DBG(cerr << "SingletonList::getFunctionNames() - Adding function '" << func->getName()
                << "' to names list.\n");
            names->push_back(func->getName());
        }
    }

};

void possibly_lost_function(int /*argc*/, libdap::BaseType */*argv*/[], libdap::DDS &/*dds*/, libdap::BaseType **btpp)
{
    string info = string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") + "<function name=\"ugr4\" version=\"0.1\">\n"
        + "Valgrind Possibly Lost Error Test.\n" + "usage: possibly_lost_test()" + "\n" + "</function>";

    auto response = make_unique<libdap::Str>("info");
    response->set_value(info);
    *btpp = response.release();
}

class PLTF: public libdap::ServerFunction {
public:
    PLTF()
    {
        setName("pltf");
        setDescriptionString(
            "This is a unit test to determine why valgrind is returning possibly lost errors on this code pattern.");
        setUsageString("pltf()");
        setRole("https://services.opendap.org/dap4/unit-tests/possibly_lost_test");
        setDocUrl("https://docs.opendap.org/index.php/unit-tests");
        setFunction(possibly_lost_function);
        setVersion("1.0");
    }
};

class PossiblyLost: public CppUnit::TestFixture {

public:
    PossiblyLost() = default;
    ~PossiblyLost() override = default;

    // setUp and tearDown are not used by this fixture. jhrg 5/25/23

    static void printFunctionNames()
    {
        vector<string> names;
        SingletonList::TheList()->getFunctionNames(&names);
        DBG(cerr << "PossiblyLost::possibly_lost_solution() - SingletonList::getFunctionNames(): " << endl);
        DBG(copy(names.begin(), names.end(), ostream_iterator<string>(cerr, ", ")));
    }

    CPPUNIT_TEST_SUITE(PossiblyLost);

    CPPUNIT_TEST(possibly_lost_fail);

    CPPUNIT_TEST_SUITE_END();

    void possibly_lost_fail()
    {
        auto ssf = std::make_unique<PLTF>();
        ssf->setName("possibly_lost_fail");

        DBG(cerr << "PossiblyLost::possibly_lost_fail() - Adding function(): " << ssf->getDescriptionString() << "\n");
        SingletonList::TheList()->add_function(ssf.release());

        printFunctionNames();
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(PossiblyLost);

int main(int argc, char *argv[])
{
    return bes_run_tests<PossiblyLost>(argc, argv, "cerr,bes") ? 0: 1;
}
