// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>, James Gallagher
// <jgallagher@opendap.org>
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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <DMR.h>

#include <BESDebug.h>

#include "DmrppArray.h"
#include "DmrppByte.h"
#include "DmrppCommon.h"
#include "DmrppD4Enum.h"
#include "DmrppD4Group.h"
#include "DmrppD4Opaque.h"
#include "DmrppD4Sequence.h"
#include "DmrppFloat32.h"
#include "DmrppFloat64.h"
#include "DmrppInt16.h"
#include "DmrppInt32.h"
#include "DmrppInt64.h"
#include "DmrppInt8.h"
#include "DmrppModule.h"
#include "DmrppStr.h"
#include "DmrppStructure.h"
#include "DmrppUInt16.h"
#include "DmrppUInt32.h"
#include "DmrppUInt64.h"
#include "DmrppUrl.h"

#include "DmrppParserSax2.h"
#include "DmrppRequestHandler.h"
#include "DmrppTypeFactory.h"

#include "GetOpt.h"
#include "test_config.h"
#include "util.h"

using namespace libdap;

static bool debug = false;

namespace dmrpp {

class DmrppTypeReadTest: public CppUnit::TestFixture {
private:
    DmrppParserSax2 parser;

public:
    // Called once before everything gets tested
    DmrppTypeReadTest() :
        parser()
    {
    }

    // Called at the end of the test
    ~DmrppTypeReadTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if (debug) BESDebug::SetUp("cerr,dmrpp");
    }

    // Called after each test
    void tearDown()
    {
    }

    /**
     * Evaluates a D4Group against exected values for the name, number of child
     * groups and the number of child variables.
     */
    void checkGroupsAndVars(D4Group *grp, string name, int expectedNumGrps, int expectedNumVars)
    {

        CPPUNIT_ASSERT(grp);
        BESDEBUG("dmrpp", "Checking D4Group '" << grp->name() << "'" << endl);

        CPPUNIT_ASSERT(grp->name() == name);

        int numGroups = grp->grp_end() - grp->grp_begin();
        BESDEBUG("dmrpp", "The D4Group '" << grp->name() << "' has " << numGroups << " child groups." << endl);
        CPPUNIT_ASSERT(numGroups == expectedNumGrps);

        int numVars = grp->var_end() - grp->var_begin();
        BESDEBUG("dmrpp", "The D4Group '" << grp->name() << "' has " << numVars << " child variables." << endl);
        CPPUNIT_ASSERT(numVars == expectedNumVars);

    }

    void test_integer_scalar() {
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);

        string int_h5 = string(TEST_DATA_DIR).append("/").append("t_int_scalar.h5.dmrpp");
        BESDEBUG("dmrpp", "Opening: " << int_h5 << endl);

        ifstream in(int_h5);
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", "Parsing complete"<< endl);

        D4Group *root = dmr->root();

        checkGroupsAndVars(root, "/", 0, 1);

        D4Group::Vars_iter v = root->var_begin();

        DmrppInt32 *di32 = dynamic_cast<DmrppInt32*>(*v);

        CPPUNIT_ASSERT(di32);

        try {
            // Hack for libcurl
            string data_url = string("file://").append(TEST_DATA_DIR).append(di32->get_data_url());
            di32->set_data_url(data_url);

            di32->read();

            BESDEBUG("dmrpp", "Value: " << di32->value() << endl);
            CPPUNIT_ASSERT(di32->value() == 45);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        CPPUNIT_ASSERT("Passed");
    }

    void test_integer_arrays() {
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);

        string int_h5 = string(TEST_DATA_DIR).append("/").append("d_int.h5.dmrpp");
        BESDEBUG("dmrpp", "Opening: " << int_h5 << endl);

        ifstream in(int_h5);
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", "Parsing complete"<< endl);

        D4Group *root = dmr->root();

        checkGroupsAndVars(root, "/", 0, 4);

        D4Group::Vars_iter v = root->var_begin();

