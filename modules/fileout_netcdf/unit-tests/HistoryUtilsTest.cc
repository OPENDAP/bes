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

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <BESDebug.h>

#include "test_config.h"
#include "history_utils.h"

using namespace std;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

#define prolog std::string("HistoryUtilsTest::").append(__func__).append("() - ")


class HistoryUtilsTest: public CppUnit::TestFixture {

private:
    string d_tmpDir;

    string fileToString(const string &fn)
    {
        ifstream is;
        is.open(fn.c_str(), ios::binary);
        if (!is || is.eof()) return "";

        // get length of file:
        is.seekg(0, ios::end);
        int length = is.tellg();

        // back to start
        is.seekg(0, ios::beg);

        // allocate memory:
        vector<char> buffer(length + 1);

        // read data as a block:
        is.read(&buffer[0], length);
        is.close();
        buffer[length] = '\0';

        return string(&buffer[0]);
    }

public:

    // Called once before everything gets tested
    HistoryUtilsTest() :
        d_tmpDir(string(TEST_BUILD_DIR) + "/tmp")
    {
        DBG(cerr << "FoJsonTest - Constructor" << endl);
    }

    // Called at the end of the test
    ~HistoryUtilsTest()
    {
        DBG(cerr << "FoJsonTest - Destructor" << endl);
    }

    // Called before each test
    void setUp() override
    {
        DBG(cerr << endl);
        // The following will show threads joined after an exception was thrown by a thread
        if (bes_debug) BESDebug::SetUp("cerr,fonc");
    }

    // Called after each test
    void tearDown()
    {
    }


#if 0
    libdap::DataDDS *makeSimpleTypesDDS()
    {
        // build a DataDDS of simple types and set values for each of the
        // simple types.
        libdap::DataDDS *dds = new libdap::DataDDS(NULL, "SimpleTypes");

        libdap::Byte b("byte");
        b.set_value(28);
        b.set_send_p(true);
        dds->add_var(&b);

        libdap::Int16 i16("i16");
        i16.set_value(-2048);
        i16.set_send_p(true);
        dds->add_var(&i16);

        libdap::Int32 i32("i32");
        i32.set_value(-105467);
        i32.set_send_p(true);
        dds->add_var(&i32);

        libdap::UInt16 ui16("ui16");
        ui16.set_value(2048);
        ui16.set_send_p(true);
        dds->add_var(&ui16);

        libdap::UInt32 ui32("ui32");
        ui32.set_value(105467);
        ui32.set_send_p(true);
        dds->add_var(&ui32);

        libdap::Float32 f32("f32");
        f32.set_value(5.7866);
        f32.set_send_p(true);
        dds->add_var(&f32);

        libdap::Float64 f64("f64");
        f64.set_value(10245.1234);
        f64.set_send_p(true);
        dds->add_var(&f64);

        libdap::Str s("str");
        s.set_value("This is a String Value");
        s.set_send_p(true);
        dds->add_var(&s);

        return dds;
    }

