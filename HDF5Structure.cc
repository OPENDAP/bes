/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Copyright (c) 2005 OPeNDAP, Inc.
// Copyright (c) 2007 HDF Group, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//         Hyo-Kyung Lee <hyoklee@hdfgroup.org>
// 
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
// 
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

#ifdef _GNUG_
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>
#include "hdf5.h"

#include "h5dds.h"
#include "HDF5Structure.h"
#include "InternalErr.h"

BaseType *
HDF5Structure::ptr_duplicate()
{
    return new HDF5Structure(*this);
}

HDF5Structure::HDF5Structure(const string & n):Structure(n)
{
    ty_id = -1;
    dset_id = -1;
}

HDF5Structure::~HDF5Structure()
{
}

HDF5Structure &
HDF5Structure::operator=(const HDF5Structure &rhs)
{
    if (this == &rhs)
        return *this;

    dynamic_cast<Structure&>(*this) = rhs; // run Structure assignment
        
    
    return *this;
}


bool
HDF5Structure::read(const string & dataset)
{
  if(read_p())
    return false;
  if(return_type(ty_id) == "Structure"){
    long buf;
    char Msgi[256];

    if (get_data(dset_id, (void *) &buf, Msgi) < 0) {    
      throw InternalErr(__FILE__, __LINE__, 
			"HDF5Structure::read(): Unimplemented method.");
    }

    set_read_p(true);
    // libdap/dods-datatypes.h 
    // dods_uint32 uint32 = (dods_uint32) buf;
    // val2buf(&uint32);
  }
  return false;
}

void
HDF5Structure::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5Structure::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t
HDF5Structure::get_did()
{
    return dset_id;
}

hid_t
HDF5Structure::get_tid()
{
    return ty_id;
}
