
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
#include <utility>

#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

#include <libdap/Type.h>

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

namespace pugi {
class xml_node;
}

namespace dmrpp {

class DMZ;
class Chunk;
class DmrppArray;
class SuperChunk;

void join_threads(pthread_t threads[], unsigned int num_threads);

/**
 * @brief hold the value used to fill empty chunks
 */
union fill_value_union {
    int8_t int8;
    int16_t int16;
    int32_t int32;
    int64_t int64;

    uint8_t uint8;
    uint16_t uint16;
    uint32_t uint32;
    uint64_t uint64;

    float f;
    double d;
};

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
    friend class DMZTest;

    bool d_compact = false;
    bool d_missing_data = false;
    bool d_disable_dio = false;

	std::string d_filters;
	std::string d_byte_order;
	std::vector<unsigned long long> d_chunk_dimension_sizes;
	std::vector<std::shared_ptr<Chunk>> d_chunks;
	bool d_twiddle_bytes = false;

    // These indicate that the chunks or attributes have been loaded into the
    // variable when the DMR++ handler is using lazy-loading of this data.
    bool d_chunks_loaded = false;
    bool d_attributes_loaded = false;

    bool d_uses_fill_value {false};
    // Convert fill_value to the correct numeric datatype at the time of use. jhrg 4/24/22
    std::string d_fill_value_str;
    libdap::Type d_fill_value_type{libdap::dods_null_c};
    fill_value_union d_fill_value;

    // We need this parameter to handle the case when the variable only contains one chunk
    // and the data in that chunk are all fill_value. It is OK to compress this filled chunk,
    // However, for some data format like HDF5, it figures out that there is only one filled
    // chunk data for this variable, so the chunk is not compressed at all. If Dmrpp applies
    // de-compression to this variable, an error will occur. So we need this parameter to
    // turn off the de-compression filter for this case. KY 07/28/22
    bool d_one_chunk_fill_value {false};

    // Each instance of DmrppByte, ..., holds a shared pointer to the DMZ so that
    // it can fetch more information from the XML if needed - this is how the lazy-load
    // feature is implemented. The xml_node object is used to simplify finding where
    // in the XML information about a variable is stored - to limit searching the
    // document, the code caches the XML node.
    std::shared_ptr<DMZ> d_dmz;
    pugi::xml_node d_xml_node;

    // var_chunks_storage_size only applies to the chunking storage. 
    // For compact and contiguous storages, this variable is always 0 since there are no chunks.
    unsigned long long var_chunks_storage_size = 0;
    std::vector<unsigned int> deflate_levels;
    bool processing_fv_chunks = false;
    bool using_linked_block = false;
    unsigned int total_linked_blocks =0;

    bool multi_linked_blocks_chunk = false;

    // Structure offset 
    std::vector<unsigned int> struct_offsets;
protected:
    virtual char *read_atomic(const std::string &name);
      virtual char *read_atomic(const std::string &name, size_t & buf_size);
    // This declaration allows code in the SuperChunky program to use the protected method.
    // jhrg 10/25/21
    friend void compute_super_chunks(dmrpp::DmrppArray *array, bool only_constrained, std::vector<dmrpp::SuperChunk *> &super_chunks);

public:
    static bool d_print_chunks;         ///< if true, print_dap4() prints chunk elements
    static std::string d_dmrpp_ns;      ///< The DMR++ XML namespace
    static std::string d_ns_prefix;     ///< The XML namespace prefix to use

    DmrppCommon() = default;

    explicit DmrppCommon(std::shared_ptr<DMZ> dmz) : d_dmz(std::move(dmz)) { }

    DmrppCommon(const DmrppCommon &) = default;

    virtual ~DmrppCommon()= default;

    /// @brief Return the names of all the filters in the order they were applied
    virtual std::string get_filters() const {
        return d_filters;
    }

    void set_filter(const std::string &value);

    const std::vector<unsigned int> & get_deflate_levels() const { return deflate_levels;}
    void set_deflate_levels(const std::vector<unsigned int>& def_levels) {
         for(const auto &def_level:def_levels)
            deflate_levels.push_back(def_level);
    }
    void set_struct_offsets(const std::vector<unsigned int>& s_offs) {
         for(const auto &s_o:s_offs)
            struct_offsets.push_back(s_o);
    }
    const std::vector<unsigned int> & get_struct_offsets() const { return struct_offsets;}

    void set_processing_fv_chunks() { processing_fv_chunks = true;}
    bool get_processing_fv_chunks() const { return processing_fv_chunks; }

    void set_using_linked_block() {  using_linked_block = true;}
    bool get_using_linked_block() const { return  using_linked_block; }

    void set_total_linked_blocks(unsigned tlbs) { total_linked_blocks = tlbs;}
    unsigned int get_total_linked_blocks() const { return total_linked_blocks; }

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

