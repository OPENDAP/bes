// -*- mode: c++; c-basic-offset:4 -*-
//
// FoW10JsonTransmitter.h
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

#ifndef A_FODmrppTransmitter_h
#define A_FODmrppTransmitter_h 1

#include <BESTransmitter.h>

/** @brief BESTransmitter class named "dmrpp" that transmits an OPeNDAP
 * data object as a DMRPP file
 *
 * The FoDapDmrppTransmitter transforms an OPeNDAP DMR object into a
 * DMRPP file and streams the new (temporary) DMRPP file back to the
 * client.
 *
 * @see BESTransmitter
 */
class ReturnAsDmrppTransmitter: public BESTransmitter {

public:
    ReturnAsDmrppTransmitter();
    ~ReturnAsDmrppTransmitter() override = default;
    ReturnAsDmrppTransmitter(const ReturnAsDmrppTransmitter&) = delete;
    ReturnAsDmrppTransmitter& operator=(const ReturnAsDmrppTransmitter&) = delete;
    ReturnAsDmrppTransmitter(const ReturnAsDmrppTransmitter&&) = delete;
    ReturnAsDmrppTransmitter& operator=(const ReturnAsDmrppTransmitter&&) = delete;

    static void send_dmrpp(BESResponseObject *obj, BESDataHandlerInterface &dhi);
};

#endif // A_FODmrppTransmitter_h

