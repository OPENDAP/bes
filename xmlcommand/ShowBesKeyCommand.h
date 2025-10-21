// -*- mode: c++; c-basic-offset:4 -*-
//
// ShowBesKeyCommand.h
//
// This file is part of the BES default command set
//
// Copyright (c) 2018 OPeNDAP, Inc
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.


#ifndef A_GetBesCommand_h
#define A_GetBesCommand_h 1

#include "BESXMLCommand.h"
#include "BESDataHandlerInterface.h"

#define SHOW_BES_KEY_RESPONSE_STR "showBesKey"
#define SHOW_BES_KEY_RESPONSE     "show.besKey"
#define BES_KEY_RESPONSE "BesKey"
#define KEY "key"

#define SBK_DEBUG_KEY             "show-bes-key"

class ShowBesKeyCommand: public BESXMLCommand {
public:
    ShowBesKeyCommand(const BESDataHandlerInterface &base_dhi);
    virtual ~ShowBesKeyCommand()
    {
    }

    virtual void parse_request(xmlNode *node);

    virtual bool has_response()
    {
        return true;
    }

    void dump(std::ostream &strm) const override;

    static BESXMLCommand * CommandBuilder(const BESDataHandlerInterface &base_dhi);
};

#endif // A_GetBesCommand_h
