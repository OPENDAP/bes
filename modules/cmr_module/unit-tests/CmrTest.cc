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

#include <memory>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
#include <cstdio>
#include <cstring>
#include <iostream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <curl/curl.h>

#include <XMLWriter.h>
#include <GetOpt.h>
#include <util.h>
#include <debug.h>

#include <BESError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TheBESKeys.h>
#include "test_config.h"

#include "RemoteHttpResource.h"
#include "CmrError.h"

#define MODULE "cmr"

using namespace libdap;
using namespace rapidjson;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace cmr {

class CmrTest: public CppUnit::TestFixture {
private:

    // char curl_error_buf[CURL_ERROR_SIZE];

    void show_file(string filename)
    {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
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
    CmrTest()
    {
    }

    // Called at the end of the test
    ~CmrTest()
    {
    }

    // Called before each test
    void setUp()
    {
        string bes_conf = BESUtil::assemblePath(TEST_SRC_DIR,"CmrTest.ini");
        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) BESDebug::SetUp("cerr,cmr");

        if (bes_debug) show_file(bes_conf);
    }

    // Called after each test
    void tearDown()
    {
    }

    void getJsonDoc(const string &url, rapidjson::Document &d){
        if(debug) cerr << endl << "Trying url: " << url << endl;
        cmr::RemoteHttpResource rhr(url);
        rhr.retrieveResource();
        FILE* fp = fopen(rhr.getCacheFileName().c_str(), "r"); // non-Windows use "r"
        char readBuffer[65536];
        rapidjson::FileReadStream frs(fp, readBuffer, sizeof(readBuffer));
        d.ParseStream(frs);
    }

    std::string getStringValue(const Value& object, const string name){
        string prolog = string(__func__) + "() - ";

        string response;
        rapidjson::Value::ConstMemberIterator itr = object.FindMember(name.c_str());
        bool result  = itr != object.MemberEnd();
        string msg = prolog + (result?"Located":"FAILED to locate") + " the value '"+name+"' in object.";
        BESDEBUG(MODULE, msg << endl);
        if(!result){
            return response;
        }

        const Value& myValue = itr->value;
        result = myValue.IsString();
        msg = prolog + "The value '"+ name +"' is" + (result?"":" NOT") + " a String type.";
        BESDEBUG(MODULE, msg << endl);
        if(!result){
            return response;
        }

        return myValue.GetString();
    }

    bool getBooleanValue(const Value& object, const string name){
        string prolog = string(__func__) + "() - ";

        rapidjson::Value::ConstMemberIterator itr = object.FindMember(name.c_str());
        bool result  = itr != object.MemberEnd();
        string msg = prolog + (result?"Located":"FAILED to locate") + " the value '"+name+"' in object.";
        BESDEBUG(MODULE, msg << endl);
        if(!result){
            return false;
        }

        const Value& myValue = itr->value;
        result = myValue.IsBool();
        msg = prolog + "The value '"+ name +"' is" + (result?"":" NOT") + " a Boolean type.";
        BESDEBUG(MODULE, msg << endl);
        if(!result){
            return false;
        }

        return myValue.GetBool();
    }

    std::string jsonDocToString(rapidjson::Document &d){
        StringBuffer buffer;
        rapidjson::PrettyWriter<StringBuffer> writer(buffer);
        d.Accept(writer);
        return buffer.GetString();
    }

    void get_collection_years(string collection_name, vector<string> &collection_years){

        string url = "https://cmr.earthdata.nasa.gov/search/granules.json?concept_id="+collection_name +"&include_facets=v2";
        rapidjson::Document doc;
        getJsonDoc(url,doc);

        string prolog = string(__func__) + "() - ";

        BESDEBUG(MODULE, __func__ << "() - Got JSON Document: "<< endl << jsonDocToString(doc) << endl);

        bool result = doc.IsObject();
        string msg = prolog + "Json document is" + (result?"":" NOT") + " an object.";
        BESDEBUG(MODULE, msg << endl);
        if(!result){
            throw CmrError(msg,__FILE__,__LINE__);
        }

        //################### feed
        rapidjson::Value::ConstMemberIterator itr = doc.FindMember("feed");
        result  = itr != doc.MemberEnd();
        msg = string(__func__) + "() - " + (result?"Located":"FAILED to locate") + " the value 'feed'.";
        BESDEBUG(MODULE, msg << endl);
        if(!result){
            throw CmrError(msg,__FILE__,__LINE__);
        }

        const Value& feed = itr->value;
        result  = feed.IsObject();
        msg = prolog + "The value 'feed' is" + (result?"":" NOT") + " an object.";
        BESDEBUG(MODULE, msg << endl);
        if(!result){
            throw CmrError(msg,__FILE__,__LINE__);
        }

        //################### facets
        itr = feed.FindMember("facets");
        result  = itr != feed.MemberEnd();
        msg =  prolog + (result?"Located":"FAILED to locate") + " the value 'facets'." ;
        BESDEBUG(MODULE, msg << endl);
        if(!result){
            throw CmrError(msg,__FILE__,__LINE__);
        }

        const Value& facets_obj = itr->value;
        result  = facets_obj.IsObject();
        msg =  prolog + "The value 'facets' is" + (result?"":" NOT") + " an object.";
        BESDEBUG(MODULE, msg << endl);
        if(!result){
            throw CmrError(msg,__FILE__,__LINE__);
        }

        //################### children
        itr = facets_obj.FindMember("children");
        result  = itr != feed.MemberEnd();
        msg = prolog + (result?"Located":"FAILED to locate") + " the value 'children' in 'facets'.";
        BESDEBUG(MODULE, msg << endl);
        if(!result){
            throw CmrError(msg,__FILE__,__LINE__);
        }

        const Value& facets = itr->value;
        result = facets.IsArray();
        msg = prolog + "The value 'children' is" + (result?"":" NOT") + " an array.";
        BESDEBUG(MODULE, msg << endl);
        if(!result){
            throw CmrError(msg,__FILE__,__LINE__);
        }

        for (SizeType i = 0; i < facets.Size(); i++) { // Uses SizeType instead of size_t
            const Value& facets_child = facets[i];

            string facet_title = getStringValue(facets_child,"title");
            string temporal_title("Temporal");
            if(facet_title == temporal_title){
                msg = prolog + "Found Temporal object.";
                BESDEBUG(MODULE, msg << endl);

                //################### children
                itr = facets_child.FindMember("children");
                result  = itr != feed.MemberEnd();
                msg = prolog + (result?"Located":"FAILED to locate") + " the value 'children' in the object titled 'Temporal'.";
                BESDEBUG(MODULE, msg << endl);
                if(!result){
                    throw CmrError(msg,__FILE__,__LINE__);
                }

                const Value& temporal_children = itr->value;
                result = temporal_children.IsArray();
                msg = prolog + "The value 'children' is" + (result?"":" NOT") + " an array.";
                BESDEBUG(MODULE, msg << endl);
                if(!result){
                    throw CmrError(msg,__FILE__,__LINE__);
                }


                for (SizeType j = 0; j < temporal_children.Size(); j++) { // Uses SizeType instead of size_t
                    const Value& temporal_child = temporal_children[j];

                    string temporal_child_title = getStringValue(temporal_child,"title");
                    string year_title("Year");
                    if(temporal_child_title == year_title){
                        msg = prolog + "Found Year object.";
                        BESDEBUG(MODULE, msg << endl);

                        itr = temporal_child.FindMember("children");
                        result  = itr != feed.MemberEnd();
                        msg = prolog + (result?"Located":"FAILED to locate") + " the value 'children' in the object titled 'Year'.";
                        BESDEBUG(MODULE, msg << endl);
                        if(!result){
                            throw CmrError(msg,__FILE__,__LINE__);
                        }

                        const Value& years = itr->value;
                        result = years.IsArray();
                        msg = prolog + "The value 'children' is" + (result?"":" NOT") + " an array.";
                        BESDEBUG(MODULE, msg << endl);
                        if(!result){
                            throw CmrError(msg,__FILE__,__LINE__);
                        }

                        for (SizeType k = 0; k < years.Size(); k++) { // Uses SizeType instead of size_t
                            const Value& year_obj = years[k];
                            string year = getStringValue(year_obj,"title");
                            collection_years.push_back(year);
                        }
                        return;
                    }
                    else {
                        msg = prolog + "The child of 'Temporal' with title '"+temporal_child_title+"' does not match 'Year'";
                        BESDEBUG(MODULE, msg << endl);
                    }
                }
            }
            else {
                msg = prolog + "The child of 'facets' with title '"+facet_title+"' does not match 'Temporal'";
                BESDEBUG(MODULE, msg << endl);
            }
        }






    }



    void get_collection_years_test()
    {
        string prolog = string(__func__) + "() - ";

        string collection_name = "C179003030-ORNL_DAAC";
        string expected[] = {
                string("1984"),
                string("1985"),
                string("1986"),
                string("1987"),
                string("1988")
        };

        vector<string> years;
        try {
            get_collection_years(collection_name,years);

            CPPUNIT_ASSERT(5 == years.size());

            stringstream msg;
            msg << prolog << "The collection '" << collection_name << "' spans " << years.size() << " years: ";
            for(size_t i=0; i<years.size(); i++){
                if(i>0) msg << ", ";
                msg << years[i];
            }
            BESDEBUG(MODULE,msg.str() << endl);

            for(size_t i=0; i<years.size(); i++){
                msg.str(std::string());
                msg << prolog << "Checking:  expected: "<< expected[i] << " received: " << years[i];
                BESDEBUG(MODULE,msg.str() << endl);
                CPPUNIT_ASSERT(expected[i] == years[i]);
            }

     }
        catch (BESError &be){
            string msg = "Caught BESError! Message: " + be.get_message() ;
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }


    }

    CPPUNIT_TEST_SUITE( CmrTest );

    CPPUNIT_TEST(get_collection_years_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CmrTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "db");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            break;
        case 'b':
            bes_debug = true;  // debug is a static global
            break;
        default:
            break;
        }

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
            test = cmr::CmrTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
