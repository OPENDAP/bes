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

// NOTE: This file is built only when the gridfields library is linked with
// the netcdf_handler (i.e., the handler's build is configured using the
// --with-gridfields=... option to the 'configure' script).

#include "config.h"

#include <limits.h>

#include <cstdlib>      // used by strtod()
#include <cerrno>
#include <cmath>
#include <iostream>
#include <sstream>
//#include <cxxabi.h>

#include <curl/curl.h>

#include <libdap/BaseType.h>
#include <libdap/Int32.h>
#include <libdap/Str.h>
#include <libdap/Array.h>
#include <libdap/Structure.h>
#include <libdap/Error.h>
#include <libdap/util.h>
#include <libdap/escaping.h>

#include "BESDebug.h"
#include "BESError.h"
#include "BESStopWatch.h"
#include <libdap/util.h>

#include "ugrid_utils.h"
#include "MeshDataVariable.h"
#include "TwoDMeshTopology.h"
#include "NDimensionalArray.h"
#include <gridfields/GFError.h>

#include "ugrid_restrict.h"

#ifdef NDEBUG
#undef BESDEBUG
#define BESDEBUG( x, y )
#endif

using namespace std;
using namespace libdap;

namespace ugrid {

/**
 * Function syntax
 */

/**
 * Function Arguments
 */
struct UgridRestrictArgs {
    /**
     * The dimension holds the UGrid "dimensionality" to which to apply the filter expression.
     *
     * 0 - node
     * 1 - edge
     * 2 - face
     * 3 - volume
     *
     */
    locationType dimension;

    /**
     * the rangeVars are libdap::Array variables from the dataset that have been identified
     * (via the constraint expression arguments to the ugrid function) as the ones to provide
     * in the subset data response, in addition the domain variables required for the ugrid to
     * remain meaningful.
     */
    vector<libdap::Array *> rangeVars;

