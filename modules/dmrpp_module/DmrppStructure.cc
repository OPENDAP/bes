
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

#include "config.h"

#include <string>

#include <libdap/XMLWriter.h>
#include <libdap/util.h>   
#include <libdap/Array.h>

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppStructure.h"
#include "byteswap_compat.h"
#include "float_byteswap.h"
#include "Base64.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

DmrppStructure &
DmrppStructure::operator=(const DmrppStructure &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<Structure &>(*this) = rhs; // run Constructor=

    dynamic_cast<DmrppCommon &>(*this) = rhs;

    return *this;
}

bool
DmrppStructure::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);
    if (!get_chunks_loaded())
        load_chunks(this);

    if (read_p())
        return true;

    size_t value_size = 0;
    char *buf_value = read_atomic(name(),value_size);
    vector<char> values(buf_value,buf_value+value_size);
 
    size_t values_offset = 0;
    structure_read(values, values_offset,this->twiddle_bytes());
    set_read_p(true);

    return true;


}

void DmrppStructure::structure_read(vector<char> &values, size_t &values_offset, bool byte_swap) {

    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);
    Constructor::Vars_iter vi = this->var_begin();
    Constructor::Vars_iter ve = this->var_end();

    for (; vi != ve; vi++) {
        BaseType *bt = *vi;
        Type t_bt = bt->type();
        if (libdap::is_simple_type(t_bt) && t_bt != dods_str_c && t_bt != dods_url_c && t_bt!= dods_enum_c && t_bt!=dods_opaque_c) {

            BESDEBUG("dmrpp", "var name is: " << bt->name() << "'" << endl);
            BESDEBUG("dmrpp", "var values_offset is: " << values_offset << "'" << endl);
#if 0
            if(t_bt == dods_int32_c) {
                Int32 *val_int = static_cast<Int32 *>(bt);
                val_int->set_value(*((dods_int32*)(values.data()+values_offset)));
                BESDEBUG("dmrpp", "int value is: " << *((dods_int32*)(values.data()+values_offset)) << "'" << endl);
            }
            else if (t_bt == dods_float32_c) {
                Float32 *val_float = static_cast<Float32 *>(bt);
                val_float->set_value(*((dods_float32*)(values.data()+values_offset)));
                BESDEBUG("dmrpp", "float value is: " << *((dods_float32*)(values.data()+values_offset)) << "'" << endl);
            }
            else 
                bt->val2buf(values.data() + values_offset);
#endif

            if (byte_swap) {
               // Need to swap the bytes.
               auto stored_value = values.data() + values_offset;
               BESDEBUG("dmrpp", "swap bytes  " << endl);
               swap_bytes_in_structure(stored_value,t_bt,1);
               bt->val2buf(stored_value);
            }
            else 
               bt->val2buf(values.data() + values_offset);
            values_offset += bt->width_ll();
        }
        else if (t_bt == dods_array_c) {

            auto t_a = dynamic_cast<Array *>(bt);
            Type t_array_var = t_a->var()->type();
            if (libdap::is_simple_type(t_array_var) && t_array_var != dods_str_c && t_array_var != dods_url_c && t_array_var!= dods_enum_c && t_array_var!=dods_opaque_c) {
                if (byte_swap) {
                    // Need to swap the bytes.
                    BESDEBUG("dmrpp", "swap array bytes  " << endl);
                    auto stored_value = values.data() + values_offset;
                    swap_bytes_in_structure(stored_value,t_array_var,t_a->length_ll());
                    bt->val2buf(stored_value);
                }
                else 
                    t_a->val2buf(values.data()+values_offset);
                // update values_offset.
                values_offset +=t_a->width_ll();
            }
            else 
                throw InternalErr(__FILE__, __LINE__, "The base type of this structure is not integer or float.  Currently it is not supported.");
        } 
        else 
            throw InternalErr(__FILE__, __LINE__, "The base type of this structure is not integer or float.  Currently it is not supported.");
    }

}
void 
DmrppStructure::swap_bytes_in_structure(char *stored_value,Type dap_type,int64_t num) const{

    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);
    switch (dap_type) {
        case dods_int16_c:
        case dods_uint16_c: {
            auto *local = reinterpret_cast<dods_uint16*>(stored_value);
            while (num--) {
                *local = bswap_16(*local);
                local++;
            }
            break;
        }
        case dods_int32_c:
        case dods_uint32_c: {
            auto *local = reinterpret_cast<dods_uint32*>(stored_value);
            while (num--) {
                *local = bswap_32(*local);
                local++;
            }
            break;
        }
        case dods_int64_c:
        case dods_uint64_c: {
            auto *local = reinterpret_cast<dods_uint64*>(stored_value);
            while (num--) {
                *local = bswap_64(*local);
                local++;
            }
            break;
        }
        case dods_float32_c: {
            swap_float32(stored_value, num);
            break;
        }
        case dods_float64_c: {
            swap_float64(stored_value, num);
            break;
        }
        default: break; // Do nothing for all other types.
    }


}
void
DmrppStructure::set_send_p(bool state)
{
    if (!get_attributes_loaded())
        load_attributes(this);

    Structure::set_send_p(state);
}

void special_structure_data_xml_element(const XMLWriter &xml, DmrppStructure *ds) {

    if (ds->type() == dods_structure_c) {
        vector<char> struct_str_buf = ds->get_structure_str_buffer();
        //string temp_struct_array_str_buf(struct_array_str_buf.begin(),struct_array_str_buf.end());
        string final_encoded_str = base64::Base64::encode((uint8_t*)(struct_str_buf.data()),struct_str_buf.size());
        
        ds->print_special_structure_element(xml, DmrppCommon::d_ns_prefix, final_encoded_str);

    }

}


void 
DmrppStructure::print_dap4(libdap::XMLWriter &writer, bool constrained) {

    if (constrained && !this->send_p()) 
        return; 
 
    if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*)this->type_name().c_str()) < 0) 
        throw InternalErr(__FILE__, __LINE__, "Could not write " + this->type_name() + " element"); 
 
    if (!this->name().empty()) { 
        if (xmlTextWriterWriteAttribute(writer.get_writer(), (const xmlChar*) "name", (const xmlChar*)this->name().c_str()) < 0) 
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name"); 
    } 

 
    if (this->is_dap4()) 
        this->attributes()->print_dap4(writer); 
    
    if (!this->is_dap4() && this->get_attr_table().get_size() > 0) 
        this->get_attr_table().print_xml_writer(writer); 

    Constructor::Vars_iter vi = this->var_begin(); 
    Constructor::Vars_iter ve = this->var_end();

    for (; vi != ve; vi++) {
        BaseType *bt = *vi;
        bt->print_dap4(writer);
    }

    if (DmrppCommon::d_print_chunks && (get_chunks_size() > 0 || get_uses_fill_value()))
        print_chunks_element(writer, DmrppCommon::d_ns_prefix);

    // Special structure string array.
    if (DmrppCommon::d_print_chunks && get_special_structure_flag() && read_p()) {
        special_structure_data_xml_element(writer, this);
    }

       // print scalar value for compact storage.
       // TODO: add this later.
    if (xmlTextWriterEndElement(writer.get_writer()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not end " + this->type_name() + " element");

}
  

void DmrppStructure::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppStructure::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    Structure::dump(strm);
    strm << BESIndent::LMarg << "value:    " << "----" << /*d_buf <<*/ endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp

