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
#ifndef __AGG_UTIL__AGG_MEMBER_DATASET_WITH_DIMENSION_CACHE_BASE_H__
#define __AGG_UTIL__AGG_MEMBER_DATASET_WITH_DIMENSION_CACHE_BASE_H__

#include "AggMemberDataset.h"
#include <vector>

namespace libdap {
class BaseType;
}

namespace agg_util {

class AggMemberDatasetWithDimensionCacheBase: public AggMemberDataset {
public:
    AggMemberDatasetWithDimensionCacheBase(const std::string& location);

    AggMemberDatasetWithDimensionCacheBase(const AggMemberDatasetWithDimensionCacheBase& proto);

    virtual ~AggMemberDatasetWithDimensionCacheBase();

    AggMemberDatasetWithDimensionCacheBase& operator=(const AggMemberDatasetWithDimensionCacheBase& rhs);

    /* This will stay pure virtual for subclasses */
    /* virtual const libdap::DDS* getDDS() = 0; */

    virtual unsigned int getCachedDimensionSize(const std::string& dimName) const;
    virtual bool isDimensionCached(const std::string& dimName) const;
    virtual void setDimensionCacheFor(const Dimension& dim, bool throwIfFound);
    virtual void fillDimensionCacheByUsingDDS();
    virtual void flushDimensionCache();

    /** Append the values in the dimension cache to the output stream */
    virtual void saveDimensionCache(std::ostream& ostr);
    virtual void loadDimensionCache(std::istream& istr);

private:
    // Helper Functions

    /** Check the _dimensionCache for the dimension with name
     * and return ptr to the location, else NULL.
     */
    Dimension* findDimension(const std::string& dimName);

    /** Go through the dimensions for the variable if it has them
     * and add to the dimension cache.
     * If a container var, recurse on its variable iterator.
     * @param pBT
     */
    void addDimensionsForVariableRecursive(libdap::BaseType& var);

    void saveDimensionCacheInternal(std::ostream& ostr);
    void loadDimensionCacheInternal(std::istream& istr);


private:
    // Data Rep
    std::vector<Dimension> _dimensionCache;

};

}

#endif /* __AGG_UTIL__AGG_MEMBER_DATASET_WITH_DIMENSION_CACHE_BASE_H__ */
