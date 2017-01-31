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
#include "GridJoinExistingAggregation.h"

#include "AggregationUtil.h" // agg_util
#include "ArrayJoinExistingAggregation.h" // agg_util
#include <Array.h> // libdap
#include "DDSLoader.h"
#include "NCMLDebug.h"

using libdap::Array;
using libdap::Grid;

// unused jhrg 4/16/14 static const bool PRINT_CONSTRAINTS = true;
static const string DEBUG_CHANNEL("ncml:2");

namespace agg_util {

GridJoinExistingAggregation::GridJoinExistingAggregation(const libdap::Grid& proto, const AMDList& memberDatasets,
    const DDSLoader& loaderProto, const Dimension& joinDim)
// init WITHOUT a template.  We need to do maps specially.
:
    GridAggregationBase(proto.name(), memberDatasets, loaderProto), _joinDim(joinDim)
{
    createRep(proto, memberDatasets);
}

GridJoinExistingAggregation::GridJoinExistingAggregation(const GridJoinExistingAggregation& proto) :
    GridAggregationBase(proto), _joinDim(proto._joinDim)
{
    duplicate(proto);
}

/* virtual */
GridJoinExistingAggregation::~GridJoinExistingAggregation()
{
    cleanup();
}

/* virtual */
GridJoinExistingAggregation*
GridJoinExistingAggregation::ptr_duplicate()
{
    return new GridJoinExistingAggregation(*this);
}

GridJoinExistingAggregation&
GridJoinExistingAggregation::operator=(const GridJoinExistingAggregation& rhs)
{
    if (this != &rhs) {
        cleanup();
        GridAggregationBase::operator=(rhs);
        duplicate(rhs);
    }
    return *this;
}

auto_ptr<ArrayJoinExistingAggregation> GridJoinExistingAggregation::makeAggregatedOuterMapVector() const
{
    BESDEBUG_FUNC(DEBUG_CHANNEL, "Making an aggregated map " << "as a coordinate variable..." << endl);
    Grid* pGridGranuleTemplate = const_cast<GridJoinExistingAggregation*>(this)->getSubGridTemplate();
    NCML_ASSERT_MSG(pGridGranuleTemplate, "Expected grid granule template but got null.");

    const Array* pMapTemplate = AggregationUtil::findMapByName(*pGridGranuleTemplate, _joinDim.name);
    NCML_ASSERT_MSG(pMapTemplate, "Expected to find a dim map for the joinExisting agg but failed!");

    // Make an array getter that pulls out the map array we are interested in.
    // Use the basic array getter to read and get from top level DDS.
    // N.B. Must use this->name() ie the gridname since that's what it will search!
    auto_ptr<agg_util::ArrayGetterInterface> mapArrayGetter(new agg_util::TopLevelGridMapArrayGetter(name()));

    auto_ptr<ArrayJoinExistingAggregation> pNewMap = auto_ptr<ArrayJoinExistingAggregation>(
        new ArrayJoinExistingAggregation(*pMapTemplate, getDatasetList(), mapArrayGetter, _joinDim));

    return pNewMap;
}

///////////////////////////////////////////////////////////
// Subclass Impl (Protected)

/* virtual */
void GridJoinExistingAggregation::transferConstraintsToSubGridHook(Grid* pSubGrid)
{
    VALID_PTR(pSubGrid);
    transferConstraintsToSubGridMaps(pSubGrid);
    transferConstraintsToSubGridArray(pSubGrid);
}

/* virtual */
const Dimension&
GridJoinExistingAggregation::getAggregationDimension() const
{
    return _joinDim;
}

///////////////////////////////////////////////////////////
// Private Impl

void GridJoinExistingAggregation::duplicate(const GridJoinExistingAggregation& rhs)
{
    _joinDim = rhs._joinDim;
}

void GridJoinExistingAggregation::cleanup() throw ()
{
}

void GridJoinExistingAggregation::createRep(const libdap::Grid& constProtoSubGrid, const AMDList& memberDatasets)
{
    // Semantically const calls below
    Grid& protoSubGrid = const_cast<Grid&>(constProtoSubGrid);

    // Set up the shape, don't add maps
    setShapeFrom(protoSubGrid, false);

    // Add the maps by hand, leaving out the first (outer dim) one
    // since we need to make add that specially.
    Grid::Map_iter firstIt = protoSubGrid.map_begin();
    Grid::Map_iter endIt = protoSubGrid.map_end();
    for (Grid::Map_iter it = firstIt; it != endIt; ++it) {
        // Skip the first one, assuming it's the join dim
        if (it == firstIt) {
            NCML_ASSERT_MSG((*it)->name() == _joinDim.name, "Expected the first map to be the outer dimension "
                "named " + _joinDim.name + " but it was not!  Logic problem.");
            continue;
        }

        // Add the others.
        Array* pMap = dynamic_cast<Array*>(*it);
        VALID_PTR(pMap);
        add_map(pMap, true); // add as a copy
    }

    BESDEBUG_FUNC(DEBUG_CHANNEL, "Replacing the Grid's data Array with an ArrayAggregateOnOuterDimension..." << endl);

    // This is the prototype we need.  It will have been set in the ctor.
    Array* pArr = static_cast<Array*>(array_var());
    NCML_ASSERT_MSG(pArr, "Expected to find a contained data Array but we did not!");

    // Create the Grid version of the read getter and make a new AAOOD from our state.
    std::auto_ptr<ArrayGetterInterface> arrayGetter(new TopLevelGridDataArrayGetter());

    // Create the subclass that does the work and replace our data array with it.
    // Note this ctor will prepend the new dimension itself, so we do not.
    std::auto_ptr<ArrayJoinExistingAggregation> aggDataArray(new ArrayJoinExistingAggregation(*pArr, // prototype, already should be setup properly _without_ the new dim
        memberDatasets, arrayGetter, _joinDim));

    // Make sure null since sink function
    // called on the auto_ptr
    NCML_ASSERT(!(arrayGetter.get()));

    // Replace our data Array with this one.  Will delete old one and may throw.
    set_array(aggDataArray.get());

    // Release here on successful set since set_array uses raw ptr only.
    // In case we threw then auto_ptr cleans up itself.
    aggDataArray.release();
}

void GridJoinExistingAggregation::transferConstraintsToSubGridMaps(Grid* pSubGrid)
{
    BESDEBUG(DEBUG_CHANNEL, "Transferring constraints to the subgrid maps..." << endl);
    Map_iter subGridMapIt = pSubGrid->map_begin();
    for (Map_iter it = map_begin(); it != map_end(); ++it) {
        // Skip the aggregated outer dimension since handled specially
        if (it != map_begin()) {
            Array* subGridMap = static_cast<Array*>(*subGridMapIt);
            Array* superGridMap = static_cast<Array*>(*it);
            agg_util::AggregationUtil::transferArrayConstraints(subGridMap, *superGridMap, false, // skipFirstDim = false since inner dim map sizes consistent
                false, // same rank, don't skip this one either
                true, // printDebug
                DEBUG_CHANNEL);
        }
        ++subGridMapIt; // keep them in sync
    }
}

void GridJoinExistingAggregation::transferConstraintsToSubGridArray(Grid* /* pSubGrid */)
{
    // Data array gets the constraints set on it directly since we replaced
    // the map with an aggregated array variable.  Thus it handles its own
    // constraints.
    // Leaving the stub here in case this changes in the future.
}

} // namespace agg_util
