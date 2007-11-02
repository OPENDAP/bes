// ServerApp.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <string>

using std::string ;

#include "BESModuleApp.h"

class TcpSocket ;
class UnixSocket ;
class PPTServer ;

class ServerApp : public BESModuleApp
{
private:
    int				_portVal ;
    bool			_gotPort ;
    string			_unixSocket ;
    bool			_secure ;
    pid_t			_mypid ;
    TcpSocket			*_ts ;
    UnixSocket			*_us ;
    PPTServer			*_ps ;

    void			set_group_id() ;
    void			set_user_id() ;
public:
    				ServerApp() ;
    virtual			~ServerApp() ;
    virtual int			initialize( int argc, char **argv ) ;
    virtual int			run() ;
    virtual int			terminate( int sig = 0 ) ;

    virtual void		dump( ostream &strm ) const ;

    static void			signalTerminate( int sig ) ;
    static void			signalInterrupt( int sig ) ;
    static void			signalRestart( int sig ) ;
} ;

