
// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2020 University Corporation for Atmospheric Research
// Author: Nathan Potter <ndp@opendap.org>
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

#include <iostream>

#include "BESError.h"
#include "StandAloneApp.h"

int main(int argc, char **argv)
{
    try {
        StandAloneApp app;
        return app.main(argc, argv);
    }
    catch (BESError &e) {
        std::cerr << "Caught BES Error while starting the command processor: " << e.get_message() << std::endl;
        return 1;
    }
    catch (std::exception &e) {
        std::cerr << "Caught C++ error while starting the command processor: " << e.what() << std::endl;
        return 2;
    }
    catch (...) {
        std::cerr << "Caught unknown error while starting the command processor." << std::endl;
        return 3;
    }
}
