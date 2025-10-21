//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////
#ifndef __NCML_MODULE__RENAMED_ARRAY_WRAPPER_H__
#define __NCML_MODULE__RENAMED_ARRAY_WRAPPER_H__

#include "config.h"
#include <libdap/Array.h>
#include <vector>

using namespace libdap;

namespace libdap {
class DDS;
class Marshaller;
class UnMarshaller;
}

namespace ncml_module {
/**
 * @brief A Decorator Pattern for wrapping a libdap::Array in order to change its
 * name efficiently in the face of buggy subclasses we cannot change
 *
 * Unfortunately, this is needed to get around problems with subclasses requiring the old name for lazy loads
 * even though set_name was called.  This causes them to exception, which is bad.
 * In particular, NCArray doesn't handle  set_name properly and will fail to serialize.
 *
 * Rather than seek out and make changes to fix these bugs, the temporary solution will be to
 * wrap the Array to rename in this class.
 * Almost all virtual functions will be passed through to the wrapped array,
 * which will retain its original name for purposes of read(), but will be forced
 * to use the new name when serializing.
 *
 */
class RenamedArrayWrapper: public libdap::Array {
public:
    RenamedArrayWrapper();
    RenamedArrayWrapper(const RenamedArrayWrapper& proto);

    /** Hand off the given Array* to be wrapped by this class.
     *  The memory is relinquished to this class.
     */
    RenamedArrayWrapper(libdap::Array* toBeWrapped);

    virtual ~RenamedArrayWrapper();

    virtual RenamedArrayWrapper* ptr_duplicate();
    RenamedArrayWrapper& operator=(const RenamedArrayWrapper& rhs);

    // Specializations

    virtual void add_constraint(Dim_iter i, int start, int stride, int stop);
    virtual void reset_constraint();

    /** @deprecated */
    virtual void clear_constraint();

    virtual string toString();
    virtual string toString() const;
    void dump(ostream &strm) const override;

    // Don't need to override this, it does what we want.
    // virtual void set_name(const string &n);
#if 0

    virtual bool is_simple_type() const;
    virtual bool is_vector_type() const;
    virtual bool is_constructor_type() const;

    virtual bool synthesized_p();
    virtual void set_synthesized_p(bool state);

    virtual int element_count(bool leaves = false);
#endif

    virtual bool read_p();
    virtual void set_read_p(bool state);

    virtual bool send_p();
    virtual void set_send_p(bool state);
#if 0

    virtual libdap::AttrTable& get_attr_table();
    virtual void set_attr_table(const libdap::AttrTable &at);

    virtual bool is_in_selection();
    virtual void set_in_selection(bool state);

    virtual void set_parent(BaseType *parent);
    virtual BaseType *get_parent() const;
#endif

    virtual BaseType *var(const string &name = "", bool exact_match = true, btp_stack *s = 0);
    virtual BaseType *var(const string &name, btp_stack &s);
    virtual void add_var(BaseType *bt, Part part = nil);
    virtual void add_var_nocopy(BaseType *bt, Part part = nil);

#if 0
    virtual bool check_semantics(string &msg, bool all = false);
    virtual bool ops(BaseType *b, int op);
#endif

#if FILE_METHODS // from BaseType.h, whether we include FILE* methods
    virtual void print_decl(FILE *out, string space = "    ",
        bool print_semi = true,
        bool constraint_info = false,
        bool constrained = false);
    virtual void print_xml(FILE *out, string space = "    ",
        bool constrained = false);
    virtual void print_val(FILE *out, string space = "",
        bool print_decl_p = true);
#endif // FILE_METHODS

#if 0
    virtual void print_decl(ostream &out, string space = "    ",
        bool print_semi = true,
        bool constraint_info = false,
        bool constrained = false);
    virtual void print_xml(ostream &out, string space = "    ",
        bool constrained = false);
    virtual void print_val(ostream &out, string space = "",
        bool print_decl_p = true);

    virtual unsigned int width(bool constrained = false);
#endif

    virtual unsigned int buf2val(void **val);
    virtual unsigned int val2buf(void *val, bool reuse = false);

