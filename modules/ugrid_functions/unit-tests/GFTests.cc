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

//#include <cstdio>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include "GetOpt.h"

#include "debug.h"
#include "util.h"

#include "gridfields/grid.h"
#include "gridfields/gridfield.h"
#include "gridfields/bind.h"
#include "gridfields/array.h"
#include "gridfields/restrict.h"
#include "gridfields/refrestrict.h"
#include "gridfields/arrayreader.h"
#include "gridfields/accumulate.h"
#include <gridfields/implicit0cells.h>

#include "LocationType.h"

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

//using namespace GF;

namespace ugrid {
#if 0
struct coordinate {
    string name;
    locationType location;
    int size;
    float *vals;
};

class ugrid {
private:
    string name;
    int nodesPerFace;
    int nodeCount;
    int faceCount;
    int *faceNodeConnectivity;
    vector<coordinate *> *coordinates;
public:
    ugrid():name(""),nodeCount(0),nodesPerFace(0),faceCount(0),faceNodeConnectivity(0),coordinates(0) {}
    ~ugrid() {
        delete faceNodeConnectivity;
        if(coordinates) {
            for(int i=0; i<coordinates->size();i++) {
                delete (*coordinates)[i]->vals;
            }
            delete coordinates;
        }
    }
};
#endif

class GFTests: public CppUnit::TestFixture {

private:

