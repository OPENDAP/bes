// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of bes, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2021 OPeNDAP, Inc.
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

#include <memory>
#include <exception>
#include <cstring>
#include <algorithm>
#include <iostream>     // std::cout, std::endl
#include <iomanip>      // std::setw

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include "zlib.h"

#include <libdap/debug.h>
#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/D4Attributes.h>
#include <libdap/D4Dimensions.h>
#include <libdap/Array.h>
#include <libdap/Byte.h>
#include <libdap/XMLWriter.h>

#include "url_impl.h"
#include "TheBESKeys.h"
#include "BESUtil.h"
#include "BESInternalError.h"

#include "DMZ.h"
#include "Chunk.h"
#include "DmrppCommon.h"
#include "DmrppArray.h"
#include "DmrppTypeFactory.h"
#include "Base64.h"
#include "vlsa_util.h"

#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

#include "modules/common/run_tests_cppunit.h"
#include "read_test_baseline.h"
#include "test_config.h"

using namespace std;
using namespace libdap;
using namespace bes;

#define prolog std::string("vlsa_util_test::").append(__func__).append("() - ")


namespace dmrpp {

class vlsa_util_test: public CppUnit::TestFixture {

private:
    unique_ptr<DMZ> d_dmz {nullptr};


    const string chunked_fourD_dmrpp = string(TEST_SRC_DIR).append("/input-files/chunked_fourD.h5.dmrpp");
    const string chunked_oneD_dmrpp = string(TEST_SRC_DIR).append("/input-files/chunked_oneD.h5.dmrpp");
    const string broken_dmrpp = string(TEST_SRC_DIR).append("/input-files/broken_elements.dmrpp");
    const string grid_2_2d_dmrpp = string(TEST_SRC_DIR).append("/input-files/grid_2_2d.h5.dmrpp");
    const string coads_climatology_dmrpp = string(TEST_SRC_DIR).append("/input-files/coads_climatology.dmrpp");
    const string test_array_6_1_dmrpp = string(TEST_SRC_DIR).append("/input-files/test_array_6.1.xml");
    const string test_simple_6_dmrpp = string(TEST_SRC_DIR).append("/input-files/test_simple_6.xml");
    const string vlsa_element_values_dmrpp = string(TEST_SRC_DIR).append("/input-files/vlsa_element_values.dmrpp");
    const string vlsa_base64_values_dmrpp = string(TEST_SRC_DIR).append("/input-files/vlsa_base64_values.dmrpp");

    const string omps = string(TEST_SRC_DIR).append("/input-files/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5.dmrpp");
    const string s5pnrtil = string(TEST_SRC_DIR).append("/input-files/S5PNRTIL2NO220180422T00470920180422T005209027060100110820180422T022729.nc.h5.dmrpp");
    const string scalar_contiguous  = string(TEST_SRC_DIR).append("/input-files/Scalar_contiguous_vlstr.h5.dmrpp");
    const string acos_l2 = string(TEST_SRC_DIR).append("/input-files/acos_L2s_110419_43_Production_v110110_L2s2800_r01_PolB_110430192739.h5.dmrpp");
    const string test_compress_doc_01 = string(TEST_SRC_DIR).append("/input-files/test_compress_doc.xml");

    string hr="# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --";

public:
    // Called once before everything gets tested
    vlsa_util_test() = default;

    // Called at the end of the test
    ~vlsa_util_test() override = default;

    // Called before each test
    void setUp() override
    {
        DBG( cerr << "\n" );
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
    }

    // This is a 'function try block' and provides a way to abstract the many
    // exception types. jhrg 10/28/21
    static void handle_fatal_exceptions() try {
        throw;
    }
    catch (const BESInternalError &e) {
        CPPUNIT_FAIL("Caught BESInternalError " + e.get_verbose_message());
    }
    catch (const BESError &e) {
        CPPUNIT_FAIL("Caught BESError " + e.get_verbose_message());
    }
    catch (const std::exception &e) {
        CPPUNIT_FAIL("Caught std::exception " + string(e.what()));
    }
    catch (...) {
        CPPUNIT_FAIL("Caught ? ");
    }


    void test_compress_1char(){
        uint8_t dest[2048];
        unsigned long dlen=100;
        const uint8_t *src = (uint8_t *) R"(+)";
        unsigned long slen=1;
        DBG(cerr << prolog << "source length: " << slen << "\n");

        auto retval = compress(dest, &dlen, src, slen);
        DBG(cerr << prolog << vlsa::zlib_msg(retval) << " (" << retval << ")\n");

        CPPUNIT_ASSERT(retval == Z_OK);
        DBG(cerr << prolog << "compressed length: " << dlen << "\n");

        if(debug) {
            cerr << prolog << "dest: \n";
            for (uint64_t i = 0; i < dlen; i++) {
                cerr << setw(2) << setfill('0') << hex  << (int) dest[i] << " ";
            }
            cerr << "\n";
        }
    }

