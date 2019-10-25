//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
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
#ifndef __AGG_UTIL__GRID_AGGREGATE_ON_OUTER_DIMENSION_H__
#define __AGG_UTIL__GRID_AGGREGATE_ON_OUTER_DIMENSION_H__

#include <memory>
#include <string>
#include <vector>

#include "AggregationUtil.h" // agg_util
#include "AggMemberDataset.h" // agg_util
#include "Dimension.h" // agg_util
#include "DDSLoader.h" // agg_util
#include <Grid.h> // libdap
#include "GridAggregationBase.h" // agg_util

namespace libdap {
class Array;
class Grid;
}

using libdap::Array;
using libdap::Grid;

namespace agg_util {
/**
 * class GridAggregateOnOuterDimension : public GridAggregationBase
 *
 * Grid that performs a joinNew aggregation by taking
 * an ordered list of datatsets which contain Grid's with
 * matching signatures (ie array name, dimensions, maps)
 * and creating a new outer dimension with cardinality the
 * number of datasets in the aggregation.
 *
 * The resulting aggregated grid will be one plus the
 * rank of the array's of the member dataset Grids.
 *
 * We assume we have created the proper shape of this
 * Grid so that the map vectors in the Grid superclass
 * are correct (as loaded from the first dataset!)
 *
 * The data itself will be loaded as needed during serialize
 * based on the response.
 *
 * TODO OPTIMIZE One major issue is we create one of these
 * objects per aggregation variable, even if these
 * vars are in the same dataset.  So when we read,
 * we load the dataset for each variable!  Does the BES
 * cache them or something?  Maybe we can avoid doing this
 * with some smarts?  Not sure...  I get the feeling
 * the other handlers do the same sort of thing,
 * but can be smarter about seeking, etc.
 */
class GridAggregateOnOuterDimension: public GridAggregationBase {
public:
    /** Create the new Grid from the template proto... we'll
     * have the same name, dimension and attributes.
     * @param proto  template to use, will be the aggVar from the FIRST member dataset!
     * @param memberDatasets  descriptors for the dataset members of the aggregation.  ASSUMED
     *                  that the contained DDS will have a Grid var with name proto.name() that
     *                  matches rank and maps, etc!!
     * @param loaderProto loaded template to use (borrow dhi from) to load the datasets
     */
    GridAggregateOnOuterDimension(const Grid& proto, const Dimension& newDim, const AMDList& memberDatasets,
        const DDSLoader& loaderProto);

    GridAggregateOnOuterDimension(const GridAggregateOnOuterDimension& proto);

    virtual ~GridAggregateOnOuterDimension();

    virtual GridAggregateOnOuterDimension* ptr_duplicate();

    GridAggregateOnOuterDimension& operator=(const GridAggregateOnOuterDimension& rhs);

protected:
    // Subclass Impl

    /**
     * For the data array and all maps, transfer the constraints
     * from the super grid (ie this) to all the grids in the
     * given prototype subgrid.
     *
     * Note that this Grid has one more outer dimension than the
     * subgrid, so the first one on this will clearly be skipped.
     *
     * @param pToGrid
     */
    virtual void transferConstraintsToSubGridHook(Grid* pSubGrid);

    virtual const Dimension& getAggregationDimension() const;

private:
    // helpers

    /** Duplicate just the local data rep */
    void duplicate(const GridAggregateOnOuterDimension& rhs);

    /** Delete any heap memory */
    void cleanup() throw ();

    /**
     * Helper for constructor to create replace our data array
     * with an ArrayAggregateOnOuterDimension based on memberDatasets.
     * @param memberDatasets the dataset description for this agg.
     */
    void createRep(const AMDList& memberDatasets);

    // Local helpers called from transferConstraintsToSubGridMapsHook()
    void transferConstraintsToSubGridMaps(Grid* pSubGrid);
    void transferConstraintsToSubGridArray(Grid* pSubGrid);

private:
    // data rep
    // The new outer dimension description
    Dimension _newDim;

};

}

#endif // __AGG_UTIL__GRID_AGGREGATE_ON_OUTER_DIMENSION_H__
