// TheBESKeys.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <sys/stat.h>
#include <cstdlib>

#include "TheBESKeys.h"
#include "BESInternalFatalError.h"

// left in for now... It would be better to not use an env var
// since these are easily hacked. jhrg 10/12/11
// Removed 8/28/13 jhrg #define BES_CONF getenv("BES_CONF")

BESKeys *TheBESKeys::_instance = 0;
string TheBESKeys::ConfigFile = "";

BESKeys *TheBESKeys::TheKeys()
{
    if (_instance == 0) {
        string use_ini = TheBESKeys::ConfigFile;
        if (use_ini == "") {
#ifdef BES_CONF
            char *ini_file = BES_CONF;
            if (!ini_file) {
#endif
                string try_ini = "/usr/local/etc/bes/bes.conf";
                struct stat buf;
                int statret = stat(try_ini.c_str(), &buf);
                if (statret == -1 || !S_ISREG( buf.st_mode )) {
                    try_ini = "/etc/bes/bes.conf";
                    int statret = stat(try_ini.c_str(), &buf);
                    if (statret == -1 || !S_ISREG( buf.st_mode )) {
                        try_ini = "/usr/etc/bes/bes.conf";
                        int statret = stat(try_ini.c_str(), &buf);
                        if (statret == -1 || !S_ISREG( buf.st_mode )) {
                            string s = "Unable to find a conf file or module version mismatch.";
#if 0
                            // This message is confusing because it often appears when
                            // the user has passed -c to besctl. It makes the command
                            // (besctl) more confusing to use. jhrg 10/12/11
                            "Unable to locate BES config file. " + "Please either pass -c "
                            + "option when starting the BES, set " + "the environment variable BES_CONF, "
                            + "or install in /usr/local/etc/bes/bes.conf " + "or /etc/bes/bes.conf.";
#endif
                            throw BESInternalFatalError(s, __FILE__, __LINE__);
                        }
                        else {
                            use_ini = try_ini;
                        }
                    }
                    else {
                        use_ini = try_ini;
                    }
                }
                else {
                    use_ini = try_ini;
                }
#ifdef BES_CONF
            }
            else {
                use_ini = ini_file;
            }
#endif
        }
        _instance = new TheBESKeys(use_ini);
    }
    return _instance;
}

#if 0
// I think adding this was a mistake because the bes really needs to restart
// to have it's configuration updated - either that or this code must get
// much more complex - e.g., reload all modules (not just their configuration
// files). jhrg 5/27/11
void TheBESKeys::updateKeys()
{
    delete _instance; _instance = 0;
    _instance = new TheBESKeys(TheBESKeys::ConfigFile);
}

void TheBESKeys::updateKeys( const string &keys_file_name )
{
    delete _instance; _instance = 0;
    TheBESKeys::ConfigFile = keys_file_name;
    _instance = new TheBESKeys(keys_file_name);
}
#endif
