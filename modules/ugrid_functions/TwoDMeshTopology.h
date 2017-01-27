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

#ifndef _TwoDMeshTopology_h
#define _TwoDMeshTopology_h 1

#include <gridfields/type.h>
#include <gridfields/gridfield.h>
#include <gridfields/grid.h>
#include <gridfields/cellarray.h>

using namespace std;
using namespace libdap;

namespace libdap {
class BaseType;
class Array;
class MeshDataVariable;
class Grid;
}

namespace ugrid {

/**
 * Identifies the location/rank/dimension that various grid components are associated with.
 */

class TwoDMeshTopology {

private:
    /**
     * REQUIRED
     *
     * The DAP dataset variable that defined the mesh_topology.
     */
    BaseType *d_meshVar;

    /**
     * REQUIRED
     *
     * The attribute dimension indicates the highest dimensionality of the geometric
     * elements; for a 2-dimensional mesh this should be 2.
     */
    string dimension;

    /**
     * REQUIRED
     *
     * The attribute node_coordinates contains a list of the whitespace separated names of
     * the auxiliary coordinate variables representing the node locations (latitude,
     * longitude, and optional elevation or other coordinates). These auxiliary coordinate
     * variables will have length nNodes.
     */
    vector<libdap::Array *> *nodeCoordinateArrays;
    string nodeDimensionName;
    int nodeCount;

    /**
     * REQUIRED
     *
     *  - - - - - -
     * 2D triangular mesh
     * The attribute face_node_connectivity points to an index variable identifying for every
     * face (here consistently triangle) the indices of its three corner nodes.
     * The corner nodes should be specified in anticlockwise (also referred to as counterclockwise)
     * direction as viewed from above (consistent with the CF-convention for bounds of p-sided cells.
     *
     * The connectivity array will thus be a matrix of size nFaces x 3. For the indexing one may use
     * either 0- or 1-based indexing; the convention used should be specified using a start_index
     * attribute to the index variable (i.e. Mesh2_face_nodes in the example below). Consistent with
     * the CF-conventions compression option, the connectivity indices are 0-based by default.
     * See https://github.com/ugrid-conventions/ugrid-conventions for more details.
     *
     * - - - - - -
     * 2D flexible mesh
     * The attribute face_node_connectivity points to an index variable identifying for every face the
     * indices of its corner nodes. The corner nodes should be specified in anticlockwise direction as
     * viewed from above (consistent with the CF-convention for bounds of p-sided cells. The connectivity
     * array will be a matrix of size nFaces x MaxNumNodesPerFace; if a face has less corner nodes than
     * MaxNumNodesPerFace then the last node indices shall be equal to _FillValue (which should obviously
     * be larger than the number of nodes in the mesh). For the indexing one may use either 0- or 1-based
     * indexing; the convention used should be specified using a start_index attribute to the index
     * variable (i.e. Mesh2_face_nodes in the example below). Consistent with the CF-conventions
     * compression option, the connectivity indices are 0-based by default. See this section on 0-/1-based
     * indexing for more details.
     *
     */
    libdap::Array *faceNodeConnectivityArray;
    libdap::Array::Dim_iter fncNodesDim, fncFacesDim;
    string faceDimensionName;
    int faceCount;

    vector<MeshDataVariable *> *rangeDataArrays;

