// -*- mode: c++; c-basic-offset:4 -*-
//
// FoDapJsonTransmitter.cc
//
// This file is part of BES DMRPP Module
//
// Copyright (c) 2023 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
// Author: Daniel Holloway <dholloway@opendap.org>
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
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include <ostream>

#include <BESDapNames.h>
#include <BESInternalError.h>
#include <BESInternalFatalError.h>
#include <BESDataHandlerInterface.h>

#include "NgapOwnedContainer.h"
#include "ReturnAsDmrppTransmitter.h"

using namespace std;
using namespace ngap;

/**
 * @brief Return a DMR++ document
 *
 * The ReturnAsDmrppTransmitter transmitter is used by the BES framework to
 * return a DMR++ document as XML. The bes command <get type="dap" ...> is
 * called with returnAs="dmrpp".
 */
ReturnAsDmrppTransmitter::ReturnAsDmrppTransmitter() {
    BESTransmitter::add_method(DAP4DATA_SERVICE, ReturnAsDmrppTransmitter::send_dmrpp);
}

/**
 * @brief The static method registered to transmit the DMR+ XML.
 *
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalFatalError if the first Container in the DataHandlerInterface
 * object is not a NgapOwnedContainer - a container designed to get DMR++ from
 * the NASA NGAP system.
 * @throws BESInteralError if there is a string output error when sending the response.
 */
void ReturnAsDmrppTransmitter::send_dmrpp(BESResponseObject *, BESDataHandlerInterface &dhi) {
    auto container = dynamic_cast<NgapOwnedContainer *>(*dhi.containers.begin());
    if (!container) throw BESInternalFatalError("expected NgapOwnedContainer", __FILE__, __LINE__);
    const auto dmrpp = container->access();

    auto &strm = dhi.get_output_stream();
    strm << dmrpp << flush;
    if (!strm) throw BESInternalError("Output stream is not set, can not return as", __FILE__, __LINE__);
}