    void buildNewOcotoPieGridField(GF::GridField **gf, GF::Grid **g, GF::Node **fncaMeshNodes,
        vector<GF::Array *> *gfAttributes)
    {

        string name = "octo-pie";
        int nodeCount = 9;
        int faceCount = 8;
        int nodesPerFace = 3;

        // This array is organized like a Ugrid Face Node Connectivity array,
        // and will need to be repackaged for the GF API. (see below)
        int octopieFNCA[] = { 1, 2, 3, 4, 5, 6, 7, 8, 2, 3, 4, 5, 6, 7, 8, 1, 9, 9, 9, 9, 9, 9, 9, 9 };

        float xcoord_values[] = { -1.0, 0.0, 1.0, 1.5, 1.0, 0.0, -1.0, -1.5, 0.0 };
        float ycoord_values[] = { 1.0, 1.5, 1.0, 0.0, -1.0, -1.5, -1.0, 0.0, 0.0 };

        float oneDnodedata[] = { 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8 };

#if 0
        // Unused
        float twoDnodedata[] = {
            0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8,
            1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8,
            2.0, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8
        };
        float oneDFaceData[] = {
            0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8
        };
#endif
        int index[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

        GF::Array *coordinate, *nodeData; // unused. 4/7/14 jhrg , *faceData;

        *g = new GF::Grid(name);

        DBG(cerr << "GFTests::getTestGrid() - Building and adding implicit range Nodes to the GF::Grid" << endl);

        // Make the implicit nodes - same size as the node coordinate arrays
        GF::AbstractCellArray *implicitNodes = new GF::Implicit0Cells(nodeCount);

        // Attach the implicit nodes to the grid at rank 0 (node)
        (*g)->setKCells(implicitNodes, node);

        // Create the Cell Array (aka the Face Node Connectivity array in the Ugrid Specification)
        *fncaMeshNodes = new GF::Node[faceCount * nodesPerFace];

        DBG(cerr << "GFTests::getTestGrid() - Re-packing and copying FNC Array array to GF:Node array." << endl);
        for (int fIndex = 0; fIndex < faceCount; fIndex++) {
            for (int nIndex = 0; nIndex < nodesPerFace; nIndex++) {
                (*fncaMeshNodes)[nodesPerFace * fIndex + nIndex] = octopieFNCA[fIndex + (faceCount * nIndex)];
            }
        }

        // Create the cell array
        GF::CellArray *faceNodeConnectivityCells = new GF::CellArray(*fncaMeshNodes, faceCount, nodesPerFace);

        // Attach the Cell Mesh to the grid at rank 2
        // This 2 stands for rank 2, or faces.
        DBG(cerr << "GFTests::getTestGrid() - Attaching Cell array to GF::Grid" << endl);
        (*g)->setKCells(faceNodeConnectivityCells, face);

        // The Grid is complete. Now we make a GridField from the Grid
        DBG(cerr << "GFTests::getTestGrid() - Constructing new GF::GridField from GF::Grid" << endl);
        *gf = new GF::GridField(*g);

        DBG(cerr << "GFTests::getTestGrid() - Adding X coordinate." << endl);
        coordinate = new GF::Array("X", GF::FLOAT);
        coordinate->copyFloatData(xcoord_values, nodeCount);
        (*gf)->AddAttribute(node, coordinate);
        gfAttributes->push_back(coordinate);

        DBG(cerr << "GFTests::getTestGrid() - Adding Y coordinate." << endl);
        coordinate = new GF::Array("Y", GF::FLOAT);
        coordinate->copyFloatData(ycoord_values, nodeCount);
        (*gf)->AddAttribute(node, coordinate);
        gfAttributes->push_back(coordinate);

        DBG(cerr << "GFTests::getTestGrid() - Adding oneDnodedata data." << endl);
        nodeData = new GF::Array("oneDnodedata", GF::FLOAT);
        nodeData->copyFloatData(oneDnodedata, nodeCount);
        (*gf)->AddAttribute(node, nodeData);
        gfAttributes->push_back(nodeData);

        DBG(cerr << "GFTests::getTestGrid() - Adding index data." << endl);
        nodeData = new GF::Array("index", GF::INT);
        nodeData->copyIntData(index, nodeCount);
        (*gf)->AddAttribute(node, nodeData);
        gfAttributes->push_back(nodeData);

#if 0
        DBG(cerr << "GFTests::getTestGrid() - Adding oneDFaceData data." << endl);
        faceData = new GF::Array("oneDFaceData", GF::FLOAT);
        faceData->copyFloatData(oneDFaceData,faceCount);
        (*gf)->AddAttribute(face, faceData);
        gfAttributes->push_back(faceData);
#endif

    }

public:

    // Called once before everything gets tested
    GFTests()
    {
    }

    // Called at the end of the test
    ~GFTests()
    {
    }

    // Called before each test
    void setup()
    {
    }

    // Called after each test
    void tearDown()
    {
    }

CPPUNIT_TEST_SUITE( GFTests );

    CPPUNIT_TEST(gf_test);

    CPPUNIT_TEST_SUITE_END()
    ;

    void gf_test()
    {
        try {

            DBG(cerr << " gf_test - BEGIN" << endl);

            GF::GridField *opieGridField;
            GF::Grid *opieGrid;
            GF::Node *fncaMeshNodes;
            vector<GF::Array *> gfAttributes;

            buildNewOcotoPieGridField(&opieGridField, &opieGrid, &fncaMeshNodes, &gfAttributes);

            // Build the restriction operator;
            DBG(
                cerr << "GFTests::gf_test() - Constructing new GF::RestrictOp using user "
                    << "supplied 'dimension' value and filter expression combined with the GF:GridField " << endl);
            GF::RestrictOp op = GF::RestrictOp("X>=0", node, opieGridField);

            // Apply the operator and get the result;
            DBG(cerr << "GFTests::gf_test() - Applying GridField operator." << endl);
            GF::GridField *resultGF = op.getResult();

            GF::Array *indexResult = resultGF->GetAttribute(node, "index");
            indexResult->print();
            GF::Array *oneDnodeDataResult = resultGF->GetAttribute(node, "oneDnodedata");
            oneDnodeDataResult->print();

            //GF::Array *oneDFaceDataResult = resultGF->GetAttribute(face, "oneDFaceData");
            //oneDFaceDataResult->print();

            delete resultGF;
            delete opieGridField;
            for (unsigned int i = 0; i < gfAttributes.size(); i++) {
                delete gfAttributes[i];
            }
            delete opieGrid;
            delete fncaMeshNodes;

            CPPUNIT_ASSERT(true);
        }
        catch (std::string &e) {
            cerr << "Error: " << e << endl;
            CPPUNIT_ASSERT(false);
        }
        catch (...) {
            cerr << "Unknown Error." << endl;
            CPPUNIT_ASSERT(false);
        }
    }

};
// BindTest

CPPUNIT_TEST_SUITE_REGISTRATION(GFTests);

} // namespace ugrid

int main(int argc, char*argv[])
{
    GetOpt getopt(argc, argv, "dh");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            std::cerr << "Usage: GFTests has the following tests:" << std::endl;
            const std::vector<CppUnit::Test*> &tests = ugrid::GFTests::suite()->getTests();
            unsigned int prefix_len = ugrid::GFTests::suite()->getName().append("::").length();
            for (std::vector<CppUnit::Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                std::cerr << (*i)->getName().replace(0, prefix_len, "") << std::endl;
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
            test = ugrid::GFTests::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

