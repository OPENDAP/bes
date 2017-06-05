// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>

#include <DDS.h>

#include <GNURegex.h>
#include <debug.h>

#include "ObjMemCache.h"

static bool debug = false;
static bool debug_2 = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#undef DBG2
#define DBG2(x) do { if (debug_2) (x); } while(false);

using namespace CppUnit;
using namespace std;
using namespace libdap;

class DDSMemCacheTest: public TestFixture {
private:
    ObjMemCache *dds_cache;
    DDS *dds;

public:
    DDSMemCacheTest() :
        dds_cache(0), dds(0)
    {
    }

    ~DDSMemCacheTest()
    {
    }

    void setUp()
    {
        DBG2(cerr << "setUp() - BEGIN" << endl);

        dds_cache = new ObjMemCache;

        // Load in 10 DDS*s and then purge
        BaseTypeFactory factory;
        auto_ptr<DDS> dds(new DDS(&factory, "empty_DDS"));

        ostringstream oss;
        for (int i = 0; i < 10; ++i) {
            oss << i << "_DDS";
            string name = oss.str();
            DBG2(cerr << "Adding name: " << name << endl);
            // Need to add new pointers since the cache will delete them
            dds_cache->add(new DDS(*dds.get()), name);
            oss.str("");
        }

        DBG2(dds_cache->dump(cerr));

        DBG2(cerr << "setUp() - END" << endl);
    }

    void tearDown()
    {
        delete dds_cache;
        delete dds;
    }

    void ctor_test()
    {
        ObjMemCache empty_cache;
        DBG2(empty_cache.dump(cerr));

        CPPUNIT_ASSERT(empty_cache.cache.size() == 0);
        CPPUNIT_ASSERT(empty_cache.index.size() == 0);

        ObjMemCache *empty_cache_ptr = new ObjMemCache;
        DBG2(empty_cache_ptr->dump(cerr));

        CPPUNIT_ASSERT(empty_cache_ptr->cache.size() == 0);
        CPPUNIT_ASSERT(empty_cache_ptr->index.size() == 0);

        delete empty_cache_ptr;
    }

    void add_one_test()
    {
        ObjMemCache *cache = new ObjMemCache;

        const string name = "first DDS";
        BaseTypeFactory factory;
        auto_ptr<DDS> dds(new DDS(&factory, "empty_DDS"));

        cache->add(new DDS(*dds.get()), name);

        DBG2(cache->dump(cerr));

        CPPUNIT_ASSERT(cache->cache.size() == 1);
        CPPUNIT_ASSERT(cache->index.size() == 1);

        delete cache;
    }

    void add_two_test()
    {
        ObjMemCache *cache = new ObjMemCache;

        BaseTypeFactory factory;
        DDS *dds = new DDS(&factory, "first DDS");
        cache->add(dds, "first DDS");

        DDS *dds2 = new DDS(&factory, "second DDS");
        cache->add(dds2, "second DDS");

        DBG2(cache->dump(cerr));

        CPPUNIT_ASSERT(cache->cache.size() == 2);
        CPPUNIT_ASSERT(cache->index.size() == 2);

        //delete dds;   the Cache will delete them, so we don't have to
        //delete dds2;
        delete cache;
    }

    void purge_test()
    {
        CPPUNIT_ASSERT(dds_cache->cache.size() == 10);
        CPPUNIT_ASSERT(dds_cache->index.size() == 10);

        dds_cache->purge(0.2);

        DBG2(dds_cache->dump(cerr));

        CPPUNIT_ASSERT(dds_cache->cache.size() == 8);
        CPPUNIT_ASSERT(dds_cache->index.size() == 8);
    }

    void test_get_obj()
    {
        string name = "0_DDS";
        CPPUNIT_ASSERT(dds_cache->index.find(name)->second == 1);

        DDS *dds = static_cast<DDS*>(dds_cache->get(name));

        CPPUNIT_ASSERT(dds != 0);
        // check that the count is updated

        CPPUNIT_ASSERT(dds_cache->index.find(name)->second == 11);
    }

    void remove_test()
    {
        CPPUNIT_ASSERT(dds_cache->cache.size() == 10);
        CPPUNIT_ASSERT(dds_cache->index.size() == 10);

        //CPPUNIT_ASSERT(dds_cache->index.count("0_DDS") == 1);

        dds_cache->remove("0_DDS");
        dds_cache->remove("9_DDS");
        dds_cache->remove("5_DDS");

        DBG2(dds_cache->dump(cerr));

        CPPUNIT_ASSERT(dds_cache->cache.size() == 7);
        CPPUNIT_ASSERT(dds_cache->index.size() == 7);
    }

CPPUNIT_TEST_SUITE( DDSMemCacheTest );

    CPPUNIT_TEST(ctor_test);
    CPPUNIT_TEST(add_one_test);
    CPPUNIT_TEST(add_two_test);
    CPPUNIT_TEST(purge_test);
    CPPUNIT_TEST(test_get_obj);
    CPPUNIT_TEST(remove_test);

    CPPUNIT_TEST_SUITE_END()
    ;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DDSMemCacheTest);

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dDh");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'D':
            debug_2 = 1;
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: DDSMemCacheTest has the following tests:" << endl;
            const std::vector<Test*> &tests = DDSMemCacheTest::suite()->getTests();
            unsigned int prefix_len = DDSMemCacheTest::suite()->getName().append("::").length();
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
            test = DDSMemCacheTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