        try {
            DmrppArray *da = dynamic_cast<DmrppArray*>(*v);

            CPPUNIT_ASSERT(da);
            CPPUNIT_ASSERT(da->name() == "d16_1");

            // Hack for libcurl
            string data_url = string("file://").append(TEST_DATA_DIR).append(da->get_data_url());
            da->set_data_url(data_url);

            da->read();

            CPPUNIT_ASSERT(da->length() == 2);

            vector<dods_int16> v16(da->length());
            da->value(&v16[0]);
            BESDEBUG("dmrpp", "v16[0]: " << v16[0] << ", v16[1]: " << v16[1] << endl);
            CPPUNIT_ASSERT(v16[0] == -32768);
            CPPUNIT_ASSERT(v16[1] == 32767);

            ++v;    // next
            da = dynamic_cast<DmrppArray*>(*v);

            CPPUNIT_ASSERT(da);
            CPPUNIT_ASSERT(da->name() == "d16_2");

            // Hack for libcurl
            da->set_data_url(data_url);

            da->read();

            CPPUNIT_ASSERT(da->length() == 4);

            vector<dods_int16> v16_2(da->length());
            da->value(&v16_2[0]);

            if (debug) copy(v16_2.begin(), v16_2.end(), ostream_iterator<dods_int16>(cerr, " "));

            CPPUNIT_ASSERT(v16_2[0] == -32768);
            CPPUNIT_ASSERT(v16_2[3] == 32767);

            ++v;    // next
            da = dynamic_cast<DmrppArray*>(*v);

            CPPUNIT_ASSERT(da);
            CPPUNIT_ASSERT(da->name() == "d32_1");

            // Hack for libcurl
            da->set_data_url(data_url);

            da->read();

            CPPUNIT_ASSERT(da->length() == 8);

            vector<dods_int32> v32(da->length());
            da->value(&v32[0]);

            if (debug) copy(v32.begin(), v32.end(), ostream_iterator<dods_int32>(cerr, " "));

            CPPUNIT_ASSERT(v32[0] == -2147483648);
            CPPUNIT_ASSERT(v32[7] == 2147483647);

            ++v;    // next
            da = dynamic_cast<DmrppArray*>(*v);

            CPPUNIT_ASSERT(da);
            CPPUNIT_ASSERT(da->name() == "d32_2");

            // Hack for libcurl
            da->set_data_url(data_url);

            da->read();

            CPPUNIT_ASSERT(da->length() == 32);

            vector<dods_int32> v32_2(da->length());
            da->value(&v32_2[0]);

            if (debug) copy(v32_2.begin(), v32_2.end(), ostream_iterator<dods_int32>(cerr, " "));

            CPPUNIT_ASSERT(v32_2[0] == -2147483648);
            CPPUNIT_ASSERT(v32_2[31] == 2147483647);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        CPPUNIT_ASSERT("Passed");
    }

    void test_float_grids() {
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);

        string grid_h5 = string(TEST_DATA_DIR).append("/").append("grid_2_2d.h5.dmrpp");
        BESDEBUG("dmrpp", "Opening: " << grid_h5 << endl);

        ifstream in(grid_h5);
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", "Parsing complete"<< endl);

        D4Group *root = dmr->root();

        // (D4Group *grp, string group_name, int expectedNumGrps, int expectedNumVars)

        checkGroupsAndVars(root, "/", 2, 0);

        // Root group iterator
        D4Group::groupsIter root_gIter = root->grp_begin();

