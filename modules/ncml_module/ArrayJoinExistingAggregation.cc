//////////////////////////////////////////////////////////////////////////////
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

#include "config.h"

#include <sstream>

#include <libdap/Marshaller.h>

#include "BESDebug.h"
#include "BESStopWatch.h"

#include "ArrayJoinExistingAggregation.h"

#include "AggregationException.h" // agg_util
#include "AggregationUtil.h" // agg_util
#include "NCMLDebug.h"

#define DEBUG_CHANNEL "ncml:2"
#define prolog string("ArrayJoinExistingAggregation::").append(__func__).append("() - ")

namespace agg_util {

ArrayJoinExistingAggregation::ArrayJoinExistingAggregation(const libdap::Array& granuleTemplate,
    AMDList memberDatasets, std::unique_ptr<ArrayGetterInterface> arrayGetter, const Dimension& joinDim) :
    ArrayAggregationBase(granuleTemplate, std::move(memberDatasets), std::move(arrayGetter)),
    _joinDim(joinDim)
{
    BESDEBUG_FUNC(DEBUG_CHANNEL, "Making the aggregated outer dimension be: " + joinDim.toString() + "\n");

    // We created the array with the given granule prototype, but the resulting
    // outer dimension size must be calculated according to the
    // value in the passed in dimension object.
    libdap::Array::dimension& rOuterDim = *(dim_begin());
    NCML_ASSERT_MSG(rOuterDim.name == joinDim.name, "The outer dimension name of this is not the expected "
        "outer dimension name!  Broken precondition:  This ctor cannot be called "
        "without this being true!");
    rOuterDim.size = joinDim.size;
    // Force it to recompute constraints since we changed size.
    reset_constraint();

    ostringstream oss;
    AggregationUtil::printDimensions(oss, *this);

    BESDEBUG_FUNC(DEBUG_CHANNEL, "Constrained Dims after set are: " + oss.str());
}

ArrayJoinExistingAggregation::ArrayJoinExistingAggregation(const ArrayJoinExistingAggregation& rhs) :
    ArrayAggregationBase(rhs), _joinDim(rhs._joinDim)
{
    duplicate(rhs);
}

/* virtual */
ArrayJoinExistingAggregation::~ArrayJoinExistingAggregation()
{
    cleanup();
}

ArrayJoinExistingAggregation&
ArrayJoinExistingAggregation::operator=(const ArrayJoinExistingAggregation& rhs)
{
    if (this != &rhs) {
        cleanup();
        ArrayAggregationBase::operator=(rhs);
        duplicate(rhs);
    }
    return *this;
}

/* virtual */
ArrayJoinExistingAggregation*
ArrayJoinExistingAggregation::ptr_duplicate()
{
    return new ArrayJoinExistingAggregation(*this);
}

// Set this to 0 to get the old behavior where the entire response
// (for this variable) is built in memory and then sent to the client.
#define PIPELINING 1

/* virtual */
// begin modifying here for the double buffering
// see notes about how this was written marked with '***'
// Following this method is an older version of serialize that
// provides no new functionality but does get run instead of the
// more general implementation in libdap::Array.
bool ArrayJoinExistingAggregation::serialize(libdap::ConstraintEvaluator &eval, libdap::DDS &dds, libdap::Marshaller &m,
    bool ce_eval)
{
    BES_STOPWATCH_START(DEBUG_CHANNEL, prolog + "Timing");

    // *** This serialize() implementation was made by starting with a simple version that
    // *** tested read_p(), calling read() if needed and tsting send_p() and is_in_selection(),
    // *** returning true if the data did not need to be sent. I moved that test here.

    // Only continue if we are supposed to serialize this object at all.
    if (!(send_p() || is_in_selection())) {
        BESDEBUG_FUNC(DEBUG_CHANNEL, "Object not in output, skipping...  name=" << name() << endl);
        return true;
    }

    // *** Add status so that we can do our magic _or_ pass off the call to libdap
    // *** and collect the result either way.
    bool status = false;

    if (!read_p()) {
        // *** copy lines from AggregationBase::read() into here in place
        // *** of the call to read()

        // call subclass impl
        transferOutputConstraintsIntoGranuleTemplateHook();

        // *** Inserted code from readConstrainedGranuleArraysAndAggregateDataHook here

        // outer one is the first in iteration
        const Array::dimension& outerDim = *(dim_begin());
        BESDEBUG("ncml",
            "Aggregating datasets array with outer dimension constraints: " << " start=" << outerDim.start << " stride=" << outerDim.stride << " stop=" << outerDim.stop << endl);

        try {
#if PIPELINING
            // assumes the constraints are already set properly on this
            m.put_vector_start(length());
#else
            reserve_value_capacity();
#endif

            // Start the iteration state for the granule.
            const AMDList& datasets = getDatasetList(); // the list
            NCML_ASSERT(!datasets.empty());
            int currDatasetIndex = 0; // index into datasets
            const AggMemberDataset* pCurrDataset = (datasets[currDatasetIndex]).get();

            int outerDimIndexOfCurrDatasetHead = 0;
            int currDatasetSize = int(pCurrDataset->getCachedDimensionSize(_joinDim.name));
            bool currDatasetWasRead = false;

            // where in this output array we are writing next
            unsigned int nextOutputBufferElementIndex = 0;

            // Traverse the outer dimension constraints,
            // Keeping track of which dataset we need to
            // be inside for the given values of the constraint.
            for (int outerDimIndex = outerDim.start; outerDimIndex <= outerDim.stop && outerDimIndex < outerDim.size;
                outerDimIndex += outerDim.stride) {
                // Figure out where the given outer index maps into in local granule space
                int localGranuleIndex = outerDimIndex - outerDimIndexOfCurrDatasetHead;

                // if this is beyond the dataset end, move state to the next dataset
                // and try again until we're in the proper interval, with proper dataset.
                while (localGranuleIndex >= currDatasetSize) {
                    localGranuleIndex -= currDatasetSize;
                    outerDimIndexOfCurrDatasetHead += currDatasetSize;
                    ++currDatasetIndex;
                    NCML_ASSERT(currDatasetIndex < int(datasets.size()));
                    pCurrDataset = datasets[currDatasetIndex].get();
                    currDatasetSize = pCurrDataset->getCachedDimensionSize(_joinDim.name);
                    currDatasetWasRead = false;

                    BESDEBUG_FUNC(DEBUG_CHANNEL,
                        "The constraint traversal passed a granule boundary " << "on the outer dimension and is stepping forward into " << "granule index=" << currDatasetIndex << endl);
                }

                // If we haven't read in this granule yet (we passed a boundary)
                // then do it now.  Map constraints into the local granule space.
                if (!currDatasetWasRead) {
                    BESDEBUG_FUNC(DEBUG_CHANNEL,
                        " Current granule dataset was traversed but not yet " "read and copied into output.  Mapping constraints " "and calling read()..." << endl);

                    // Set up a constraint object for the actual granule read
                    // so that it only loads the data values in which we are
                    // interested.
                    Array& granuleConstraintTemplate = getGranuleTemplateArray();

                    // The inner dim constraints were set up in the containing read() call.
                    // The outer dim was left open for us to fix now...
                    Array::Dim_iter outerDimIt = granuleConstraintTemplate.dim_begin();

                    // modify the outerdim size to match the dataset we need to
                    // load.  The inners MUST match so we can let those get
                    //checked later...
                    outerDimIt->size = currDatasetSize;
                    outerDimIt->c_size = currDatasetSize; // this will get recalc below?

                    // find the mapped endpoint
                    // Basically, the fullspace endpoint mapped to local offset,
                    // clamped into the local granule size.

                    // TODO: we will rewrite the following when we need to add the big array support.
                    // Now we will replace the std:min() with our own version to avoid the
                    // inconsistent type comparison. KY 2022-12-28              
#if 0
                   int granuleStopIndex = std::min(outerDim.stop - outerDimIndexOfCurrDatasetHead,
                        (unsigned long long)currDatasetSize - 1);
#endif
                    int granuleStopIndex = outerDim.stop - outerDimIndexOfCurrDatasetHead;
                    if (granuleStopIndex > (currDatasetSize - 1))
                        granuleStopIndex = currDatasetSize - 1;

                    // we must clamp the stride to the interval of the
                    // dataset in order to avoid an exception in
                    // add_constraint on stride being larger than dataset.
                    // TODO: we will rewrite the following when we need to add the big array support.
                    // Now we will replace the std:min() with our own version to avoid the
                    // inconsistent type comparison. KY 2022-12-28              
#if 0
                    int clampedStride = std::min(outerDim.stride, (unsigned long long)currDatasetSize);
#endif
                    int clampedStride = outerDim.stride;
                    if (clampedStride > currDatasetSize) 
                        clampedStride = currDatasetSize;

                    // mapped endpoint clamped within this granule
                    granuleConstraintTemplate.add_constraint(outerDimIt, localGranuleIndex, clampedStride,
                        granuleStopIndex);
                    Array* pDatasetArray = AggregationUtil::readDatasetArrayDataForAggregation(
                        getGranuleTemplateArray(), name(), const_cast<AggMemberDataset&>(*pCurrDataset),
                        getArrayGetterInterface(), DEBUG_CHANNEL);
#if PIPELINING
                    m.put_vector_part(pDatasetArray->get_buf(), getGranuleTemplateArray().length(), var()->width(),
                        var()->type());
#else
                    this->set_value_slice_from_row_major_vector(*pDatasetArray, nextOutputBufferElementIndex);
#endif

                    pDatasetArray->clear_local_data();

                    // Jump output buffer index forward by the amount we added.
                    nextOutputBufferElementIndex += getGranuleTemplateArray().length();
                    currDatasetWasRead = true;

                    BESDEBUG_FUNC(DEBUG_CHANNEL,
                        " The granule index " << currDatasetIndex << " was read with constraints and copied into the aggregation output." << endl);
                } // !currDatasetWasRead
            } // for loop over outerDim
        } // end of try
        catch (AggregationException& ex) {
            THROW_NCML_PARSE_ERROR(-1, ex.what());
        }

        // *** end of code inserted from readConstrainedGranuleArraysAndAggregateDataHook

#if PIPELINING
        m.put_vector_end();
        status = true;
#else
        set_read_p(true);
        status = libdap::Array::serialize(eval, dds, m, ce_eval);
#endif
    }
    else {
        status = libdap::Array::serialize(eval, dds, m, ce_eval);
    }

    return status;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Impl Below

void ArrayJoinExistingAggregation::duplicate(const ArrayJoinExistingAggregation& rhs)
{
    _joinDim = rhs._joinDim;
}

void ArrayJoinExistingAggregation::cleanup() noexcept
{
}

/* virtual */
void ArrayJoinExistingAggregation::transferOutputConstraintsIntoGranuleTemplateHook()
{
    // transfer the constraints from this object into the subArray template
    // skipping our first dim which is the join dim we need to do specially every
    // granule in the read hook.
    agg_util::AggregationUtil::transferArrayConstraints(&(getGranuleTemplateArray()), // into this template
        *this, // from this
        true, // skip first dim in the copy since we handle it special
        true, // also skip it in the toArray for the same reason.
        true, // print debug
        DEBUG_CHANNEL); // on this channel
}

void ArrayJoinExistingAggregation::readConstrainedGranuleArraysAndAggregateDataHook()
{
    BES_STOPWATCH_START(DEBUG_CHANNEL, prolog + "Timing");

    // outer one is the first in iteration
    const Array::dimension& outerDim = *(dim_begin());
    BESDEBUG("ncml",
        "Aggregating datasets array with outer dimension constraints: " << " start=" << outerDim.start << " stride=" << outerDim.stride << " stop=" << outerDim.stop << endl);

    try {
        // assumes the constraints are already set properly on this
        reserve_value_capacity();

        // Start the iteration state for the granule.
        const AMDList& datasets = getDatasetList(); // the list
        NCML_ASSERT(!datasets.empty());
        int currDatasetIndex = 0; // index into datasets
        const AggMemberDataset* pCurrDataset = (datasets[currDatasetIndex]).get();

        int outerDimIndexOfCurrDatasetHead = 0;
        int currDatasetSize = int(pCurrDataset->getCachedDimensionSize(_joinDim.name));
        bool currDatasetWasRead = false;

        // where in this output array we are writing next
        unsigned int nextOutputBufferElementIndex = 0;

        // Traverse the outer dimension constraints,
        // Keeping track of which dataset we need to
        // be inside for the given values of the constraint.
        for (int outerDimIndex = outerDim.start; outerDimIndex <= outerDim.stop && outerDimIndex < outerDim.size;
            outerDimIndex += outerDim.stride) {
            // Figure out where the given outer index maps into in local granule space
            int localGranuleIndex = outerDimIndex - outerDimIndexOfCurrDatasetHead;

            // if this is beyond the dataset end, move state to the next dataset
            // and try again until we're in the proper interval, with proper dataset.
            while (localGranuleIndex >= currDatasetSize) {
                localGranuleIndex -= currDatasetSize;
                outerDimIndexOfCurrDatasetHead += currDatasetSize;
                ++currDatasetIndex;
                NCML_ASSERT(currDatasetIndex < int(datasets.size()));
                pCurrDataset = datasets[currDatasetIndex].get();
                currDatasetSize = pCurrDataset->getCachedDimensionSize(_joinDim.name);
                currDatasetWasRead = false;

                BESDEBUG_FUNC(DEBUG_CHANNEL,
                    "The constraint traversal passed a granule boundary " << "on the outer dimension and is stepping forward into " << "granule index=" << currDatasetIndex << endl);
            }

            // If we haven't read in this granule yet (we passed a boundary)
            // then do it now.  Map constraints into the local granule space.
            if (!currDatasetWasRead) {
                BESDEBUG_FUNC(DEBUG_CHANNEL,
                    " Current granule dataset was traversed but not yet " "read and copied into output.  Mapping constraints " "and calling read()..." << endl);

                // Set up a constraint object for the actual granule read
                // so that it only loads the data values in which we are
                // interested.
                Array& granuleConstraintTemplate = getGranuleTemplateArray();

                // The inner dim constraints were set up in the containing read() call.
                // The outer dim was left open for us to fix now...
                Array::Dim_iter outerDimIt = granuleConstraintTemplate.dim_begin();

                // modify the outerdim size to match the dataset we need to
                // load.  The inners MUST match so we can let those get
                //checked later...
                outerDimIt->size = currDatasetSize;
                outerDimIt->c_size = currDatasetSize; // this will get recalc below?

                // find the mapped endpoint
                // Basically, the fullspace endpoint mapped to local offset,
                // clamped into the local granule size.

                // TODO: we will rewrite the following when we need to add the big array support.
                // Now we will replace the std:min() with our own version to avoid the
                // inconsistent type comparison. KY 2022-12-28              
#if 0
                int granuleStopIndex = std::min(outerDim.stop - outerDimIndexOfCurrDatasetHead, (unsigned long long)currDatasetSize - 1);
#endif
                int granuleStopIndex = outerDim.stop - outerDimIndexOfCurrDatasetHead;
                if (granuleStopIndex > (currDatasetSize - 1))
                    granuleStopIndex = currDatasetSize - 1;

                // we must clamp the stride to the interval of the
                // dataset in order to avoid an exception in
                // add_constraint on stride being larger than dataset.

                // TODO: we will rewrite the following when we need to add the big array support.
                // Now we will replace the std:min() with our own version to avoid the
                // inconsistent type comparison. KY 2022-12-28              
#if 0
                int clampedStride = std::min(outerDim.stride, (unsigned long long) currDatasetSize);
#endif
                int clampedStride = outerDim.stride;
                if (clampedStride > currDatasetSize)
                    clampedStride = currDatasetSize;
                
                // mapped endpoint clamped within this granule
                granuleConstraintTemplate.add_constraint(outerDimIt, localGranuleIndex, clampedStride, granuleStopIndex);

                // Do the constrained read and copy it into this output buffer
                agg_util::AggregationUtil::addDatasetArrayDataToAggregationOutputArray(*this, // into the output buffer of this object
                    nextOutputBufferElementIndex, // into the next open slice
                    getGranuleTemplateArray(), // constraints we just setup
                    name(), // aggvar name
                    const_cast<AggMemberDataset&>(*pCurrDataset), // Dataset who's DDS should be searched
                    getArrayGetterInterface(), DEBUG_CHANNEL);

                // Jump output buffer index forward by the amount we added.
                nextOutputBufferElementIndex += getGranuleTemplateArray().length();
                currDatasetWasRead = true;

                BESDEBUG_FUNC(DEBUG_CHANNEL,
                    " The granule index " << currDatasetIndex << " was read with constraints and copied into the aggregation output." << endl);
            } // !currDatasetWasRead
        } // for loop over outerDim
    } // try

    catch (AggregationException& ex) {
        THROW_NCML_PARSE_ERROR(-1, ex.what());
    }

}

} // namespace agg_util
