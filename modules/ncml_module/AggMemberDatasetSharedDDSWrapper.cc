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
#include "AggMemberDatasetSharedDDSWrapper.h"
#include <DDS.h> // libdap
#include "DDSAccessInterface.h"
#include "NCMLDebug.h"

namespace agg_util
{
  AggMemberDatasetSharedDDSWrapper::AggMemberDatasetSharedDDSWrapper()
  : AggMemberDatasetWithDimensionCacheBase("") // empty location for the wrapper
  , _pDDSHolder(0) // NULL, really shouldn't create a default.
  {
  }

  AggMemberDatasetSharedDDSWrapper::AggMemberDatasetSharedDDSWrapper(const DDSAccessRCInterface* pDDSHolder)
  : AggMemberDatasetWithDimensionCacheBase("") // empty location
  , _pDDSHolder(pDDSHolder)
  {
    if (_pDDSHolder)
      {
        _pDDSHolder->ref();
      }
  }

  AggMemberDatasetSharedDDSWrapper::~AggMemberDatasetSharedDDSWrapper()
  {
    BESDEBUG("ncml:memory", "~AggMemberDatasetDDSWrapper() called..." << endl);
    cleanup(); // will unref()
  }

  AggMemberDatasetSharedDDSWrapper::AggMemberDatasetSharedDDSWrapper(const AggMemberDatasetSharedDDSWrapper& proto)
  : RCObjectInterface()
  , AggMemberDatasetWithDimensionCacheBase(proto)
  , _pDDSHolder(0)
  {
    copyRepFrom(proto);
  }

  AggMemberDatasetSharedDDSWrapper&
  AggMemberDatasetSharedDDSWrapper::operator=(const AggMemberDatasetSharedDDSWrapper& that)
  {
    if (this != &that)
      {
        // deal with old reference
        cleanup();
        // super changes
        AggMemberDatasetWithDimensionCacheBase::operator=(that);
        // local changes
        copyRepFrom(that);
      }
    return *this;
  }


  const libdap::DDS*
  AggMemberDatasetSharedDDSWrapper::getDDS()
  {
    const libdap::DDS* pDDS = 0;
    if (_pDDSHolder)
      {
        pDDS = _pDDSHolder->getDDS();
      }
    return dynamic_cast<const libdap::DDS*>(pDDS);
  }

  /////////////////////////////// Private Helpers ////////////////////////////////////

  void
  AggMemberDatasetSharedDDSWrapper::cleanup() throw()
  {
    if (_pDDSHolder)
      {
        _pDDSHolder->unref();
        _pDDSHolder = 0;
      }
  }

  void
  AggMemberDatasetSharedDDSWrapper::copyRepFrom(const AggMemberDatasetSharedDDSWrapper& rhs)
  {
    NCML_ASSERT(!_pDDSHolder);
    _pDDSHolder = rhs._pDDSHolder;
    if (_pDDSHolder)
      {
        _pDDSHolder->ref();
      }
  }
}
