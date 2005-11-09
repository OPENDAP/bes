// DODSReporterList.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#ifndef I_DODSReporterList_h
#define I_DODSReporterList_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "DODSDataHandlerInterface.h"

class DODSReporter ;

class DODSReporterList {
private:
    static DODSReporterList *	_instance ;
    map< string, DODSReporter * > _reporter_list ;
protected:
				DODSReporterList(void) ;
public:
    virtual			~DODSReporterList(void) ;

    typedef map< string, DODSReporter * >::const_iterator Reporter_citer ;
    typedef map< string, DODSReporter * >::iterator Reporter_iter ;

    virtual bool		add_reporter( string reporter_name,
					      DODSReporter * handler ) ;
    virtual DODSReporter *	remove_reporter( string reporter_name ) ;
    virtual DODSReporter *	find_reporter( string reporter_name ) ;

    virtual void		report( const DODSDataHandlerInterface &dhi ) ;

    static DODSReporterList *	TheList() ;
};

#endif // I_DODSReporterList_h

// $Log: DODSReporterList.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