    libdap::DataDDS *makeTestDDS()
    {
        // build a DataDDS of simple types and set values for each of the
        // simple types.
        libdap::DataDDS *dds = new libdap::DataDDS(NULL, "TestDataset");

        // ###################  SIMPLE TYPES ###################
        libdap::Byte b("byte");
        b.set_value(28);
        b.set_send_p(true);
        dds->add_var(&b);

        libdap::Int16 i16("i16");
        i16.set_value(-2048);
        i16.set_send_p(true);
        dds->add_var(&i16);

        libdap::Int32 i32("i32");
        i32.set_value(-105467);
        i32.set_send_p(true);
        dds->add_var(&i32);

        libdap::UInt16 ui16("ui16");
        ui16.set_value(2048);
        ui16.set_send_p(true);
        dds->add_var(&ui16);

        libdap::UInt32 ui32("ui32");
        ui32.set_value(105467);
        ui32.set_send_p(true);
        dds->add_var(&ui32);

        libdap::Float32 f32("f32");
        f32.set_value(5.7866);
        f32.set_send_p(true);
        dds->add_var(&f32);

        libdap::Float64 f64("f64");
        f64.set_value(10245.1234);
        f64.set_send_p(true);
        dds->add_var(&f64);

        libdap::Str s("str");
        s.set_value("This is a String Value");
        s.set_send_p(true);
        dds->add_var(&s);

        // ###################  ARRAYS OF SIMPLE TYPES ###################

        libdap::Float64 tmplt("oneDArrayF64");
        libdap::Array oneDArrayF64("oneDArrayF64", &tmplt);

        int dim1Size = 2;
        double pi = atan(1) * 4;
        libdap::dods_float64 oneDdata[dim1Size];
        for (long i = 0; i < dim1Size; i++)
            oneDdata[i] = pi * (i * 0.1);

        oneDArrayF64.append_dim(dim1Size, "dim1");
        oneDArrayF64.set_value(oneDdata, dim1Size);
        oneDArrayF64.set_send_p(true);
        dds->add_var(&oneDArrayF64);

        libdap::Float64 tmplt2("twoDArrayF64");
        libdap::Array twoDArrayF64("twoDArrayF64", &tmplt2);

        int dim2Size = 4;
        int totalSize = dim1Size * dim2Size;
        libdap::dods_float64 twoDdata[totalSize];
        for (long i = 0; i < totalSize; i++)
            twoDdata[i] = pi * (i * 0.01);

        twoDArrayF64.append_dim(dim1Size, "dim1");
        twoDArrayF64.append_dim(dim2Size, "dim2");
        twoDArrayF64.set_value(twoDdata, totalSize);
        twoDArrayF64.set_send_p(true);
        dds->add_var(&twoDArrayF64);

        libdap::UInt32 tmplt4("twoDArrayUI32");
        libdap::Array twoDArrayUI32("twoDArrayUI32", &tmplt4);

        totalSize = dim1Size * dim2Size;
        libdap::dods_uint32 uint32data[totalSize];
        unsigned int val = 0;
        for (long i = 0; i < totalSize; i++) {
            val = i + val;
            uint32data[i] = val;
        }

        twoDArrayUI32.append_dim(dim1Size, "dim1");
        twoDArrayUI32.append_dim(dim2Size, "dim2");
        twoDArrayUI32.set_value(uint32data, totalSize);
        twoDArrayUI32.set_send_p(true);
        dds->add_var(&twoDArrayUI32);

        libdap::Float64 tmplt3("threeDArrayF64");
        libdap::Array threeDArrayF64("threeDArrayF64", &tmplt3);

        int dim3Size = 5;
        totalSize = dim1Size * dim2Size * dim3Size;
        libdap::dods_float64 threeDdata[totalSize];
        for (long i = 0; i < totalSize; i++)
            threeDdata[i] = pi * (i * 0.001);

        threeDArrayF64.append_dim(dim1Size, "dim1");
        threeDArrayF64.append_dim(dim2Size, "dim2");
        threeDArrayF64.append_dim(dim3Size, "dim3");
        threeDArrayF64.set_value(threeDdata, totalSize);
        threeDArrayF64.set_send_p(true);
        dds->add_var(&threeDArrayF64);

        // ###################  STRUCTURE   ###################
        libdap::Structure structure("test_structure");

        libdap::Byte sb("byte");
        sb.set_value(238);
        sb.set_send_p(true);
        structure.add_var(&sb);

        libdap::Int16 si16("i16");
        si16.set_value(-1041);
        si16.set_send_p(true);
        structure.add_var(&si16);

        libdap::Str fooStr("fooStr");
        fooStr.set_value("This is the structure foo string.");
        fooStr.set_send_p(true);
        structure.add_var(&fooStr);

        structure.set_send_p(true);
        dds->add_var(&structure);

        // ###################  SEQUENCE   ###################
        libdap::Sequence sequence("test_sequence");

        libdap::Byte sqb("byte");
        sqb.set_value(238);
        sqb.set_send_p(true);
        sequence.add_var(&sqb);

        libdap::Int16 sqi16("i16");
        sqi16.set_value(-1041);
        sqi16.set_send_p(true);
        sequence.add_var(&sqi16);

        libdap::Str sfooStr("fooStr");
        sfooStr.set_value("This is the sequence foo string.");
        sfooStr.set_send_p(true);
        sequence.add_var(&sfooStr);

        sequence.set_send_p(true);
        dds->add_var(&sequence);

        // ###################  GRID   ###################
        libdap::Grid grid("test_grid");

        libdap::Float64 sstTemplate("test_grid");
        libdap::Array sstArray("test_grid", &sstTemplate);

        libdap::Float64 lngtemplate("longitude");
        libdap::Array lngArray("longitude", &lngtemplate);

        libdap::Float64 lattemplate("latitude");
        libdap::Array latArray("latitude", &lattemplate);

        int lngSize = 36;
        int latSize = 18;
        totalSize = lngSize * latSize;
        libdap::dods_float64 testData[totalSize];
        libdap::dods_float64 latData[latSize];
        libdap::dods_float64 lngData[lngSize];
        //unsigned int val = 0;
        //for(long i=0; i<totalSize ;i++){
        //	sstData[i] = pi * (i * 0.01);
        //}
        int i = 0;
        for (int lngVal = 0; lngVal < lngSize; lngVal++) {
            lngData[lngVal] = (lngVal - lngSize / 2) + 0.0;
            for (int latVal = 0; latVal < latSize; latVal++) {
                latData[latVal] = (latVal - latSize / 2) + 0.0;
                testData[i] = pi * ((lngData[lngVal] + latData[latVal]) * 0.01);
                i++;
            }
        }
        sstArray.append_dim(lngSize, "longitude");
        sstArray.append_dim(latSize, "latitude");
        sstArray.set_value(testData, totalSize);  // creates space and uses memcopy to transfer values.
        sstArray.set_send_p(true);
        grid.add_var(&sstArray, libdap::array); // add a copy

        lngArray.append_dim(lngSize, "longitude");
        lngArray.set_value(lngData, lngSize);  // creates space and uses memcopy to transfer values.
        lngArray.set_send_p(true);
        //grid.add_var(&lngArray, maps);   // add a copy
        grid.add_map(&lngArray, true);

        latArray.append_dim(latSize, "latitude");
        latArray.set_value(latData, latSize);  // creates space and uses memcopy to transfer values.
        latArray.set_send_p(true);
        //grid.add_var(&latArray, maps);   // add a copy
        grid.add_map(&latArray, true);

        grid.set_send_p(true);
        dds->add_var(&grid);       // add a copy

        DBG(cerr << "FoJsonTest::makeTestDDS(): " << endl);
        DBG(dds->print_constrained(cerr));

        return dds;
    }
#endif

