
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

using namespace libdap;

static bool debug = false;

namespace dmrpp {

class DmrppParserTest: public CppUnit::TestFixture {
private:
    DmrppParserSax2 parser;

public:
    // Called once before everything gets tested
    DmrppParserTest() :parser()
    {
    }

    // Called at the end of the test
    ~DmrppParserTest()
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

    /******************************************************
     *
     */
   void test_integer_scalar()
    {

        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);

        string int_h5 = string(TEST_DATA_DIR).append("/").append("t_int_scalar.h5.dmrpp");
        BESDEBUG("dmrpp", "Opening: " << int_h5 << endl);

        ifstream in(int_h5);
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", "Parsing complete"<< endl);

        D4Group *root = dmr->root();
        CPPUNIT_ASSERT(root);
        BESDEBUG("dmrpp", "Got root Group"<< endl);

        CPPUNIT_ASSERT(root->var_end() - root->var_begin() == 1);
        BESDEBUG("dmrpp", "Found one variable."<< endl);


        D4Group::Vars_iter v = root->var_begin();
        BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
        CPPUNIT_ASSERT((*v)->name() == "scalar");
        DmrppCommon *dc = dynamic_cast<DmrppCommon*>(*v);
        //CPPUNIT_ASSERT(dc->get_offset() == 2144);
        // CPPUNIT_ASSERT(dc->get_size() == 4);


    }

   /******************************************************
    *
    */
  void test_integer_arrays()
   {
       auto_ptr<DMR> dmr(new DMR);
       DmrppTypeFactory dtf;
       dmr->set_factory(&dtf);

       string int_h5 = string(TEST_DATA_DIR).append("/").append("d_int.h5.dmrpp");
       BESDEBUG("dmrpp", "Opening: " << int_h5 << endl);

       ifstream in(int_h5);
       parser.intern(in, dmr.get(), debug);

       D4Group *root = dmr->root();
       CPPUNIT_ASSERT(root);
       BESDEBUG("dmrpp", "Got root Group"<< endl);

       CPPUNIT_ASSERT(root->var_end() - root->var_begin() == 4);
       BESDEBUG("dmrpp", "Found 4 variables."<< endl);

       D4Group::Vars_iter v = root->var_begin();
       BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
       CPPUNIT_ASSERT((*v)->name() == "d16_1");
       DmrppCommon *dc = dynamic_cast<DmrppCommon*>(*v);
       CPPUNIT_ASSERT(dc->get_offset() == 2216);
       CPPUNIT_ASSERT(dc->get_size() == 4);

       v++;

       BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
       CPPUNIT_ASSERT((*v)->name() == "d16_2");
       dc = dynamic_cast<DmrppCommon*>(*v);
       CPPUNIT_ASSERT(dc->get_offset() == 2220);
       CPPUNIT_ASSERT(dc->get_size() == 8);

       v++;

       BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
       CPPUNIT_ASSERT((*v)->name() == "d32_1");
       dc = dynamic_cast<DmrppCommon*>(*v);
       CPPUNIT_ASSERT(dc->get_offset() == 2228);
       CPPUNIT_ASSERT(dc->get_size() == 32);

       v++;

       BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
       CPPUNIT_ASSERT((*v)->name() == "d32_2");
       dc = dynamic_cast<DmrppCommon*>(*v);
       CPPUNIT_ASSERT(dc->get_offset() == 2260);
       CPPUNIT_ASSERT(dc->get_size() == 128);
   }



  /******************************************************
   *
   */
  void test_float_arrays()
  {
      auto_ptr<DMR> dmr(new DMR);
      DmrppTypeFactory dtf;
      dmr->set_factory(&dtf);

      string float_h5 = string(TEST_DATA_DIR).append("/").append("t_float.h5.dmrpp");
      BESDEBUG("dmrpp", "Opening: " << float_h5 << endl);

      ifstream in(float_h5);
      parser.intern(in, dmr.get(), debug);

      D4Group *root = dmr->root();
      CPPUNIT_ASSERT(root);

      CPPUNIT_ASSERT(root->var_end() - root->var_begin() == 4);

      D4Group::Vars_iter v = root->var_begin();
      BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
      CPPUNIT_ASSERT((*v)->name() == "d32_1");
      DmrppCommon *dc = dynamic_cast<DmrppCommon*>(*v);
      CPPUNIT_ASSERT(dc->get_offset() == 2216);
      CPPUNIT_ASSERT(dc->get_size() == 8);

      v++;

      BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
      CPPUNIT_ASSERT((*v)->name() == "d32_2");
      dc = dynamic_cast<DmrppCommon*>(*v);
      CPPUNIT_ASSERT(dc->get_offset() == 2224);
      CPPUNIT_ASSERT(dc->get_size() == 16);

      v++;

      BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
      CPPUNIT_ASSERT((*v)->name() == "d64_1");
      dc = dynamic_cast<DmrppCommon*>(*v);
      CPPUNIT_ASSERT(dc->get_offset() == 2240);
      CPPUNIT_ASSERT(dc->get_size() == 64);

      v++;

      BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
      CPPUNIT_ASSERT((*v)->name() == "d64_2");
      dc = dynamic_cast<DmrppCommon*>(*v);
      CPPUNIT_ASSERT(dc->get_offset() == 2304);
      CPPUNIT_ASSERT(dc->get_size() == 256);
  }