    /// @brief Returns true if this object describes the missing data.
    virtual bool is_missing_data() const {
        return d_missing_data;
    }

    /// @brief Set the value of the missing data.
    void set_missing_data(bool value) {
        d_missing_data = value;
    }

    /// @brief Returns true if this object contains a chunk that have multiple linked blocks .
    virtual bool is_multi_linked_blocks_chunk() const {
        return multi_linked_blocks_chunk;
    }

    /// @brief Set the value of the boolean variable that indicates this variable contains multiple linked blocks in a chunk.
    void set_multi_linked_blocks_chunk(bool value) {
        multi_linked_blocks_chunk = value;
    }


    /// @brief Returns true if this object describes the missing data.
    virtual bool is_disable_dio() const {
        return d_disable_dio;
    }

    /// @brief Set the value of the compact property
    void set_disable_dio(bool value) {
        d_disable_dio = value;
    }

    /// @brief Returns true if this object utilizes shuffle compression.
    virtual bool twiddle_bytes() const { return d_twiddle_bytes; }

    // @brief Provide access to the DMZ instance bound to this variable
    // virtual const std::shared_ptr<DMZ> &get_dmz() const { return d_dmz; }

    /// @brief Have the chunks been loaded?
    virtual bool get_chunks_loaded()  const { return d_chunks_loaded; }
    virtual void set_chunks_loaded(bool state) {  d_chunks_loaded = state; }

    /// @brief Have the attributes been loaded?
    virtual bool get_attributes_loaded()  const { return d_attributes_loaded; }
    virtual void set_attributes_loaded(bool state) {  d_attributes_loaded = state; }

    virtual const pugi::xml_node &get_xml_node() const { return d_xml_node; }
    virtual void set_xml_node(pugi::xml_node node) { d_xml_node = node; }

    /// @brief A const reference to the vector of chunks
    virtual const std::vector<std::shared_ptr<Chunk>> &get_immutable_chunks() const {
        return d_chunks;
    }

    std::vector<std::shared_ptr<Chunk>> get_chunks() const { return d_chunks; }

    /// @brief Use this when the number of chunks is needed
    /// @return the number of Chunk objects for this variable
    virtual size_t get_chunks_size() const { return d_chunks.size(); }

    /// @brief The chunk dimension sizes held in a const vector
    /// @return A reference to a const vector of chunk dimension sizes
    virtual const std::vector<unsigned long long> &get_chunk_dimension_sizes() const {
    	return d_chunk_dimension_sizes;
    }

    /// @brief Get the number of elements in this chunk
    /// @return The number of elements; multiply by element size to get the number of bytes.
    virtual unsigned long long get_chunk_size_in_elements() const {
        unsigned long long elements = 1;
        for (auto d_chunk_dimension_size : d_chunk_dimension_sizes) {
            elements *= d_chunk_dimension_size;
        }

        return elements;
    }

    /// @brief Set the uses_fill_value property
    virtual void set_uses_fill_value(bool ufv) { d_uses_fill_value = ufv; }

    /// @brief Set the fill value (using a string)
    virtual void set_fill_value_string(const std::string &fv) { d_fill_value_str = fv; }

    /// @brief Set the libdap data type to use with the fill value
    virtual void set_fill_value_type(libdap::Type t) { d_fill_value_type = t; }

    /// @return Return true if the the chunk uses 'fill value.'
    virtual bool get_uses_fill_value() const { return d_uses_fill_value; }

    /// @return Return the fill value as a string or "" if get_fill_value() is false
    virtual std::string get_fill_value() const { return d_fill_value_str; }

    /// @return Return the fill value as a string or "" if get_fill_value() is false
    virtual libdap::Type get_fill_value_type() const { return d_fill_value_type; }

    /// @brief Set the one_chunk_fill_value property
    virtual void set_one_chunk_fill_value(bool ufv) { d_one_chunk_fill_value = ufv; }

    /// @return Return true if this variable contains one chunk and the data are all 'fill value.'
    virtual bool get_one_chunk_fill_value() const { return d_one_chunk_fill_value; }

    void print_chunks_element(libdap::XMLWriter &xml, const std::string &name_space = "");

    void print_compact_element(libdap::XMLWriter &xml, const std::string &name_space = "", const std::string &encoded = "") const;
    void print_missing_data_element(const libdap::XMLWriter &xml, const std::string &name_space = "", const std::string &encoded = "") const;
    void print_missing_data_element(const libdap::XMLWriter &xml, const std::string &name_space, const char *data, int length) const;
    void print_special_structure_element(const libdap::XMLWriter &xml, const std::string &name_space = "", const std::string &encoded = "") const;

    void print_dmrpp(libdap::XMLWriter &writer, bool constrained = false);

    /// @brief Set the value of the chunk dimension sizes given a vector of HDF5 hsize_t
    void set_chunk_dimension_sizes(const std::vector<unsigned long long> &chunk_dims) {
        d_chunk_dimension_sizes.clear();
        //d_chunk_dimension_sizes.resize(chunk_dims.size());
        for (auto chunk_dim : chunk_dims) {
            d_chunk_dimension_sizes.emplace_back(chunk_dim);
        }
    }