    virtual bool set_value(dods_byte *val, int sz);
    virtual bool set_value(dods_int8 *val, int sz);
    virtual bool set_value(dods_int16 *val, int sz);
    virtual bool set_value(dods_uint16 *val, int sz);
    virtual bool set_value(dods_int32 *val, int sz);
    virtual bool set_value(dods_uint32 *val, int sz);
    virtual bool set_value(dods_int64 *val, int sz);
    virtual bool set_value(dods_uint64 *val, int sz);
    virtual bool set_value(dods_float32 *val, int sz);
    virtual bool set_value(dods_float64 *val, int sz);
    virtual bool set_value(string *val, int sz);

    virtual bool set_value(vector<dods_byte> &val, int sz);
    virtual bool set_value(vector<dods_int8> &val, int sz);
    virtual bool set_value(vector<dods_int16> &val, int sz);
    virtual bool set_value(vector<dods_uint16> &val, int sz);
    virtual bool set_value(vector<dods_int32> &val, int sz);
    virtual bool set_value(vector<dods_uint32> &val, int sz);
    virtual bool set_value(vector<dods_int64> &val, int sz);
    virtual bool set_value(vector<dods_uint64> &val, int sz);
    virtual bool set_value(vector<dods_float32> &val, int sz);
    virtual bool set_value(vector<dods_float64> &val, int sz);
    virtual bool set_value(vector<string> &val, int sz);

    virtual void value(dods_byte *b) const;
    virtual void value(dods_int8 *b) const;
    virtual void value(dods_int16 *b) const;
    virtual void value(dods_uint16 *b) const;
    virtual void value(dods_int32 *b) const;
    virtual void value(dods_uint32 *b) const;
    virtual void value(dods_int64 *b) const;
    virtual void value(dods_uint64 *b) const;
    virtual void value(dods_float32 *b) const;
    virtual void value(dods_float64 *b) const;
    virtual void value(vector<string> &b) const;

    virtual void value(vector<unsigned int> *indices, dods_byte *b) const;
    virtual void value(vector<unsigned int> *indices, dods_int8 *b) const;
    virtual void value(vector<unsigned int> *indices, dods_int16 *b) const;
    virtual void value(vector<unsigned int> *indices, dods_uint16 *b) const;
    virtual void value(vector<unsigned int> *indices, dods_int32 *b) const;
    virtual void value(vector<unsigned int> *indices, dods_uint32 *b) const;
    virtual void value(vector<unsigned int> *indices, dods_int64 *b) const;
    virtual void value(vector<unsigned int> *indices, dods_uint64 *b) const;
    virtual void value(vector<unsigned int> *indices, dods_float32 *b) const;
    virtual void value(vector<unsigned int> *indices, dods_float64 *b) const;
    virtual void value(vector<unsigned int> *index, vector<string> &b) const;

    virtual void *value();

    virtual bool read();
    virtual void intern_data(ConstraintEvaluator &eval, DDS &dds);
    virtual bool serialize(ConstraintEvaluator &eval, DDS &dds, Marshaller &m, bool ce_eval = true);
    virtual bool deserialize(UnMarshaller &um, DDS *dds, bool reuse = false);

private:
    // Private methods

    /** Copy the local privates from proto */
    void copyLocalRepFrom(const RenamedArrayWrapper& proto);

    /** Clean local rep */
    void destroy();

#if 0
    /** Set the wrapped array to have name() == this->name() for now. */
    void withNewName();

    /** Set the wrapped array to have name() == this->_orgName for now */
    void withOrgName();
#endif

    /** Force the local shape (including constraints) into the wrapped array.
     * We use this helper in almost every function, but feel this is OK
     * since it's:
     * 1) Easier to maintain (not dealing with making many Array funcs
     *    virtual and keeping state in sync that way
     * 2) We don't expect that the syncing will really take that long
     *    compared to other calls and should amortize
     * 3) It's not clear how often this class will be used.
     **/
    void syncConstraints() const
    {
        const_cast<RenamedArrayWrapper*>(this)->syncConstraints();
    }
    void syncConstraints();


    template <typename T> void value_worker(T *v) const;
    template <typename T> void value_worker(vector<unsigned int> *indices, T *b) const;

    template <typename T> bool set_value_worker(T *v, int sz);
    template <typename T> bool set_value_worker(vector<T> &v, int sz);

private:
    // Data rep

    /** the Array we are decorating.  WE OWN THIS MEMORY once it is passed in and must
     * clean it up on destroy() */
    libdap::Array* _pArray;
    string _orgName; // the original, underlying name of the array, cached.
};

}

#endif /* __NCML_MODULE__RENAMED_ARRAY_WRAPPER_H__ */
