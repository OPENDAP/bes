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
#ifndef __AGG_UTIL__AGG_MEMBER_DATASET_DDS_WRAPPER_H__
#define __AGG_UTIL__AGG_MEMBER_DATASET_DDS_WRAPPER_H__

#include "AggMemberDatasetWithDimensionCacheBase.h" // super

namespace agg_util {

// fdecls
class DDSAccessInterface;

/**
 * class AggMemberDatasetDDSWrapper: concrete subclass of AggMemberDataset
 * designed to hold a weak reference to an object containing
 * a DataDDS (DDSAccessInterface).
 *
 * Compare to AggMemberDatasetSharedDDSWrapper which use a ref-counted
 * DDSAccessRCInterface.  The weak one doesn't use ref counting in
 * order to avoid circular ref dependencies.
 *
 */
class AggMemberDatasetDDSWrapper: public AggMemberDatasetWithDimensionCacheBase {
public:
    AggMemberDatasetDDSWrapper();
    AggMemberDatasetDDSWrapper(const DDSAccessInterface* pDDSHolder);
    virtual ~AggMemberDatasetDDSWrapper();

    AggMemberDatasetDDSWrapper(const AggMemberDatasetDDSWrapper& proto);
    AggMemberDatasetDDSWrapper& operator=(const AggMemberDatasetDDSWrapper& that);

    /**
     * Access via the wrapped DDSAccessInterface.
     * If the wrapped DDS is NOT a DataDDS, we return NULL!
     * @return null or the wrapped DataDDS
     */
    virtual const libdap::DDS* getDDS();

private:

    void cleanup() throw ();

    void copyRepFrom(const AggMemberDatasetDDSWrapper& rhs);

    // data rep

    const DDSAccessInterface* _pDDSHolder;

};
// class AggMemberDatasetDDSWrapper

}

// namespace agg_util

#endif /* __AGG_UTIL__AGG_MEMBER_DATASET_DDS_WRAPPER_H__ */
