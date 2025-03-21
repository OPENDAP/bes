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

#include <libdap/Marshaller.h>
#include <libdap/ConstraintEvaluator.h>

#include "ArrayAggregationBase.h"
#include "NCMLDebug.h"
#include "BESDebug.h"
#include "BESStopWatch.h"

// BES debug channel we output to
#define DEBUG_CHANNEL "agg_util"

#if 0
// Local flag for whether to print constraints, to help debugging
static const bool PRINT_CONSTRAINTS = false;
#endif

using namespace libdap;
using namespace std;

#define MODULE "agg_util"
#define prolog string("ArrayAggregationBase::").append(__func__).append("() - ")


namespace agg_util {
ArrayAggregationBase::ArrayAggregationBase(const libdap::Array& proto, AMDList aggMembers, unique_ptr<ArrayGetterInterface> arrayGetter) :
    Array(proto), _pSubArrayProto(dynamic_cast<Array*>(const_cast<Array&>(proto).ptr_duplicate())),
    _pArrayGetter(std::move(arrayGetter)), _datasetDescs(std::move(aggMembers))
{
}

ArrayAggregationBase::ArrayAggregationBase(const ArrayAggregationBase& rhs) :
    Array(rhs)// , _pSubArrayProto(0) // duplicate() handles this
        //, _pArrayGetter(0) // duplicate() handles this
        // , _datasetDescs()
{
    BESDEBUG(DEBUG_CHANNEL, "ArrayAggregationBase() copy ctor called!" << endl);
    duplicate(rhs);
}

/* virtual */
ArrayAggregationBase::~ArrayAggregationBase()
{
    cleanup();
}

ArrayAggregationBase&
ArrayAggregationBase::operator=(const ArrayAggregationBase& rhs)
{
    if (this != &rhs) {
        cleanup();
        Array::operator=(rhs);
        duplicate(rhs);
    }
    return *this;
}

/* virtual */
ArrayAggregationBase*
ArrayAggregationBase::ptr_duplicate()
{
    return new ArrayAggregationBase(*this);
}

/* virtual */
// In child classes we specialize the BaseType::serialize() method so that
// as data are read they are also set (using Marshaller::put_vector_part()).
// In those cases this method is actually not called. We keep this version
// so that code that depends on read() actually reading in all of the data
// will still work.
bool ArrayAggregationBase::read()
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing");

    BESDEBUG_FUNC(DEBUG_CHANNEL, " function entered..." << endl);

    // Early exit if already done, avoid doing it twice!
    if (read_p()) {
        BESDEBUG_FUNC(DEBUG_CHANNEL, "read_p() set, early exit!");
        return true;
    }

    // Only continue if we are supposed to serialize this object at all.
    if (!(send_p() || is_in_selection())) {
        BESDEBUG_FUNC(DEBUG_CHANNEL, "Object not in output, skipping...  name=" << name() << endl);
        return true;
    }

#if 0
    if (PRINT_CONSTRAINTS) {
        BESDEBUG_FUNC(DEBUG_CHANNEL, "Constraints on this Array are:" << endl);
        printConstraints(*this);
    }
#endif

    // call subclass impl
    transferOutputConstraintsIntoGranuleTemplateHook();

#if 0
    if (PRINT_CONSTRAINTS) {
        BESDEBUG_FUNC(DEBUG_CHANNEL, "After transfer, constraints on the member template Array are: " << endl);
        printConstraints(getGranuleTemplateArray());
    }
#endif

    // Call the subclass specific algorithms to do the read
    // and stream
    readConstrainedGranuleArraysAndAggregateDataHook();

    // Set the cache bit to avoid recomputing
    set_read_p(true);
    return true;
}

const AMDList&
ArrayAggregationBase::getDatasetList() const
{
    return _datasetDescs;
}

///////////////////////////// Non Public Helpers

void ArrayAggregationBase::printConstraints(const Array& fromArray)
{
    ostringstream oss;
    AggregationUtil::printConstraints(oss, fromArray);
    BESDEBUG(DEBUG_CHANNEL, "Constraints for Array: " << name() << ": " << oss.str() << endl);
}

libdap::Array&
ArrayAggregationBase::getGranuleTemplateArray()
{
    VALID_PTR(_pSubArrayProto.get());
    return *(_pSubArrayProto.get());
}

const ArrayGetterInterface&
ArrayAggregationBase::getArrayGetterInterface() const
{
    VALID_PTR(_pArrayGetter.get());
    return *(_pArrayGetter.get());
}

void ArrayAggregationBase::duplicate(const ArrayAggregationBase& rhs)
{
    // Clone the template if it isn't null.
    _pSubArrayProto.reset(((rhs._pSubArrayProto.get()) ? (static_cast<Array*>(rhs._pSubArrayProto->ptr_duplicate())) : (nullptr)));

    // Clone the ArrayGetterInterface as well.
    _pArrayGetter.reset(((rhs._pArrayGetter.get()) ? (rhs._pArrayGetter->clone()) : (nullptr)));

    // full copy, will do the proper thing with refcounts.
    _datasetDescs = rhs._datasetDescs;
}

void ArrayAggregationBase::cleanup() noexcept
{
    _datasetDescs.clear();
    _datasetDescs.resize(0);
}

/* virtual */
void ArrayAggregationBase::transferOutputConstraintsIntoGranuleTemplateHook()
{
    NCML_ASSERT_MSG(false, "** Unimplemented function: "
        "ArrayAggregationBase::transferOutputConstraintsIntoGranuleTemplateHook(): "
        "needs to be overridden and implemented in a base class.");
}

/* virtual */
void ArrayAggregationBase::readConstrainedGranuleArraysAndAggregateDataHook()
{
    NCML_ASSERT_MSG(false, "** Unimplemented function: "
        "ArrayAggregationBase::readConstrainedGranuleArraysAndAggregateData(): "
        "needs to be overridden and implemented in a base class.");
}

}