    // These two functions duplicate code in DMZ but provides access to the DMZ::load_chunks()
    // method without having to cast a BaseType to a DmrppCommon in order to use it. jhrg 11/12/21
    virtual void load_chunks(libdap::BaseType *btp);
    virtual void load_attributes(libdap::BaseType *btp);

    virtual void parse_chunk_dimension_sizes(const std::string &chunk_dim_sizes_string);

    virtual void ingest_compression_type(const std::string &compression_type_string);

    virtual void ingest_byte_order(const std::string &byte_order_string);
    virtual std::string get_byte_order() const { return d_byte_order; }
    virtual void set_byte_order(const std::string &byte_order_string)  { d_byte_order = byte_order_string; }

    // There are two main versions of add_chunk: One that takes a size and offset
    // and one that takes a fill value. However, for each of those, there are versions
    // that take a data URL (or not) and versions that take the 'chunk position in
    // array' information as a string or as a vector< uint64_t >. Thus, there are
    // a total of eight of these 'add_chunk()' functions. jhrg 4/22/22
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
            unsigned int filter_mask,
            const std::string &position_in_array);

     virtual unsigned long add_chunk(
            std::shared_ptr<http::url> d_data_url,
            const std::string &byte_order,
            unsigned long long size,
            unsigned long long offset,
            bool linked_block,
            unsigned int linked_block_index);

    virtual unsigned long add_chunk(
            std::shared_ptr<http::url> d_data_url,
            const std::string &byte_order,
            unsigned long long size,
            unsigned long long offset,
            const std::vector<unsigned long long> &position_in_array, bool multi_linked_blocks, unsigned int multi_linked_block_index_in_dmrpp_file);


    virtual unsigned long add_chunk(
            std::shared_ptr<http::url> d_data_url,
            const std::string &byte_order,
            unsigned long long size,
            unsigned long long offset,
            const std::vector<unsigned long long> &position_in_array);


    virtual unsigned long add_chunk(
            std::shared_ptr<http::url> d_data_url,
            const std::string &byte_order,
            unsigned long long size,
            unsigned long long offset,
            unsigned int filter_mask,
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
            bool linked_block,
            unsigned int linked_block_index);

    // Multi-linked block handling for build_dmrpp
    virtual unsigned long add_chunk(
            const std::string &byte_order,
            unsigned long long size,
            unsigned long long offset,
            const std::vector<unsigned long long> &position_in_array,
            bool multi_linked_blocks,
            unsigned int linked_block_index);


    virtual unsigned long add_chunk(
            const std::string &byte_order,
            unsigned long long size,
            unsigned long long offset,
            const std::vector<unsigned long long> &position_in_array);

    // Multi-linked block handling for retrieving the data

    virtual unsigned long add_chunk(
            std::shared_ptr<http::url> d_data_url,
            const std::string &byte_order,
            const std::string &position_in_array,
            const std::vector<std::pair<unsigned long long, unsigned long long>> &lb_offset_length);

    virtual unsigned long add_chunk(
            const std::string &byte_order,
            const std::string &position_in_array,
            const std::vector<std::pair<unsigned long long, unsigned long long>> &lb_offset_length);

    virtual unsigned long add_chunk(
            const std::string &byte_order,
            unsigned long long size,
            unsigned long long offset,
            unsigned int filter_mask,
            const std::vector<unsigned long long> &position_in_array);

    virtual unsigned long add_chunk(
            const std::string &byte_order,
            const std::string &fill_value,
            libdap::Type fv_type,
            unsigned long long chunk_size,
            const std::vector<unsigned long long> &position_in_array);

    // For structure, if we don't have structure offset information, we will calculate the structure size based on our best knowledge.
    virtual unsigned long add_chunk(
            const std::string &byte_order,
            const std::string &fill_value,
            libdap::Type fv_type,
            unsigned long long chunk_size,
            const std::vector<unsigned long long> &position_in_array,
            const std::vector<std::pair<libdap::Type,int>> &structure_type_element);

    // If we know the structure offset information, we will pass the structure offset to the chunk and this will be used for fill value handling.
    virtual unsigned long add_chunk(
            const std::string &byte_order,
            const std::string &fill_value,
            libdap::Type fv_type,
            unsigned long long chunk_size,
            const std::vector<unsigned long long> &position_in_array,
            unsigned int struct_size);


    void accumlate_storage_size(unsigned long long chunk_storage_size) {var_chunks_storage_size += chunk_storage_size; }
    unsigned long long get_var_chunks_storage_size() const {return var_chunks_storage_size; }
    virtual void dump(std::ostream & strm) const;
};

} // namespace dmrpp

#endif // _dmrpp_common_h