    void json_append_entry_to_array_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        //string hj_entry_str = get_hj_entry("OHMYOHMYOHMY");
       // string history_json=R"([{"$schema":"https:\/\/harmony.earthdata.nasa.gov\/schemas\/history\/0.1.0\/history-0.1.0.json","date_time":"2021-06-25T13:28:48.951+0000","program":"hyrax","version":"@HyraxVersion@","parameters":[{"request_url":"http:\/\/localhost:8080\/opendap\/hj\/coads_climatology.nc.dap.nc4?GEN1"}]}])";

        string target_array = R"([ {"thing1":"one_fish"}, {"thing2":"two_fish"} ])";
        string new_entry = R"({"thing3":"red_fish"})";
        string expected = R"([{"thing1":"one_fish"},{"thing2":"two_fish"},{"thing3":"red_fish"}])";
        string result = json_append_entry_to_array(target_array,new_entry);

        DBG(cerr << prolog << "target_array: " << target_array << endl);
        DBG(cerr << prolog << "new_entry: " << new_entry << endl);
        DBG(cerr << prolog << "result: " << result << endl);

        CPPUNIT_ASSERT( result == expected );

        new_entry = R"({"thing4":"blue_fish"})";
        expected = R"([{"thing1":"one_fish"},{"thing2":"two_fish"},{"thing3":"red_fish"},{"thing4":"blue_fish"}])";

        result = json_append_entry_to_array(result,new_entry);
        DBG(cerr << prolog << "new_entry: " << new_entry << endl);
        DBG(cerr << prolog << "result: " << result << endl);
        CPPUNIT_ASSERT( result == expected );



        DBG(cerr << prolog << "END" << endl);
    }

    CPPUNIT_TEST_SUITE( HistoryUtilsTest );

        CPPUNIT_TEST(json_append_entry_to_array_test);

    CPPUNIT_TEST_SUITE_END();


};

CPPUNIT_TEST_SUITE_REGISTRATION(HistoryUtilsTest);


int main(int argc, char *argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());


    int option_char;
    while ((option_char = getopt(argc, argv, "db")) != -1)
        switch (option_char) {
            case 'd':
                debug = true;  // debug is a static global
                break;
            case 'b':
                debug = true;  // debug is a static global
                bes_debug = true;  // debug is a static global
                break;
            default:
                break;
        }

    argc -= optind;
    argv += optind;

    bool wasSuccessful = true;
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << prolog << "Running " << argv[i] << endl;
            string test = HistoryUtilsTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}

