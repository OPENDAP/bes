
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
     * Evaluates a H4ByteStream instance.
     * This checks the offset, size, md5, and uuid attributes
     * against expected values passed as parameters.
     */
    void checkByteStream(string name, H4ByteStream h4bs,
    		unsigned long long offset,
    		unsigned long long size,
    		string md5,
    		string uuid){

		CPPUNIT_ASSERT(h4bs.get_offset() == offset);
		BESDEBUG("dmrpp", name << " offset: " << offset << endl);
		CPPUNIT_ASSERT(h4bs.get_size() == size);
		BESDEBUG("dmrpp", name << " size: " << size << endl);
		CPPUNIT_ASSERT(h4bs.get_md5() == md5);
		BESDEBUG("dmrpp", name << " md5: " << md5 << endl);
		CPPUNIT_ASSERT(h4bs.get_uuid() == uuid);
		BESDEBUG("dmrpp", name << " uuid: " << uuid << endl);
    }

    /**
     * Evaluates a BaseType pointer believed to be an instance of DrmppCommon
     * with a single "chunk" (H4ByteStream) member.
     * This checks the variables name, offset, size, md5, and uuid attributes
     * against expected values passed as parameters.
     */
    void checkDmrppVariableWithSingleChunk(BaseType *bt,
    		string name,
    		unsigned long long offset,
    		unsigned long long size,
    		string md5,
    		string uuid){

		CPPUNIT_ASSERT(bt);

		BESDEBUG("dmrpp", "Looking at variable: " << bt->name() << endl);
		CPPUNIT_ASSERT(bt->name() == name);
		DmrppCommon *dc = dynamic_cast<DmrppCommon*>(bt);
		CPPUNIT_ASSERT(dc);

		vector<H4ByteStream> chunks = dc->get_immutable_chunks();
		CPPUNIT_ASSERT(chunks.size() == 1);
		checkByteStream(bt->name(), chunks[0],offset,size,md5,uuid);

    }

    /**
     * Evaluates a D4Group against exected values for the name, number of child
     * groups and the number of child variables.
     */
    void checkGroupsAndVars(D4Group *grp,  string name, int expectedNumGrps,  int expectedNumVars) {

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

        ifstream in(int_h5.c_str());
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", "Parsing complete"<< endl);

        D4Group *root = dmr->root();

        checkGroupsAndVars(root,"/",0,1);

        D4Group::Vars_iter v = root->var_begin();

        checkDmrppVariableWithSingleChunk(*v,
        		"scalar",
        		2144,
        		4,
        		"1ebc4541e985d612a5ff7ed2ee92bf3d",
        		"6609c41e-0feb-4c00-a11b-48ae9a493542");

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

       ifstream in(int_h5.c_str());
       parser.intern(in, dmr.get(), debug);

       D4Group *root = dmr->root();
       checkGroupsAndVars(root,"/",0,4);

       D4Group::Vars_iter v = root->var_begin();

       checkDmrppVariableWithSingleChunk(*v,
    		   "d16_1",
    		   2216,
    		   4,
    		   "094e70793148a97742191430ccea74c7",
    		   "4a0e86ee-484a-44f7-8c03-9c39b202d680");

       v++;

       checkDmrppVariableWithSingleChunk(*v,
    		   "d16_2",
    		   2220,
    		   8,
    		   "7d881b5a33bbc8bb7d21d3a24f803c6c",
    		   "2ebf051a-cb32-45ae-b846-172a4ea451f9");

       v++;

       checkDmrppVariableWithSingleChunk(*v,
    		   "d32_1",
    		   2228,
    		   32,
    		   "a9a3743b60524ab66f4d16546893f06b",
    		   "0ba99033-541d-44c6-8b7c-330ba3ce782e");

       v++;

       checkDmrppVariableWithSingleChunk(*v,
    		   "d32_2",
    		   2260,
    		   128,
    		   "780c27c624c158fb88305f41a767459d",
    		   "49d12a6c-cda0-49b5-8fce-12e8d160b4f7");
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

      ifstream in(float_h5.c_str());
      parser.intern(in, dmr.get(), debug);

      D4Group *root = dmr->root();

      checkGroupsAndVars(root,"/",0,4);


      D4Group::Vars_iter v = root->var_begin();

      checkDmrppVariableWithSingleChunk(*v,
    		  "d32_1",
    		  2216,
    		  8,
    		  "ec2b3d664cfbfd9217e74738bbcd281f",
    		  "c498ba62-0915-4641-bbda-e583daeae899");

      v++;

      checkDmrppVariableWithSingleChunk(*v,
    		  "d32_2",
    		  2224,
    		  16,
    		  "aeaafe45df3d2e57fe3c68f9887c60b0",
    		  "e5cb51bc-ed3f-4f66-89da-9793fdbd7667");

      v++;

      checkDmrppVariableWithSingleChunk(*v,
    		  "d64_1",
    		  2240,
    		  64,
    		  "f1e57dab82f2500507cad0888d520c08",
    		  "eef57d69-e7ee-4199-a14b-c97e63e865c6");

      v++;

      checkDmrppVariableWithSingleChunk(*v,
    		  "d64_2",
    		  2304,
    		  256,
    		  "88a376f14f1b9b5564b5930b931d3a1a",
    		  "e16c36b1-9d21-4015-9c8f-b1c88de67078");
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

      ifstream in(grid_2d.c_str());
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

      checkDmrppVariableWithSingleChunk(*v,
    		  "temperature",
    		  40672,
    		  128,
    		  "3b37566bd3a2587a88e4787820e0d36f",
    		  "3fd2c024-4934-4732-ad47-063539472602");

      top_level_grp_itr++;
      D4Group *hdfeos_info_grp = *top_level_grp_itr;

      checkGroupsAndVars(hdfeos_info_grp,"HDFEOS INFORMATION",0,1);

      v = hdfeos_info_grp->var_begin();

      checkDmrppVariableWithSingleChunk(*v,
    		  "StructMetadata.0",
    		  5304,
    		  32000,
    		  "a1d84a9da910f58677226bf71fa9d1dd",
    		  "1721dd71-90df-4781-af2f-4098eb28baca");
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

      ifstream in(nc4_group_atomic.c_str());
      parser.intern(in, dmr.get(), debug);

      D4Group *root = dmr->root();

      checkGroupsAndVars(root, "/", 1, 2);

      D4Group::Vars_iter v = root->var_begin();
      checkDmrppVariableWithSingleChunk(*v,
    		  "dim1",
    		  6192,
    		  8,
    		  "a068b90f3c19bf407f80db4945944e29",
    		  "d6d6cbc9-04bf-4d3d-8827-4d7c52939e6d");
      v++;
      checkDmrppVariableWithSingleChunk(*v,
    		  "d1",
    		  6200,
    		  8,
    		  "53163d5fb838cd8dabfd4425feda2b12",
    		  "55d3ca3d-4e1b-472e-822b-23d48838cf4d");

      D4Group::groupsIter top_level_grp_itr = root->grp_begin();
      D4Group *g1_grp = (*top_level_grp_itr);

      checkGroupsAndVars(g1_grp, "g1", 0, 2);

      v = g1_grp->var_begin();
      checkDmrppVariableWithSingleChunk(*v,
    		  "dim2",
    		  6208,
    		  12,
    		  "1719bc13ae9f2ad5aa50c720edc400a6",
    		  "4c91c3f7-73a1-43d5-a0fc-cfa23e92b3d2");

      v++;
      checkDmrppVariableWithSingleChunk(*v,
    		  "d2",
    		  6220,
    		  24,
    		  "2162ed0aecd0db6abb21fd1b4d56af73",
    		  "62569081-e56e-47b5-ab10-ea9132cc8ef2");
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