    /**
     * Holds a domain filter expression that will be passed to the ugrid library.
     */
    string filterExpression;
};

/**
 * Evaluates the rangeVar and determines which meshTopology it is associated with. If one hasn't been found
 * a new mesh topology is created. Once the associated mesh topology had been found (or created), the rangeVar
 * is added to the vector of rangeVars held by the mesh topology for later evaluation.
 */
static void addRangeVar(DDS *dds, libdap::Array *rangeVar, map<string, vector<MeshDataVariable *> *> *rangeVariables)
{
    MeshDataVariable *mdv = new MeshDataVariable();
    mdv->init(rangeVar);
    string meshVarName = mdv->getMeshName();

    BaseType *meshVar = dds->var(meshVarName);

    if (meshVar == 0) {
        string msg = "The range variable '" + mdv->getName() + "' references the mesh variable '" + meshVarName
            + "' which cannot be located in this dataset.";
        throw Error(malformed_expr, msg);
    }

    // Get the rangeVariable vector for this mesh name from the map.
    vector<MeshDataVariable *> *requestedRangeVarsForMesh;
    map<string, vector<MeshDataVariable *> *>::iterator mit = rangeVariables->find(meshVarName);
    if (mit == rangeVariables->end()) {
        // Not there? Make a new one.
        BESDEBUG("ugrid",
            "addRangeVar() - MeshTopology object for '" << meshVarName <<"' does not exist. Getting a new one... " << endl);

        requestedRangeVarsForMesh = new vector<MeshDataVariable *>();
        (*rangeVariables)[meshVarName] = requestedRangeVarsForMesh;
    }
    else {
        // Sweet! Found it....
        BESDEBUG("ugrid",
            "addRangeVar() - MeshTopology object for '" << meshVarName <<"' exists. Retrieving... " << endl);
        requestedRangeVarsForMesh = mit->second;
    }

    requestedRangeVarsForMesh->push_back(mdv);
}


string usage(string fnc){

    string usage = fnc+"(rangeVariable:string, [rangeVariable:string, ... ] condition:string)";

    return usage;
}

/**
 * Process the functions arguments and return the structure containing their values.
 */
static UgridRestrictArgs processUgrArgs(string func_name, locationType dimension, int argc, BaseType *argv[])
{

    BESDEBUG("ugrid", "processUgrArgs() - BEGIN" << endl);

    UgridRestrictArgs args;
    args.dimension = dimension;
    BESDEBUG("ugrid", "args.dimension: " << libdap::long_to_string(args.dimension) << endl);

    args.rangeVars = vector<libdap::Array *>();

    // Check number of arguments;
    if (argc < 2)
        throw Error(malformed_expr,
            "Wrong number of arguments to ugrid restrict function: " + usage(func_name) + " was passed " + long_to_string(argc)
                + " argument(s)");

    BaseType * bt;

#if 0
    // ---------------------------------------------
    // Process the first arg, which is the rank of the Restriction Clause
    bt = argv[0];
    if (bt->type() != dods_int32_c)
        throw Error(malformed_expr,
            "Wrong type for first argument, expected DAP Int32. " + ugrSyntax + "  was passed a/an " + bt->type_name());

    args.dimension = (locationType) dynamic_cast<Int32&>(*argv[0]).value();
    BESDEBUG("ugrid", "args.dimension: " << libdap::long_to_string(args.dimension) << endl);
#endif

    // ---------------------------------------------
    // Process the last argument, the relational/filter expression used to restrict the ugrid content.
    bt = argv[argc - 1];
    if (bt->type() != dods_str_c)
        throw Error(malformed_expr,
            "Wrong type for third argument, expected DAP String. " + usage(func_name) + "  was passed a/an "
                + bt->type_name());

    args.filterExpression = dynamic_cast<Str&>(*bt).value();
    BESDEBUG("ugrid", "args.filterExpression: '" << args.filterExpression << "' (AS RECEIVED)" << endl);

    args.filterExpression = www2id(args.filterExpression);

    BESDEBUG("ugrid", "args.filterExpression: '" << args.filterExpression << "' (URL DECODED)" << endl);

    // --------------------------------------------------
    // Process the range variables selected by the user.
    // We know that argc>=3, because we checked so the
    // following loop will try to find at least one rangeVar,
    // and it won't try to process the first or last members
    // of argv.
    for (int i = 0; i < (argc - 1); i++) {
        bt = argv[i];
        if (bt->type() != dods_array_c)
            throw Error(malformed_expr,
                "Wrong type for second argument, expected DAP Array. " + usage(func_name) + "  was passed a/an "
                    + bt->type_name());

        libdap::Array *newRangeVar = dynamic_cast<libdap::Array*>(bt);
        if (newRangeVar == 0) {
            throw Error(malformed_expr,
                "Wrong type for second argument. " + usage(func_name) + "  was passed a/an " + bt->type_name());
        }
        args.rangeVars.push_back(newRangeVar);
    }

    BESDEBUG("ugrid", "processUgrArgs() - END" << endl);

    return args;
}

static string arrayState(libdap::Array *dapArray, string indent)
{

    libdap::Array::Dim_iter thisDim;
    stringstream arrayState;
    arrayState << endl;
    arrayState << indent << "dapArray: '" << dapArray->name() << "'   ";
    arrayState << "type: '" << dapArray->type_name() << "'  ";

    arrayState << "read(): " << dapArray->read() << "  ";
    arrayState << "read_p(): " << dapArray->read_p() << "  ";
    arrayState << endl;

    for (thisDim = dapArray->dim_begin(); thisDim != dapArray->dim_end(); thisDim++) {

        arrayState << indent << "  dim:  '" << dapArray->dimension_name(thisDim) << "' ";
        arrayState << indent << "    start:  " << dapArray->dimension_start(thisDim) << " ";
        arrayState << indent << "    stride: " << dapArray->dimension_stride(thisDim) << " ";
        arrayState << indent << "    stop:   " << dapArray->dimension_stop(thisDim) << " ";
        arrayState << endl;
    }
    return arrayState.str();
}

static void copyUsingSubsetIndex(libdap::Array *sourceArray, vector<unsigned int> *subsetIndex, void *result)
{
    BESDEBUG("ugrid", "ugrid::copyUsingIndex() - BEGIN" << endl);

    switch (sourceArray->var()->type()) {
    case dods_byte_c:
        sourceArray->value(subsetIndex, (dods_byte *) result);
        break;
    case dods_uint16_c:
        sourceArray->value(subsetIndex, (dods_uint16 *) result);
        break;
    case dods_int16_c:
        sourceArray->value(subsetIndex, (dods_int16 *) result);
        break;
    case dods_uint32_c:
        sourceArray->value(subsetIndex, (dods_uint32 *) result);
        break;
    case dods_int32_c:
        sourceArray->value(subsetIndex, (dods_int32 *) result);
        break;

    case dods_float32_c:
        sourceArray->value(subsetIndex, (dods_float32 *) result);
        break;
    case dods_float64_c:
        sourceArray->value(subsetIndex, (dods_float64 *) result);
        break;

    default:
        throw InternalErr(__FILE__, __LINE__, "ugrid::hgr5::copyUsingSubsetIndex() - Unknown DAP type encountered.");
    }

    BESDEBUG("ugrid", "ugrid::copyUsingIndex() - END" << endl);
}

// This is only used for the ugrid2 BESDEBUG lines.
static string vectorToString(vector<unsigned int> *index)
{
    BESDEBUG("ugrid", "indexToString() - BEGIN"<< endl);
    BESDEBUG("ugrid", "indexToString() - index.size(): " << libdap::long_to_string(index->size()) << endl);
    stringstream s;
    s << "[";

    for (unsigned int i = 0; i < index->size(); ++i) {
        s << ((i > 0) ? ", " : " ");
        s << (*index)[i];
    }
    s << "]";
    BESDEBUG("ugrid", "indexToString() - END"<< endl);

    return s.str();
}

/**
 *
 */
static void rDAWorker(MeshDataVariable *mdv, libdap::Array::Dim_iter thisDim, vector<unsigned int> *slab_subset_index,
    NDimensionalArray *results)
{
    libdap::Array *dapArray = mdv->getDapArray();

    // For real data, this output is huge. jhrg 4/15/15
    BESDEBUG("ugrid2",
        "rDAWorker() - slab_subset_index" << vectorToString(slab_subset_index) << " size: " << slab_subset_index->size() << endl);

    // The locationCoordinateDimension is the dimension of the array that is associated with the ugrid "rank" - e.g. it is the
    // dimension that ties the variable to the 'nodes' (rank 0) or 'edges' (rank 1) or 'faces' (rank 2) of the ugrid.
    libdap::Array::Dim_iter locationCoordinateDim = mdv->getLocationCoordinateDimension();

    BESDEBUG("ugrid", "rdaWorker() - thisDim: '" << dapArray->dimension_name(thisDim) << "'" << endl);
    BESDEBUG("ugrid",
        "rdaWorker() - locationCoordinateDim: '" << dapArray->dimension_name(locationCoordinateDim) << "'" << endl);
    // locationCoordinateDim is, e.g., 'nodes'. jhrg 10/25/13
    if (thisDim != locationCoordinateDim) {
        unsigned int start = dapArray->dimension_start(thisDim, true);
        unsigned int stride = dapArray->dimension_stride(thisDim, true);
        unsigned int stop = dapArray->dimension_stop(thisDim, true);

        BESDEBUG("ugrid", "rdaWorker() - array state: " << arrayState(dapArray, "    "));

        for (unsigned int dimIndex = start; dimIndex <= stop; dimIndex += stride) {
            dapArray->add_constraint(thisDim, dimIndex, 1, dimIndex);
            rDAWorker(mdv, thisDim + 1, slab_subset_index, results);
        }

        // Reset the constraint for this dimension.
        dapArray->add_constraint(thisDim, start, stride, stop);
    }
    else {
        BESDEBUG("ugrid", "rdaWorker() - Found locationCoordinateDim" << endl);

        if ((thisDim + 1) != dapArray->dim_end()) {
            string msg =
                "rDAWorker() - The location coordinate dimension is not the last dimension in the array. Hyperslab subsetting of this dimension is not supported.";
            BESDEBUG("ugrid", msg << endl);
            throw Error(malformed_expr, msg);
        }

        BESDEBUG("ugrid", "rdaWorker() - Arrived At Last Dimension" << endl);

        dapArray->set_read_p(false);

        BESDEBUG("ugrid", "rdaWorker() - dap array: " << arrayState(dapArray, "    "));

        vector<unsigned int> lastDimHyperSlabLocation;
        NDimensionalArray::retrieveLastDimHyperSlabLocationFromConstrainedArrray(dapArray, &lastDimHyperSlabLocation);

        BESDEBUG("ugrid",
            "rdaWorker() - lastDimHyperSlabLocation: " << NDimensionalArray::vectorToIndices(&lastDimHyperSlabLocation) << endl);

        // unused. 4/7/14 jhrg. unsigned int elementCount;

        void *slab;
        //results->getLastDimensionHyperSlab(&lastDimHyperSlabLocation, &slab, &elementCount);
        results->getNextLastDimensionHyperSlab(&slab);

        dapArray->read();

        copyUsingSubsetIndex(dapArray, slab_subset_index, slab);
    }
}

/**
 * Now, for each variable array on the mesh we have to hyper-slab the array such that all the dimensions, with the
 * exception of the rank/location dimension (the one that is either the number of nodes, edges, or faces depending
 * on which of these locations is the data is associated with), have size one.
 *  Each of these slabs is then added to the GridField, subset and the result must be packed back
 *  into the result array so that things work out.
 **/
static libdap::Array *restrictRangeVariableByOneDHyperSlab(MeshDataVariable *mdv,
    vector<unsigned int> *slab_subset_index)
{

    long restrictedSlabSize = slab_subset_index->size();

    BESDEBUG("ugrid2",
        "restrictRangeVariableByOneDHyperSlab() - slab_subset_index"<< vectorToString(slab_subset_index) << " size: " << libdap::long_to_string(restrictedSlabSize) << endl);

    libdap::Array *sourceDapArray = mdv->getDapArray();

    BESDEBUG("ugrid",
        "restrictDapArrayByOneDHyperSlab() - locationCoordinateDim: '" << sourceDapArray->dimension_name(mdv->getLocationCoordinateDimension()) << "'" << endl);

    // We want the manipulate the Array's Dimensions so that only a single dimensioned slab of the location coordinate dimension
    // is read at a time. We need to cache the original constrained dimensions so that we can build the correct collection of
    // location coordinate dimension slabs.

    // Now we need to compute the shape of the final subset result array from the source range variable array and the slab subset.
    // What's the shape of the source array with any constraint applied?
    vector<unsigned int> resultArrayShape(sourceDapArray->dimensions(true));
    NDimensionalArray::computeConstrainedShape(sourceDapArray, &resultArrayShape);

    stringstream msg;
    for (unsigned int i = 0; i < resultArrayShape.size(); i++) {
        msg << "[" << resultArrayShape[i] << "]";
    }
    BESDEBUG("ugrid", "restrictDapArrayByOneDHyperSlab() - Constrained source array shape" << msg.str() << endl);
    msg.str("");

    // Now, we know that the result array has a last dimension slab size determined by the slab_subset_index (whichwas made by the ugrid sub-setting),
    // so we make the result array shape reflect that

    resultArrayShape[sourceDapArray->dimensions(true) - 1] = restrictedSlabSize; // jhrg 10/25/13 resultArrayShape.last() = ...
    libdap::Type dapType = sourceDapArray->var()->type();

    BESDEBUG("ugrid",
        "restrictDapArrayByOneDHyperSlab() - UGrid restricted HyperSlab has  "<< restrictedSlabSize << " elements." << endl);
    BESDEBUG("ugrid",
        "restrictDapArrayByOneDHyperSlab() - Array is of type '"<< libdap::type_name(dapType) << "'" << endl);

    for (unsigned int i = 0; i < resultArrayShape.size(); i++) {
        msg << "[" << resultArrayShape[i] << "]";
    }
    BESDEBUG("ugrid", "restrictDapArrayByOneDHyperSlab() - resultArrayShape" << msg.str() << endl);
    msg.str(std::string());

    // Now we make a new NDimensionalArray instance that we will use to hold the results.
    NDimensionalArray *result = new NDimensionalArray(&resultArrayShape, dapType);

    // And we pass that along with other stuff into the recursive rDAWorker that's going to go get all the stuff
    rDAWorker(mdv, sourceDapArray->dim_begin(), slab_subset_index, result);

    // And now that the recursion we grab have the NDimensionalArray cough up the rteuslt as a libdap::Array
    libdap::Array *resultDapArray = result->getArray(sourceDapArray);

    // Delete the NDimensionalArray
    delete result;

    // Return the result as libdap:Array
    return resultDapArray;
}

/**
 Subset an irregular mesh (aka unstructured grid).

 @param func_name Name of the function being called (used for error and info messages)
 @param location The location in the grid (node, edge, or face) to which the filter expression will be applied.
 @param argc Count of the function's arguments
 @param argv Array of pointers to the functions arguments
 @param dds Reference to the DDS object for the complete dataset.
 This holds pointers to all of the variables and attributes in the
 dataset.
 @param btpp Return the function result in an instance of BaseType
 referenced by this pointer to a pointer. We could have used a
 BaseType reference, instead of pointer to a pointer, but we didn't.
 This is a value-result parameter.

 @return void

 @exception Error Thrown If the Array is not a one dimensional
 array. */
void ugrid_restrict(string func_name, locationType location, int argc, BaseType *argv[], DDS &dds, BaseType **btpp)
{
    try { // This top level try block is used to catch gridfields library errors.

        BESStopWatch sw;
        if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start("ugrid::ugrid_restrict()");

        BESDEBUG("ugrid", "ugrid_restrict() - BEGIN" << endl);

        string info = string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
            + "<function name=\"ugrid_restrict\" version=\"0.1\">\n" + "Server function for Unstructured grid operations.\n"
            + "usage: " + usage(func_name) + "\n" + "</function>";

        if (argc == 0) {
            Str *response = new Str("info");
            response->set_value(info);
            *btpp = response;
            return;
        }

        // Process and QC the arguments
        UgridRestrictArgs args = processUgrArgs(func_name, location, argc, argv);

        // Each range variable is associated with a "mesh" i.e. a mesh topology variable. Since there may be more than one mesh in a
        // dataset, and the user may request more than one range variable for each mesh we need to sift through the list of requested
        // range variables and organize them by mesh topology variable name.
        map<string, vector<MeshDataVariable *> *> *meshToRangeVarsMap = new map<string, vector<MeshDataVariable *> *>();

        // For every Range variable in the arguments list, locate it and ingest it.
        vector<libdap::Array *>::iterator it;
        for (it = args.rangeVars.begin(); it != args.rangeVars.end(); ++it) {
            addRangeVar(&dds, *it, meshToRangeVarsMap);
        }
        BESDEBUG("ugrid", "ugrid_restrict() - The user requested "<< args.rangeVars.size() << " range data variables." << endl);
        BESDEBUG("ugrid",
            "ugrid_restrict() - The user's request referenced "<< meshToRangeVarsMap->size() << " mesh topology variables." << endl);

        // ----------------------------------
        // OK, so up to this point we have not read any data from the data set, but we have QC'd the inputs and verified that
        // it looks like the request is consistent with the semantics of the dataset.
        // Now it's time to read some data and pack it into the GridFields library...

        // TODO This returns a single structure but it would make better sense to the
        // world if it could return a vector of objects and have them appear at the
        // top level of the DDS.
        // FIXME fix the names of the variables in the mesh_topology attributes
        // If the server side function can be made to return a DDS or a collection of BaseType's then the
        // names won't change and the original mesh_topology variable and it's metadata will be valid
        Structure *dapResult = new Structure("ugr_result_unwrap");

        // Now we need to grab an top level metadata (attriubutes) and copy them into the dapResult Structure
        // Add any global attributes to the netcdf file
#if 1
        AttrTable &globals = dds.get_attr_table();
        BESDEBUG("ugrid", "ugrid_restrict() - Copying Global Attributes" << endl << globals << endl);

        AttrTable *newDatasetAttrTable = new AttrTable(globals);
        dapResult->set_attr_table(*newDatasetAttrTable);
        BESDEBUG("ugrid", "ugrid_restrict() - Result Structure attrs: " << endl << dapResult->get_attr_table() << endl);


#endif

        // Since we only want each ugrid structure to appear in the results one time  (cause otherwise we might be trying to add
        // the same variables with the same names to the result multiple times.) we grind on this by iterating over the
        // names of the mesh topology names.
        map<string, vector<MeshDataVariable *> *>::iterator mit;
        for (mit = meshToRangeVarsMap->begin(); mit != meshToRangeVarsMap->end(); ++mit) {

            string meshVariableName = mit->first;
            vector<MeshDataVariable *> *requestedRangeVarsForMesh = mit->second;

            // Building the restricted TwoDMeshTopology without adding any range variables and then converting the result
            // Grid field to Dap Objects should return all of the Ugrid structural stuff - mesh variable, node coordinate variables,
            // face and edge coordinate variables if present.
            BESDEBUG("ugrid",
                "ugrid_restrict() - Adding restricted mesh_topology structure for mesh '" << meshVariableName << "' to DAP response." << endl);

            TwoDMeshTopology *tdmt = new TwoDMeshTopology();
            tdmt->init(meshVariableName, &dds);

            tdmt->buildBasicGfTopology();
            tdmt->addIndexVariable(node);
            tdmt->addIndexVariable(face);
            tdmt->applyRestrictOperator(args.dimension, args.filterExpression);

            // 3: because there are nodes (rank = 0), edges (rank = 1), and faces (rank = 2). jhrg 10/25/13
            vector<vector<unsigned int> *> location_subset_indices(3);

            long nodeResultSize = tdmt->getResultGridSize(node);
            BESDEBUG("ugrid", "ugrid_restrict() - there are "<< nodeResultSize << " nodes in the subset." << endl);
            vector<unsigned int> node_subset_index(nodeResultSize);
            if (nodeResultSize > 0) {
                tdmt->getResultIndex(node, node_subset_index.data());
            }
            location_subset_indices[node] = &node_subset_index;

            BESDEBUG("ugrid2", "ugrid_restrict() - node_subset_index"<< vectorToString(&node_subset_index) << endl);

            long faceResultSize = tdmt->getResultGridSize(face);
            BESDEBUG("ugrid", "ugrid_restrict() - there are "<< faceResultSize << " faces in the subset." << endl);
            vector<unsigned int> face_subset_index(faceResultSize);
            if (faceResultSize > 0) {
                tdmt->getResultIndex(face, face_subset_index.data());
            }
            location_subset_indices[face] = &face_subset_index;
            BESDEBUG("ugrid2", "ugrid_restrict() - face_subset_index: "<< vectorToString(&face_subset_index) << endl);

            // This gets all the stuff that's attached to the grid - which at this point does not include the range variables but does include the
            // index variable. good enough for now but need to drop the index....
            vector<BaseType *> dapResults;
            tdmt->convertResultGridFieldStructureToDapObjects(&dapResults);

            BESDEBUG("ugrid",
                "ugrid_restrict() - Restriction of mesh_topology '"<< tdmt->getMeshVariable()->name() << "' structure completed." << endl);

            // now that we have the mesh topology variable we are going to look at each of the requested
            // range variables (aka MeshDataVariable instances) and we're going to subset that using the
            // gridfields library and add its subset version to the results.
            vector<MeshDataVariable *>::iterator rvit;
            for (rvit = requestedRangeVarsForMesh->begin(); rvit != requestedRangeVarsForMesh->end(); rvit++) {
                MeshDataVariable *mdv = *rvit;

                BESDEBUG("ugrid",
                    "ugrid_restrict() - Processing MeshDataVariable  '"<< mdv->getName() << "' associated with rank/location: "<< mdv->getGridLocation() << endl);

                tdmt->setLocationCoordinateDimension(mdv);

                /**
                 * Here is where we will do the range variable sub-setting including decomposing the requested variable
                 * into 1-dimensional hyper-slabs that can be fed into the gridfields library
                 */
                libdap::Array *restrictedRangeVarArray = restrictRangeVariableByOneDHyperSlab(mdv,
                    location_subset_indices[mdv->getGridLocation()]);

                BESDEBUG("ugrid",
                    "ugrid_restrict() - Adding resulting dapArray  '"<< restrictedRangeVarArray->name() << "' to dapResults." << endl);

                dapResults.push_back(restrictedRangeVarArray);
            }

            delete tdmt;

            BESDEBUG("ugrid", "ugrid_restrict() - Adding GF::GridField results to DAP structure " << dapResult->name() << endl);

            for (vector<BaseType *>::iterator i = dapResults.begin(); i != dapResults.end(); ++i) {
                BESDEBUG("ugrid",
                    "ugrid_restrict() - Adding variable "<< (*i)->name() << " to DAP structure " << dapResult->name() << endl);
                dapResult->add_var_nocopy(*i);
            }
        }

        *btpp = dapResult;

        // TODO replace with auto_ptr. jhrg 4/17/15

        BESDEBUG("ugrid", "ugrid_restrict() - Releasing maps and vectors..." << endl);
        for (mit = meshToRangeVarsMap->begin(); mit != meshToRangeVarsMap->end(); ++mit) {
            vector<MeshDataVariable *> *requestedRangeVarsForMesh = mit->second;
            vector<MeshDataVariable *>::iterator rvit;
            for (rvit = requestedRangeVarsForMesh->begin(); rvit != requestedRangeVarsForMesh->end(); rvit++) {
                MeshDataVariable *mdv = *rvit;
                delete mdv;
            }
            delete requestedRangeVarsForMesh;
        }
        delete meshToRangeVarsMap;

        BESDEBUG("ugrid", "ugrid_restrict() - END" << endl);
    }
    catch (GFError &gfe) {
        throw BESError(gfe.get_message(), gfe.get_error_type(), gfe.get_file(), gfe.get_line());
    }

    return;
}





/**
 @brief Subset an irregular mesh (aka unstructured grid) by evaluating a filter expression
 against the node values of the ugrid.

 @param argc Count of the function's arguments
 @param argv Array of pointers to the functions arguments
 @param dds Reference to the DDS object for the complete dataset.
 This holds pointers to all of the variables and attributes in the
 dataset.
 @param btpp Return the function result in an instance of BaseType
 referenced by this pointer to a pointer. We could have used a
 BaseType reference, instead of pointer to a pointer, but we didn't.
 This is a value-result parameter.

 @return void

 @exception Error Thrown If the Array is not a one dimensional
 array. */
void ugnr(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) {
    ugrid_restrict("ugnr",node,argc,argv,dds,btpp);
}



/**
 @brief Subset an irregular mesh (aka unstructured grid) by evaluating a filter expression
 against the edge values of the ugrid.

 @param argc Count of the function's arguments
 @param argv Array of pointers to the functions arguments
 @param dds Reference to the DDS object for the complete dataset.
 This holds pointers to all of the variables and attributes in the
 dataset.
 @param btpp Return the function result in an instance of BaseType
 referenced by this pointer to a pointer. We could have used a
 BaseType reference, instead of pointer to a pointer, but we didn't.
 This is a value-result parameter.

 @return void

 @exception Error Thrown If the Array is not a one dimensional
 array. */
void uger(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) {
    ugrid_restrict("uger",edge,argc,argv,dds,btpp);
}


/**
 @brief Subset an irregular mesh (aka unstructured grid) by evaluating a filter expression
 against the face values of the ugrid.

 @param argc Count of the function's arguments
 @param argv Array of pointers to the functions arguments
 @param dds Reference to the DDS object for the complete dataset.
 This holds pointers to all of the variables and attributes in the
 dataset.
 @param btpp Return the function result in an instance of BaseType
 referenced by this pointer to a pointer. We could have used a
 BaseType reference, instead of pointer to a pointer, but we didn't.
 This is a value-result parameter.

 @return void

 @exception Error Thrown If the Array is not a one dimensional
 array. */
void ugfr(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) {
    ugrid_restrict("ugfr",face,argc,argv,dds,btpp);
}




} // namespace ugrid_restrict