    /**
     * OPTIONAL
     *
     * The "Optionally required" attribute edge_node_connectivity is required only if you want to store data on
     * the edges (i.e. if you mind the numbering order of the edges).
     * The edge_node_connectivity attribute contains the name of the array that maps edges to nodes. Although the
     * face to node mapping implicitly also defines the location of the edges, it does not specify the global
     * numbering of the edges. Again the indexing convention of edge_node_connectivity should be specified
     * using the start_index attribute to the index variable (i.e. Mesh2_edge_nodes in the example below)
     * and 0-based indexing is the default.
     *
     */
    //string edgeNodeConnectivityArrayName;
    /**
     * OPTIONAL
     *
     * The face_edge_connectivity attribute points to an index variable identifying for every face
     * (here consistently triangle) the indices of its three edges. The edges should be specified
     * in anticlockwise direction as viewed from above. This connectivity array will thus be a matrix
     * of size nFaces x 3. Again the indexing convention of face_edge_connectivity should be specified
     * using the start_index attribute to the index variable (i.e. Mesh2_face_edges in the example
     * below) and 0-based indexing is the default.
     */
    //string faceEdgeConnectivityArrayName;
    //Array *faceEdgeConnectivityArray;
    /**
     * OPTIONAL
     *
     * The face_face_connectivity attribute points to an index variable identifying pairs of faces
     * (here consistently triangle) that share an edge, i.e. are neighbors. TODO: CHECK DEFINITION
     * This connectivity array will thus be a matrix of size nFacePairs x 2. Again the indexing
     * convention of face_face_connectivity should be specified using the start_index attribute to
     * the index variable (i.e. Mesh2_face_links in the example below) and 0-based indexing is the default.
     *
     *
     */
    //string faceFaceConnectivityArrayName;
    //Array *faceFaceConnectivityArray;
    /**
     * OPTIONAL
     *
     * The face_coordinates attribute points to the auxiliary coordinate variables
     * associated with the characteristic location of the faces. These auxiliary coordinate
     * variables will have length nFaces, and may have in turn a bounds
     * attribute that specifies the bounding coordinates of the face or edge (thereby duplicating the
     * data in the node_coordinates variables).
     *
     */
    // Not used. jhrg 4/15/15 vector<string> *faceCoordinateNames;
    vector<libdap::Array *> *faceCoordinateArrays;

    /**
     * OPTIONAL
     *
     * The edge_coordinates attribute points to the auxiliary coordinate variables
     * associated with the characteristic location of the  edges. These auxiliary coordinate
     * variables will have length nEdges, and may have in turn a bounds
     * attribute that specifies the bounding coordinates of the edge (thereby duplicating the
     * data in the node_coordinates variables).
     *
     */
    //vector<string> *edgeCoordinateNames;
    //vector<Array *> *edgeCoordinateArrays;
    GF::Grid *gridTopology;
    GF::GridField *d_inputGridField;
    GF::GridField *resultGridField;

    vector<int *> *sharedIntArrays;
    vector<float *> *sharedFloatArrays;

    vector<GF::Array *> gfArrays;

    GF::Node *fncCellArray;

    bool _initialized;

    void ingestFaceNodeConnectivityArray(libdap::BaseType *meshTopology, libdap::DDS *dds);
    void ingestNodeCoordinateArrays(libdap::BaseType *meshTopology, libdap::DDS *dds);
    void ingestFaceCoordinateArrays(libdap::BaseType *meshTopology, libdap::DDS *dds);

    GF::Node *getFncArrayAsGFCells(libdap::Array *fncVar);
    int getStartIndex(libdap::Array *array);
    GF::CellArray *getFaceNodeConnectivityCells();

    libdap::Array *getGFAttributeAsDapArray(libdap::Array *sourceArray, locationType rank,
        GF::GridField *resultGridField);
    libdap::Array *getGridFieldCellArrayAsDapArray(GF::GridField *resultGridField, libdap::Array *sourceFcnArray);
    // libdap::Array *getNewFncDapArray(libdap::Array *templateArray, int N);

    void setNodeCoordinateDimension(MeshDataVariable *mdv);
    void setFaceCoordinateDimension(MeshDataVariable *mdv);

public:
    TwoDMeshTopology();
    ~TwoDMeshTopology();

    void init(string meshVarName, libdap::DDS *dds);

    string meshVarName() const
    {
        return getMeshVariable()->name();
    }

    libdap::BaseType *getMeshVariable() const
    {
        return d_meshVar;
    }

    void buildBasicGfTopology();
    void applyRestrictOperator(locationType loc, string filterExpression);

    int getInputGridSize(locationType location);
    int getResultGridSize(locationType location);

    void convertResultGridFieldStructureToDapObjects(vector<libdap::BaseType *> *results);

    void setLocationCoordinateDimension(MeshDataVariable *mdv);

    void addIndexVariable(locationType location);
    void getResultIndex(locationType location, void *target);

    void getResultGFAttributeValues(string attrName, libdap::Type type, locationType rank, void *target);
};

} // namespace ugrid

#endif // _TwoDMeshTopology_h