  /******************************************************
   *
   */
  void test_grid_1_2d()
  {
      auto_ptr<DMR> dmr(new DMR);
      DmrppTypeFactory dtf;
      dmr->set_factory(&dtf);

      string grid_2d = string(TEST_DATA_DIR).append("/").append("grid_1_2d.h5.dmrpp");
      BESDEBUG("dmrpp", "Opening: " << grid_2d << endl);

      ifstream in(grid_2d);
      parser.intern(in, dmr.get(), debug);

      D4Group *root = dmr->root();
      CPPUNIT_ASSERT(root);

      int numGroups = root->grp_end() - root->grp_begin();
      BESDEBUG("dmrpp", "Found " << numGroups << " top level groups." << endl);
      CPPUNIT_ASSERT( numGroups == 2);

      int numVars = root->var_end() - root->var_begin();
      BESDEBUG("dmrpp", "Found " << numVars << " top level variables." << endl);
      CPPUNIT_ASSERT( numVars == 0);

      D4Group::groupsIter top_level_grp_itr = root->grp_begin();
      D4Group *hdfeos_grp = (*top_level_grp_itr);
      BESDEBUG("dmrpp", "Looking at top level group: " << hdfeos_grp->name() << endl);
      CPPUNIT_ASSERT(hdfeos_grp->name() == "HDFEOS");

      numGroups = hdfeos_grp->grp_end() - hdfeos_grp->grp_begin();
      BESDEBUG("dmrpp", "Found " << numGroups << " HDFEOS child groups." << endl);
      CPPUNIT_ASSERT( numGroups == 2);

      numVars = hdfeos_grp->var_end() - hdfeos_grp->var_begin();
      BESDEBUG("dmrpp", "Found " << numVars << " HDFEOS child variables." << endl);
      CPPUNIT_ASSERT( numVars == 0);


      D4Group::groupsIter hdfeos_child_grp_itr = hdfeos_grp->grp_begin();
      BESDEBUG("dmrpp", "Looking at HDFEOS child group: " << (*hdfeos_child_grp_itr)->name() << endl);
      CPPUNIT_ASSERT((*hdfeos_child_grp_itr)->name() == "ADDITIONAL");

      hdfeos_child_grp_itr++;

      D4Group *grids_grp = *hdfeos_child_grp_itr;
      BESDEBUG("dmrpp", "Looking at HDFEOS child group: " << grids_grp->name() << endl);
      CPPUNIT_ASSERT(grids_grp->name() == "GRIDS");

      numGroups = grids_grp->grp_end() - grids_grp->grp_begin();
      BESDEBUG("dmrpp", "Found " << numGroups << " GRIDS child groups." << endl);
      CPPUNIT_ASSERT( numGroups == 1);

      numVars = grids_grp->var_end() - grids_grp->var_begin();
      BESDEBUG("dmrpp", "Found " << numVars << " GRIDS child variables." << endl);
      CPPUNIT_ASSERT( numVars == 0);



      D4Group *geogrid_grp = *(grids_grp->grp_begin());
      CPPUNIT_ASSERT(geogrid_grp->name() == "GeoGrid");

      numGroups = geogrid_grp->grp_end() - geogrid_grp->grp_begin();
      BESDEBUG("dmrpp", "Found " << numGroups << " GeoGrid child groups." << endl);
      CPPUNIT_ASSERT( numGroups == 1);

      numVars = geogrid_grp->var_end() - geogrid_grp->var_begin();
      BESDEBUG("dmrpp", "Found " << numVars << " GeoGrid child variables." << endl);
      CPPUNIT_ASSERT( numVars == 0);




      D4Group *datafields_grp = *(geogrid_grp->grp_begin());
      CPPUNIT_ASSERT(datafields_grp->name() == "Data Fields");

      numGroups = datafields_grp->grp_end() - datafields_grp->grp_begin();
      BESDEBUG("dmrpp", "Found " << numGroups << " 'Data Fields' child groups." << endl);
      CPPUNIT_ASSERT( numGroups == 0);

      numVars = datafields_grp->var_end() - datafields_grp->var_begin();
      BESDEBUG("dmrpp", "Found " << numVars << " 'Data Fields' child variables." << endl);
      CPPUNIT_ASSERT( numVars == 1);


      D4Group::Vars_iter v = datafields_grp->var_begin();
      BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
      CPPUNIT_ASSERT((*v)->name() == "temperature");
      DmrppCommon *dc = dynamic_cast<DmrppCommon*>(*v);
      CPPUNIT_ASSERT(dc->get_offset() == 40672);
      CPPUNIT_ASSERT(dc->get_size() == 128);

      top_level_grp_itr++;
      BESDEBUG("dmrpp", "Looking at top level group: " << (*top_level_grp_itr)->name() << endl);
      CPPUNIT_ASSERT((*top_level_grp_itr)->name() == "HDFEOS INFORMATION");

      D4Group *hdfeos_info_grp = *top_level_grp_itr;

      numVars = hdfeos_info_grp->var_end() - hdfeos_info_grp->var_begin();
      BESDEBUG("dmrpp", "Found " << numVars << " 'HDFEOS INFORMATION' child variables." << endl);
      CPPUNIT_ASSERT( numVars == 1);

      numGroups = hdfeos_info_grp->grp_end() - hdfeos_info_grp->grp_begin();
      BESDEBUG("dmrpp", "Found " << numGroups << " GRIDS child groups." << endl);
      CPPUNIT_ASSERT( numGroups == 0);


      v = hdfeos_info_grp->var_begin();
      BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
      CPPUNIT_ASSERT((*v)->name() == "StructMetadata.0");

      dc = dynamic_cast<DmrppCommon*>(*v);
      CPPUNIT_ASSERT(dc->get_offset() == 5304);
      CPPUNIT_ASSERT(dc->get_size() == 32000);


  }

