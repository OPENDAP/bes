// BESDefine.h

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

#ifndef BESDefine_h_
#define BESDefine_h_ 1

#include <string>
#include <list>

#include "BESObj.h"
#include "BESContainer.h"

class BESDefine : public BESObj {
    std::list<BESContainer *> _containers;
    std::string _agg_cmd;
    std::string _agg_handler;

public:
    BESDefine() = default;

    ~BESDefine() override;

    using containers_iter = std::list<BESContainer *>::iterator;
    using containers_citer = std::list<BESContainer *>::const_iterator;

    void add_container(BESContainer *container);

    BESDefine::containers_citer first_container() { return _containers.begin(); }
    BESDefine::containers_citer end_container() { return _containers.end(); }

    void set_agg_cmd(const std::string &cmd) {
        _agg_cmd = cmd;
    }

    const std::string &get_agg_cmd() {
        return _agg_cmd;
    }

    void set_agg_handler(const std::string &handler) {
        _agg_handler = handler;
    }

    const std::string &get_agg_handler() {
        return _agg_handler;
    }

    void dump(std::ostream &strm) const override;
};

#endif // BESDefine_h_
