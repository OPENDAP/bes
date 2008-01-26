// TheBESKeys.cc

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

#include "sys/stat.h"
#include "TheBESKeys.h"
#include "BESInternalFatalError.h"
#include "config.h"

#define BES_CONF getenv("BES_CONF")

BESKeys *TheBESKeys::_instance = 0;
string TheBESKeys::ConfigFile = "";

BESKeys *TheBESKeys::TheKeys()
{
    if (_instance == 0) {
        string use_ini = TheBESKeys::ConfigFile;
        if (use_ini == "") {
            char *ini_file = BES_CONF;
            if (!ini_file)
	    {
		string try_ini = "/usr/local/etc/bes/bes.conf" ;
		struct stat buf;
		int statret = stat( try_ini.c_str(), &buf ) ;
		if ( statret == -1 || !S_ISREG( buf.st_mode ) )
		{
		    try_ini = "/etc/bes/bes.conf" ;
		    int statret = stat( try_ini.c_str(), &buf ) ;
		    if ( statret == -1 || !S_ISREG( buf.st_mode ) )
		    {
			try_ini = "/usr/etc/bes/bes.conf" ;
			int statret = stat( try_ini.c_str(), &buf ) ;
			if ( statret == -1 || !S_ISREG( buf.st_mode ) )
			{
			    string s = (string)"Unable to locate BES config file. "
				    + "Please either pass -c "
				    + "option when starting the BES, set "
				    + "the environment variable BES_CONF, "
				    + "or install in /usr/local/etc/bes/bes.conf "
				    + "or /etc/bes/bes.conf." ;
			    throw BESInternalFatalError(s, __FILE__, __LINE__);
			}
			else
			{
			    use_ini = try_ini ;
			}
		    }
		    else
		    {
			use_ini = try_ini ;
		    }
		}
		else
		{
		    use_ini = try_ini ;
		}
            } else {
                use_ini = ini_file;
            }
        }
        _instance = new TheBESKeys(use_ini);
    }
    return _instance;
}

