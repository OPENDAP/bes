
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


    /**
     * Evaluates a BaseType pointer believed to be an instance of DrmppCommon.
     * This checks the variables name, offset, and size. against expected values
     * passed as parameters.
     */
    void checkDmrppVariable(BaseType *bt, string name, unsigned long long offset, unsigned long long size){

  	  CPPUNIT_ASSERT(bt);

  	  BESDEBUG("dmrpp", "Looking at variable: " << bt->name() << endl);
        CPPUNIT_ASSERT(bt->name() == name);
        DmrppCommon *dc = dynamic_cast<DmrppCommon*>(bt);
        CPPUNIT_ASSERT(dc);
        CPPUNIT_ASSERT(dc->get_offset() == offset);
        CPPUNIT_ASSERT(dc->get_size() == size);
    }

    /**
     * Evaluates a D4Group against exected values for the name, number of child
     * groups and the number of child variables.
     */
    void checkGroupsAndVars(D4Group *grp,  string name, int expectedNumGrps,  int expectedNumVars){

        CPPUNIT_ASSERT(grp);
        BESDEBUG("dmrpp", "Checking D4Group '" << grp->name() << "'" << endl);

        CPPUNIT_ASSERT(grp->name() ==  name);

  	  int numGroups = grp->grp_end() - grp->grp_begin();
        BESDEBUG("dmrpp", "The D4Group '" << grp->name() << "' has " << numGroups << " child groups." << endl);
        CPPUNIT_ASSERT( numGroups == expectedNumGrps);

        int numVars = grp->var_end() - grp->var_begin();
        BESDEBUG("dmrpp", "The D4Group '" << grp->name() << "' has " << numVars << " child variables." << endl);
        CPPUNIT_ASSERT( numVars == expectedNumVars);

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

        checkGroupsAndVars(root,"/",0,1);

        D4Group::Vars_iter v = root->var_begin();

        checkDmrppVariable(*v,"scalar",2144,4);

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
       checkGroupsAndVars(root,"/",0,4);

       D4Group::Vars_iter v = root->var_begin();

       checkDmrppVariable(*v,"d16_1",2216,4);

       v++;

       checkDmrppVariable(*v,"d16_2",2220,8);

       v++;

       checkDmrppVariable(*v,"d32_1",2228,32);

       v++;

       checkDmrppVariable(*v,"d32_2",2260,128);

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

      checkGroupsAndVars(root,"/",0,4);


      D4Group::Vars_iter v = root->var_begin();

      checkDmrppVariable(*v,"d32_1",2216,8);

      v++;

      checkDmrppVariable(*v,"d32_2",2224,16);

      v++;

      checkDmrppVariable(*v,"d64_1",2240,64);

      v++;

      checkDmrppVariable(*v,"d64_2",2304,256);

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

      checkGroupsAndVars(root,"/",2,0);

      D4Group::groupsIter top_level_grp_itr = root->grp_begin();

      D4Group *hdfeos_grp = (*top_level_grp_itr);

      checkGroupsAndVars(hdfeos_grp,"HDFEOS",2,0);


      D4Group::groupsIter hdfeos_child_grp_itr = hdfeos_grp->grp_begin();

      checkGroupsAndVars(*hdfeos_child_grp_itr,"ADDITIONAL",1,0);

      hdfeos_child_grp_itr++;

      D4Group *grids_grp = *hdfeos_child_grp_itr;

      checkGroupsAndVars(*hdfeos_child_grp_itr,"GRIDS",1,0);

      D4Group *geogrid_grp = *(grids_grp->grp_begin());

      checkGroupsAndVars(geogrid_grp,"GeoGrid",1,0);

      D4Group *datafields_grp = *(geogrid_grp->grp_begin());

      checkGroupsAndVars(datafields_grp,"Data Fields",0,1);

      D4Group::Vars_iter v = datafields_grp->var_begin();

      checkDmrppVariable(*v,"temperature",40672,128);

      top_level_grp_itr++;
      D4Group *hdfeos_info_grp = *top_level_grp_itr;

      checkGroupsAndVars(hdfeos_info_grp,"HDFEOS INFORMATION",0,1);

      v = hdfeos_info_grp->var_begin();

      checkDmrppVariable(*v,"StructMetadata.0",5304,32000);

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

      checkGroupsAndVars(root, "/", 1, 2);

      D4Group::Vars_iter v = root->var_begin();
      checkDmrppVariable(*v,"dim1",6192,8);
      v++;
      checkDmrppVariable(*v,"d1",6200,8);

      D4Group::groupsIter top_level_grp_itr = root->grp_begin();
      D4Group *g1_grp = (*top_level_grp_itr);

      checkGroupsAndVars(g1_grp, "g1", 0, 2);

      v = g1_grp->var_begin();
      checkDmrppVariable(*v,"dim2",6208,12);

      v++;
      checkDmrppVariable(*v,"d2",6220,24);

  }

    CPPUNIT_TEST_SUITE( DmrppParserTest );

    CPPUNIT_TEST(test_integer_scalar);
    CPPUNIT_TEST(test_integer_arrays);
    CPPUNIT_TEST(test_float_arrays);

    CPPUNIT_TEST(test_grid_1_2d);
    CPPUNIT_TEST(test_nc4_group_ataomic);

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

