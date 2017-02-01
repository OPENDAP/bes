///////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2010 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////

#ifndef __AGG_UTIL__GRID_AGGREGATION_BASE_H__
#define __AGG_UTIL__GRID_AGGREGATION_BASE_H__

#include "AggMemberDataset.h" // agg_util
#include "DDSLoader.h" // agg_util
#include <Grid.h> // libdap
#include <memory> // std

namespace libdap {
class Array;
class Grid;
class D4Group;
}

namespace agg_util {

class GridAggregationBase: public libdap::Grid {
public:
    GridAggregationBase(const libdap::Grid& proto, const AMDList& memberDatasets, const DDSLoader& loaderProto);

    /** Construct an EMPTY Grid structure
     * with given name.
     * NOTE: The result is an incomplete type!
     *
     * The shape prototype will be added by
     * hand later for this case, where the
     * caller needs more control than initializing
     * the Grid from another Grid.
     *
     * @see setShapeFrom()
     *
     * @param name  name to give the grid
     * @param memberDatasets  the granules defining the aggregation
     * @param loaderProto  the laoder to use
     */
    GridAggregationBase(const string& name, const AMDList& memberDatasets, const DDSLoader& loaderProto);

    GridAggregationBase(const GridAggregationBase& proto);

    virtual ~GridAggregationBase();

    GridAggregationBase& operator=(const GridAggregationBase& rhs);

    BaseType *transform_to_dap4(libdap::D4Group *root, libdap::Constructor *container);

    /**
     * Use the data array and maps from protoSubGrid as the initial
     * point for the shape of the Grid.  It may still not be complete
     * until the callers adds maps, etc.
     * @param protoSubGrid  describes the data array and map templates
     *          to use for the granules (modulo any agg subclass changes).
     * @param addMaps  if true, add's all the maps in protoSubGrid into this.
     *                 if false, does not.
     */
    void setShapeFrom(const libdap::Grid& protoSubGrid, bool addMaps);

    /**
     * Accessor for the dataset description list that describes
     * this aggregation.
     * @return a reference to the AggMemberDataset list.
     */
    virtual const AMDList& getDatasetList() const;

    /**
     * Read in only those datasets that are in the constrained output
     * making sure to apply the internal dimension constraints to the
     * member datasets properly before reading them!
     * Stream the data into the output buffer correctly.
     *
     * NOTE: Subclasses should implement the protected hooks if possible rather
     * than overriding this function!
     * @return success.
     */
    virtual bool read();

    virtual bool serialize(libdap::ConstraintEvaluator &eval, libdap::DDS &dds, libdap::Marshaller &m, bool ce_eval);


protected:
    // subclass interface

    /**
     * Called from read()!  Invokes the user hooks eventually.
     * Can be overridden, but default calls should suffice for now.
     */
    virtual void readAndAggregateConstrainedMapsHook();

    /** Reveals the raw ptr, but only to subclasses.  Don't delete it,
     * but can be changed etc.
     */
    Grid* getSubGridTemplate();

    /** Get the contained aggregation dimension info */
    virtual const Dimension& getAggregationDimension() const = 0;

    void printConstraints(const libdap::Array& fromArray);

    /** Transfer constraints properly from this object's maps
     * and read in the proto subgrid entirely (respecting constraints) */
    void readProtoSubGrid();

    // Support calls for the read()....

    /**
     * Copy the template's read in subgrid maps into this.
     * Skip any map found in the subgrid named aggDim.name
     * since we handle the aggregation dimension map specially.
     * @param aggDim  a map with aggDim.name is NOT copied.
     */
    void copyProtoMapsIntoThisGrid(const Dimension& aggDim);

    /** To be specialized in subclass to copy constraints on this
     * object properly into the given pSubGrid map list
     * and data array for read.
     *
     * Should handle the aggregation dimension properly, hence
     * the specialization.
     *
     * Called from readProtoSubGrid
     * @param pSubGrid Grid to modify with the constraints
     */
    virtual void transferConstraintsToSubGridHook(Grid* pSubGrid);

private:
    // helpers

    /** Duplicate just the local data rep */
    void duplicate(const GridAggregationBase& rhs);

    /** Delete any heap memory */
    void cleanup();

    static libdap::Grid* cloneSubGridProto(const libdap::Grid& proto);

private:
    // data rep

    // Use this to laod the member datasets as needed
    DDSLoader _loader;

    // A template for the unaggregated (sub) Grids.
    // It will be used to validate other datasets as they are loaded.
    std::auto_ptr<Grid> _pSubGridProto;

    // Maintain a copy here... we may want to move this down...
    AMDList _memberDatasets;

};
// class GridAggregationBase

}// namespace agg_util

#endif /* __AGG_UTIL__GRID_AGGREGATION_BASE_H__ */
