// OPeNDAPApp.h

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

#ifndef A_OPeNDAPApp_H
#define A_OPeNDAPApp_H

#include <string>

using std::string ;

#include "OPeNDAPObj.h"

class OPeNDAPApp : public OPeNDAPObj
{
protected:
    string			_appName ;
    bool			_debug ;
    static OPeNDAPApp *		_theApplication;
                                OPeNDAPApp(void) {};
public:
    virtual			~OPeNDAPApp() {}
    virtual int			main(int argC, char **argV) = 0;
    virtual int			initialize(int argC, char **argV) = 0;
    virtual int			run(void) = 0;
    virtual int			terminate(int sig = 0) = 0;
    string			appName( void ) { return _appName ; }
    bool			debug( void ) { return _debug ; }
    void			setDebug( bool debug ) { _debug = debug ; }

    static OPeNDAPApp *		TheApplication(void) { return _theApplication; }
};

#endif

