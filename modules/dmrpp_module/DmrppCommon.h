
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

#ifndef _dmrpp_common_h
#define _dmrpp_common_h 1

#include <string>
#include <vector>
#include <memory>

namespace libdap {
class DMR;
class BaseType;
class D4BaseTypeFactory;
class D4Group;
class D4Attributes;
class D4EnumDef;
class D4Dimension;
class XMLWriter;
}

namespace http {
class url;
}

namespace dmrpp {

class DMZ;
class Chunk;
class DmrppArray;
class SuperChunk;

void join_threads(pthread_t threads[], unsigned int num_threads);

/**
 * @brief Size and offset information of data included in DMR++ files.
 *
 * A mixin class the provides common behavior for the libdap types
 * when they are used with the DMR++ handler. This includes instances
 * of the Chunk object, code to help the parser break apart the info
 * in the DMR++ XML documents, and other stuff.
 *
 * Included in this class is the read_atomic() method that reads the
 * atomic types like Byte, Int32, ... Str.
 *
 */
class DmrppCommon {

	friend class DmrppCommonTest;
    friend class DmrppParserTest;

private:
    bool d_compact;
	std::string d_filters;
	std::string d_byte_order;
	std::vector<unsigned long long> d_chunk_dimension_sizes;
	std::vector<std::shared_ptr<Chunk>> d_chunks;
	bool d_twiddle_bytes;

    // These indicate that the chunks or attributes have been loaded into the
    // variable when the DMR++ handler is using lazy-loading of this data.
    bool d_chunks_loaded;
    bool d_attributes_loaded;

    std::shared_ptr<DMZ> d_dmz;

protected:
    /// @brief Returns a copy of the internal Chunk vector.
    /// @see get_immutable_chunks()
    virtual std::vector<std::shared_ptr<Chunk>> get_chunks() {
    	return d_chunks;
    }

    virtual char *read_atomic(const std::string &name);

    // This declaration allows code in the SuperChunky program to use the protected method.
    // jhrg 10/25/21
    friend void compute_super_chunks(dmrpp::DmrppArray *array, bool only_constrained, std::vector<dmrpp::SuperChunk *> &super_chunks);

public:
    static bool d_print_chunks;         ///< if true, print_dap4() prints chunk elements
    static std::string d_dmrpp_ns;      ///< The DMR++ XML namespace
    static std::string d_ns_prefix;     ///< The XML namespace prefix to use

    DmrppCommon() : d_compact(false), d_twiddle_bytes(false), d_chunks_loaded(false), d_attributes_loaded(false)
    {
    }

    DmrppCommon(std::shared_ptr<DMZ> dmz) : d_compact(false), d_twiddle_bytes(false), d_chunks_loaded(false),
        d_attributes_loaded(false), d_dmz(dmz)
    {
    }

    DmrppCommon(const DmrppCommon &dc) = default;

    virtual ~DmrppCommon()= default;

    /// @brief Return the names of all the filters in the order they were applied
    virtual std::string get_filters() const {
        return d_filters;
    }

    void set_filter(const std::string &value);

    virtual bool is_filters_empty() const {
        return d_filters.empty();
    }

    /// @brief Returns true if this object utilizes COMPACT layout.
    virtual bool is_compact_layout() const {
        return d_compact;
    }

    /// @brief Set the value of the compact property
    void set_compact(bool value) {
        d_compact = value;
    }

    /// @brief Returns true if this object utilizes shuffle compression.
    virtual bool twiddle_bytes() const { return d_twiddle_bytes; }

    // TODO Remove this. jhrg 11/12/21
    /// @brief Provide access to the DMZ instance bound to this variable
    virtual const std::shared_ptr<DMZ> &get_dmz() const { return d_dmz; }

    /// @brief Have the chunks been loaded?
    virtual bool get_chunks_loaded()  const { return d_chunks_loaded; }
    virtual void set_chunks_loaded(bool state) {  d_chunks_loaded = state; }

    /// @brief Have the attributes been loaded?
    virtual bool get_attributes_loaded()  const { return d_attributes_loaded; }
    virtual void set_attributes_loaded(bool state) {  d_attributes_loaded = state; }


    /// @brief A const reference to the vector of chunks
    /// @see get_chunks()
    virtual const std::vector<std::shared_ptr<Chunk>> &get_immutable_chunks() const {
        return d_chunks;
    }

    /// @brief The chunk dimension sizes held in a const vector
    /// @return A reference to a const vector of chunk dimension sizes
    virtual const std::vector<unsigned long long> &get_chunk_dimension_sizes() const {
    	return d_chunk_dimension_sizes;
    }

    /**
     * @brief Get the number of elements in this chunk
     *
     * @return The number of elements; multiply by element size to get the number of bytes.
     */
    virtual unsigned long long get_chunk_size_in_elements() const {
        unsigned long long elements = 1;
        for (auto d_chunk_dimension_size : d_chunk_dimension_sizes) {
            elements *= d_chunk_dimension_size;
        }

        return elements;
    }

    void print_chunks_element(libdap::XMLWriter &xml, const std::string &name_space = "");

    void print_compact_element(libdap::XMLWriter &xml, const std::string &name_space = "", const std::string &encoded = "");

    void print_dmrpp(libdap::XMLWriter &writer, bool constrained = false);

    // Replaced hsize_t with size_t. This eliminates a dependency on hdf5. jhrg 9/7/18
    /// @brief Set the value of the chunk dimension sizes given a vector of HDF5 hsize_t
    void set_chunk_dimension_sizes(const std::vector<size_t> &chunk_dims)
    {
        // tried using copy(chunk_dims.begin(), chunk_dims.end(), d_chunk_dimension_sizes.begin())
        // it didn't work, maybe because of the differing element types?
        for (auto chunk_dim : chunk_dims) {
            d_chunk_dimension_sizes.push_back(chunk_dim);
        }
    }

#if 1
    // TODO remove since this duplicates code in DMZ. jhrg 11/12/21
    void load_chunks(libdap::BaseType *btp);
#endif

    virtual void parse_chunk_dimension_sizes(const std::string &chunk_dim_sizes_string);

    virtual void ingest_compression_type(const std::string &compression_type_string);

    virtual void ingest_byte_order(const std::string &byte_order_string);
    virtual std::string get_byte_order() const { return d_byte_order; }

    virtual unsigned long add_chunk(
            std::shared_ptr<http::url> d_data_url,
            const std::string &byte_order,
            unsigned long long size,
            unsigned long long offset,
            const std::string &position_in_array);

    virtual unsigned long add_chunk(
            std::shared_ptr<http::url> d_data_url,
            const std::string &byte_order,
            unsigned long long size,
            unsigned long long offset,
            const std::vector<unsigned long long> &position_in_array);

    virtual unsigned long add_chunk(
            const std::string &byte_order,
            unsigned long long size,
            unsigned long long offset,
            const std::string &position_in_array);

    virtual unsigned long add_chunk(
            const std::string &byte_order,
            unsigned long long size,
            unsigned long long offset,
            const std::vector<unsigned long long> &position_in_array);

    virtual void dump(std::ostream & strm) const;
};

} // namespace dmrpp

#endif // _dmrpp_common_h