  /******************************************************
   *
   */
  void test_nc4_group_ataomic()
  {
      auto_ptr<DMR> dmr(new DMR);
      DmrppTypeFactory dtf;
      dmr->set_factory(&dtf);

      string nc4_group_atomic = string(TEST_DATA_DIR).append("/").append("nc4_group_atomic.h5.dmrpp");
      BESDEBUG("dmrpp", "Opening: " << nc4_group_atomic << endl);

      ifstream in(nc4_group_atomic);
      parser.intern(in, dmr.get(), debug);

      D4Group *root = dmr->root();
      CPPUNIT_ASSERT(root);

      int numVars = root->var_end() - root->var_begin();
      BESDEBUG("dmrpp", "Found " << numVars << " top level entities." << endl);

      CPPUNIT_ASSERT( numVars == 2);

      D4Group::Vars_iter v = root->var_begin();
      BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
      CPPUNIT_ASSERT((*v)->name() == "dim1");
      DmrppCommon *dc = dynamic_cast<DmrppCommon*>(*v);
      CPPUNIT_ASSERT(dc->get_offset() == 6192);
      CPPUNIT_ASSERT(dc->get_size() == 8);

      v++;

      BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
      CPPUNIT_ASSERT((*v)->name() == "d1");
      dc = dynamic_cast<DmrppCommon*>(*v);
      CPPUNIT_ASSERT(dc->get_offset() == 6200);
      CPPUNIT_ASSERT(dc->get_size() == 8);

      v++;

      BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
      CPPUNIT_ASSERT((*v)->name() == "d64_1");
      dc = dynamic_cast<DmrppCommon*>(*v);
      CPPUNIT_ASSERT(dc->get_offset() == 2240);
      CPPUNIT_ASSERT(dc->get_size() == 64);

      v++;

      BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
      CPPUNIT_ASSERT((*v)->name() == "d64_2");
      dc = dynamic_cast<DmrppCommon*>(*v);
      CPPUNIT_ASSERT(dc->get_offset() == 2304);
      CPPUNIT_ASSERT(dc->get_size() == 256);
  }

    CPPUNIT_TEST_SUITE( DmrppParserTest );

    CPPUNIT_TEST(test_integer_scalar);
    CPPUNIT_TEST(test_integer_arrays);
    CPPUNIT_TEST(test_float_arrays);

    CPPUNIT_TEST(test_grid_1_2d);
    //CPPUNIT_TEST(test_nc4_group_ataomic);

    CPPUNIT_TEST_SUITE_END();
        
 };

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppParserTest);

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
            BESDebug::SetUp("cerr,ugrid");
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
            test = string("dmrpp::DmrppParserTest::") + argv[i++];

            cerr << endl << "Running test " << test << endl << endl;

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

