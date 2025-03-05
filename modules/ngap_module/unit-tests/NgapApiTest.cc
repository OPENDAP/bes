// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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

#include <memory>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <unistd.h>
#include <libdap/util.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESCatalogList.h"
#include "TheBESKeys.h"

#include "url_impl.h"

#include "NgapApi.h"
#include "NgapNames.h"

#include "test_config.h"
#include "run_tests_cppunit.h"
#include "read_test_baseline.h"

using namespace std;
using namespace rapidjson;

#define prolog std::string("NgapApiTest::").append(__func__).append("() - ")

namespace ngap {

class NgapApiTest: public CppUnit::TestFixture {
private:
    static void show_file(const string &filename)
    {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            const string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            cout << endl << "##################################################################" << endl;
            cout << "file: " << filename << endl;
            cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
            cout << file_content << endl;
            cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
        }
    }

public:
    // Called once before everything gets tested
    NgapApiTest() = default;

    // Called at the end of the test
    ~NgapApiTest() override = default;

    // Called before each test
    void setUp() override
    {
        DBG(cerr << endl);
        DBG2(cerr << "setUp() - BEGIN" << endl);
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
        DBG2(cerr << "setUp() - Using BES configuration: " << bes_conf << endl);

        TheBESKeys::ConfigFile = bes_conf;

        if (debug2) show_file(bes_conf);

        DBG2(cerr << "setUp() - END" << endl);
    }

    static void show_vector(const vector<string> &v) {
        cerr << "show_vector(): Found " << v.size() << " elements." << endl;
        for(size_t i=0;  i < v.size(); i++){
            cerr << "show_vector:    v["<< i << "]: " << v[i] << endl;
        }
    }

    static void compare_results(const string &granule_name, const string &data_access_url, const string &expected_data_access_url) {
        if (debug) cerr << prolog << "TEST: Is the URL longer than the granule name? " << endl;
        CPPUNIT_ASSERT (data_access_url.size() > granule_name.size() );

        if (debug) cerr << prolog << "TEST: Does the URL end with the granule name? " << endl;
        bool endsWithGranuleName = data_access_url.substr(data_access_url.size()-granule_name.size(), granule_name.size()) == granule_name;
        CPPUNIT_ASSERT( endsWithGranuleName == true );

        if (debug) cerr << prolog << "TEST: Does the returned URL match the expected URL? " << endl;
        if (debug) cerr << prolog << "CMR returned DataAccessURL: " << data_access_url << endl;
        if (debug) cerr << prolog << "The expected DataAccessURL: " << expected_data_access_url << endl;
        CPPUNIT_ASSERT (expected_data_access_url == data_access_url);
    }

