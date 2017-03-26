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

#include <unistd.h>

#include "TheBESKeys.h"
#include "BESInternalFatalError.h"

BESKeys *TheBESKeys::_instance = 0;
string TheBESKeys::ConfigFile = "";

BESKeys *TheBESKeys::TheKeys()
{
    if (_instance)
        return _instance;

    if (!TheBESKeys::ConfigFile.empty()) {
        _instance = new TheBESKeys(TheBESKeys::ConfigFile);
        return _instance;
    }

    // _instance is a nullptr and TheBESKeys::ConfigFile is ""
    // so lets try some obvious places...

    string try_ini = "/usr/local/etc/bes/bes.conf";
    if (access(try_ini.c_str(), R_OK) == 0) {
        TheBESKeys::ConfigFile = try_ini;
        _instance = new TheBESKeys(TheBESKeys::ConfigFile);
        return _instance;
    }

    try_ini = "/etc/bes/bes.conf";
    if (access(try_ini.c_str(), R_OK) == 0) {
        TheBESKeys::ConfigFile = try_ini;
        _instance = new TheBESKeys(TheBESKeys::ConfigFile);
        return _instance;
    }

    try_ini = "/usr/etc/bes/bes.conf";
    if (access(try_ini.c_str(), R_OK) == 0) {
        TheBESKeys::ConfigFile = try_ini;
        _instance = new TheBESKeys(TheBESKeys::ConfigFile);
        return _instance;
    }

    throw BESInternalFatalError("Unable to find a conf file or module version mismatch.", __FILE__, __LINE__);
}
