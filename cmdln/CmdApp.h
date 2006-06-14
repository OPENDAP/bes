// CmdApp.h

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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <fstream>

using std::ofstream ;
using std::ifstream ;

#include "BESBaseApp.h"

class CmdClient ;

class CmdApp : public BESBaseApp
{
private:
    CmdClient *			_client ;
    string			_hostStr ;
    string			_unixStr ;
    int				_portVal ;
    string			_cmd ;
    ofstream *			_outputStrm ;
    ifstream *			_inputStrm ;
    bool			_createdInputStrm ;
    int				_timeoutVal ;

    void			showVersion() ;
    void			showUsage() ;
    void			registerSignals() ;
public:
    				CmdApp() ;
    virtual			~CmdApp() ;
    virtual int			initialize( int argc, char **argv ) ;
    virtual int			run() ;

    CmdClient *			client() { return _client ; }
    static void			signalCannotConnect( int sig ) ;
    static void			signalInterrupt( int sig ) ;
    static void			signalTerminate( int sig ) ;
    static void			signalBrokenPipe( int sig ) ;
} ;

