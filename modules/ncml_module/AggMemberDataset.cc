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
#include "AggMemberDataset.h"

#include "BESDataDDSResponse.h" // bes
#include "DataDDS.h"
#include "DDS.h" // libdap
#include "DDSLoader.h" // agg_util
#include "NCMLDebug.h" // ncml_module
#include "NCMLUtil.h" // ncml_module

namespace agg_util
{

  AggMemberDataset::AggMemberDataset(const std::string& location)
  : RCObject(0)
  , _location(location)
  {
    // no rep yet
  }

  AggMemberDataset::AggMemberDataset(const AggMemberDataset& proto)
  : RCObjectInterface()
  , RCObject(proto)
  {
      _location = proto._location;	// jhrg 3/16/11
    // no rep yet
  }

  AggMemberDataset::~AggMemberDataset()
  {
    _location = "";
  }

  const std::string&
  AggMemberDataset::getLocation() const
  {
    return _location;
  }

  AggMemberDataset&
  AggMemberDataset::operator=(const AggMemberDataset& rhs)
  {
    if (&rhs == this)
      {
        return *this;
      }
    _location = rhs._location;
    return *this;
  }


}
