
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

#ifndef _dods_time_factory_h
#define _dods_time_factory_h


#include "DODS_Time.h"
#include "DAS.h"
#include "DDS.h"
#include "BaseType.h"

using namespace libdap ;

/** Read times from datasets based on text configuration values. Times are
    returned using DODS\_Time objects.

    @see DODS_Time
    @author James Gallagher */

class DODS_Time_Factory {
private:
    BaseType *_hours;
    BaseType *_minutes;
    BaseType *_seconds;
    bool _gmt;

protected:
    DODS_Time_Factory() {}

public:
    virtual ~DODS_Time_Factory() {}

    /** Note that the default constructor is private.
	@name Constructors */

    //@{
    /** Read the configuration information and decide how to build DODS\_Time
	objects. The DODS\_Time\_Factory member function #get_time()# will
	return DODS\_Time objects.

	@see get()
	@param dds The DDS of the dataset from which times are to be read.
	@param das The DAS of the dataset from which times are to be read.
	@param attribute_name The name of the attribute container in the DAS
	that holds configuration inforamtion for the instance of DODS_Time. */
    DODS_Time_Factory(DDS &dds, const string &attribute_name = "DODS_Time");
    //@}

    /** @name Access */

    //@{
    /** Read a time value from a dataset.

	@return The DODS\_Time object associated with the time. */
    virtual DODS_Time get();
    //@}
};

// $Log: DODS_Time_Factory.h,v $
// Revision 1.6  2003/02/10 23:01:52  jimg
// Merged with 3.2.5
//
// Revision 1.5  2001/10/14 01:36:17  jimg
// Merged with release-3-2-4.
//
// Revision 1.4.2.2  2002/01/22 02:19:35  jimg
// Fixed bug 62. Users built fmt files that used types other than int32
// for date and time components (e.g. int16). I fixed the factory classes
// so that DODS_Date and DODS_Time objects will be built correctly when
// any of the integer (or in the case of seconds, float) data types are
// used. In so doing I also refactored the factory classes so that code
// duplication was reduced (by using inhertiance).
// Added two tests for the new capabilities (see date_time.1.exp, the last
// two tests).
//
// Revision 1.4.2.1  2001/10/11 17:42:09  jimg
// Fixed a bug in the Time, StartTime and EndTime factory calsses. A local
// variable _gmt shadowed the class member _gmt.
//
// Revision 1.4  2000/10/11 19:37:56  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.3  1999/01/08 22:08:19  jimg
// Fixed doc++ comments.
//
// Revision 1.2  1999/01/05 00:42:42  jimg
// Switched to simpler method names.
// Added _gmt field.
//
// Revision 1.1  1998/12/28 19:08:05  jimg
// Initial version of the DODS_Time factory object. This is a test
// implementation. 
//

#endif // _dods_time_factory_h 
