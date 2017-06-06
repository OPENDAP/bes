// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003,2011,2012 OPeNDAP, Inc.
// Authors: Nathan Potter <ndp@opendap.org>
//          James Gallagher <jgallagher@opendap.org>
//          Scott Moe <smeest1@gmail.com>
//          Bill Howe <billhowe@cs.washington.edu>
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

#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>

#include <gridfields/type.h>
#include <gridfields/gridfield.h>
#include <gridfields/grid.h>
#include <gridfields/implicit0cells.h>
#include <gridfields/gridfieldoperator.h>
#include <gridfields/restrict.h>
#include <gridfields/refrestrict.h>
#include <gridfields/array.h>
#include <gridfields/GFError.h>

#include "BaseType.h"
#include "Int32.h"
#include "Float64.h"
#include "Array.h"
#include "util.h"

#include "ugrid_utils.h"
//#include "NDimensionalArray.h"
#include "MeshDataVariable.h"
#include "TwoDMeshTopology.h"

#include "BESDebug.h"
#include "BESError.h"

#ifdef NDEBUG
#undef BESDEBUG
#define BESDEBUG( x, y )
#endif

using namespace std;
using namespace libdap;
using namespace ugrid;

namespace ugrid {

/* not used. faceCoordinateNames(0), */
TwoDMeshTopology::TwoDMeshTopology() :
    d_meshVar(0), nodeCoordinateArrays(0), nodeCount(0), faceNodeConnectivityArray(0), faceCount(0), faceCoordinateArrays(
        0), gridTopology(0), d_inputGridField(0), resultGridField(0), fncCellArray(0), _initialized(false)
{
    rangeDataArrays = new vector<MeshDataVariable *>();
    sharedIntArrays = new vector<int *>();
    sharedFloatArrays = new vector<float *>();
}

TwoDMeshTopology::~TwoDMeshTopology()
{
    BESDEBUG("ugrid", "~TwoDMeshTopology() - BEGIN" << endl);
    BESDEBUG("ugrid", "~TwoDMeshTopology() - (" << this << ")" << endl);

    BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting GF::GridField 'resultGridField'." << endl);
    delete resultGridField;

    BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting GF::GridField 'inputGridField'." << endl);
    delete d_inputGridField;

    BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting GF::Grid 'gridTopology'." << endl);
    delete gridTopology;

    BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting GF::Arrays..." << endl);
    for (vector<GF::Array *>::iterator it = gfArrays.begin(); it != gfArrays.end(); ++it) {
        GF::Array *gfa = *it;
        BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting GF:Array  '" << gfa->getName() << "'" << endl);
        delete gfa;
    }
    BESDEBUG("ugrid", "~TwoDMeshTopology() - GF::Arrays deleted." << endl);

    BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting sharedIntArrays..." << endl);
    for (vector<int *>::iterator it = sharedIntArrays->begin(); it != sharedIntArrays->end(); ++it) {
        int *i = *it;
        BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting integer array '" << i << "'" << endl);
        delete[] i;
    }
    delete sharedIntArrays;

    BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting sharedFloatArrays..." << endl);
    for (vector<float *>::iterator it = sharedFloatArrays->begin(); it != sharedFloatArrays->end(); ++it) {
        float *f = *it;
        BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting float array '" << f << "'" << endl);
        delete[] f;
    }
    delete sharedFloatArrays;

    BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting range data vector" << endl);
    delete rangeDataArrays;
    BESDEBUG("ugrid", "~TwoDMeshTopology() - Range data vector deleted." << endl);

    BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting vector of node coordinate arrays." << endl);
    delete nodeCoordinateArrays;

    BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting vector of face coordinate arrays." << endl);
    delete faceCoordinateArrays;

    BESDEBUG("ugrid", "~TwoDMeshTopology() - Deleting face node connectivity cell array (GF::Node's)." << endl);
    delete[] fncCellArray;

    BESDEBUG("ugrid", "~TwoDMeshTopology() - END" << endl);
}

/**
 * @TODO only call this from the constructor?? Seems like the thing to do, but then the constructor will be possibly throwing an Error - Is that OK?
 */
void TwoDMeshTopology::init(string meshVarName, DDS *dds)
{
    if (_initialized) return;

    d_meshVar = dds->var(meshVarName);

    if (!d_meshVar) throw Error("Unable to locate variable: " + meshVarName);

    dimension = getAttributeValue(d_meshVar, UGRID_TOPOLOGY_DIMENSION);
    if (dimension.empty()) dimension = getAttributeValue(d_meshVar, UGRID_DIMENSION);

    if (dimension.empty()) {
        string msg = "ugr5(): The mesh topology variable  '" + d_meshVar->name()
            + "' is missing the required attribute named '" + UGRID_TOPOLOGY_DIMENSION + "'";
        BESDEBUG("ugrid", "TwoDMeshTopology::init() - " << msg);
        throw Error(msg);
    }

    // Retrieve the node coordinate arrays for the mesh
    ingestNodeCoordinateArrays(d_meshVar, dds);

    // Why would we retrieve this? I think Bill said that this needs to be recomputed after a restrict operation.
    // @TODO Verify that Bill actually said that this needs to be recomputed.
    // Retrieve the face coordinate arrays (if any)  for the mesh
    ingestFaceCoordinateArrays(d_meshVar, dds);

    // Inspect and QC the face node connectivity array for the mesh
    ingestFaceNodeConnectivityArray(d_meshVar, dds);

    // Load d_meshVar with some data value - needed because this variable
    // will be returned as part of the result and DAP does not allow
    // empty variables. Since this code is designed to work with the UGrid
    // specification being developed as an extension to (or in conjunction
    // with) CF, I am assuming the UGrids will always be netCDF files and
    // that the dummy mesh variable will always get a value when read()
    // is called because netCDF guarantees that reading missing values
    // returns the 'fill value'.
    // See https://www.unidata.ucar.edu/software/netcdf/docs/netcdf-c/Fill-Values.html
    // jhrg 4/15/15
    try {
        d_meshVar->read();    // read() sets read_p to true
    }
    catch (Error &e) {
        throw Error(malformed_expr,
            "ugr5(): While trying to read the UGrid mesh variable, an error occurred: " + e.get_error_message());
    }
    catch (std::exception &e) {
        throw Error(malformed_expr,
            string("ugr5(): While trying to read the UGrid mesh variable, an error occurred: ") + e.what());
    }

    _initialized = true;
}

void TwoDMeshTopology::setNodeCoordinateDimension(MeshDataVariable *mdv)
{
    BESDEBUG("ugrid", "TwoDMeshTopology::setNodeCoordinateDimension() - BEGIN" << endl);
    libdap::Array *dapArray = mdv->getDapArray();
    libdap::Array::Dim_iter ait1;

    for (ait1 = dapArray->dim_begin(); ait1 != dapArray->dim_end(); ++ait1) {

        string dimName = dapArray->dimension_name(ait1);

        if (dimName.compare(nodeDimensionName) == 0) { // are the names the same?
            BESDEBUG("ugrid",
                "TwoDMeshTopology::setNodeCoordinateDimension() - Found dimension name matching nodeDimensionName '"<< nodeDimensionName << "'" << endl);
            int size = dapArray->dimension_size(ait1, true);
            if (size == nodeCount) { // are they the same size?
                BESDEBUG("ugrid",
                    "TwoDMeshTopology::setNodeCoordinateDimension() - Dimension sizes match (" << libdap::long_to_string(nodeCount) << ") - DONE" << endl);
                // Yay! We found the node coordinate dimension
                mdv->setLocationCoordinateDimension(ait1);
                return;
            }
        }
    }

    throw Error(
        "Unable to determine the node coordinate dimension of the range variable '" + mdv->getName()
            + "'  The node dimension is named '" + nodeDimensionName + "'  with size "
            + libdap::long_to_string(nodeCount));

}

void TwoDMeshTopology::setFaceCoordinateDimension(MeshDataVariable *mdv)
{

    libdap::Array *dapArray = mdv->getDapArray();
    libdap::Array::Dim_iter ait1;

    for (ait1 = dapArray->dim_begin(); ait1 != dapArray->dim_end(); ++ait1) {
        string dimName = dapArray->dimension_name(ait1);

        if (dimName.compare(faceDimensionName) == 0) { // are the names the same?
            int size = dapArray->dimension_size(ait1, true);
            if (size == faceCount) { // are they the same size?
                // Yay! We found the coordinate dimension
                mdv->setLocationCoordinateDimension(ait1);
                return;
            }
        }

    }
    throw Error(
        "Unable to determine the face coordinate dimension of the range variable '" + mdv->getName()
            + "'  The face coordinate dimension is named '" + faceDimensionName + "' with size "
            + libdap::long_to_string(faceCount));

}

void TwoDMeshTopology::setLocationCoordinateDimension(MeshDataVariable *mdv)
{

    BESDEBUG("ugrid", "TwoDMeshTopology::setLocationCoordinateDimension() - BEGIN" << endl);

    // libdap::Array *dapArray = mdv->getDapArray();

    string locstr;
    switch (mdv->getGridLocation()) {

    case node: {
        BESDEBUG("ugrid",
            "TwoDMeshTopology::setLocationCoordinateDimension() - Checking Node variable  '"<< mdv->getName() << "'" << endl);
        // Locate and set the MDV's node coordinate dimension.
        setNodeCoordinateDimension(mdv);
        locstr = "node";
    }
        break;

    case face: {
        BESDEBUG("ugrid",
            "TwoDMeshTopology::setLocationCoordinateDimension() - Checking Face variable  '"<< mdv->getName() << "'" << endl);
        // Locate and set the MDV's face coordinate dimension.
        setFaceCoordinateDimension(mdv);
        locstr = "face";
    }
        break;

    case edge:
    default: {
        string msg = "TwoDMeshTopology::setLocationCoordinateDimension() - Unknown/Unsupported location value '"
            + libdap::long_to_string(mdv->getGridLocation()) + "'";
        BESDEBUG("ugrid", msg << endl);
        throw Error(msg);
    }
        break;
    }
    BESDEBUG("ugrid",
        "TwoDMeshTopology::setLocationCoordinateDimension() - MeshDataVariable '"<< mdv->getName() << "' is a "<< locstr <<" variable," << endl);
    BESDEBUG("ugrid",
        "TwoDMeshTopology::setLocationCoordinateDimension() - MeshDataVariable '"<< mdv->getName() << "' location coordinate dimension is '" << /*dapArray*/mdv->getDapArray()->dimension_name(mdv->getLocationCoordinateDimension()) << "'" << endl);

    BESDEBUG("ugrid", "TwoDMeshTopology::setLocationCoordinateDimension() - DONE" << endl);

}

/**
 * Locates the the DAP variable identified by the face_node_connectivity attribute of the
 * meshTopology variable. The located variable is QC'd against the expectations of the fnoc var
 * and an exception throw if there is a problem. Finally the dimensions are checked to determine
 * which one represents the set of faces. The assumption is that the number of faces will always
 * be larger than the (max) number of nodes per face.
 */
void TwoDMeshTopology::ingestFaceNodeConnectivityArray(libdap::BaseType *meshTopology, libdap::DDS *dds)
{

    BESDEBUG("ugrid", "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - Locating FNCA" << endl);

    string face_node_connectivity_var_name;
    AttrTable at = meshTopology->get_attr_table();

    AttrTable::Attr_iter iter_fnc = at.simple_find(UGRID_FACE_NODE_CONNECTIVITY);
    if (iter_fnc != at.attr_end()) {
        face_node_connectivity_var_name = at.get_attr(iter_fnc, 0);
    }
    else {
        throw Error(
            "Could not locate the " UGRID_FACE_NODE_CONNECTIVITY " attribute in the " UGRID_MESH_TOPOLOGY " variable! "
            "The mesh_topology variable is named " + meshTopology->name());
    }
    BESDEBUG("ugrid",
        "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - Located the '" << UGRID_FACE_NODE_CONNECTIVITY << "' attribute." << endl);

    // Find the variable using the name

    BaseType *btp = dds->var(face_node_connectivity_var_name);

    if (btp == 0)
        throw Error(
            "Could not locate the " UGRID_FACE_NODE_CONNECTIVITY " variable named '" + face_node_connectivity_var_name
                + "'! " + "The mesh_topology variable is named " + meshTopology->name());

    // Is it an array?
    libdap::Array *fncArray = dynamic_cast<libdap::Array*>(btp);
    if (fncArray == 0) {
        throw Error(malformed_expr,
            "Face Node Connectivity variable '" + face_node_connectivity_var_name
                + "' is not an Array type. It's an instance of " + btp->type_name());
    }

    BESDEBUG("ugrid",
        "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - Located FNC Array '" << fncArray->name() << "'." << endl);

    // It's got to have exactly 2 dimensions - [max#nodes_per_face][#faces]
    int numDims = fncArray->dimensions(true);
    if (numDims != 2) {
        throw Error(malformed_expr,
            "Face Node Connectivity variable '" + face_node_connectivity_var_name
                + "' Must have two (2) dimensions. It has " + libdap::long_to_string(numDims));
    }

    BESDEBUG("ugrid",
        "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - FNC Array '" << fncArray->name() << "' has two (2) dimensions." << endl);

    // We just need to have both dimensions handy, so get the first and the second
    libdap::Array::Dim_iter firstDim = fncArray->dim_begin();
    libdap::Array::Dim_iter secondDim = fncArray->dim_begin(); // same as the first for a moment...
    secondDim++; // now it's second!

    if (faceDimensionName.empty()) {
        // By now we know it only has two dimensions, but since there is no promise that they'll be in a particular order
        // we punt: We'll assume that smallest of the two is in fact the nodes per face and the larger the face index dimensions.
        int sizeFirst = fncArray->dimension_size(firstDim, true);
        int sizeSecond = fncArray->dimension_size(secondDim, true);
        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - sizeFirst: "<< libdap::long_to_string(sizeFirst) << endl);
        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - sizeSecond: "<< libdap::long_to_string(sizeSecond) << endl);

        if (sizeFirst < sizeSecond) {
            BESDEBUG("ugrid",
                "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - FNC Array first dimension is smaller than second." << endl);
            fncNodesDim = firstDim;
            fncFacesDim = secondDim;
        }
        else {
            BESDEBUG("ugrid",
                "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - FNC Array second dimension is smaller than first." << endl);
            fncNodesDim = secondDim;
            fncFacesDim = firstDim;
        }

        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - fncFacesDim meshVarName: '" << fncArray->dimension_name(fncFacesDim) << "'" << endl);
        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - fncFacesDim size: '" << fncArray->dimension_size(fncFacesDim,true) << "'" << endl);

        faceDimensionName = fncArray->dimension_name(fncFacesDim);
    }
    else {
        // There is already a faceDimensionName defined - possibly from loading face coordinate variables.

        // Does it match the name of the first or second dimensions of the fncArray? It better!
        if (faceDimensionName.compare(fncArray->dimension_name(firstDim)) == 0) {
            fncNodesDim = secondDim;
            fncFacesDim = firstDim;
        }
        else if (faceDimensionName.compare(fncArray->dimension_name(secondDim)) == 0) {
            fncNodesDim = firstDim;
            fncFacesDim = secondDim;
        }
        else {
            string msg = "The face coordinate dimension of the Face Node Connectivity variable '"
                + face_node_connectivity_var_name + "' Has dimension name.'" + fncArray->dimension_name(fncFacesDim)
                + "' which does not match the existing face coordinate dimension meshVarName '" + faceDimensionName
                + "'";
            BESDEBUG("ugrid", msg << endl);
            throw Error(msg);
        }
        BESDEBUG("ugrid", "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - Face dimension names match." << endl);

    }

    BESDEBUG("ugrid",
        "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - Face dimension name: '" << faceDimensionName << "'" << endl);

    // Check to see if faceCount is initialized and do so if needed
    if (faceCount == 0) {
        faceCount = fncArray->dimension_size(fncFacesDim, true);
        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - Face count: "<< libdap::long_to_string(faceCount) << endl);
    }
    else {
        // Make sure the face counts match.

        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - Face count: "<< libdap::long_to_string(faceCount) << endl);
        if (faceCount != fncArray->dimension_size(fncFacesDim, true)) {
            string msg = "The faces dimension of the Face Node Connectivity variable '"
                + face_node_connectivity_var_name + "' Has size "
                + libdap::long_to_string(fncArray->dimension_size(fncFacesDim, true))
                + " which does not match the existing face count of " + libdap::long_to_string(faceCount);
            BESDEBUG("ugrid", msg << endl);
            throw Error(msg);
        }
        BESDEBUG("ugrid", "TwoDMeshTopology::ingestFaceNodeConnectivityArray() - Face counts match!" << endl);
    }

    faceNodeConnectivityArray = fncArray;

    BESDEBUG("ugrid", "TwoDMeshTopology::getFaceNodeConnectivityArray() - Got FCNA '"+fncArray->name()+"'" << endl);

}
/**
 * Returns the coordinate variables identified in the meshTopology variable's node_coordinates attribute.
 * throws an error if the node_coordinates attribute is missing, if the coordinates are not arrays, and
 * if the arrays are not all the same shape.
 */
void TwoDMeshTopology::ingestFaceCoordinateArrays(libdap::BaseType *meshTopology, libdap::DDS *dds)
{
    BESDEBUG("ugrid",
        "TwoDMeshTopology::ingestFaceCoordinateArrays() - BEGIN Gathering face coordinate arrays..." << endl);

    if (faceCoordinateArrays == 0) faceCoordinateArrays = new vector<libdap::Array *>();

    faceCoordinateArrays->clear();

    string face_coordinates;
    AttrTable at = meshTopology->get_attr_table();

    AttrTable::Attr_iter iter_nodeCoors = at.simple_find(UGRID_FACE_COORDINATES);
    if (iter_nodeCoors != at.attr_end()) {
        face_coordinates = at.get_attr(iter_nodeCoors, 0);

        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestFaceCoordinateArrays() - Located '"<< UGRID_FACE_COORDINATES << "' attribute." << endl);

        // Split the node_coordinates string up on spaces
        vector<string> faceCoordinateNames = split(face_coordinates, ' ');

        // Find each variable in the resulting list
        vector<string>::iterator coorName_it;
        for (coorName_it = faceCoordinateNames.begin(); coorName_it != faceCoordinateNames.end(); ++coorName_it) {
            string faceCoordinateName = *coorName_it;

            BESDEBUG("ugrid",
                "TwoDMeshTopology::ingestFaceCoordinateArrays() - Processing face coordinate '"<< faceCoordinateName << "'." << endl);

            //Now that we have the name of the coordinate variable get it from the DDS!!
            BaseType *btp = dds->var(faceCoordinateName);
            if (btp == 0)
                throw Error(
                    "Could not locate the " UGRID_FACE_COORDINATES " variable named '" + faceCoordinateName + "'! "
                        + "The mesh_topology variable is named " + meshTopology->name());

            libdap::Array *newFaceCoordArray = dynamic_cast<libdap::Array*>(btp);
            if (newFaceCoordArray == 0) {
                throw Error(malformed_expr,
                    "Face coordinate variable '" + faceCoordinateName + "' is not an Array type. It's an instance of "
                        + btp->type_name());
            }

            BESDEBUG("ugrid",
                "TwoDMeshTopology::ingestFaceCoordinateArrays() - Found face coordinate array '"<< faceCoordinateName << "'." << endl);

            // Coordinate arrays MUST be single dimensioned.
            if (newFaceCoordArray->dimensions(true) != 1) {
                throw Error(malformed_expr,
                    "Face coordinate variable '" + faceCoordinateName
                        + "' has more than one dimension. That's just not allowed. It has "
                        + long_to_string(newFaceCoordArray->dimensions(true)) + " dimensions.");
            }
            BESDEBUG("ugrid",
                "TwoDMeshTopology::ingestFaceCoordinateArrays() - Face coordinate array '"<< faceCoordinateName << "' has a single dimension." << endl);

            // Make sure this node coordinate variable has the same size and meshVarName as all the others on the list - error if not true.
            string dimName = newFaceCoordArray->dimension_name(newFaceCoordArray->dim_begin());
            int dimSize = newFaceCoordArray->dimension_size(newFaceCoordArray->dim_begin(), true);

            BESDEBUG("ugrid",
                "TwoDMeshTopology::ingestFaceCoordinateArrays() - dimName: '"<< dimName << "' dimSize: " << libdap::long_to_string(dimSize) << endl);

            if (faceDimensionName.empty()) {
                faceDimensionName = dimName;
            }
            BESDEBUG("ugrid",
                "TwoDMeshTopology::ingestFaceCoordinateArrays() - faceDimensionName: '"<< faceDimensionName << "' " << endl);

            if (faceDimensionName.compare(dimName) != 0) {
                throw Error(
                    "The face coordinate array '" + faceCoordinateName + "' has the named dimension '" + dimName
                        + "' which differs from the expected  dimension meshVarName '" + faceDimensionName
                        + "'. The mesh_topology variable is named " + meshTopology->name());
            }
            BESDEBUG("ugrid", "TwoDMeshTopology::ingestFaceCoordinateArrays() - Face dimension names match." << endl);

            if (faceCount == 0) {
                faceCount = dimSize;
            }
            BESDEBUG("ugrid",
                "TwoDMeshTopology::ingestFaceCoordinateArrays() - faceCount: "<< libdap::long_to_string(faceCount) << endl);

            if (faceCount != dimSize) {
                throw Error(
                    "The face coordinate array '" + faceCoordinateName + "' has a dimension size of "
                        + libdap::long_to_string(dimSize) + " which differs from the the expected size of "
                        + libdap::long_to_string(faceCount) + " The mesh_topology variable is named "
                        + meshTopology->name());
            }
            BESDEBUG("ugrid", "TwoDMeshTopology::ingestFaceCoordinateArrays() - Face counts match." << endl);

            // Add variable to faceCoordinateArrays.
            faceCoordinateArrays->push_back(newFaceCoordArray);
            BESDEBUG("ugrid",
                "TwoDMeshTopology::ingestFaceCoordinateArrays() - Face coordinate array '"<< faceCoordinateName << "' ingested." << endl);
        }
        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestFaceCoordinateArrays() - Located "<< libdap::long_to_string(faceCoordinateArrays->size()) << " face coordinate arrays." << endl);

    }
    else {
        BESDEBUG("ugrid", "TwoDMeshTopology::ingestFaceCoordinateArrays() - No Face Coordinates Found." << endl);
    }

    BESDEBUG("ugrid", "TwoDMeshTopology::ingestFaceCoordinateArrays() - DONE" << endl);

}

/**
 * Returns the coordinate variables identified in the meshTopology variable's node_coordinates attribute.
 * throws an error if the node_coordinates attribute is missing, if the coordinates are not arrays, and
 * if the arrays are not all the same shape.
 */
void TwoDMeshTopology::ingestNodeCoordinateArrays(libdap::BaseType *meshTopology, libdap::DDS *dds)
{
    BESDEBUG("ugrid",
        "TwoDMeshTopology::ingestNodeCoordinateArrays() - BEGIN Gathering node coordinate arrays..." << endl);

    string node_coordinates;
    AttrTable at = meshTopology->get_attr_table();

    AttrTable::Attr_iter iter_nodeCoors = at.simple_find(UGRID_NODE_COORDINATES);
    if (iter_nodeCoors != at.attr_end()) {
        node_coordinates = at.get_attr(iter_nodeCoors, 0);
    }
    else {
        throw Error("Could not locate the " UGRID_NODE_COORDINATES " attribute in the " UGRID_MESH_TOPOLOGY
        " variable! The mesh_topology variable is named " + meshTopology->name());
    }
    BESDEBUG("ugrid",
        "TwoDMeshTopology::ingestNodeCoordinateArrays() - Located '"<< UGRID_NODE_COORDINATES << "' attribute." << endl);

    if (nodeCoordinateArrays == 0) nodeCoordinateArrays = new vector<libdap::Array *>();

    nodeCoordinateArrays->clear();

    // Split the node_coordinates string up on spaces
    // TODO make this work on situations where multiple spaces in the node_coorindates string doesn't hose the split()
    vector<string> nodeCoordinateNames = split(node_coordinates, ' ');

    // Find each variable in the resulting list
    vector<string>::iterator coorName_it;
    for (coorName_it = nodeCoordinateNames.begin(); coorName_it != nodeCoordinateNames.end(); ++coorName_it) {
        string nodeCoordinateName = *coorName_it;

        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestNodeCoordinateArrays() - Processing node coordinate '"<< nodeCoordinateName << "'." << endl);

        //Now that we have the name of the coordinate variable get it from the DDS!!
        BaseType *btp = dds->var(nodeCoordinateName);
        if (btp == 0)
            throw Error(
                "Could not locate the " UGRID_NODE_COORDINATES " variable named '" + nodeCoordinateName + "'! "
                    + "The mesh_topology variable is named " + meshTopology->name());

        libdap::Array *newNodeCoordArray = dynamic_cast<libdap::Array*>(btp);
        if (newNodeCoordArray == 0) {
            throw Error(malformed_expr,
                "Node coordinate variable '" + nodeCoordinateName + "' is not an Array type. It's an instance of "
                    + btp->type_name());
        }
        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestNodeCoordinateArrays() - Found node coordinate array '"<< nodeCoordinateName << "'." << endl);

        // Coordinate arrays MUST be single dimensioned.
        if (newNodeCoordArray->dimensions(true) != 1) {
            throw Error(malformed_expr,
                "Node coordinate variable '" + nodeCoordinateName
                    + "' has more than one dimension. That's just not allowed. It has "
                    + long_to_string(newNodeCoordArray->dimensions(true)) + " dimensions.");
        }
        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestNodeCoordinateArrays() - Node coordinate array '"<< nodeCoordinateName << "' has a single dimension." << endl);

        // Make sure this node coordinate variable has the same size and meshVarName as all the others on the list - error if not true.
        string dimName = newNodeCoordArray->dimension_name(newNodeCoordArray->dim_begin());
        int dimSize = newNodeCoordArray->dimension_size(newNodeCoordArray->dim_begin(), true);

        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestNodeCoordinateArrays() - dimName: '"<< dimName << "' dimSize: " << libdap::long_to_string(dimSize) << endl);

        if (nodeDimensionName.empty()) {
            nodeDimensionName = dimName;
        }
        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestNodeCoordinateArrays() - nodeDimensionName: '"<< nodeDimensionName << "' " << endl);
        if (nodeDimensionName.compare(dimName) != 0) {
            throw Error(
                "The node coordinate array '" + nodeCoordinateName + "' has the named dimension '" + dimName
                    + "' which differs from the expected  dimension meshVarName '" + nodeDimensionName
                    + "'. The mesh_topology variable is named " + meshTopology->name());
        }
        BESDEBUG("ugrid", "TwoDMeshTopology::ingestNodeCoordinateArrays() - Node dimension names match." << endl);

        if (nodeCount == 0) {
            nodeCount = dimSize;
        }
        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestNodeCoordinateArrays() - nodeCount: "<< libdap::long_to_string(nodeCount) << endl);

        if (nodeCount != dimSize) {
            throw Error(
                "The node coordinate array '" + nodeCoordinateName + "' has a dimension size of "
                    + libdap::long_to_string(dimSize) + " which differs from the the expected size of "
                    + libdap::long_to_string(nodeCount) + " The mesh_topology variable is named "
                    + meshTopology->name());
        }
        BESDEBUG("ugrid", "TwoDMeshTopology::ingestNodeCoordinateArrays() - Node counts match." << endl);

        // Add variable to nodeCoordinateArrays vector.
        nodeCoordinateArrays->push_back(newNodeCoordArray);
        BESDEBUG("ugrid",
            "TwoDMeshTopology::ingestNodeCoordinateArrays() - Coordinate array '"<< nodeCoordinateName << "' ingested." << endl);

    }

    BESDEBUG("ugrid", "TwoDMeshTopology::ingestNodeCoordinateArrays() - DONE" << endl);

}

void TwoDMeshTopology::buildBasicGfTopology()
{

    BESDEBUG("ugrid",
        "TwoDMeshTopology::buildBasicGfTopology() - Building GridFields objects for mesh_topology variable "<< getMeshVariable()->name() << endl);
    // Start building the Grid for the GridField operation.
    BESDEBUG("ugrid",
        "TwoDMeshTopology::buildGridFieldsTopology() - Constructing new GF::Grid for "<< meshVarName() << endl);
    gridTopology = new GF::Grid(meshVarName());

    // 1) Make the implicit nodes - same size as the node coordinate arrays
    BESDEBUG("ugrid",
        "TwoDMeshTopology::buildGridFieldsTopology() - Building and adding implicit range Nodes to the GF::Grid" << endl);
    GF::AbstractCellArray *nodes = new GF::Implicit0Cells(nodeCount);
    // Attach the implicit nodes to the grid at rank 0
    gridTopology->setKCells(nodes, node);

    // @TODO Do I need to add implicit k-cells for faces (rank 2) if I plan to add range data on faces later?
    // Apparently not...

    // Attach the Mesh to the grid.
    // Get the face node connectivity cells (i think these correspond to the GridFields K cells of Rank 2)
    // FIXME Read this array once! It is read again below..
    BESDEBUG("ugrid",
        "TwoDMeshTopology::buildGridFieldsTopology() - Building face node connectivity Cell array from the DAP version" << endl);
    GF::CellArray *faceNodeConnectivityCells = getFaceNodeConnectivityCells();

    // Attach the Mesh to the grid at rank 2
    // This 2 stands for rank 2, or faces.
    BESDEBUG("ugrid", "TwoDMeshTopology::buildGridFieldsTopology() - Attaching Cell array to GF::Grid" << endl);
    gridTopology->setKCells(faceNodeConnectivityCells, face);

    // The Grid is complete. Now we make a GridField from the Grid
    BESDEBUG("ugrid",
        "TwoDMeshTopology::buildGridFieldsTopology() - Construct new GF::GridField from GF::Grid" << endl);
    d_inputGridField = new GF::GridField(gridTopology);
    // TODO Question for Bill: Can we delete the GF::Grid (tdmt->gridTopology) here?

    // We read and add the coordinate data (using GridField->addAttribute()) to the GridField at
    // grid dimension/rank/dimension 0 (a.k.a. node)
    vector<libdap::Array *>::iterator ncit;
    for (ncit = nodeCoordinateArrays->begin(); ncit != nodeCoordinateArrays->end(); ++ncit) {
        libdap::Array *nca = *ncit;
        BESDEBUG("ugrid",
            "TwoDMeshTopology::buildGridFieldsTopology() - Adding node coordinate "<< nca->name() << " to GF::GridField at rank 0" << endl);
        GF::Array *gfa = extractGridFieldArray(nca, sharedIntArrays, sharedFloatArrays);
        gfArrays.push_back(gfa);
        d_inputGridField->AddAttribute(node, gfa);
    }

    // We read and add the coordinate data (using GridField->addAttribute() to the GridField at
    // grid dimension/rank/dimension 0 (a.k.a. node)
    for (ncit = faceCoordinateArrays->begin(); ncit != faceCoordinateArrays->end(); ++ncit) {
        libdap::Array *fca = *ncit;
        BESDEBUG("ugrid",
            "TwoDMeshTopology::buildGridFieldsTopology() - Adding face coordinate "<< fca->name() << " to GF::GridField at rank " << face << endl);
        GF::Array *gfa = extractGridFieldArray(fca, sharedIntArrays, sharedFloatArrays);
        gfArrays.push_back(gfa);
        d_inputGridField->AddAttribute(face, gfa);
    }
}

int TwoDMeshTopology::getResultGridSize(locationType dim)
{
    return resultGridField->Size(dim);
}

/**
 * Takes a Face node connectivity DAP array, either Nx3 or 3xN,
 * and converts it to a collection GF::Cells organized as
 * 0,N,2N; 1,1+N,1+2N;
 *
 * This is the inverse operation to getGridFieldCellArrayAsDapArray()
 *
 * FIXME Make this use less memory. Certainly consider reading the values directly from
 * the DAP array (after it's read() method has been called)
 */
GF::Node *TwoDMeshTopology::getFncArrayAsGFCells(libdap::Array *fncVar)
{
    BESDEBUG("ugrid", "TwoDMeshTopology::getFncArrayAsGFCells() - BEGIN" << endl);

    int nodesPerFace = fncVar->dimension_size(fncNodesDim, true);
    int faceCount = fncVar->dimension_size(fncFacesDim, true);

    GF::Node *cells = 0;        // return the result in this var

    if (fncVar->dim_begin() == fncNodesDim) {
        // This dataset/file stores the face-node connectivity array as a
        // 3xN, but gridfields needs that information in an Nx3; twiddle
        BESDEBUG("ugrid", "Reorganizing the data from teh DAP FNC Array for GF." << endl);

        cells = new GF::Node[faceCount * nodesPerFace];
        GF::Node *temp_nodes = 0;
        // optimize; extractArray uses an extra copy in this case
        if (fncVar->type() == dods_int32_c || fncVar->type() == dods_uint32_c) {
            temp_nodes = new GF::Node[faceCount * nodesPerFace];
            fncVar->value(temp_nodes);
        }
        else {
            // cover the odd case when the FNC Array is not a int/uint.
            temp_nodes = ugrid::extractArray<GF::Node>(fncVar);
        }

        for (int fIndex = 0; fIndex < faceCount; fIndex++) {
            for (int nIndex = 0; nIndex < nodesPerFace; nIndex++) {
                cells[nodesPerFace * fIndex + nIndex] = *(temp_nodes + (fIndex + (faceCount * nIndex)));
            }
        }

        delete[] temp_nodes;
    }
    else {
        // This dataset/file stores the face-node connectivity array as an Nx3 which is what
        // gridfields expects. We can use the libdap::BaseType::value() method to slurp
        // up the stuff.

        if (fncVar->type() == dods_int32_c || fncVar->type() == dods_uint32_c) {
            cells = new GF::Node[faceCount * nodesPerFace];
            fncVar->value(cells);
        }
        else {
            // cover the odd case when the FNC Array is not a int/uint.
            cells = ugrid::extractArray<GF::Node>(fncVar);
        }
    }

    BESDEBUG("ugrid", "TwoDMeshTopology::getFncArrayAsGFCells() - DONE" << endl);
    return cells;
}

/**
 * Returns the value of the "start_index" attribute for the passed Array. If the start_index
 * is missing the value 0 is returned.
 */
int TwoDMeshTopology::getStartIndex(libdap::Array *array)
{
    AttrTable &at = array->get_attr_table();
    AttrTable::Attr_iter start_index_iter = at.simple_find(UGRID_START_INDEX);
    if (start_index_iter != at.attr_end()) {
        BESDEBUG("ugrid", "TwoDMeshTopology::getStartIndex() - Found the "<< UGRID_START_INDEX <<" attribute." << endl);
        AttrTable::entry *start_index_entry = *start_index_iter;
        if (start_index_entry->attr->size() == 1) {
            string val = (*start_index_entry->attr)[0];
            BESDEBUG("TwoDMeshTopology::getStartIndex", "value: " << val << endl);
            stringstream buffer(val);
            // what happens if string cannot be converted to an integer?
            int start_index;
            buffer >> start_index;
            return start_index;
        }
        else {
            throw Error(malformed_expr,
                "Index origin attribute exists, but either no value supplied, or more than one value supplied.");
        }
    }

    return 0;
}

/**
 * Converts a Face node connectivity DAP array (either 3xN or Nx3)
 * into a GF::CellArray
 */
GF::CellArray *TwoDMeshTopology::getFaceNodeConnectivityCells()
{
    BESDEBUG("ugrid",
        "TwoDMeshTopology::getFaceNodeConnectivityCells() - Building face node connectivity Cell array from the Array '" << faceNodeConnectivityArray->name() << "'" << endl);

    int nodesPerFace = faceNodeConnectivityArray->dimension_size(fncNodesDim);
    int total_size = nodesPerFace * faceCount;

    BESDEBUG("ugrid",
        "TwoDMeshTopology::getFaceNodeConnectivityCells() - Converting FNCArray to GF::Node array." << endl);
    fncCellArray = getFncArrayAsGFCells(faceNodeConnectivityArray);

    // adjust for the start_index (cardinal or ordinal array access)
    int startIndex = getStartIndex(faceNodeConnectivityArray);
    if (startIndex != 0) {
        BESDEBUG("ugrid",
            "TwoDMeshTopology::getFaceNodeConnectivityCells() - Applying startIndex to GF::Node array." << endl);
        for (int j = 0; j < total_size; j++) {
            fncCellArray[j] -= startIndex;
        }
    }
    // Create the cell array
    GF::CellArray *rankTwoCells = new GF::CellArray(fncCellArray, faceCount, nodesPerFace);

    BESDEBUG("ugrid", "TwoDMeshTopology::getFaceNodeConnectivityCells() - DONE" << endl);
    return rankTwoCells;

}

void TwoDMeshTopology::applyRestrictOperator(locationType loc, string filterExpression)
{

    BESDEBUG("ugrid", "TwoDMeshTopology::applyRestrictOperator() - BEGIN" << endl);

    // I think this function could be done with just the following single line:
    // resultGridField = GF::RefRestrictOp::Restrict(filterExpression,loc,d_inputGridField);

    // Build the restriction operator
    BESDEBUG("ugrid",
        "TwoDMeshTopology::applyRestrictOperator() - Constructing new GF::RestrictOp using user "<< "supplied 'dimension' value and filter expression combined with the GF:GridField " << endl);
    GF::RestrictOp op = GF::RestrictOp(filterExpression, loc, d_inputGridField);

    // Apply the operator and get the result;
    BESDEBUG("ugrid", "TwoDMeshTopology::applyRestrictOperator() - Applying GridField operator." << endl);
    GF::GridField *resultGF = op.getResult();

    resultGridField = resultGF;
    BESDEBUG("ugrid",
        "TwoDMeshTopology::applyRestrictOperator() - GridField operator applied and result obtained." << endl);

    BESDEBUG("ugrid", "TwoDMeshTopology::applyRestrictOperator() - END" << endl);
}

void TwoDMeshTopology::convertResultGridFieldStructureToDapObjects(vector<BaseType *> *results)
{
    BESDEBUG("ugrid", "TwoDMeshTopology::convertResultGridFieldStructureToDapObjects() - BEGIN" << endl);

    BESDEBUG("ugrid", "TwoDMeshTopology::convertResultGridFieldStructureToDapObjects() - Normalizing Grid." << endl);
    resultGridField->GetGrid()->normalize();

    BESDEBUG("ugrid",
        "TwoDMeshTopology::convertResultGridFieldStructureToDapObjects() - resultGridField->MaxRank(): "<< resultGridField->MaxRank() << endl);

    if (resultGridField->MaxRank() < 0) {
        throw BESError("Oops! The ugrid constraint expression resulted in an empty response.", BES_SYNTAX_USER_ERROR,
            __FILE__, __LINE__);
    }

    // Add the node coordinate arrays to the results.
    BESDEBUG("ugrid",
        "TwoDMeshTopology::convertResultGridFieldStructureToDapObjects() - Converting the node coordinate arrays to DAP arrays." << endl);
    vector<libdap::Array *>::iterator it;
    for (it = nodeCoordinateArrays->begin(); it != nodeCoordinateArrays->end(); ++it) {
        libdap::Array *sourceCoordinateArray = *it;
        libdap::Array *resultCoordinateArray = getGFAttributeAsDapArray(sourceCoordinateArray, node, resultGridField);
        results->push_back(resultCoordinateArray);
    }

#if 1
    // Add the face coordinate arrays to the results.
    BESDEBUG("ugrid",
        "TwoDMeshTopology::convertResultGridFieldStructureToDapObjects() - Converting the face coordinate arrays to DAP arrays." << endl);
    for (it = faceCoordinateArrays->begin(); it != faceCoordinateArrays->end(); ++it) {
        libdap::Array *sourceCoordinateArray = *it;
        libdap::Array *resultCoordinateArray = getGFAttributeAsDapArray(sourceCoordinateArray, face, resultGridField);
        results->push_back(resultCoordinateArray);
    }
#endif

    // Add the new face node connectivity array - make sure it has the same attributes as the original.
    BESDEBUG("ugrid",
        "TwoDMeshTopology::convertResultGridFieldStructureToDapObjects() - Adding the new face node connectivity array to the response." << endl);
    libdap::Array *resultFaceNodeConnectivityDapArray = getGridFieldCellArrayAsDapArray(resultGridField,
        faceNodeConnectivityArray);
    results->push_back(resultFaceNodeConnectivityDapArray);

    results->push_back(getMeshVariable()->ptr_duplicate());

    BESDEBUG("ugrid", "TwoDMeshTopology::convertResultGridFieldStructureToDapObjects() - END" << endl);
}

/**
 * Takes a GF::GridField, extracts it's rank 2 GF::CellArray. The GF::CellArray content (Nx3) is
 * extracted and if needed, re-packed into a DAP Array to match the source dataset (3xN or Nx3 depending).
 * This is the inverse operation to getFncArrayAsGFCells()
 */
libdap::Array *TwoDMeshTopology::getGridFieldCellArrayAsDapArray(GF::GridField *resultGridField,
    libdap::Array *sourceFcnArray)
{
    BESDEBUG("ugrid", "TwoDMeshTopology::getGridFieldCellArrayAsDapArray() - BEGIN" << endl);

    // Get the rank 2 k-cells from the GridField object.
    GF::CellArray* gfCellArray = (GF::CellArray*) (resultGridField->GetGrid()->getKCells(2));

    // This is a vector of size N holding vectors of size 3
    vector<vector<int> > nodes2 = gfCellArray->makeArrayInts();

    libdap::Array *resultFncDapArray = new libdap::Array(sourceFcnArray->name(), new Int32(sourceFcnArray->name()));

    // Is the sourceFcnArray a Nx3 (follows the ugrid 0.9 spec) or 3xN - both
    // commonly appear. Make the resultFncDapArray match the source's organization
    // modulo that 'N' is a different value now given that the ugrid has been
    // subset. jhrg 4/17/15
    bool follows_ugrid_09_spec = true;
    libdap::Array::Dim_iter di = sourceFcnArray->dim_begin();
    if (di->size == 3) {
        follows_ugrid_09_spec = false;

        resultFncDapArray->append_dim(3, di->name);
        ++di;
        resultFncDapArray->append_dim(nodes2.size(), di->name);
    }
    else {
        resultFncDapArray->append_dim(nodes2.size(), di->name);
        ++di;
        resultFncDapArray->append_dim(3, di->name);
    }

    // Copy the attributes of the template array to our new array.
    resultFncDapArray->set_attr_table(sourceFcnArray->get_attr_table());
    resultFncDapArray->reserve_value_capacity(3 * nodes2.size());

    // adjust for the start_index (cardinal or ordinal array access)
    int startIndex = getStartIndex(sourceFcnArray);

    // Now transfer the index information returned by Gridfields to the newly
    // made DAP array. Since GF returns those indexes as an Nx3 using zero-based
    // indexing, we may have to transform the values.
    if (!follows_ugrid_09_spec) {
        // build a new set of values and set them as the value of the result fnc array.
        vector<dods_int32> node_data(3 * nodes2.size());
        vector<dods_int32>::iterator ndi = node_data.begin();

        if (startIndex == 0) {
            for (unsigned int i = 0; i < 3; ++i) {
                for (unsigned int n = 0; n < nodes2.size(); ++n) {
                    *ndi++ = nodes2[n][i];
                }
            }
        }
        else {
            for (unsigned int i = 0; i < 3; ++i) {
                for (unsigned int n = 0; n < nodes2.size(); ++n) {
                    *ndi++ = nodes2[n][i] + startIndex;
                }
            }
        }

        resultFncDapArray->set_value(node_data, node_data.size());
    }
    else {
        // build a new set of values and set them as the value of the result fnc array.
        vector<dods_int32> node_data(nodes2.size() * 3);
        vector<dods_int32>::iterator ndi = node_data.begin();

        if (startIndex == 0) {
            for (unsigned int n = 0; n < nodes2.size(); ++n) {
                for (unsigned int i = 0; i < 3; ++i) {
                    *ndi++ = nodes2[n][i];
                }
            }
        }
        else {
            for (unsigned int n = 0; n < nodes2.size(); ++n) {
                for (unsigned int i = 0; i < 3; ++i) {
                    *ndi++ = nodes2[n][i] + startIndex;
                }
            }
        }

        resultFncDapArray->set_value(node_data, node_data.size());
    }

    BESDEBUG("ugrid", "TwoDMeshTopology::getGridFieldCellArrayAsDapArray() - DONE" << endl);

    return resultFncDapArray;
}

/**
 * Helper function used by TwoDMeshTopology::getGFAttributeAsDapArray.
 * Copy the size of dimension(s) from sourceArray to dapArray and return
 * the meshVarName of the 'data dimension' (the first dimension that is not size
 * one).
 *
 * @param sourceArray
 * @param dapArray
 * @return The meshVarName of the data dimension
 */
static string copySizeOneDimensions(libdap::Array *sourceArray, libdap::Array *dapArray)
{
    for (libdap::Array::Dim_iter srcArrIt = sourceArray->dim_begin(); srcArrIt != sourceArray->dim_end(); ++srcArrIt) {
        // Get the original dimension size
        int dimSize = sourceArray->dimension_size(srcArrIt, true);
        string dimName = sourceArray->dimension_name(srcArrIt);

        // Preserve single dimensions
        if (dimSize == 1)
            dapArray->append_dim(dimSize, dimName);
        else
            return dimName;
    }

    return "";
}

#if 0
// the original version jhrg 4/15/15

static libdap::Array::Dim_iter copySizeOneDimensions(libdap::Array *sourceArray, libdap::Array *dapArray)
{
    libdap::Array::Dim_iter dataDimension;
    for (libdap::Array::Dim_iter srcArrIt = sourceArray->dim_begin(); srcArrIt != sourceArray->dim_end(); ++srcArrIt) {

        // Get the original dimension size
        int dimSize = sourceArray->dimension_size(srcArrIt, true);
        string dimName = sourceArray->dimension_name(srcArrIt);

        // Preserve single dimensions
        if (dimSize == 1) {
            BESDEBUG("ugrid", "TwoDMeshTopology::copySizeOneDimensions() - Adding size one dimension '"<< dimName << "' from source array into result." << endl);
            dapArray->append_dim(dimSize, dimName);
        }
        else {
            BESDEBUG("ugrid", "TwoDMeshTopology::copySizeOneDimensions() - Located data dimension '"<< dimName << "' in source array." << endl);
            dataDimension = srcArrIt;
        }
    }

    BESDEBUG("ugrid", "TwoDMeshTopology::copySizeOneDimensions() - Returning dimension iterator pointing to '"<< sourceArray->dimension_name(dataDimension) << "'." << endl);
    return dataDimension;
}
#endif

/**
 * Retrieves a single dimensional rank 0 GF attribute array from a GF::GridField and places the data into
 * DAP array of the appropriate type.
 */
libdap::Array *TwoDMeshTopology::getGFAttributeAsDapArray(libdap::Array *templateArray, locationType rank,
    GF::GridField *resultGridField)
{
    BESDEBUG("ugrid", "TwoDMeshTopology::getGFAttributeAsDapArray() - BEGIN" << endl);

    // The result variable is assumed to be bound to the GridField at 'rank'
    // Try to get the Attribute from 'rank' with the same name as the source array
    BESDEBUG("ugrid",
        "TwoDMeshTopology::getGFAttributeAsDapArray() - Retrieving rank "<< rank << " GF::GridField Attribute '" << templateArray->name() << "'" << endl);
    GF::Array* gfa = 0;
    gfa = resultGridField->GetAttribute(rank, templateArray->name());

    libdap::Array *dapArray;
    BaseType *templateVar = templateArray->var();
    string dimName;

    switch (templateVar->type()) {
    case dods_byte_c:
    case dods_uint16_c:
    case dods_int16_c:
    case dods_uint32_c:
    case dods_int32_c: {
        // Get the data
        vector<dods_int32> GF_ints = gfa->makeArray();

        // Make a DAP array to put the data into.
        dapArray = new libdap::Array(templateArray->name(), new libdap::Int32(templateVar->name()) /*&tmpltVar*/);

        // copy the dimensions whose size is "1" from the source array to the result.
        dimName = copySizeOneDimensions(templateArray, dapArray);

        // Add the result dimension
        BESDEBUG("ugrid", "TwoDMeshTopology::getGFAttributeAsDapArray() - Adding dimension " << dimName << endl);

        dapArray->append_dim(GF_ints.size(), dimName);

        // Add the data
        dapArray->set_value(GF_ints, GF_ints.size());
        break;
    }
    case dods_float32_c:
    case dods_float64_c: {
        // Get the data
        vector<dods_float64> GF_floats = gfa->makeArrayf();

        // Make a DAP array to put the data into.
        dapArray = new libdap::Array(templateArray->name(), new libdap::Float64(templateVar->name()) /*&tmpltVar*/);

        // copy the dimensions whose size is "1" from the source array to the result.
        dimName = copySizeOneDimensions(templateArray, dapArray);

        // Add the result dimension
        dapArray->append_dim(GF_floats.size(), dimName);

        // Add the data
        dapArray->set_value(GF_floats, GF_floats.size());
        break;
    }
    default:
        throw InternalErr(__FILE__, __LINE__, "Unknown DAP type encountered when converting to gridfields array");
    }

    // Copy the source objects attributes.
    BESDEBUG("ugrid",
        "TwoDMeshTopology::getGFAttributeAsDapArray() - Copying libdap::Attribute's from template array " << templateArray->name() << endl);
    dapArray->set_attr_table(templateArray->get_attr_table());

    BESDEBUG("ugrid", "TwoDMeshTopology::getGFAttributeAsDapArray() - DONE" << endl);

    return dapArray;
}

/**
 * Retrieves a single dimensional GF attribute array from a GF::GridField and places the data into
 * DAP array of the appropriate type.
 */
void TwoDMeshTopology::getResultGFAttributeValues(string attrName, libdap::Type dapType, locationType rank,
    void *target)
{
    BESDEBUG("ugrid", "TwoDMeshTopology::getResultGFAttributeValues() - BEGIN" << endl);

    // The result variable is assumed to be bound to the GridField at 'rank'
    // Try to get the Attribute from 'rank' with the same name as the source array
    BESDEBUG("ugrid",
        "TwoDMeshTopology::getResultGFAttributeValues() - Retrieving GF::GridField Attribute '" << attrName << "'" << endl);

    GF::Array *gfa = 0;
    if (resultGridField->IsAttribute(rank, attrName)) {
        gfa = resultGridField->GetAttribute(rank, attrName);
    }
    else {
        string msg = "Oddly, the requested attribute '" + attrName + "' associated with rank "
            + libdap::long_to_string(rank) + " does not appear in the resultGridField object! \n"
            + "resultGridField->MaxRank(): " + libdap::long_to_string(resultGridField->MaxRank());

        throw InternalErr(__FILE__, __LINE__, "ERROR  - Unable to locate requested GridField attribute. " + msg);
    }

    switch (dapType) {
    case dods_byte_c:
    case dods_uint16_c:
    case dods_int16_c:
    case dods_uint32_c:
    case dods_int32_c: {
        // Get the data
        BESDEBUG("ugrid",
            "TwoDMeshTopology::getResultGFAttributeValues() - Retrieving GF::Array as libdap::Array of libdap::Int32." << endl);
        vector<dods_int32> GF_ints = gfa->makeArray();
#if 0
        // Removing this reduced the runtime by ~1s for one test case
        // (from real 0m6.285s; user 0m5.816s to real 0m5.136s; user 0m4.808s)
        // jhrg 4/15/15
        stringstream s;
        for(unsigned int i=0; i<GF_ints.size(); i++) {
            s << "GF_ints[" << i << "]: " << GF_ints[i] << endl;
        }
        BESDEBUG("ugrid", "TwoDMeshTopology::getResultGFAttributeValues() - Retrieved GF_ints: "<< endl << s.str());
#endif

        BESDEBUG("ugrid",
            "TwoDMeshTopology::getResultGFAttributeValues() - Copying GF result to target memory" << endl);
        memcpy(target, &GF_ints[0], GF_ints.size() * sizeof(dods_int32));
        break;
    }
    case dods_float32_c:
    case dods_float64_c: {
        // Get the data
        BESDEBUG("ugrid",
            "TwoDMeshTopology::getResultGFAttributeValues() - Retrieving GF::Array as libdap::Array of libdap::Float64." << endl);
        vector<dods_float64> GF_floats = gfa->makeArrayf();
#if 0
        stringstream s;
        for(unsigned int i=0; i<GF_floats.size(); i++) {
            s << "GF_ints[" << i << "]: " << GF_floats[i] << endl;
        }
        BESDEBUG("ugrid", "TwoDMeshTopology::getResultGFAttributeValues() - Retrieved GF_floats: "<< endl << s.str());
#endif
        BESDEBUG("ugrid",
            "TwoDMeshTopology::getResultGFAttributeValues() - Copying GF result to target memory" << endl);
        memcpy(target, &GF_floats[0], GF_floats.size() * sizeof(dods_float64));
        break;

    }
    default:
        throw InternalErr(__FILE__, __LINE__,
            "Unknown DAP type encountered when converting to gridfields result values");
    }

    BESDEBUG("ugrid", "TwoDMeshTopology::getResultGFAttributeValues() - END" << endl);
}

static string getIndexVariableName(locationType location)
{
    switch (location) {

    case node:
        return "node_index";

    case face:
        return "face_index";

    case edge:
    default:
        break;
    }

    string msg = "ugr5(): Unknown/Unsupported location value '" + libdap::long_to_string(location) + "'";
    BESDEBUG("ugrid", "TwoDMeshTopology::getIndexVariableName() - " << msg << endl);
    throw Error(malformed_expr, msg);
}

int TwoDMeshTopology::getInputGridSize(locationType location)
{
    switch (location) {
    case node:
        return nodeCount;

    case face:
        return faceCount;

    case edge:
    default:
        break;
    }

    string msg = "ugr5(): Unknown/Unsupported location value '" + libdap::long_to_string(location) + "'";
    BESDEBUG("ugrid", "TwoDMeshTopology::getInputGridSize() - " << msg << endl);
    throw Error(malformed_expr, msg);
}

/**
 * @brief Adds an index variable at the gridfields rank as indicated by the passed locationType.
 */
void TwoDMeshTopology::addIndexVariable(locationType location)
{
    int size = getInputGridSize(location);
    string name = getIndexVariableName(location);

    BESDEBUG("ugrid",
        "TwoDMeshTopology::addIndexVariable() - Adding index variable '" << name << "'  size: " << libdap::long_to_string(size) << " at rank " << libdap::long_to_string(location) << endl);

    GF::Array *indexArray = newGFIndexArray(name, size, sharedIntArrays);
    d_inputGridField->AddAttribute(location, indexArray);
    gfArrays.push_back(indexArray);
}

/**
 * @brief
 */
void TwoDMeshTopology::getResultIndex(locationType location, void *target)
{
    string name = getIndexVariableName(location);
    getResultGFAttributeValues(name, dods_int32_c, location, target);
}

} // namespace ugrid
