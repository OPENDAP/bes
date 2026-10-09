// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES dmrpp_module/ngap_container.

// Copyright (c) 2026 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#include <string>

#include "BESInternalError.h"
#include "HttpError.h"

#include "NgapServiceChaining.h"

#include "run_tests_cppunit.h"

using namespace std;

namespace ngap {

class NgapServiceChainingTest : public CppUnit::TestFixture {
public:
    NgapServiceChainingTest() = default;
    ~NgapServiceChainingTest() override = default;

    void setUp() override { set_ngap_request(false); }
    void tearDown() override { set_ngap_request(false); }

    void test_is_http_5xx() {
        CPPUNIT_ASSERT(!is_http_5xx(0));
        CPPUNIT_ASSERT(!is_http_5xx(200));
        CPPUNIT_ASSERT(!is_http_5xx(404));
        CPPUNIT_ASSERT(!is_http_5xx(499));
        CPPUNIT_ASSERT(is_http_5xx(500));
        CPPUNIT_ASSERT(is_http_5xx(503));
        CPPUNIT_ASSERT(is_http_5xx(599));
        CPPUNIT_ASSERT(!is_http_5xx(600));
    }

    void test_ngap_request_flag() {
        CPPUNIT_ASSERT(!is_ngap_request());
        set_ngap_request(true);
        CPPUNIT_ASSERT(is_ngap_request());
        set_ngap_request(false);
        CPPUNIT_ASSERT(!is_ngap_request());
    }

    void test_add_service_chaining_message_http_error() {
        http::HttpError e("original", CURLE_OK, 503, "https://origin", "https://redirect", __FILE__, __LINE__);
        add_service_chaining_message(e, "prolog - ", "doing a thing.");
        CPPUNIT_ASSERT_EQUAL(string("prolog - Hyrax encountered a Service Chaining Error while doing a thing.\noriginal"),
                             e.get_message());
        CPPUNIT_ASSERT_EQUAL(503L, e.http_status());
    }

    void test_add_service_chaining_message_internal_error() {
        BESInternalError e("original", __FILE__, __LINE__);
        add_service_chaining_message(e, "", "doing a thing.");
        CPPUNIT_ASSERT_EQUAL(string("Hyrax encountered a Service Chaining Error while doing a thing.\noriginal"),
                             e.get_message());
    }

    CPPUNIT_TEST_SUITE(NgapServiceChainingTest);

    CPPUNIT_TEST(test_is_http_5xx);
    CPPUNIT_TEST(test_ngap_request_flag);
    CPPUNIT_TEST(test_add_service_chaining_message_http_error);
    CPPUNIT_TEST(test_add_service_chaining_message_internal_error);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NgapServiceChainingTest);

} // namespace ngap

int main(int argc, char *argv[]) {
    return bes_run_tests<ngap::NgapServiceChainingTest>(argc, argv, "cerr,ngap") ? 0 : 1;
}
