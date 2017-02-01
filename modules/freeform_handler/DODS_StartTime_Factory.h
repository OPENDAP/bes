
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ff_handler a FreeForm API handler for the OPeNDAP
// DAP2 data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.


// (c) COPYRIGHT URI/MIT 1998
// Please read the full copyright statement in the file COPYRIGHT.  
//
// Authors:
//      jhrg,jimg       James Gallagher (jgallagher@gso.uri.edu)
//      dan             Daniel Holloway (dholloway@gso.uri.edu)

#ifndef _dods_starttime_factory_h
#define _dods_starttime_factory_h


#include "DODS_Time.h"
#include "DODS_Time_Factory.h"
#include "DAS.h"
#include "DDS.h"
#include "BaseType.h"

/** Read times from datasets based on text configuration values. Times are
    returned using DODS\_Time objects.

    @see DODS_Time
    @author Daniel Holloway
    @author James Gallagher */

class DODS_StartTime_Factory : public DODS_Time_Factory {
private:

    DODS_StartTime_Factory() {}	/* Prevent the creation of empty objects. */

public:
    virtual ~DODS_StartTime_Factory() {}

    /** Note that the default constructor is private.
	@name Constructors */

    //@{
    /** Read the configuration information and decide how to build DODS\_Time
	objects. The DODS\_Time\_Factory member function #get_time()# will
	return DODS\_Time objects.

	@see get_time()
	@param dds The DDS of the dataset from which times are to be read.
	@param das The DAS of the dataset from which times are to be read. */
    
    DODS_StartTime_Factory(DDS &dds) : DODS_Time_Factory(dds, "DODS_StartTime") {}
    //@}
};

#endif // _dods_starttime_factory_h 
