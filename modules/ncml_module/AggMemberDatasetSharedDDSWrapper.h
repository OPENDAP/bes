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
#ifndef __AGG_UTIL__AGG_MEMBER_DATASET_SHARED_DDS_WRAPPER_H__
#define __AGG_UTIL__AGG_MEMBER_DATASET_SHARED_DDS_WRAPPER_H__

#include "AggMemberDatasetWithDimensionCacheBase.h" // super

namespace agg_util
{
  // fdecls
  class DDSAccessRCInterface;

  /**
   * class AggMemberDatasetSharedDDSWrapper: concrete subclass of AggMemberDataset
   * designed to hold a ref-counted reference to an object containing
   * a DataDDS (DDSAccessRCInterface).
   *
   * This allows an object containing an aggregated DDS (DDSAccessRCInterface)
   * to exist outside of its normal scope by letting the wrapper hold
   * a strong (counted) reference to it until the aggregation serialization
   * is complete.
   *
   * The wrapped DDSAccessRCInterface will be ref() upon construction
   * or being added into this by assignment or copy construction and will
   * be unref() upon destruction or loss of reference via assignment operator.
   *
   */
  class AggMemberDatasetSharedDDSWrapper : public AggMemberDatasetWithDimensionCacheBase
  {
  public:
    AggMemberDatasetSharedDDSWrapper();
    explicit AggMemberDatasetSharedDDSWrapper(const DDSAccessRCInterface* pDDSHolder);
    ~AggMemberDatasetSharedDDSWrapper() override;

    AggMemberDatasetSharedDDSWrapper(const AggMemberDatasetSharedDDSWrapper& proto);
    AggMemberDatasetSharedDDSWrapper& operator=(const AggMemberDatasetSharedDDSWrapper& that);

    /**
     * Access via the wrapped DDSAccessRCInterface.
     * If the wrapped DDS is NOT a DataDDS, we return NULL!
     * @return null or the wrapped DataDDS
     */
    const libdap::DDS* getDDS() override;

  private:

      // If _pDDSHolder, unref() and null it.
      void cleanup() noexcept;

      // If rhs._pDDSHodler, we ref() it
      // and maintain an alias ourselves.
      // ASSUMES: !_pDDSHolder
      void copyRepFrom(const AggMemberDatasetSharedDDSWrapper& rhs);

      // data rep

      // Invariant: If not-null, we maintain a strong ref() to it
      // and unref() in cleanup().
      const DDSAccessRCInterface* _pDDSHolder {nullptr};

  }; // class AggMemberDatasetSharedDDSWrapper

} // namespace agg_util

#endif /* __AGG_UTIL__AGG_MEMBER_DATASET_SHARED_DDS_WRAPPER_H__ */
