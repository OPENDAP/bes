// constraintT.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
// Refactored for C++14: Google Gemini (2025)
// Adapted to use bes_run_tests: Google Gemini (2025)
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
// pwest Patrick West <pwest@ucar.edu>
// jgarcia Jose Garcia <jgarcia@ucar.edu>

// CppUnit includes are now likely handled by bes_run_tests_cppunit.h,
// but keeping them doesn't hurt for clarity or if used directly elsewhere.
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <iostream>
#include <memory> // Required for std::unique_ptr, std::make_unique
#include <string> // Required for std::string
#include <vector> // Required for std::vector
#include <cstdlib> // Required for exit codes

// Include necessary BES headers
#include "BESFileContainer.h"
#include "BESDataHandlerInterface.h"
#include "BESConstraintFuncs.h"
#include "BESDataNames.h"
#include "TheBESKeys.h"
#include "test_config.h" // Assumed to define TEST_SRC_DIR

#include "modules/common/run_tests_cppunit.h"

// Using directives kept from original for style consistency
using std::cerr;
using std::cout;
using std::endl;
using std::string;
// Note: We don't need 'using namespace CppUnit' if TextTestRunner etc. are
// only used inside run_tests_cppunit.h, but keep TestFixture for the class.
using CppUnit::TestFixture;

class constraintT final : public TestFixture {

public:
    // Constructor and Destructor remain simple
    constraintT() = default;
    ~constraintT() override = default; // Use override for virtual destructor

    // setUp remains the same
    void setUp() override {
        const string bes_conf = static_cast<string>(TEST_SRC_DIR) + "/empty.ini";
        TheBESKeys::ConfigFile = bes_conf;
    }

    // tearDown remains empty
    void tearDown() override {}

    CPPUNIT_TEST_SUITE(constraintT);

    // Register the specific test methods
    CPPUNIT_TEST(testPostAppend_BothContainersConstrained);
    CPPUNIT_TEST(testPostAppend_OnlySecondContainerConstrained);
    CPPUNIT_TEST(testPostAppend_OnlyFirstContainerConstrained);

    CPPUNIT_TEST_SUITE_END();

    /**
     * @brief Tests BESConstraintFuncs::post_append when both containers have constraints.
     */
    void testPostAppend_BothContainersConstrained() {
        DBG(cout << "Running testPostAppend_BothContainersConstrained" << endl);
        BESDataHandlerInterface dhi;

        auto d1 = std::make_unique<BESFileContainer>("sym1", "real1", "type1");
        d1->set_constraint("var1");
        dhi.containers.push_back(d1.get());

        auto d2 = std::make_unique<BESFileContainer>("sym2", "real2", "type2");
        d2->set_constraint("var2");
        dhi.containers.push_back(d2.get());

        dhi.first_container();
        BESConstraintFuncs::post_append(dhi);
        dhi.next_container();
        BESConstraintFuncs::post_append(dhi);

        const string should_be = "sym1.var1,sym2.var2";
        const string actual = dhi.data[POST_CONSTRAINT];

        DBG(cout << "  post constraint = " << actual << endl);
        DBG(cout << "  should be       = " << should_be << endl);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Post constraint mismatch when both containers have constraints",
                                     should_be, actual);
    }

    /**
     * @brief Tests BESConstraintFuncs::post_append when only the second container has a constraint.
     */
    void testPostAppend_OnlySecondContainerConstrained() {
        DBG(cout << "Running testPostAppend_OnlySecondContainerConstrained" << endl);
        BESDataHandlerInterface dhi;

        auto d1 = std::make_unique<BESFileContainer>("sym1", "real1", "type1");
        dhi.containers.push_back(d1.get());

        auto d2 = std::make_unique<BESFileContainer>("sym2", "real2", "type2");
        d2->set_constraint("var2");
        dhi.containers.push_back(d2.get());

        dhi.first_container();
        BESConstraintFuncs::post_append(dhi);
        dhi.next_container();
        BESConstraintFuncs::post_append(dhi);

        const string should_be = "sym1,sym2.var2";
        const string actual = dhi.data[POST_CONSTRAINT];

        DBG(cout << "  post constraint = " << actual << endl);
        DBG(cout << "  should be       = " << should_be << endl);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Post constraint mismatch when only second container has constraint",
                                     should_be, actual);
    }

    /**
     * @brief Tests BESConstraintFuncs::post_append when only the first container has a constraint.
     */
    void testPostAppend_OnlyFirstContainerConstrained() {
        DBG(cout << "Running testPostAppend_OnlyFirstContainerConstrained" << endl);
        BESDataHandlerInterface dhi;

        auto d1 = std::make_unique<BESFileContainer>("sym1", "real1", "type1");
        d1->set_constraint("var1");
        dhi.containers.push_back(d1.get());

        auto d2 = std::make_unique<BESFileContainer>("sym2", "real2", "type2");
        dhi.containers.push_back(d2.get());

        dhi.first_container();
        BESConstraintFuncs::post_append(dhi);
        dhi.next_container();
        BESConstraintFuncs::post_append(dhi);

        const string should_be = "sym1.var1,sym2";
        const string actual = dhi.data[POST_CONSTRAINT];

        DBG(cout << "  post constraint = " << actual << endl);
        DBG(cout << "  should be       = " << should_be << endl);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Post constraint mismatch when only first container has constraint",
                                     should_be, actual);
    }

}; // End class constraintT

CPPUNIT_TEST_SUITE_REGISTRATION(constraintT);

int main(const int argc, char *argv[]) {
    const bool success = bes_run_tests<constraintT>(argc, argv, ""); // Using empty context string

    // Return 0 for success (true from bes_run_tests), 1 for failure (false).
    return success ? 0 : 1;
}