    void test_vlsa_api(){
        unsigned long source_size;
        unsigned int delta = 1;
        uint64_t start = 1;
        uint64_t stop = 1000;


        DBG(cerr << hr << "\n");
        DBG(cerr << prolog << "Testing " << test_compress_doc_01 << "\n");

        string source_string = BESUtil::file_to_string(test_compress_doc_01);
        source_size = source_string.size();
        DBG(cerr << prolog << "source_string.size() " << dec << source_size << "\n");
        DBG(cout << "sample_string.size()" << ", " << "encoded.size()" << "\n");

        for(uint64_t sample_size = start;  sample_size <= stop  && sample_size <= source_size ; sample_size += delta){
            libdap::XMLWriter xml_writer; // freshy each pass
            bool encode_failed = false;
            string decoded;

            DBG(cerr << hr << "\n");
            DBG(cerr << prolog << "sample_size: " << sample_size << "\n");

            auto sample_string = source_string.substr(0,sample_size);
            DBG(cerr << prolog << "sample_string.size() " << sample_string.size() << "\n");
            // DBG(cerr << prolog << "sample_string: \n" << sample_string << "\n");

            vector<string> values;
            values.emplace_back(sample_string);

            try {
                // We write the value to the XML document encode etc handled inside.
                // Since we only made a single value above, when we encoded, and not a bunch
                // of values we use vlsa::write_value)_ to write just the value element into
                // the xml_writer.
                vlsa::write_value(xml_writer, sample_string);
                DBG(cerr << prolog << "xml_writer.get_doc(): \n\n" << xml_writer.get_doc() << "\n");

            }
            catch (BESInternalError bie) {
                DBG(cerr << prolog << "Failed to encode the string. message: " << bie.get_verbose_message() << "\n");
                encode_failed = true;
                throw bie;
            }
            catch (...) {
                handle_fatal_exceptions();
            }

            if(!encode_failed){
                try {
                    // We use pugi::xml to parse the XML we made above, with the encoded vlsa value on board
                    pugi::xml_document  result_xml_doc;
                    pugi::xml_parse_result result = result_xml_doc.load_string(xml_writer.get_doc());

                    // The we grab the top level elememt, which we know is the value element because
                    // we used vlsa::write_value() above.
                    auto value_element = result_xml_doc.document_element();

                    // pass the value element into read_value
                    vlsa::read_value(value_element, decoded);

                    if(sample_string == decoded){
                        DBG(cerr << prolog << "SUCCESS.\n");
                    }
                    else {
                        DBG(cerr << prolog << "TRANSFORMATION FAILED.\n");
                        DBG(cerr << prolog << "baseline: '");
                        DBG(cerr << sample_string << "'\n");
                        DBG(cerr << prolog << "  result: '");
                        DBG(cerr << decoded << "'\n");
                    }
                }
                catch (...) {
                    handle_fatal_exceptions();
                }
            }
            DBG(cerr << "\n");
        }
    }

    /**
     *
     */
    void test_compress_base64(){
        unsigned long source_size;
        uint64_t start = 1;
        uint64_t stop = 1000;
        unsigned int delta = 1;

        DBG(cerr << hr << "\n");

        DBG(cerr << prolog << "Testing " << test_compress_doc_01 << "\n");

        string source_string = BESUtil::file_to_string(test_compress_doc_01);
        source_size = source_string.size();
        DBG(cerr << prolog << "source_string.size() " << dec << source_size << "\n");
        DBG(cout << "sample_string.size()" << ", " << "encoded.size()" << "\n");

        for(uint64_t sample_size = start;  sample_size <= stop  && sample_size <= source_size ; sample_size += delta){
            libdap::XMLWriter xml; // freshy each pass
            bool encode_failed = false;
            string encoded;
            string decoded;

            DBG(cerr << hr << "\n");

            auto sample_string = source_string.substr(0,sample_size);
            DBG(cerr << prolog << "sample_string.size() " << sample_string.size() << "\n");
            try {
                encoded = vlsa::encode(sample_string);
                //DBG(cerr << prolog << "encoded: '" << encoded << "'\n");
            }
            catch (BESInternalError bie) {
                DBG(cerr << prolog << "Failed to encode the string. message: " << bie.get_verbose_message() << "\n");
                encode_failed = true;
            }
            catch (...) {
                handle_fatal_exceptions();
            }

            if(!encode_failed){
                try {
                    decoded = vlsa::decode(encoded, sample_string.size());
                    //DBG(cerr << prolog << "decoded: '" << decoded << "'\n");

                    if(sample_string == decoded){
                        DBG(cerr << prolog << "SUCCESS.\n");
                        DBG( cout << sample_string.size() << ", " << encoded.size() << "\n" );
                    }
                    else {
                        DBG(cerr << prolog << "TRANSFORMATION FAILED.\n");
                        DBG(cerr << prolog << "baseline: '");
                        DBG(cerr << sample_string << "'\n");
                        DBG(cerr << prolog << " decoded: '");
                        DBG(cerr << decoded << "'\n");
                    }
                }
                    //   catch (BESInternalError bie) {
                    //     DBG(cerr << prolog << "Failed to decode the string. message: \n" << bie.get_verbose_message() << "\n");
                    // }
                catch (...) {
                    handle_fatal_exceptions();
                }

            }


            //CPPUNIT_ASSERT( decoded == sample_string);
            DBG(cerr << "\n");
        }
    }

CPPUNIT_TEST_SUITE( vlsa_util_test );

        CPPUNIT_TEST(test_compress_1char);
        CPPUNIT_TEST(test_vlsa_api);
        CPPUNIT_TEST(test_compress_base64);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(vlsa_util_test);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    auto bes_debug_args="cerr,vlsa";

    return bes_run_tests<dmrpp::vlsa_util_test>(argc, argv, bes_debug_args) ? 0 : 1;
}
