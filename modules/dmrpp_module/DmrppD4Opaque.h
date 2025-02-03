
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#ifndef _dmrpp_d4opaque_h
#define _dmrpp_d4opaque_h 1

#include <memory>
#include <string>

#include <libdap/D4Opaque.h>
#include "DmrppCommon.h"

namespace libdap {
class XMLWriter;
}

namespace dmrpp {

class DmrppD4Opaque: public libdap::D4Opaque, public DmrppCommon {
    void insert_chunk(std::shared_ptr<Chunk>  chunk);
    void read_chunks();

public:
    DmrppD4Opaque(const std::string &n) : libdap::D4Opaque(n), DmrppCommon() { }
    DmrppD4Opaque(const std::string &n, const std::string &d) : libdap::D4Opaque(n, d), DmrppCommon() { }
    DmrppD4Opaque(const std::string &n, std::shared_ptr<DMZ> dmz) : libdap::D4Opaque(n), DmrppCommon(std::move(dmz)) { }
    DmrppD4Opaque(const std::string &n, const std::string &d, std::shared_ptr<DMZ> dmz) : libdap::D4Opaque(n, d), DmrppCommon(std::move(dmz)) { }
    DmrppD4Opaque(const DmrppD4Opaque &) = default;

    ~DmrppD4Opaque() override = default;

    DmrppD4Opaque &operator=(const DmrppD4Opaque &rhs);

    libdap::BaseType *ptr_duplicate() override {
        return new DmrppD4Opaque(*this);
    }

    /**
     * @brief Get a pointer to start of the Opaque data buffer
     * @note This returns a pointer to the internal memory managed by
     * libdap::D4Opaque. Make sure that the \arg d_buf field has enough
     * memory allocated. There is no method in the libdap::D4Opaque
     * class to allocate memory. This class must do that.
     */
    virtual unsigned char *get_buf()
    {
        return d_buf.data();
    }

    /**
     * @brief Allocate \arg size bytes for the opaque data
     * @param size
     */
    virtual void resize(unsigned long long size)
    {
        d_buf.resize(size);
    }

    bool read() override;
    void set_send_p(bool state) override;

    void print_dap4(libdap::XMLWriter &writer, bool constrained = false) override
    {
        DmrppCommon::print_dmrpp(writer, constrained);
    }

    void dump(ostream & strm) const override;
};

} // namespace dmrpp

#endif // _dmrpp_d4opaque_h
