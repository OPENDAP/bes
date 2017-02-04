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
#ifndef __AGG_UTIL__AGG_MEMBER_DATASET_H__
#define __AGG_UTIL__AGG_MEMBER_DATASET_H__

#include "Dimension.h" // agg_util
#include "RCObject.h"
#include <string>
#include <vector>

namespace libdap {
class DDS;
}

using libdap::DDS;

namespace agg_util {
/**
 * Abstract helper superclass for allowing lazy access to the DDS
 * for an aggregation. This is used during a read() if the dataset
 * is needed in an aggregation.
 *
 * Note: This inherits from RCObject so is a ref-counted object to avoid
 * making excessive copies of it and especially of any contained DDS.
 *
 * Currently, there are two concrete subclasses:
 *
 * o AggMemberDatasetUsingLocationRef: to load an external location
 *            into a DDS as needed
 *            (lazy eval so not loaded unless in the output of read() )
 *
 * o AggMemberDatasetDDSWrapper: to hold a pre-loaded DDS for the
 *            case of virtual or pre-loaded datasets
 *            (data declared in NcML file, nested aggregations, e.g.)
 *            In this case, getLocation() is presumed empty().
 */
class AggMemberDataset: public RCObject {
public:
    AggMemberDataset(const std::string& location);
    virtual ~AggMemberDataset();

    AggMemberDataset& operator=(const AggMemberDataset& rhs);
    AggMemberDataset(const AggMemberDataset& proto);

    /** The location to which the AggMemberDataset refers
     * Note: this could be "" for some subclasses
     * if they are virtual or nested */
    const std::string& getLocation() const;

    /**
     * Return the DDS for the location, loading it in
     * if it hasn't yet been loaded.
     * Can return NULL if there's a problem.
     * (Not const due to lazy eval)
     * @return the DDS ptr containing the loaded dataset or NULL.
     */
    virtual const libdap::DDS* getDDS() = 0;

    // TODO Consider adding freeDDS() or equivalent
    // to clear the memory made by getDDS if it was
    // loaded so we can tighten up the memory usage
    // for large AMD Lists.

    /**
     * Get the size of the given dimension named dimName
     * cached within the dataset.  If not found in cache, throws.
     *
     * If a cached value exists from a prior load of the DDS
     * using loadDimensionCacheFromDDS() or from a call to
     * setDimensionCacheFor(), return that.
     *
     * Otherwise, this must load the DDS to get the values.
     *
     * Implementation is left up to subclasses for efficiency.
     *
     * @return the size of the dimension if found
     * @throw agg_util::DimensionNotFoundException if not located
     * via any means.
     */
    virtual unsigned int getCachedDimensionSize(const std::string& dimName) const = 0;

    /** Return whether the dimension is already cached,
     * or would have to be loaded to be found. */
    virtual bool isDimensionCached(const std::string& dimName) const = 0;

    /**
     * Seed the dimension cache using the given dimension,
     * so that later calls to getDimensionSize for dim.name will
     * return the dim.size immediately without checking or
     * loading the actual DDS.
     *
     * If it already exists and throwIfFound then will throw
     * an AggregationException.
     *
     * If it exists and !throwIfFound, will replace the old one.
     *
     * @param dim the dimension to seed
     * @param if true, throw if name take.  Else replace original.
     */
    virtual void setDimensionCacheFor(const Dimension& dim, bool throwIfFound) = 0;

    /**
     * Uses the getDDS() call in order to find all named dimensions
     * within it and to seed them into the dimension cache table for
     * faster later lookups.
     * Potentially slow!
     */
    virtual void fillDimensionCacheByUsingDDS() = 0;

    /**
     * Flush out any cache for the Dimensions so that it will
     * have to be loaded.
     */
    virtual void flushDimensionCache() = 0;

    /** Append the values in the dimension cache to the output stream */
    virtual void saveDimensionCache(std::ostream& ostr) = 0;

    /** Load the values in the dimension cache from the input stream */
    virtual void loadDimensionCache(std::istream& istr) = 0;


private:
    // data rep
    std::string _location; // non-empty location from which to load DDS
};

// List is ref-counted ptrs to AggMemberDataset concrete subclasses.
typedef std::vector<RCPtr<AggMemberDataset> > AMDList;

}
;
// namespace agg_util

#endif /* __AGG_UTIL__AGG_MEMBER_DATASET_H__ */
