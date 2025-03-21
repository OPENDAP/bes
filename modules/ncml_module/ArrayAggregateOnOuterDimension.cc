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

#include "ArrayAggregateOnOuterDimension.h"
#include "AggregationException.h"

#include <libdap/DataDDS.h> // libdap::DataDDS
#include <libdap/Marshaller.h>

// only NCML backlinks we want in this agg_util class.
#include "NCMLDebug.h" // BESDEBUG and throw macros
#include "BESDebug.h"
#include "BESStopWatch.h"

#define DEBUG_CHANNEL "agg_util"
#define prolog string("ArrayAggregateOnOuterDimension::").append(__func__).append("() - ")

namespace agg_util {

ArrayAggregateOnOuterDimension::ArrayAggregateOnOuterDimension(const libdap::Array& proto,
    AMDList memberDatasets, unique_ptr<ArrayGetterInterface> arrayGetter, const Dimension& newDim) :
    ArrayAggregationBase(proto, std::move(memberDatasets),std::move(arrayGetter)), // no new dim yet in super chain
    _newDim(newDim)
{
    BESDEBUG(DEBUG_CHANNEL, "ArrayAggregateOnOuterDimension: ctor called!" << endl);

    // Up the rank of the array using the new dimension as outer (prepend)
    BESDEBUG(DEBUG_CHANNEL, "ArrayAggregateOnOuterDimension: adding new outer dimension: " << _newDim.name << endl);
    prepend_dim(_newDim.size, _newDim.name);
}

ArrayAggregateOnOuterDimension::ArrayAggregateOnOuterDimension(const ArrayAggregateOnOuterDimension& proto) :
    ArrayAggregationBase(proto), _newDim()
{
    BESDEBUG(DEBUG_CHANNEL, "ArrayAggregateOnOuterDimension() copy ctor called!" << endl);
    duplicate(proto);
}

ArrayAggregateOnOuterDimension::~ArrayAggregateOnOuterDimension()
{
    BESDEBUG(DEBUG_CHANNEL, "~ArrayAggregateOnOuterDimension() dtor called!" << endl);
    cleanup();
}

ArrayAggregateOnOuterDimension*
ArrayAggregateOnOuterDimension::ptr_duplicate()
{
    return new ArrayAggregateOnOuterDimension(*this);
}

ArrayAggregateOnOuterDimension&
ArrayAggregateOnOuterDimension::operator=(const ArrayAggregateOnOuterDimension& rhs)
{
    if (this != &rhs) {
        cleanup();
        ArrayAggregationBase::operator=(rhs);
        duplicate(rhs);
    }
    return *this;
}

// Set this to 0 to get the old behavior where the entire response
// (for this variable) is built in memory and then sent to the client.
#define PIPELINING 1

/**
 * Specialization that implements a simple pipelining scheme. If an
 * aggregation is made up from many 'slices' of different arrays, each
 * will be read individually. This version sends each part as soon as
 * it is read instead of building the entire response in memory and
 * then sending it.
 *
 * If this method is called and the variable has read_p set to true,
 * then libdap::Array::serialize() will be called.
 *
 * @note The read() method of ArrayAggregationBase can be used to read
 * all of the data in one shot.
 *
 * @param eval
 * @param dds
 * @param m
 * @param ce_eval
 * @return true
 */

bool ArrayAggregateOnOuterDimension::serialize(libdap::ConstraintEvaluator &eval, libdap::DDS &dds,
    libdap::Marshaller &m, bool ce_eval)
{

    BES_STOPWATCH_START(DEBUG_CHANNEL, prolog + "Timing");

    // Only continue if we are supposed to serialize this object at all.
    if (!(send_p() || is_in_selection())) {
        BESDEBUG_FUNC(DEBUG_CHANNEL, "Object not in output, skipping...  name=" << name() << endl);
        return true;
    }

    bool status = false;

    delete bes_timing::elapsedTimeToReadStart;
    bes_timing::elapsedTimeToReadStart = 0;

    if (!read_p()) {
        // call subclass impl
        transferOutputConstraintsIntoGranuleTemplateHook();
        // outer one is the first in iteration
        const Array::dimension& outerDim = *(dim_begin());
        BESDEBUG(DEBUG_CHANNEL,
            "Aggregating datasets array with outer dimension constraints: " << " start=" << outerDim.start << " stride=" << outerDim.stride << " stop=" << outerDim.stop << endl);

        // Be extra sure we have enough datasets for the given request
        if (static_cast<unsigned int>(outerDim.size) != getDatasetList().size()) {
            // Not sure whose fault it was, but tell the author
            THROW_NCML_PARSE_ERROR(-1, "The new outer dimension of the joinNew aggregation doesn't "
                " have the same size as the number of datasets in the aggregation!");
        }

#if PIPELINING
        // Prepare our output buffer for our constrained length
        m.put_vector_start(length());
#else
        reserve_value_capacity();
#endif
        // this index pointing into the value buffer for where to write.
        // The buffer has a stride equal to the _pSubArrayProto->size().

        // Keep this to do some error checking
        int nextElementIndex = 0;

        // Traverse the dataset array respecting hyperslab
        for (int i = outerDim.start; i <= outerDim.stop && i < outerDim.size; i += outerDim.stride) {
            AggMemberDataset& dataset = *((getDatasetList())[i]);

            try {
                Array* pDatasetArray = AggregationUtil::readDatasetArrayDataForAggregation(getGranuleTemplateArray(),
                    name(), dataset, getArrayGetterInterface(), DEBUG_CHANNEL);
#if PIPELINING
                delete bes_timing::elapsedTimeToTransmitStart;
                bes_timing::elapsedTimeToTransmitStart = 0;
                m.put_vector_part(pDatasetArray->get_buf(), getGranuleTemplateArray().length(), var()->width(),
                    var()->type());
#else
                this->set_value_slice_from_row_major_vector(*pDatasetArray, nextElementIndex);
#endif

                pDatasetArray->clear_local_data();
            }
            catch (agg_util::AggregationException& ex) {
                std::ostringstream oss;
                oss << "Got AggregationException while streaming dataset index=" << i << " data for location=\""
                    << dataset.getLocation() << "\" The error msg was: " << std::string(ex.what());
                THROW_NCML_PARSE_ERROR(-1, oss.str());
            }

            // Jump forward by the amount we added.
            nextElementIndex += getGranuleTemplateArray().length();
        }

        // If we succeeded, we are at the end of the array!
        NCML_ASSERT_MSG(nextElementIndex == length(), "Logic error:\n"
            "ArrayAggregateOnOuterDimension::read(): "
            "At end of aggregating, expected the nextElementIndex to be the length of the "
            "aggregated array, but it wasn't!");

#if PIPELINING
        m.put_vector_end();
        status = true;
#else
        // Set the cache bit to avoid recomputing
        set_read_p(true);

        delete bes_timing::elapsedTimeToTransmitStart;
        bes_timing::elapsedTimeToTransmitStart = 0;
        status = libdap::Array::serialize(eval, dds, m, ce_eval);
#endif
    }
    else {
        status = libdap::Array::serialize(eval, dds, m, ce_eval);
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////////////////
// helpers

void ArrayAggregateOnOuterDimension::duplicate(const ArrayAggregateOnOuterDimension& rhs)
{
    _newDim = rhs._newDim;
}

void ArrayAggregateOnOuterDimension::cleanup() const noexcept
{
    // not implemented
}

/* virtual */
void ArrayAggregateOnOuterDimension::transferOutputConstraintsIntoGranuleTemplateHook()
{
    // transfer the constraints from this object into the subArray template
    // skipping our first dim which is the new one and not in the subArray.
    agg_util::AggregationUtil::transferArrayConstraints(&(getGranuleTemplateArray()), // into this template
        *this, // from this
        true, // skip first dim in the copy since we handle it special
        false, // also skip it in the toArray for the same reason.
        true, // print debug
        DEBUG_CHANNEL); // on this channel
}

/* virtual */
// In this version of the code, I broke apart the call to
// agg_util::AggregationUtil::addDatasetArrayDataToAggregationOutputArray()
// into two calls: AggregationUtil::readDatasetArrayDataForAggregation()
// and this->set_value_slice_from_row_major_vector(). This
void ArrayAggregateOnOuterDimension::readConstrainedGranuleArraysAndAggregateDataHook()
{
    BES_STOPWATCH_START(DEBUG_CHANNEL, prolog + "Timing");

    // outer one is the first in iteration
    const Array::dimension& outerDim = *(dim_begin());
    BESDEBUG(DEBUG_CHANNEL,
        "Aggregating datasets array with outer dimension constraints: " << " start=" << outerDim.start << " stride=" << outerDim.stride << " stop=" << outerDim.stop << endl);

    // Be extra sure we have enough datasets for the given request
    if (static_cast<unsigned int>(outerDim.size) != getDatasetList().size()) {
        // Not sure whose fault it was, but tell the author
        THROW_NCML_PARSE_ERROR(-1, "The new outer dimension of the joinNew aggregation doesn't "
            " have the same size as the number of datasets in the aggregation!");
    }

    // Prepare our output buffer for our constrained length
    reserve_value_capacity();

    // this index pointing into the value buffer for where to write.
    // The buffer has a stride equal to the _pSubArrayProto->size().
    int nextElementIndex = 0;

    // Traverse the dataset array respecting hyperslab
    for (int i = outerDim.start; i <= outerDim.stop && i < outerDim.size; i += outerDim.stride) {
        AggMemberDataset& dataset = *((getDatasetList())[i]);

        try {
            agg_util::AggregationUtil::addDatasetArrayDataToAggregationOutputArray(*this, // into the output buffer of this object
                nextElementIndex, // into the next open slice
                getGranuleTemplateArray(), // constraints template
                name(), // aggvar name
                dataset, // Dataset who's DDS should be searched
                getArrayGetterInterface(), DEBUG_CHANNEL);
        }
        catch (agg_util::AggregationException& ex) {
            std::ostringstream oss;
            oss << "Got AggregationException while streaming dataset index=" << i << " data for location=\""
                << dataset.getLocation() << "\" The error msg was: " << std::string(ex.what());
            THROW_NCML_PARSE_ERROR(-1, oss.str());
        }

        // Jump forward by the amount we added.
        nextElementIndex += getGranuleTemplateArray().length();
    }

    // If we succeeded, we are at the end of the array!
    NCML_ASSERT_MSG(nextElementIndex == length(), "Logic error:\n"
        "ArrayAggregateOnOuterDimension::read(): "
        "At end of aggregating, expected the nextElementIndex to be the length of the "
        "aggregated array, but it wasn't!");
}

}