        try {
        	// Now we drill down into the Group Hierarchy
        	// First, the top Level HDFEOS group

        	// TODO move this to a different test. jhrg
            // TODO I fail to see why you would want to remove this. Drilling down
            // TODO through the complex HDF5 Group stack requires that
            // TODO we check to see if each layer contains the expected content.
            // TODO Otherwise we can;t be sure we're looking at the right thing
            // TODO Also: null pointer exceptions. ndp
        	D4Group *grp_hdfeos = dynamic_cast<D4Group*>(*root_gIter);
            checkGroupsAndVars(grp_hdfeos, "HDFEOS", 2, 0);

            // Look at the child groups
            D4Group::groupsIter hdfeos_gIter = grp_hdfeos->grp_begin();
        	D4Group *grp_additional = dynamic_cast<D4Group*>(*hdfeos_gIter);
            checkGroupsAndVars(grp_additional, "ADDITIONAL", 1, 0);

        	hdfeos_gIter++;
        	D4Group *grp_grids = dynamic_cast<D4Group*>(*hdfeos_gIter);
            checkGroupsAndVars(grp_grids, "GRIDS", 2, 0);

            // Drill into the GRIDS group
            D4Group::groupsIter grids_gIter = grp_grids->grp_begin();
            //First child is GeoGrid1
        	D4Group *grp_geogrid_1 = dynamic_cast<D4Group*>(*grids_gIter);
            checkGroupsAndVars(grp_geogrid_1, "GeoGrid1", 1, 0);

            D4Group *grp_data_fields = dynamic_cast<D4Group*>(*grp_geogrid_1->grp_begin());
            checkGroupsAndVars(grp_data_fields, "Data Fields", 0, 1);

            // TODO This is where the read() test starts. jhrg
            // "Data Fields" contains a single Float32 var called temperature
			DmrppArray *temperature = dynamic_cast<DmrppArray*>(*grp_data_fields->var_begin());
			CPPUNIT_ASSERT(temperature);
			CPPUNIT_ASSERT(temperature->name() == "temperature");

            string data_url = string("file://").append(TEST_DATA_DIR).append(temperature->get_data_url());
            temperature->set_data_url(data_url);

            temperature->read();
            BESDEBUG("dmrpp", "temperature->length(): " << temperature->length() << endl);
            CPPUNIT_ASSERT(temperature->length() == 32);

            {
            vector<dods_float32> f32(temperature->length());
            temperature->value(&f32[0]);
            BESDEBUG("dmrpp", "temperature[0]:  " << f32[0] << endl);
            CPPUNIT_ASSERT(f32[0] == 10);
            BESDEBUG("dmrpp", "temperature[8]:  " << f32[8] << endl);
            CPPUNIT_ASSERT(f32[8] == 11);
            BESDEBUG("dmrpp", "temperature[16]: " << f32[16] << endl);
            CPPUNIT_ASSERT(f32[16] == 12);
            BESDEBUG("dmrpp", "temperature[24]: " << f32[24] << endl);
            CPPUNIT_ASSERT(f32[24] == 13);
            }

            // Next child of  GRIDS group

            // TODO Same as above. just test read() jhrg

            grids_gIter++;
        	D4Group *grp_geogrid_2 = dynamic_cast<D4Group*>(*grids_gIter);
            checkGroupsAndVars(grp_geogrid_2, "GeoGrid2", 1, 0);

            grp_data_fields = dynamic_cast<D4Group*>(*grp_geogrid_2->grp_begin());
            checkGroupsAndVars(grp_data_fields, "Data Fields", 0, 1);

            // "Data Fields" contains a single Float32 var called temperature
			temperature = dynamic_cast<DmrppArray*>(*grp_data_fields->var_begin());
			CPPUNIT_ASSERT(temperature);
			CPPUNIT_ASSERT(temperature->name() == "temperature");

            data_url = string("file://").append(TEST_DATA_DIR).append(temperature->get_data_url());
            temperature->set_data_url(data_url);

            temperature->read();
            BESDEBUG("dmrpp", "temperature->length(): " << temperature->length() << endl);
            CPPUNIT_ASSERT(temperature->length() == 32);

            {
            vector<dods_float32> f32(temperature->length());
            temperature->value(&f32[0]);
            BESDEBUG("dmrpp", "temperature[0]:  " << f32[0] << endl);
            CPPUNIT_ASSERT(f32[0] == 10);
            BESDEBUG("dmrpp", "temperature[8]:  " << f32[8] << endl);
            CPPUNIT_ASSERT(f32[8] == 11);
            BESDEBUG("dmrpp", "temperature[16]: " << f32[16] << endl);
            CPPUNIT_ASSERT(f32[16] == 12);
            BESDEBUG("dmrpp", "temperature[24]: " << f32[24] << endl);
            CPPUNIT_ASSERT(f32[24] == 13);
            }

        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }
        CPPUNIT_ASSERT("Passed");
    }

    void test_constrained_arrays() {
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);

        string grid_h5 = string(TEST_DATA_DIR).append("/").append("grid_2_2d.h5.dmrpp");
        BESDEBUG("dmrpp", "Opening: " << grid_h5 << endl);

        ifstream in(grid_h5);
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", "Parsing complete"<< endl);

        D4Group *root = dmr->root();

        // (D4Group *grp, string group_name, int expectedNumGrps, int expectedNumVars)


        checkGroupsAndVars(root, "/", 2, 0);

        // Root group iterator
        D4Group::groupsIter root_gIter = root->grp_begin();

        try {
        	// Now we drill down into the Group Hierarchy
        	// Firswt, the top Level HDFEOS group
        	D4Group *grp_hdfeos = dynamic_cast<D4Group*>(*root_gIter);
            checkGroupsAndVars(grp_hdfeos, "HDFEOS", 2, 0);

            // Look at the child groups
            D4Group::groupsIter hdfeos_gIter = grp_hdfeos->grp_begin();
        	D4Group *grp_additional = dynamic_cast<D4Group*>(*hdfeos_gIter);
            checkGroupsAndVars(grp_additional, "ADDITIONAL", 1, 0);

        	hdfeos_gIter++;
        	D4Group *grp_grids = dynamic_cast<D4Group*>(*hdfeos_gIter);
            checkGroupsAndVars(grp_grids, "GRIDS", 2, 0);

            // Drill into the GRIDS group
            D4Group::groupsIter grids_gIter = grp_grids->grp_begin();
            //First child is GeoGrid1
        	D4Group *grp_geogrid_1 = dynamic_cast<D4Group*>(*grids_gIter);
            checkGroupsAndVars(grp_geogrid_1, "GeoGrid1", 1, 0);

            D4Group *grp_data_fields = dynamic_cast<D4Group*>(*grp_geogrid_1->grp_begin());
            checkGroupsAndVars(grp_data_fields, "Data Fields", 0, 1);

            // "Data Fields" contains a single Float32 var called temperature
			DmrppArray *temperature = dynamic_cast<DmrppArray*>(*grp_data_fields->var_begin());
			CPPUNIT_ASSERT(temperature);
			CPPUNIT_ASSERT(temperature->name() == "temperature");

			// TODO Change this. Do one dimension then two dims. separeate tests. make up data if needed jhrg

            BESDEBUG("dmrpp", "temperature->dimensions(): " << temperature->dimensions() << endl);
            CPPUNIT_ASSERT(temperature->dimensions() == 2);

            for(DmrppArray::Dim_iter dimIter = temperature->dim_begin();
            		dimIter!=temperature->dim_end(); dimIter++){
                BESDEBUG("dmrpp", "Dimension name: '" <<
                		temperature->dimension_name(dimIter) <<
						"' size: " << temperature->dimension_size(dimIter,true) <<
						endl);
            }


            // Now we constrain the temperature array
            DmrppArray::Dim_iter dimIter = temperature->dim_begin();
            temperature->add_constraint(dimIter,1,1,2);

            {
            ostringstream ss;
            ss  << "[" << temperature->dimension_start(dimIter,true) <<
                               ":" << temperature->dimension_stride(dimIter,true) <<
							   ":" << temperature->dimension_stop(dimIter,true) << "]";
            dimIter++;
            temperature->add_constraint(dimIter,1,2,5);
            ss  << "[" << temperature->dimension_start(dimIter,true) <<
                               ":" << temperature->dimension_stride(dimIter,true) <<
							   ":" << temperature->dimension_stop(dimIter,true) << "]";

            BESDEBUG("dmrpp", "Constrained array temperature" << ss.str() << endl);
            }

            string data_url = string("file://").append(TEST_DATA_DIR).append(temperature->get_data_url());
            temperature->set_data_url(data_url);
            BESDEBUG("dmrpp", "temperature->get_data_url(): " << temperature->get_data_url() << endl);

            temperature->read();
            BESDEBUG("dmrpp", "temperature->length(): " << temperature->length() << endl);
            CPPUNIT_ASSERT(temperature->length() == 6);

            {
            vector<dods_float32> f32(temperature->length());
            temperature->value(&f32[0]);
            BESDEBUG("dmrpp", "temperature[0]: " << f32[0] << endl);
            CPPUNIT_ASSERT(f32[0] == 11);
            BESDEBUG("dmrpp", "temperature[1]:  " << f32[1] << endl);
            CPPUNIT_ASSERT(f32[1] == 11);
            BESDEBUG("dmrpp", "temperature[2]:  " << f32[2] << endl);
            CPPUNIT_ASSERT(f32[2] == 11);
            BESDEBUG("dmrpp", "temperature[3]: " << f32[3] << endl);
            CPPUNIT_ASSERT(f32[3] == 12);
            BESDEBUG("dmrpp", "temperature[4]: " << f32[4] << endl);
            CPPUNIT_ASSERT(f32[4] == 12);
            BESDEBUG("dmrpp", "temperature[5]: " << f32[5] << endl);
            CPPUNIT_ASSERT(f32[5] == 12);
            }

            BESDEBUG("dmrpp", "------------------------" << endl);

            // Next child of  GRIDS group
            grids_gIter++;
        	D4Group *grp_geogrid_2 = dynamic_cast<D4Group*>(*grids_gIter);
            checkGroupsAndVars(grp_geogrid_2, "GeoGrid2", 1, 0);

            grp_data_fields = dynamic_cast<D4Group*>(*grp_geogrid_2->grp_begin());
            checkGroupsAndVars(grp_data_fields, "Data Fields", 0, 1);

            // "Data Fields" contains a single Float32 var called temperature
            DmrppArray *t2 = dynamic_cast<DmrppArray*>(*grp_data_fields->var_begin());
			CPPUNIT_ASSERT(t2);
			CPPUNIT_ASSERT(t2->name() == "temperature");


            dimIter = t2->dim_begin();
            t2->add_constraint(dimIter,0,1,3);
            {
            ostringstream ss;
            ss  << "[" << t2->dimension_start(dimIter,true) <<
                               ":" << t2->dimension_stride(dimIter,true) <<
							   ":" << t2->dimension_stop(dimIter,true) << "]";
            dimIter++;
            t2->add_constraint(dimIter,0,1,0);
            ss  << "[" << t2->dimension_start(dimIter,true) <<
                               ":" << t2->dimension_stride(dimIter,true) <<
							   ":" << t2->dimension_stop(dimIter,true) << "]";

            BESDEBUG("dmrpp", "Constrained array t2" << ss.str() << endl);
            }

            t2->set_data_url(data_url);
            BESDEBUG("dmrpp", "t2->get_data_url(): " << t2->get_data_url() << endl);
            t2->read();
            BESDEBUG("dmrpp", "t2->length(): " << t2->length() << endl);
            CPPUNIT_ASSERT(t2->length() == 4);

            {
            vector<dods_float32> f32(t2->length());
            t2->value(&f32[0]);
            BESDEBUG("dmrpp", "t2[0]:  " << f32[0] << endl);
            CPPUNIT_ASSERT(f32[0] == 10);
            BESDEBUG("dmrpp", "t2[1]:  " << f32[1] << endl);
            CPPUNIT_ASSERT(f32[1] == 11);
            BESDEBUG("dmrpp", "t2[2]: " << f32[2] << endl);
            CPPUNIT_ASSERT(f32[2] == 12);
            BESDEBUG("dmrpp", "t2[3]: " << f32[3] << endl);
            CPPUNIT_ASSERT(f32[3] == 13);
            }

        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }
        CPPUNIT_ASSERT("Passed");
    }

    void read_var_check_name_and_length(DmrppArray *array, string name, int length){
        BESDEBUG("dmrpp", "array->name(): " << array->name() << endl);
        CPPUNIT_ASSERT(array->name() == name);
        string data_url = string("file://").append(TEST_DATA_DIR).append(array->get_data_url());
        array->set_data_url(data_url);
        BESDEBUG("dmrpp", "array->get_data_url(): " << array->get_data_url() << endl);
        array->read();
        BESDEBUG("dmrpp", "array->length(): " << array->length() << endl);
        CPPUNIT_ASSERT(array->length() == length);
    }

    void test_read_coads_climatology() {
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);
        dods_float32 test_float32;
        dods_float64 test_float64;
        int index;

        string coads = string(TEST_DATA_DIR).append("/").append("coads_climatology.dmrpp");
        BESDEBUG("dmrpp", "Opening: " << coads << endl);

        ifstream in(coads);
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", "Parsing complete"<< endl);

        // Check to make sure we have something that smells like coads_climatology
        D4Group *root = dmr->root();
        checkGroupsAndVars(root, "/", 0, 7);
        // Walk the vars and testy testy
        D4Group::Vars_iter vIter = root->var_begin();
        try {
            // ######################################
            // Check COADSX variable
            DmrppArray *coadsx = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(coadsx,"COADSX",180);
            vector<dods_float64> coadsx_vals(coadsx->length());
            coadsx->value(&coadsx_vals[0]);
            // first element
            index = 0;
            test_float64 = 21.0;
            BESDEBUG("dmrpp", "coadsx_vals[" << index << "]: " << coadsx_vals[index] << "  test_float64: " << test_float64 << endl);
            CPPUNIT_ASSERT(double_eq(coadsx_vals[index], test_float64 ));
            // last element
            index = 179;
            test_float64 = 379.0;
            BESDEBUG("dmrpp", "coadsx_vals[" << index << "]: " << coadsx_vals[index] << "  test_float64: " << test_float64 << endl);
            CPPUNIT_ASSERT(double_eq(coadsx_vals[index], test_float64 ));

            // ######################################
            // Check COADSY variable
            vIter++;
            DmrppArray *coadsy = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(coadsy,"COADSY",90);
            vector<dods_float64> coadsy_vals(coadsy->length());
            coadsy->value(&coadsy_vals[0]);
            // first element
            index = 0;
            test_float64 = -89.0;
            BESDEBUG("dmrpp", "coadsy_vals[" << index << "]: " << coadsy_vals[index] << "  test_float64: " << test_float64 << endl);
            CPPUNIT_ASSERT(double_eq(coadsy_vals[index], test_float64 ));
            // last element
            index = 89;
            test_float64 = 89.0;
            BESDEBUG("dmrpp", "coadsy_vals[" << index << "]: " << coadsy_vals[index] << "  test_float64: " << test_float64 << endl);
            CPPUNIT_ASSERT(double_eq(coadsy_vals[index], test_float64 ));

            // ######################################
            // Check TIME variable
            vIter++;
            DmrppArray *time = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(time,"TIME",12);
            vector<dods_float64> time_vals(time->length());
            time->value(&time_vals[0]);
            // first element
            index = 0;
            test_float64 = 366.0;
            BESDEBUG("dmrpp", "time_vals[" << index << "]: " << time_vals[index] << "  test_float64: " << test_float64 << endl);
            CPPUNIT_ASSERT(double_eq(time_vals[index], test_float64 ));
            // last element
            index = 11;
            test_float64 = 8401.335;
            BESDEBUG("dmrpp", "time_vals[" << index << "]: " << time_vals[index] << "  test_float64: " << test_float64 << endl);
            CPPUNIT_ASSERT(double_eq(time_vals[index], test_float64 ));


            // ######################################
            // Check SST variable
            vIter++;
            DmrppArray *sst = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(sst,"SST",194400);
            vector<dods_float32> sst_vals(sst->length());
            sst->value(&sst_vals[0]);

            // check [0][0][0]
            index = 0;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "sst_vals["<< index << "]: " << sst_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(sst_vals[index], test_float32 ));

            // check [0][13][0]
            index =  13*180;
            test_float32 = 0.88125;
            BESDEBUG("dmrpp", "sst_vals["<< index << "]: " << sst_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(sst_vals[index], test_float32 ));

            // check [11][15][2]
            index =  11*90*180  + 15*180 + 2;
            test_float32 = -0.3675;
            BESDEBUG("dmrpp", "sst_vals["<< index << "]: " << sst_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(sst_vals[index], test_float32 ));

            // check [11][89][179]
            index =  12*90*180 - 1 ;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "sst_vals["<< index << "]: " << sst_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(sst_vals[index], test_float32 ));

            // ######################################
            // Check AIRT variable
            vIter++;
            DmrppArray *airt = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(airt,"AIRT",194400);
            vector<dods_float32> airt_vals(airt->length());
            airt->value(&airt_vals[0]);

            // check [0][0][0]
            index = 0;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "airt_vals["<< index << "]: " << airt_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(airt_vals[index], test_float32 ));

            // check [3][73][0]
            index =  3*90*180  + 73*180 + 0;
            test_float32 = 3.3;
            BESDEBUG("dmrpp", "airt_vals["<< index << "]: " << airt_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(airt_vals[index], test_float32 ));

            // check [9][24][0]
            index =  9*90*180  + 24*180 + 0;
            test_float32 = 14.764;
            BESDEBUG("dmrpp", "airt_vals["<< index << "]: " << airt_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(airt_vals[index], test_float32 ));

            // check [11][89][179]
            index =  12*90*180 - 1 ;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "airt_vals["<< index << "]: " << airt_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(airt_vals[index], test_float32 ));

            // ######################################
            // Check UWND variable
            vIter++;
            DmrppArray *uwnd = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(uwnd,"UWND",194400);
            vector<dods_float32> uwnd_vals(uwnd->length());
            uwnd->value(&uwnd_vals[0]);

            // check [0][0][0]
            index = 0;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "uwnd_vals["<< index << "]: " << uwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(uwnd_vals[index], test_float32 ));

            // check [5][77][0]
            index =  5*90*180  + 77*180 + 0;
            test_float32 =  0.42;
            BESDEBUG("dmrpp", "uwnd_vals["<< index << "]: " << uwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(uwnd_vals[index], test_float32 ));

            // check [5][77][1]
            index =  index + 1;
            test_float32 = 1.38103;
            BESDEBUG("dmrpp", "uwnd_vals["<< index << "]: " << uwnd_vals[index] << " test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(uwnd_vals[index], test_float32 ));

            // check [9][60][0]
            index =  9*90*180  + 60*180 + 0;
            test_float32 =  0.5075;
            BESDEBUG("dmrpp", "uwnd_vals["<< index << "]: " << uwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(uwnd_vals[index], test_float32 ));

            // check [11][89][179]
            index =  12*90*180 - 1 ;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "uwnd_vals["<< index << "]: " << uwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(uwnd_vals[index], test_float32 ));

            // ######################################
            // Check VWND variable
            vIter++;
            DmrppArray *vwnd = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(vwnd,"VWND",194400);
            vector<dods_float32> vwnd_vals(vwnd->length());
            vwnd->value(&vwnd_vals[0]);

            // check [0][0][0]
            index = 0;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "vwnd_vals["<< index << "]: " << vwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(vwnd_vals[index], test_float32 ));

            // check [2][37][13]
            index = 2*90*180 + 37*180 + 13;
            test_float32 = 0.3;
            BESDEBUG("dmrpp", "vwnd_vals["<< index << "]: " << vwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(vwnd_vals[index], test_float32 ));

            // check [4][36][8]
            index = 4*90*180 + 36*180 + 8;
            test_float32 = 3.35;
            BESDEBUG("dmrpp", "vwnd_vals["<< index << "]: " << vwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(vwnd_vals[index], test_float32 ));

            // check [11][89][179]
            index =  12*90*180 - 1 ;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "vwnd_vals["<< index << "]: " << vwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(vwnd_vals[index], test_float32 ));



        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        CPPUNIT_ASSERT("Passed");
    }

    void test_constrained_coads_climatology() {
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);
        dods_float32 test_float32;
        dods_float64 test_float64;
        int index;

        string coads = string(TEST_DATA_DIR).append("/").append("coads_climatology.dmrpp");
        BESDEBUG("dmrpp", "Opening: " << coads << endl);

        ifstream in(coads);
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", "Parsing complete"<< endl);

        // Check to make sure we have something that smells like coads_climatology
        D4Group *root = dmr->root();
        checkGroupsAndVars(root, "/", 0, 7);

        // Walk the vars and testy testy
        D4Group::Vars_iter vIter = root->var_begin();
        vIter++; // COADSY
        vIter++; // TIME
        vIter++; // SST  (woot!)

        try {
        	DmrppArray *sst = dynamic_cast<DmrppArray*>(*vIter);

            DmrppArray::Dim_iter dimIter = sst->dim_begin(); // Time
            sst->add_constraint(dimIter++,0,1,0); // Set TIME constraint and bump dim
            sst->add_constraint(dimIter++,0,1,0); // Set COADSY constraint and bump dim
            sst->add_constraint(dimIter++,0,18,179); // Set COADSX constraint and bump dim

            CPPUNIT_ASSERT(dimIter == sst->dim_end());


        	read_var_check_name_and_length(sst,"SST",1);


#if 0

        // Walk the vars and testy testy
        D4Group::Vars_iter vIter = root->var_begin();
        try {
            // ######################################
            // Check COADSX variable
            DmrppArray *coadsx = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(coadsx,"COADSX",180);
            vector<dods_float64> coadsx_vals(coadsx->length());
            coadsx->value(&coadsx_vals[0]);
            // first element
            index = 0;
            test_float64 = 21.0;
            BESDEBUG("dmrpp", "coadsx_vals[" << index << "]: " << coadsx_vals[index] << "  test_float64: " << test_float64 << endl);
            CPPUNIT_ASSERT(double_eq(coadsx_vals[index], test_float64 ));
            // last element
            index = 179;
            test_float64 = 379.0;
            BESDEBUG("dmrpp", "coadsx_vals[" << index << "]: " << coadsx_vals[index] << "  test_float64: " << test_float64 << endl);
            CPPUNIT_ASSERT(double_eq(coadsx_vals[index], test_float64 ));

            // ######################################
            // Check COADSY variable
            vIter++;
            DmrppArray *coadsy = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(coadsy,"COADSY",90);
            vector<dods_float64> coadsy_vals(coadsy->length());
            coadsy->value(&coadsy_vals[0]);
            // first element
            index = 0;
            test_float64 = -89.0;
            BESDEBUG("dmrpp", "coadsy_vals[" << index << "]: " << coadsy_vals[index] << "  test_float64: " << test_float64 << endl);
            CPPUNIT_ASSERT(double_eq(coadsy_vals[index], test_float64 ));
            // last element
            index = 89;
            test_float64 = 89.0;
            BESDEBUG("dmrpp", "coadsy_vals[" << index << "]: " << coadsy_vals[index] << "  test_float64: " << test_float64 << endl);
            CPPUNIT_ASSERT(double_eq(coadsy_vals[index], test_float64 ));

            // ######################################
            // Check TIME variable
            vIter++;
            DmrppArray *time = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(time,"TIME",12);
            vector<dods_float64> time_vals(time->length());
            time->value(&time_vals[0]);
            // first element
            index = 0;
            test_float64 = 366.0;
            BESDEBUG("dmrpp", "time_vals[" << index << "]: " << time_vals[index] << "  test_float64: " << test_float64 << endl);
            CPPUNIT_ASSERT(double_eq(time_vals[index], test_float64 ));
            // last element
            index = 11;
            test_float64 = 8401.335;
            BESDEBUG("dmrpp", "time_vals[" << index << "]: " << time_vals[index] << "  test_float64: " << test_float64 << endl);
            CPPUNIT_ASSERT(double_eq(time_vals[index], test_float64 ));


            // ######################################
            // Check SST variable
            vIter++;
            DmrppArray *sst = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(sst,"SST",194400);
            vector<dods_float32> sst_vals(sst->length());
            sst->value(&sst_vals[0]);

            // check [0][0][0]
            index = 0;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "sst_vals["<< index << "]: " << sst_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(sst_vals[index], test_float32 ));

            // check [0][13][0]
            index =  13*180;
            test_float32 = 0.88125;
            BESDEBUG("dmrpp", "sst_vals["<< index << "]: " << sst_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(sst_vals[index], test_float32 ));

            // check [11][15][2]
            index =  11*90*180  + 15*180 + 2;
            test_float32 = -0.3675;
            BESDEBUG("dmrpp", "sst_vals["<< index << "]: " << sst_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(sst_vals[index], test_float32 ));

            // check [11][89][179]
            index =  12*90*180 - 1 ;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "sst_vals["<< index << "]: " << sst_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(sst_vals[index], test_float32 ));

            // ######################################
            // Check AIRT variable
            vIter++;
            DmrppArray *airt = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(airt,"AIRT",194400);
            vector<dods_float32> airt_vals(airt->length());
            airt->value(&airt_vals[0]);

            // check [0][0][0]
            index = 0;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "airt_vals["<< index << "]: " << airt_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(airt_vals[index], test_float32 ));

            // check [3][73][0]
            index =  3*90*180  + 73*180 + 0;
            test_float32 = 3.3;
            BESDEBUG("dmrpp", "airt_vals["<< index << "]: " << airt_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(airt_vals[index], test_float32 ));

            // check [9][24][0]
            index =  9*90*180  + 24*180 + 0;
            test_float32 = 14.764;
            BESDEBUG("dmrpp", "airt_vals["<< index << "]: " << airt_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(airt_vals[index], test_float32 ));

            // check [11][89][179]
            index =  12*90*180 - 1 ;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "airt_vals["<< index << "]: " << airt_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(airt_vals[index], test_float32 ));

            // ######################################
            // Check UWND variable
            vIter++;
            DmrppArray *uwnd = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(uwnd,"UWND",194400);
            vector<dods_float32> uwnd_vals(uwnd->length());
            uwnd->value(&uwnd_vals[0]);

            // check [0][0][0]
            index = 0;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "uwnd_vals["<< index << "]: " << uwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(uwnd_vals[index], test_float32 ));

            // check [5][77][0]
            index =  5*90*180  + 77*180 + 0;
            test_float32 =  0.42;
            BESDEBUG("dmrpp", "uwnd_vals["<< index << "]: " << uwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(uwnd_vals[index], test_float32 ));

            // check [5][77][1]
            index =  index + 1;
            test_float32 = 1.38103;
            BESDEBUG("dmrpp", "uwnd_vals["<< index << "]: " << uwnd_vals[index] << " test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(uwnd_vals[index], test_float32 ));

            // check [9][60][0]
            index =  9*90*180  + 60*180 + 0;
            test_float32 =  0.5075;
            BESDEBUG("dmrpp", "uwnd_vals["<< index << "]: " << uwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(uwnd_vals[index], test_float32 ));

            // check [11][89][179]
            index =  12*90*180 - 1 ;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "uwnd_vals["<< index << "]: " << uwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(uwnd_vals[index], test_float32 ));

            // ######################################
            // Check VWND variable
            vIter++;
            DmrppArray *vwnd = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_length(vwnd,"VWND",194400);
            vector<dods_float32> vwnd_vals(vwnd->length());
            vwnd->value(&vwnd_vals[0]);

            // check [0][0][0]
            index = 0;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "vwnd_vals["<< index << "]: " << vwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(vwnd_vals[index], test_float32 ));

            // check [2][37][13]
            index = 2*90*180 + 37*180 + 13;
            test_float32 = 0.3;
            BESDEBUG("dmrpp", "vwnd_vals["<< index << "]: " << vwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(vwnd_vals[index], test_float32 ));

            // check [4][36][8]
            index = 4*90*180 + 36*180 + 8;
            test_float32 = 3.35;
            BESDEBUG("dmrpp", "vwnd_vals["<< index << "]: " << vwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(vwnd_vals[index], test_float32 ));

            // check [11][89][179]
            index =  12*90*180 - 1 ;
            test_float32 = -1e+34;
            BESDEBUG("dmrpp", "vwnd_vals["<< index << "]: " << vwnd_vals[index] << "  test_float32: " << test_float32 << endl);
            CPPUNIT_ASSERT(double_eq(vwnd_vals[index], test_float32 ));

#endif


        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }
        CPPUNIT_ASSERT("Passed");
    }



    CPPUNIT_TEST_SUITE( DmrppTypeReadTest );

    CPPUNIT_TEST(test_integer_scalar);
    CPPUNIT_TEST(test_integer_arrays);
    CPPUNIT_TEST(test_float_grids);
    CPPUNIT_TEST(test_constrained_arrays);
    CPPUNIT_TEST(test_read_coads_climatology);
    CPPUNIT_TEST(test_constrained_coads_climatology);


    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppTypeReadTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "d");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
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
            test = string("dmrpp::DmrppTypeReadTest::") + argv[i++];

            cerr << endl << "Running test " << test << endl << endl;

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