    /**
     * This test exercises the legacy 3 component restified path model
     * /providers/<provider_id>/collections/<entry_title>/granules/<granule_ur>
     */
    void resty_path_to_cmr_query_test_01() {
        DBG(cerr << prolog << "BEGIN" << endl);

        string resty_path("providers/POCLOUD"
                          "/collections/Sentinel-6A MF/Jason-CS L2 Advanced Microwave Radiometer (AMR-C) NRT Geophysical Parameters"
                          "/granules/S6A_MW_2__AMR_____NR_001_227_20201130T133814_20201130T153340_F00");
        DBG(cerr << prolog << "resty_path: " << resty_path << endl);

        string expected_cmr_url(
                "https://cmr.earthdata.nasa.gov/search/granules.umm_json_v1_4"
                "?" CMR_PROVIDER "=POCLOUD"
                "&" CMR_ENTRY_TITLE "=Sentinel-6A%20MF%2FJason-CS%20L2%20Advanced%20Microwave%20Radiometer%20%28AMR-C%29%20NRT%20Geophysical%20Parameters"
                "&" CMR_GRANULE_UR "=S6A_MW_2__AMR_____NR_001_227_20201130T133814_20201130T153340_F00"
        );
        try {
            string cmr_query_url;
            cmr_query_url = NgapApi::build_cmr_query_url(resty_path);
            DBG(cerr << prolog << "expected_cmr_url: " << expected_cmr_url << endl);
            DBG(cerr << prolog << "   cmr_query_url: " << cmr_query_url << endl);
            CPPUNIT_ASSERT( cmr_query_url == expected_cmr_url );
        }
        catch(BESError e){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << e.get_verbose_message() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        DBG(cerr << prolog << "END" << endl);
    }

    /**
     * This test exercises the new (12/2020) 2 component restified path model
     * /collections/<collection_concept_id>/granules/<granule_ur>
     * Example:
     * https://opendap.earthdata.nasa.gov/collections/C1443727145-LAADS/granules/MOD08_D3.A2020308.061.2020309092644.hdf.nc
     */
    void resty_path_to_cmr_query_test_02() {
        DBG(cerr << prolog << "BEGIN" << endl);

        string resty_path("/collections/C1443727145-LAADS/MOD08_D3.v6.1/granules/MOD08_D3.A2020308.061.2020309092644.hdf.nc");
        DBG(cerr << prolog << "resty_path: " << resty_path << endl);

        string expected_cmr_url(
                "https://cmr.earthdata.nasa.gov/search/granules.umm_json_v1_4?"
                CMR_COLLECTION_CONCEPT_ID "=C1443727145-LAADS&"
                CMR_GRANULE_UR "=MOD08_D3.A2020308.061.2020309092644.hdf.nc"
        );
        try {
            string cmr_query_url;
            cmr_query_url = NgapApi::build_cmr_query_url(resty_path);
            DBG(cerr << prolog << "expected_cmr_url: " << expected_cmr_url << endl);
            DBG(cerr << prolog << "   cmr_query_url: " << cmr_query_url << endl);
            CPPUNIT_ASSERT( cmr_query_url == expected_cmr_url );
        }
        catch(BESError e){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << e.get_verbose_message() << endl;
            CPPUNIT_FAIL(msg.str());
        }
    }

    /**
     * This test exercises the new (12/2020) 2 component restified path model with the optional shortname and version
     * /collections/<collection_concept_id>[/short_name.version]/granules/<granule_ur>
     * Example:
     * https://opendap.earthdata.nasa.gov/collections/C1443727145-LAADS/MOD08_D3.v6.1/granules/MOD08_D3.A2020308.061.2020309092644.hdf.nc
     */
    void resty_path_to_cmr_query_test_03() {
        DBG(cerr << prolog << "BEGIN" << endl);

        string resty_path("/collections/C1443727145-LAADS/MOD08_D3.v6.1/granules/MOD08_D3.A2020308.061.2020309092644.hdf.nc");
        DBG(cerr << prolog << "resty_path: " << resty_path << endl);

        string expected_cmr_url(
                "https://cmr.earthdata.nasa.gov/search/granules.umm_json_v1_4?"
                CMR_COLLECTION_CONCEPT_ID "=C1443727145-LAADS&"
                CMR_GRANULE_UR "=MOD08_D3.A2020308.061.2020309092644.hdf.nc"
        );
        try {
            string cmr_query_url;
            cmr_query_url = NgapApi::build_cmr_query_url(resty_path);
            DBG(cerr << prolog << "expected_cmr_url: " << expected_cmr_url << endl);
            DBG(cerr << prolog << "   cmr_query_url: " << cmr_query_url << endl);
            CPPUNIT_ASSERT( cmr_query_url == expected_cmr_url );
        }
        catch(BESError e){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << e.get_verbose_message() << endl;
            CPPUNIT_FAIL(msg.str());
        }
    }

    void signed_url_is_expired_test(){
        DBG(cerr << prolog << "BEGIN" << endl);
        std::map<std::string,std::string> url_info;

        const string signed_url_str = "https://ghrcw-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20200512v7.nc?"
              "A-userid=hyrax"
              "&X-Amz-Algorithm=AWS4-HMAC-SHA256"
              "&X-Amz-Credential=SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
              "&X-Amz-Date=20200621T161744Z"
              "&X-Amz-Expires=86400"
              "&X-Amz-Security-Token=FwoGZXIvYXdzENL%2F%2F%2F%2F%2F%2F%2F%2F%2F%2FwEaDKmu"
              "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
              "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
              "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
              "SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffingSomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing"
              "&X-Amz-SignedHeaders=host"
              "&X-Amz-Signature=SomeBigMessyAwfulEncodedEscapeBunchOfCryptoPhaffing";

        http::url signed_url(signed_url_str);

        time_t now;
        time(&now);
        stringstream ingest_time;
        time_t then = now - 82810; // 23 hours and 10 seconds ago.

        signed_url.set_ingest_time(then);
        bool is_expired = NgapApi::signed_url_is_expired(signed_url);
        CPPUNIT_ASSERT(is_expired == true);
        DBG(cerr << prolog << "END" << endl);
    }

    void test_find_get_data_url_in_granules_umm_json_v1_4_lpdaac() {
        // not a test baseline, but a canned response from LPDAAC
        string cmr_canned_response_lpdaac = bes::read_test_baseline(string(TEST_SRC_DIR) + "/cmr_json_responses/ECOv002_L1B_GEO_22172_008_20220604T024955_0700_01.json");
        rapidjson::Document cmr_response;
        cmr_response.Parse(cmr_canned_response_lpdaac.c_str());

        string data_url = NgapApi::find_get_data_url_in_granules_umm_json_v1_4("placeholder_for_restified_url", cmr_response);

        DBG(cerr << prolog << "data_url: " << data_url << endl);
        CPPUNIT_ASSERT_MESSAGE("data_url should not be empty", !data_url.empty());
        string expected = "https://data.lpdaac.earthdatacloud.nasa.gov/lp-prod-protected/ECO_L1B_GEO.002/ECOv002_L1B_GEO_22172_008_20220604T024955_0700_01/ECOv002_L1B_GEO_22172_008_20220604T024955_0700_01.h5";
        CPPUNIT_ASSERT_MESSAGE("data_url should be '" + expected + " but was '" + data_url, data_url == expected);
    }

    // C2251464384-POCLOUD, cyg04.ddmi.s20230410-000000-e20230410-235959.l1.power-brcs.a21.d21.json. jhrg 5/22/24
    void test_find_get_data_url_in_granules_umm_json_v1_4_podaac() {
        string cmr_canned_response_podaac = bes::read_test_baseline(string(TEST_SRC_DIR) + "/cmr_json_responses/cyg04.ddmi.s20230410-000000-e20230410-235959.l1.power-brcs.a21.d21.json");
        rapidjson::Document cmr_response;
        cmr_response.Parse(cmr_canned_response_podaac.c_str());

        string data_url = NgapApi::find_get_data_url_in_granules_umm_json_v1_4("placeholder_for_restified_url", cmr_response);

        DBG(cerr << prolog << "data_url: " << data_url << endl);
        CPPUNIT_ASSERT_MESSAGE("data_url should not be empty", !data_url.empty());
        string expected = "https://archive.podaac.earthdata.nasa.gov/podaac-ops-cumulus-protected/CYGNSS_L1_V2.1/2023/100/cyg04.ddmi.s20230410-000000-e20230410-235959.l1.power-brcs.a21.d21.nc";
        CPPUNIT_ASSERT_MESSAGE("data_url should be '" + expected + " but was '" + data_url, data_url == expected);
    }

    CPPUNIT_TEST_SUITE( NgapApiTest );

        CPPUNIT_TEST(resty_path_to_cmr_query_test_01);
        CPPUNIT_TEST(resty_path_to_cmr_query_test_02);
        CPPUNIT_TEST(resty_path_to_cmr_query_test_03);
        CPPUNIT_TEST(signed_url_is_expired_test);
        CPPUNIT_TEST(test_find_get_data_url_in_granules_umm_json_v1_4_lpdaac);
        CPPUNIT_TEST(test_find_get_data_url_in_granules_umm_json_v1_4_podaac);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NgapApiTest);

} // namespace dmrpp

int main(int argc, char *argv[]) {
    return bes_run_tests<ngap::NgapApiTest>(argc, argv, "cerr,ngap,http") ? 0 : 1;
}
