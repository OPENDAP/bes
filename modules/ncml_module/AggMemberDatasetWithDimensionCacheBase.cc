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
#include "AggMemberDatasetWithDimensionCacheBase.h"

#include <string.h>
#include <errno.h>

#include <sys/stat.h>

#include <sstream>
#include <algorithm>
#include <fstream>

#include "Array.h" // libdap
#include "BaseType.h" // libdap
#include "Constructor.h" // libdap
#include "DataDDS.h" // libdap
#include "DDS.h" // libdap

#include "AggregationException.h" // agg_util
#include "AggMemberDatasetDimensionCache.h"
#include "NCMLDebug.h"
#include "TheBESKeys.h"

using std::string;
using libdap::BaseType;
using libdap::Constructor;
using libdap::DataDDS;
using libdap::DDS;

#if 0
static const string BES_DATA_ROOT("BES.Data.RootDirectory");
static const string BES_CATALOG_ROOT("BES.Catalog.catalog.RootDirectory");
#endif

#define MAX_DIMENSION_COUNT_KEY "NCML.DimensionCache.maxDimensions"
#define DEFAULT_MAX_DIMENSIONS 100

static const string DEBUG_CHANNEL("agg_util");

namespace agg_util {

// Used to init the DimensionCache below with an estimated number of dimensions
static const unsigned int DIMENSION_CACHE_INITIAL_SIZE = 0;

AggMemberDatasetWithDimensionCacheBase::AggMemberDatasetWithDimensionCacheBase(const std::string& location) :
    AggMemberDataset(location), _dimensionCache(DIMENSION_CACHE_INITIAL_SIZE)
{
}

/* virtual */
AggMemberDatasetWithDimensionCacheBase::~AggMemberDatasetWithDimensionCacheBase()
{
    _dimensionCache.clear();
    _dimensionCache.resize(0);
}

AggMemberDatasetWithDimensionCacheBase::AggMemberDatasetWithDimensionCacheBase(
    const AggMemberDatasetWithDimensionCacheBase& proto) :
    RCObjectInterface(), AggMemberDataset(proto), _dimensionCache(proto._dimensionCache)
{
}

AggMemberDatasetWithDimensionCacheBase&
AggMemberDatasetWithDimensionCacheBase::operator=(const AggMemberDatasetWithDimensionCacheBase& rhs)
{
    if (&rhs != this) {
        AggMemberDataset::operator=(rhs);
        _dimensionCache.clear();
        _dimensionCache = rhs._dimensionCache;
    }
    return *this;
}

/* virtual */
unsigned int AggMemberDatasetWithDimensionCacheBase::getCachedDimensionSize(const std::string& dimName) const
{
    Dimension* pDim = const_cast<AggMemberDatasetWithDimensionCacheBase*>(this)->findDimension(dimName);
    if (pDim) {
        return pDim->size;
    }
    else {
        std::ostringstream oss;
        oss << __PRETTY_FUNCTION__ << " Dimension " << dimName << " was not found in the cache!";
        throw DimensionNotFoundException(oss.str());
    }
}

/* virtual */
bool AggMemberDatasetWithDimensionCacheBase::isDimensionCached(const std::string& dimName) const
{
    return bool(const_cast<AggMemberDatasetWithDimensionCacheBase*>(this)->findDimension(dimName));
}

/* virtual */
void AggMemberDatasetWithDimensionCacheBase::setDimensionCacheFor(const Dimension& dim, bool throwIfFound)
{
    Dimension* pExistingDim = findDimension(dim.name);
    if (pExistingDim) {
        if (!throwIfFound) {
            // This discards the object that was in the vector with this name
            // and replaces it with the information passed in via 'dim'. NB:
            // the values of the object referenced by 'dim' are copied into
            // the object pointed to by 'pExistingDim'.
            *pExistingDim = dim;
        }
        else {
            std::ostringstream msg;
            msg << __PRETTY_FUNCTION__ << " Dimension name=" << dim.name
                << " already exists and we were asked to set uniquely!";
            throw AggregationException(msg.str());
        }
    }
    else {
        _dimensionCache.push_back(dim);
    }
}

/* virtual */
void AggMemberDatasetWithDimensionCacheBase::fillDimensionCacheByUsingDDS()
{
    // Get the dds
    DDS* pDDS = const_cast<DDS*>(getDDS());
    VALID_PTR(pDDS);

    // Recursive add on all of them
    for (DataDDS::Vars_iter it = pDDS->var_begin(); it != pDDS->var_end(); ++it) {
        BaseType* pBT = *it;
        VALID_PTR(pBT);
        addDimensionsForVariableRecursive(*pBT);
    }
}

/* virtual */
void AggMemberDatasetWithDimensionCacheBase::flushDimensionCache()
{
    _dimensionCache.clear();
}

/* virtual */
void AggMemberDatasetWithDimensionCacheBase::saveDimensionCache(std::ostream& ostr)
{
    saveDimensionCacheInternal(ostr);
}

/* virtual */
void AggMemberDatasetWithDimensionCacheBase::loadDimensionCache(std::istream& istr)
{
    loadDimensionCacheInternal(istr);
}

Dimension*
AggMemberDatasetWithDimensionCacheBase::findDimension(const std::string& dimName)
{
    Dimension* ret = 0;
    for (vector<Dimension>::iterator it = _dimensionCache.begin(); it != _dimensionCache.end(); ++it) {
        if (it->name == dimName) {
            ret = &(*it);
        }
    }
    BESDEBUG(DEBUG_CHANNEL,"AggMemberDatasetWithDimensionCacheBase::findDimension(dimName='"<<dimName<<"') -  " << (ret?"Found " + ret->name:"Dimension Not Found") << endl);

    return ret;
}

void AggMemberDatasetWithDimensionCacheBase::addDimensionsForVariableRecursive(libdap::BaseType& var)
{
    BESDEBUG_FUNC(DEBUG_CHANNEL, "Adding dimensions for variable name=" << var.name() << endl);

    if (var.type() == libdap::dods_array_c) {
        BESDEBUG(DEBUG_CHANNEL, " Adding dimensions for array variable name = " << var.name() << endl);

        libdap::Array& arrVar = dynamic_cast<libdap::Array&>(var);
        libdap::Array::Dim_iter it;
        for (it = arrVar.dim_begin(); it != arrVar.dim_end(); ++it) {
            libdap::Array::dimension& dim = *it;
            if (!isDimensionCached(dim.name)) {
                Dimension newDim(dim.name, dim.size);
                setDimensionCacheFor(newDim, false);

                BESDEBUG(DEBUG_CHANNEL,
                    " Adding dimension: " << newDim.toString() << " to the dataset granule cache..." << endl);
            }
        }
    }

    else if (var.is_constructor_type()) // then recurse
    {
        BESDEBUG(DEBUG_CHANNEL, " Recursing on all variables for constructor variable name = " << var.name() << endl);

        libdap::Constructor& containerVar = dynamic_cast<libdap::Constructor&>(var);
        libdap::Constructor::Vars_iter it;
        for (it = containerVar.var_begin(); it != containerVar.var_end(); ++it) {
            BESDEBUG(DEBUG_CHANNEL, " Recursing on variable name=" << (*it)->name() << endl);

            addDimensionsForVariableRecursive(*(*it));
        }
    }
}

// Sort function
static bool sIsDimNameLessThan(const Dimension& lhs, const Dimension& rhs)
{
    return (lhs.name < rhs.name);
}

void AggMemberDatasetWithDimensionCacheBase::saveDimensionCacheInternal(std::ostream& ostr)
{
    BESDEBUG("ncml", "Saving dimension cache for dataset location = " << getLocation() << " ..." << endl);

    // Not really necessary, but might help with trying to read output
    std::sort(_dimensionCache.begin(), _dimensionCache.end(), sIsDimNameLessThan);

    // Save out the location first, ASSUMES \n is NOT in the location for read back
    const std::string& loc = getLocation();
    ostr << loc << '\n';

    // Now save each dimension
    unsigned int n = _dimensionCache.size();
    ostr << n << '\n';
    for (unsigned int i = 0; i < n; ++i) {
        const Dimension& dim = _dimensionCache.at(i);
        // @TODO This assumes the dimension names don't contain spaces. We should fix this, and the loader, to work with any name.
        ostr << dim.name << '\n' << dim.size << '\n';
    }
}


void AggMemberDatasetWithDimensionCacheBase::loadDimensionCacheInternal(std::istream& istr)
{
    BESDEBUG("ncml", "Loading dimension cache for dataset location = " << getLocation() << endl);

    string maxDimsStr;
    unsigned long maxDims;
    bool found;
    TheBESKeys::TheKeys()->get_value(MAX_DIMENSION_COUNT_KEY,maxDimsStr, found);
    if(found){
        maxDims = strtoul(maxDimsStr.c_str(), 0, 0);
        if (maxDims == 0)
            throw BESError(string("The value '") + maxDimsStr + "' is not valid: " + strerror(errno),
                BES_SYNTAX_USER_ERROR, __FILE__, __LINE__);
        // Replace 2011 string function for Debian (and CentOS?).
        // jhrg 10/27/15
        // maxDims = stoul(maxDimsStr,0);
    }
    else {
        maxDims = DEFAULT_MAX_DIMENSIONS;
    }

    // Read in the location string
    std::string loc;
    getline(istr, loc, '\n');

    // Make sure the location we read is the same as the location
    // for this AMD or there's an unrecoverable serialization bug
    if (loc != getLocation()) {
        stringstream ss;
        ss << "Serialization error: the location loaded from the "
            "dimensions cache was: \"" << loc << "\" but we expected it to be " << getLocation()
            << "\".  Unrecoverable!";
        THROW_NCML_INTERNAL_ERROR(ss.str());
    }

#if 0
    unsigned int n = 0;
    istr >> n >> ws;
    BESDEBUG("ncml", "AggMemberDatasetWithDimensionCacheBase::loadDimensionCacheInternal() - n: " << n << endl);
    for (unsigned int i = 0; i < n; ++i) {
        Dimension newDim;
        istr >> newDim.name >> ws;
        BESDEBUG("ncml", "AggMemberDatasetWithDimensionCacheBase::loadDimensionCacheInternal() - newDim.name: " << newDim.name << endl);
        istr >> newDim.size >> ws;
        BESDEBUG("ncml", "AggMemberDatasetWithDimensionCacheBase::loadDimensionCacheInternal() - newDim.size: " << newDim.size << endl);
        if (istr.bad()) {
            // Best we can do is throw an internal error for now.
            // Perhaps later throw something else that causes a
            // recreation of the cache
            THROW_NCML_INTERNAL_ERROR("Parsing dimension cache failed to deserialize from stream.");
        }
        _dimensionCache.push_back(newDim);
    }
#endif

    unsigned long numDims = 0;
    unsigned int dimCount = 0;
    string dimName;

    istr >> numDims >> ws;
    if(istr.fail()){
        THROW_NCML_INTERNAL_ERROR("Parsing dimension cache FAIL. Unable to read number of dimensions from cache file.");
    }
    if(numDims>maxDims){
        stringstream msg;
        msg << "Parsing dimension cache FAIL. Dimension count exceeds limits. Changing value of the ncml module configuration "
            "key " << MAX_DIMENSION_COUNT_KEY << " may help. numDims: "<< numDims << "  maxDims: "<< maxDims;
        THROW_NCML_INTERNAL_ERROR(msg.str());
    }
    BESDEBUG("ncml", "AggMemberDatasetWithDimensionCacheBase::loadDimensionCacheInternal() - numDims: " << numDims << endl);

    while(istr.peek()!=EOF){
        Dimension newDim;
        istr >> newDim.name >> ws;
        if(istr.fail()){
            THROW_NCML_INTERNAL_ERROR("Parsing dimension cache FAIL. Unable to read dimension name from cache.");
        }
        BESDEBUG("ncml", "AggMemberDatasetWithDimensionCacheBase::loadDimensionCacheInternal() - newDim.name: " << newDim.name << endl);


        if(istr.peek()==EOF){
            THROW_NCML_INTERNAL_ERROR("Parsing dimension cache FAIL. Unexpected EOF. Expected to find dimension size value.");
        }

        istr >> newDim.size >> ws;
        if(istr.fail()){
            THROW_NCML_INTERNAL_ERROR("Parsing dimension cache FAIL. Unable to read dimension size from cache.");
        }
        BESDEBUG("ncml", "AggMemberDatasetWithDimensionCacheBase::loadDimensionCacheInternal() - newDim.size: " << newDim.size << endl);

        dimCount++;

        if(dimCount > numDims){
            stringstream msg;
            msg << "Parsing the dimension cache failed because the number of dimensions found in the cache did "
                "not match the number indicated in the cache header. Expected " << numDims << " Found: " << dimCount;
            BESDEBUG("ncml", "AggMemberDatasetWithDimensionCacheBase::loadDimensionCacheInternal() - " << msg.str() << endl);
            THROW_NCML_INTERNAL_ERROR(msg.str());
        }
        _dimensionCache.push_back(newDim);
    }

    if(dimCount != numDims){
        stringstream msg;
        msg << "Parsing the dimension cache failed because the number of dimensions found in the cache did "
            "not match the number indicated in the cache header. Expected " << numDims << " Found: " << dimCount;
        BESDEBUG("ncml", "AggMemberDatasetWithDimensionCacheBase::loadDimensionCacheInternal() - " << msg.str() << endl);
        THROW_NCML_INTERNAL_ERROR(msg.str());
    }


    BESDEBUG("ncml", "Loaded dimension cache ("<< numDims << " dimensions) for dataset location = " << getLocation() << endl);


}

}
