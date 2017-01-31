// -*- mode: c++; c-basic-offset:4 -*-
//
// W10nJsonTransform.cc
//
// This file is part of BES JSON File Out Module
//
// Copyright (c) 2014 OPeNDAP, Inc.
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
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "W10NNames.h"
#include "W10nJsonTransform.h"
#include "config.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <stddef.h>
#include <string>
#include <typeinfo>

using std::ostringstream;
using std::istringstream;

#include <DDS.h>
#include <Type.h>
#include <Structure.h>
#include <Constructor.h>
#include <Array.h>
#include <Grid.h>
#include <Str.h>
#include <Sequence.h>
#include <Str.h>
#include <Url.h>

#include <BESDebug.h>
#include <BESInternalError.h>
#include <BESContextManager.h>
#include <BESSyntaxUserError.h>

#include <w10n_utils.h>

/**
 *  @TODO Handle String and URL Arrays including backslash escaping double quotes in values.
 *
 */
template<typename T>
unsigned int W10nJsonTransform::json_simple_type_array_worker(ostream *strm, T *values,
    unsigned int indx, vector<unsigned int> *shape, unsigned int currentDim, bool flatten)
{
    if (currentDim == 0 || !flatten) *strm << "[";

    unsigned int currentDimSize = (*shape)[currentDim];

    for (unsigned int i = 0; i < currentDimSize; i++) {
        if (currentDim < shape->size() - 1) {
            indx = json_simple_type_array_worker<T>(strm, values, indx, shape, currentDim + 1, flatten);
            if (i + 1 != currentDimSize) *strm << ", ";
        }
        else {
            if (i) *strm << ", ";
            if (typeid(T) == typeid(std::string)) {
                // Strings need to be escaped to be included in a JSON object.
                // std::string val = ((std::string *) values)[indx++]; replaced w/below jhrg 9/7/16
                std::string val = reinterpret_cast<std::string*>(values)[indx++];
                *strm << "\"" << w10n::escape_for_json(val) << "\"";
            }
            else {
                *strm << values[indx++];
            }
        }
    }

    if (currentDim == 0 || !flatten) *strm << "]";

    return indx;
}

void W10nJsonTransform::json_array_starter(ostream *strm, libdap::Array *a, std::string indent)
{

    bool found_w10n_callback = false;
    std::string w10n_callback = BESContextManager::TheManager()->get_context(W10N_CALLBACK_KEY, found_w10n_callback);
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::json_simple_type_array() - w10n_callback: "<< w10n_callback << endl);

    BESDEBUG(W10N_DEBUG_KEY,
        "W10nJsonTransform::json_simple_type_array() - Processing Array of " << a->var()->type_name() << endl);

    if (found_w10n_callback) {
        *strm << w10n_callback << "(";
    }

    *strm << "{" << endl;

    std::string child_indent = indent + _indent_increment;

    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::json_simple_type_array() - Writing variable metadata..." << endl);

    writeVariableMetadata(strm, a, child_indent);
    *strm << "," << endl;

    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::json_simple_type_array() - Writing variable data..." << endl);

    // Data
    *strm << child_indent << "\"data\": ";

}
void W10nJsonTransform::json_array_ender(ostream *strm, std::string indent)
{

    bool found_w10n_meta_object = false;
    std::string w10n_meta_object = BESContextManager::TheManager()->get_context(W10N_META_OBJECT_KEY,
        found_w10n_meta_object);
    BESDEBUG(W10N_DEBUG_KEY,
        "W10nJsonTransform::json_simple_type_array_ender() - w10n_meta_object: "<< w10n_meta_object << endl);

    bool found_w10n_callback = false;
    std::string w10n_callback = BESContextManager::TheManager()->get_context(W10N_CALLBACK_KEY, found_w10n_callback);
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::json_simple_type_array() - w10n_callback: "<< w10n_callback << endl);

    std::string child_indent = indent + _indent_increment;

    if (found_w10n_meta_object)
        *strm << "," << endl << child_indent << w10n_meta_object << endl;
    else
        *strm << endl;

    *strm << indent << "}" << endl;

    if (found_w10n_callback) {
        *strm << ")";
    }

    *strm << endl;

}
/**
 * Writes the w10n json representation of the passed DAP Array of simple types. If the
 * parameter "sendData" evaluates to true then data will also be sent.
 */
template<typename T> void W10nJsonTransform::json_simple_type_array_sender(ostream *strm, libdap::Array *a)
{

    bool found_w10n_flatten = false;
    std::string w10n_flatten = BESContextManager::TheManager()->get_context(W10N_FLATTEN_KEY, found_w10n_flatten);
    BESDEBUG(W10N_DEBUG_KEY,
        "W10nJsonTransform::json_simple_type_array_sender() - w10n_flatten: "<< w10n_flatten << endl);

    int numDim = a->dimensions(true);
    vector<unsigned int> shape(numDim);
    long length = w10n::computeConstrainedShape(a, &shape);

    vector<T> src(length);
    a->value(&src[0]);
    unsigned int indx = json_simple_type_array_worker(strm, &src[0], 0, &shape, 0, found_w10n_flatten);

    if (length != indx)
        BESDEBUG(W10N_DEBUG_KEY,
            "json_simple_type_array_sender() - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);

}

/**
 * Writes the w10n json representation of the passed DAP Array of simple types. If the
 * parameter "sendData" evaluates to true then data will also be sent.
 */
void W10nJsonTransform::json_string_array_sender(ostream *strm, libdap::Array *a)
{

    bool found_w10n_flatten = false;
    std::string w10n_flatten = BESContextManager::TheManager()->get_context(W10N_FLATTEN_KEY, found_w10n_flatten);
    BESDEBUG(W10N_DEBUG_KEY,
        "W10nJsonTransform::json_simple_type_array_sender() - w10n_flatten: "<< w10n_flatten << endl);

    int numDim = a->dimensions(true);
    vector<unsigned int> shape(numDim);
    long length = w10n::computeConstrainedShape(a, &shape);

    // The string type utilizes a specialized version of libdap:Array.value()
    vector<std::string> sourceValues;
    a->value(sourceValues);
    unsigned int indx = json_simple_type_array_worker(strm, (std::string *) (&sourceValues[0]), 0, &shape, 0,
        found_w10n_flatten);

    if (length != indx)
        BESDEBUG(W10N_DEBUG_KEY,
            "json_simple_type_array_sender() - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);

}

/**
 * Writes the w10n json representation of the passed DAP Array of simple types. If the
 * parameter "sendData" evaluates to true then data will also be sent.
 */
template<typename T> void W10nJsonTransform::json_simple_type_array(ostream *strm, libdap::Array *a, std::string indent)
{
    json_array_starter(strm, a, indent);
    json_simple_type_array_sender<T>(strm, a);
    json_array_ender(strm, indent);
}

/**
 * Writes the w10n json representation of the passed DAP Array of simple types. If the
 * parameter "sendData" evaluates to true then data will also be sent.
 */
void W10nJsonTransform::json_string_array(ostream *strm, libdap::Array *a, std::string indent)
{
    json_array_starter(strm, a, indent);
    json_string_array_sender(strm, a);
    json_array_ender(strm, indent);
}

/**
 * Writes the w10n json opener for the Dataset, including name and top level DAP attributes.
 */
void W10nJsonTransform::writeDatasetMetadata(ostream *strm, libdap::DDS *dds, std::string indent)
{
    // Name
    *strm << indent << "\"name\": \"" << dds->get_dataset_name() << "\"," << endl;

    //Attributes
    writeAttributes(strm, dds->get_attr_table(), indent);
    *strm << "," << endl;
}

/**
 * Writes w10n json opener for a DAP object that is seen as a "leaf" in w10n semantics.
 * Header includes object name. attributes, and w10n type.
 */
void W10nJsonTransform::writeVariableMetadata(ostream *strm, libdap::BaseType *bt, std::string indent)
{

    // Name
    *strm << indent << "\"name\": \"" << bt->name() << "\"," << endl;
    libdap::BaseType *var = bt;

    // w10n type
    if (bt->type() == libdap::dods_array_c) {
        libdap::Array *a = (libdap::Array *) bt;
        var = a->var();
    }
    if (!var->is_constructor_type()) *strm << indent << "\"type\": \"" << var->type_name() << "\"," << endl;

    //Attributes
    writeAttributes(strm, bt->get_attr_table(), indent);

}

/** @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
 *
 * @param dds DataDDS object that contains the data structure, attributes
 * and data
 * @param dhi The data interface containing information about the current
 * request
 * @param localfile netcdf to create and write the information to
 * @throws BESInternalError if dds provided is empty or not read, if the
 * file is not specified or failed to create the netcdf file
 */
W10nJsonTransform::W10nJsonTransform(libdap::DDS *dds, BESDataHandlerInterface &, const std::string &localfile) :
    _dds(dds), _localfile(localfile), _indent_increment("  "), _ostrm(0), _usingTempFile(false)
{
    if (!_dds) {
        std::string msg = "W10nJsonTransform:  ERROR! A null DDS reference was passed to the constructor";
        BESDEBUG(W10N_DEBUG_KEY, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    if (_localfile.empty()) {
        std::string msg = "W10nJsonTransform:  An empty local file name passed to constructor";
        BESDEBUG(W10N_DEBUG_KEY, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
}

W10nJsonTransform::W10nJsonTransform(libdap::DDS *dds, BESDataHandlerInterface &, std::ostream *ostrm) :
    _dds(dds), _localfile(""), _indent_increment("  "), _ostrm(ostrm), _usingTempFile(false)
{
    if (!_dds) {
        std::string msg = "W10nJsonTransform:  ERROR! A null DDS reference was passed to the constructor";
        BESDEBUG(W10N_DEBUG_KEY, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    if (!_ostrm) {
        std::string msg = "W10nJsonTransform:  ERROR! A null std::ostream pointer was passed to the constructor";
        BESDEBUG(W10N_DEBUG_KEY, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
}

/** @brief Destructor
 *
 * Cleans up any temporary data created during the transformation
 */
W10nJsonTransform::~W10nJsonTransform()
{
}

/** @brief dumps information about this transformation object for debugging
 * purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including all of the FoJson objects converted from DAP objects that are
 * to be sent to the netcdf file.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void W10nJsonTransform::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "W10nJsonTransform::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "temporary file = " << _localfile << endl;
    if (_dds != 0) {
        _dds->print(strm);
    }
    BESIndent::UnIndent();
}

/**
 * Write the w10n json representation of the passed DAP AttrTable instance.
 * Supports multi-valued attributes and nested attributes.
 */
void W10nJsonTransform::writeAttributes(ostream *strm, libdap::AttrTable &attr_table, std::string indent)
{

    std::string child_indent = indent + _indent_increment;

    // Start the attributes block
    *strm << indent << "\"attributes\": [";

//	if(attr_table.get_name().length()>0)
//		*strm  << endl << child_indent << "{\"name\": \"name\", \"value\": \"" << attr_table.get_name() << "\"},";

// Only do more if there are actually attributes in the table
    if (attr_table.get_size() != 0) {
        *strm << endl;
        libdap::AttrTable::Attr_iter begin = attr_table.attr_begin();
        libdap::AttrTable::Attr_iter end = attr_table.attr_end();

        for (libdap::AttrTable::Attr_iter at_iter = begin; at_iter != end; at_iter++) {

            switch (attr_table.get_attr_type(at_iter)) {
            case libdap::Attr_container: {
                libdap::AttrTable *atbl = attr_table.get_attr_table(at_iter);

                // not first thing? better use a comma...
                if (at_iter != begin) *strm << "," << endl;

                // Attribute Containers need to be opened and then a recursive call gets made
                *strm << child_indent << "{" << endl;

                // If the table has a name, write it out as a json property.
                if (atbl->get_name().length() > 0)
                    *strm << child_indent + _indent_increment << "\"name\": \"" << atbl->get_name() << "\"," << endl;

                // Recursive call for child attribute table.
                writeAttributes(strm, *atbl, child_indent + _indent_increment);
                *strm << endl << child_indent << "}";

                break;

            }
            default: {
                // not first thing? better use a comma...
                if (at_iter != begin) *strm << "," << endl;

                // Open attribute object, write name
                *strm << child_indent << "{\"name\": \"" << attr_table.get_name(at_iter) << "\", ";

                // Open value array
                *strm << "\"value\": [";
                vector<std::string> *values = attr_table.get_attr_vector(at_iter);
                // write values
                for (std::vector<std::string>::size_type i = 0; i < values->size(); i++) {

                    // not first thing? better use a comma...
                    if (i > 0) *strm << ",";

                    // Escape the double quotes found in String and URL type attribute values.
                    if (attr_table.get_attr_type(at_iter) == libdap::Attr_string
                        || attr_table.get_attr_type(at_iter) == libdap::Attr_url) {
                        *strm << "\"";
                        std::string value = (*values)[i];
                        *strm << w10n::escape_for_json(value);
                        *strm << "\"";
                    }
                    else {

                        *strm << (*values)[i];
                    }

                }
                // close value array
                *strm << "]}";
                break;
            }

            }
        }
        *strm << endl << indent;

    }

    // close AttrTable JSON

    *strm << "]";

}

std::ostream *W10nJsonTransform::getOutputStream()
{
    // used to ensure the _ostrm is closed only when it's a temp file
    _usingTempFile = false;
    std::fstream _tempFile;

    if (!_ostrm) {
        _tempFile.open(_localfile.c_str(), std::fstream::out);
        if (!_tempFile) {
            std::string msg = "Could not open temp file: " + _localfile;
            BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::getOutputStream() - ERROR! "<< msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }
        _ostrm = &_tempFile;
        _usingTempFile = true;
    }

    return _ostrm;
}

void W10nJsonTransform::releaseOutputStream()
{
    if (_usingTempFile) {
        ((std::fstream *) _ostrm)->close();
        _ostrm = 0;
    }
}

void W10nJsonTransform::sendW10nMetaForDDS()
{

    std::ostream *strm = getOutputStream();
    try {
        sendW10nMetaForDDS(strm, _dds, "");
        releaseOutputStream();
    }
    catch (...) {
        releaseOutputStream();
        throw;
    }

}

void W10nJsonTransform::sendW10nMetaForDDS(ostream *strm, libdap::DDS *dds, std::string indent)
{

    bool found_w10n_meta_object = false;
    std::string w10n_meta_object = BESContextManager::TheManager()->get_context(W10N_META_OBJECT_KEY,
        found_w10n_meta_object);
    BESDEBUG(W10N_DEBUG_KEY,
        "W10nJsonTransform::json_simple_type_array() - w10n_meta_object: "<< w10n_meta_object << endl);

    bool found_w10n_callback = false;
    std::string w10n_callback = BESContextManager::TheManager()->get_context(W10N_CALLBACK_KEY, found_w10n_callback);
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::json_simple_type_array() - w10n_callback: "<< w10n_callback << endl);

    /**
     * w10 sees the world in terms of leaves and nodes. Leaves have data, nodes have other nodes and leaves.
     */
    vector<libdap::BaseType *> leaves;
    vector<libdap::BaseType *> nodes;

    libdap::DDS::Vars_iter vi = dds->var_begin();
    libdap::DDS::Vars_iter ve = dds->var_end();
    for (; vi != ve; vi++) {
        libdap::BaseType *v = *vi;
        if (v->send_p()) {
            libdap::Type type = v->type();
            if (type == libdap::dods_array_c) {
                type = v->var()->type();
            }
            if (v->is_constructor_type() || (v->is_vector_type() && v->var()->is_constructor_type())) {
                nodes.push_back(v);
            }
            else {
                leaves.push_back(v);
            }
        }
    }

    if (found_w10n_callback) {
        *strm << w10n_callback << "(";
    }

    // Declare the top level node
    *strm << "{" << endl;
    std::string child_indent = indent + _indent_increment;

    // Write the top level node's metadata (name & attributes)
    writeDatasetMetadata(strm, dds, child_indent);

    // Write down the leaves
    *strm << child_indent << "\"leaves\": [";
    if (leaves.size() > 0) *strm << endl;
    for (std::vector<libdap::BaseType *>::size_type l = 0; l < leaves.size(); l++) {
        libdap::BaseType *v = leaves[l];
        BESDEBUG(W10N_DEBUG_KEY, "Processing LEAF: " << v->name() << endl);
        if (l > 0) {
            *strm << "," << endl;
        }

        sendW10nMetaForVariable(strm, v, child_indent + _indent_increment, false);
    }
    if (leaves.size() > 0) *strm << endl << child_indent;
    *strm << "]," << endl;

    // Write down the child nodes
    *strm << child_indent << "\"nodes\": [";
    if (nodes.size() > 0) *strm << endl;
    for (std::vector<libdap::BaseType *>::size_type n = 0; n < nodes.size(); n++) {
        libdap::BaseType *v = nodes[n];
        BESDEBUG(W10N_DEBUG_KEY, "Processing NODE: " << v->name() << endl);
        if (n > 0) {
            *strm << "," << endl;
        }
        sendW10nMetaForVariable(strm, v, child_indent + _indent_increment, false);
    }
    if (nodes.size() > 0) *strm << endl << child_indent;

    *strm << "]";

    if (found_w10n_meta_object)
        *strm << "," << endl << child_indent << w10n_meta_object << endl;
    else
        *strm << endl;

    *strm << "}";

    if (found_w10n_callback) {
        *strm << ")";
    }

    *strm << endl;

}

void W10nJsonTransform::sendW10nMetaForVariable(ostream *strm, libdap::BaseType *bt, std::string indent, bool isTop)
{

    bool found_w10n_meta_object = false;
    std::string w10n_meta_object = BESContextManager::TheManager()->get_context(W10N_META_OBJECT_KEY,
        found_w10n_meta_object);
    BESDEBUG(W10N_DEBUG_KEY,
        "W10nJsonTransform::json_simple_type_array() - w10n_meta_object: "<< w10n_meta_object << endl);

    bool found_w10n_callback = false;
    std::string w10n_callback = BESContextManager::TheManager()->get_context(W10N_CALLBACK_KEY, found_w10n_callback);
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::json_simple_type_array() - w10n_callback: "<< w10n_callback << endl);

    bool found_w10n_flatten = false;
    std::string w10n_flatten = BESContextManager::TheManager()->get_context(W10N_FLATTEN_KEY, found_w10n_flatten);
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::json_simple_type_array() - w10n_flatten: "<< w10n_flatten << endl);

    bool found_w10n_traverse = false;
    std::string w10n_traverse = BESContextManager::TheManager()->get_context(W10N_TRAVERSE_KEY, found_w10n_traverse);
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::json_simple_type_array() - w10n_traverse: "<< w10n_traverse << endl);

    if (isTop && found_w10n_callback) {
        *strm << w10n_callback << "(";
    }

    *strm << indent << "{" << endl;\
    std::string child_indent = indent + _indent_increment;

    writeVariableMetadata(strm, bt, child_indent);

    if (bt->type() == libdap::dods_array_c) {
        *strm << "," << endl;

        libdap::Array *a = (libdap::Array *) bt;
        int numDim = a->dimensions(true);
        vector<unsigned int> shape(numDim);
        long length = w10n::computeConstrainedShape(a, &shape);

        if (found_w10n_flatten) {
            *strm << child_indent << "\"shape\": [" << length << "]";

        }
        else {
            *strm << child_indent << "\"shape\": [";
            for (std::vector<unsigned int>::size_type i = 0; i < shape.size(); i++) {
                if (i > 0) *strm << ",";
                *strm << shape[i];
            }
            *strm << "]";
        }
    }
    else {
        if (bt->is_constructor_type() && (found_w10n_traverse || isTop)) {
            *strm << "," << endl;

            libdap::Constructor *ctor = (libdap::Constructor *) bt;

            vector<libdap::BaseType *> leaves;
            vector<libdap::BaseType *> nodes;
            libdap::Constructor::Vars_iter vi = ctor->var_begin();
            libdap::Constructor::Vars_iter ve = ctor->var_end();
            for (; vi != ve; vi++) {
                libdap::BaseType *v = *vi;
                if (v->send_p()) {
                    libdap::Type type = v->type();
                    if (type == libdap::dods_array_c) {
                        type = v->var()->type();
                    }
                    if (v->is_constructor_type() || (v->is_vector_type() && v->var()->is_constructor_type())) {
                        nodes.push_back(v);
                    }
                    else {
                        leaves.push_back(v);
                    }
                }
            }

            // Write down the leaves
            *strm << child_indent << "\"leaves\": [";
            if (leaves.size() > 0) *strm << endl;
            for (std::vector<libdap::BaseType *>::size_type l = 0; l < leaves.size(); l++) {
                libdap::BaseType *v = leaves[l];
                BESDEBUG(W10N_DEBUG_KEY, "Processing LEAF: " << v->name() << endl);
                if (l > 0) {
                    *strm << ",";
                    *strm << endl;
                }

                sendW10nMetaForVariable(strm, v, child_indent + _indent_increment, false);
            }
            if (leaves.size() > 0) *strm << endl << child_indent;
            *strm << "]," << endl;

            // Write down the child nodes
            *strm << child_indent << "\"nodes\": [";
            if (nodes.size() > 0) *strm << endl;
            for (std::vector<libdap::BaseType *>::size_type n = 0; n < nodes.size(); n++) {
                libdap::BaseType *v = nodes[n];
                BESDEBUG(W10N_DEBUG_KEY, "Processing NODE: " << v->name() << endl);
                if (n > 0) {
                    *strm << "," << endl;
                }
                sendW10nMetaForVariable(strm, v, child_indent + _indent_increment, false);
            }
            if (nodes.size() > 0) *strm << endl << child_indent;

            *strm << "]";

        }
        else {
            if (!bt->is_constructor_type()) {
                // *strm << endl;
                // *strm << "," << endl;
                // *strm << child_indent << "\"shape\": [1]";
            }

        }
    }

    if (isTop && found_w10n_meta_object) {
        *strm << "," << endl << child_indent << w10n_meta_object << endl;
    }

    *strm << endl << indent << "}";

    if (isTop && found_w10n_callback) {
        *strm << ")";
    }

}

void W10nJsonTransform::sendW10nMetaForVariable(std::string &vName, bool isTop)
{

    libdap::BaseType *bt = _dds->var(vName);

    if (!bt) {
        std::string msg = "The dataset does not contain a variable named '" + vName + "'";
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::sendW10nMetaForVariable() - ERROR! " << msg << endl);
        throw BESSyntaxUserError(msg, __FILE__, __LINE__);
    }

    std::ostream *strm = getOutputStream();
    try {
        sendW10nMetaForVariable(strm, bt, "", isTop);
        *strm << endl;
        releaseOutputStream();
    }
    catch (...) {
        releaseOutputStream();
        throw;
    }

}

void W10nJsonTransform::sendW10nDataForVariable(std::string &vName)
{

    libdap::BaseType *bt = _dds->var(vName);

    if (!bt) {
        std::string msg = "The dataset does not contain a variable named '" + vName + "'";
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::sendW10nDataForVariable() - ERROR! " << msg << endl);
        throw BESSyntaxUserError(msg, __FILE__, __LINE__);
    }

    std::ostream *strm = getOutputStream();
    try {
        sendW10nDataForVariable(strm, bt, "");
        releaseOutputStream();
    }
    catch (...) {
        releaseOutputStream();
        throw;
    }

}

void W10nJsonTransform::sendW10nDataForVariable(ostream *strm, libdap::BaseType *bt, std::string indent)
{

    if (bt->is_simple_type()) {

        sendW10nData(strm, bt, indent);

    }
    else if (bt->type() == libdap::dods_array_c && bt->var()->is_simple_type()) {
        sendW10nData(strm, (libdap::Array *) bt, indent);

    }
    else {
        std::string msg = "The variable '" + bt->name() + "' is not a simple type or an Array of simple types. ";
        msg += "The w10n protocol does not support the transmission of data for complex types.";
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::sendW10nDataForVariable() - ERROR! " << msg << endl);
        throw BESSyntaxUserError(msg, __FILE__, __LINE__);
    }

}

/**
 * Write the w10n json data for the passed BaseType instance - which had better be one of the
 * atomic DAP types.
 */
void W10nJsonTransform::sendW10nData(ostream *strm, libdap::BaseType *b, std::string indent)
{

    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::sendW10nData() - Sending data for simple type "<< b->name() << endl);

    bool found_w10n_meta_object = false;
    std::string w10n_meta_object = BESContextManager::TheManager()->get_context(W10N_META_OBJECT_KEY,
        found_w10n_meta_object);
    BESDEBUG(W10N_DEBUG_KEY,
        "W10nJsonTransform::json_simple_type_array() - w10n_meta_object: "<< w10n_meta_object << endl);

    bool found_w10n_callback = false;
    std::string w10n_callback = BESContextManager::TheManager()->get_context(W10N_CALLBACK_KEY, found_w10n_callback);
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::json_simple_type_array() - w10n_callback: "<< w10n_callback << endl);

    bool found_w10n_flatten = false;
    std::string w10n_flatten = BESContextManager::TheManager()->get_context(W10N_FLATTEN_KEY, found_w10n_flatten);
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::json_simple_type_array() - w10n_flatten: "<< w10n_flatten << endl);

    std::string child_indent = indent + _indent_increment;

    if (found_w10n_callback) {
        *strm << w10n_callback << "(";
    }

    *strm << "{" << endl;

    writeVariableMetadata(strm, b, child_indent);
    *strm << "," << endl;

    *strm << child_indent << "\"data\": ";

    if (b->type() == libdap::dods_str_c || b->type() == libdap::dods_url_c) {
        libdap::Str *strVar = (libdap::Str *) b;
        *strm << "\"" << w10n::escape_for_json(strVar->value()) << "\"";
    }
    else {
        b->print_val(*strm, "", false);
    }

    if (found_w10n_meta_object)
        *strm << "," << endl << child_indent << w10n_meta_object << endl;
    else
        *strm << endl;

    *strm << "}";

    if (found_w10n_callback) {
        *strm << ")";
    }
    *strm << endl;

    // *strm << "]";

}

void W10nJsonTransform::sendW10nData(ostream *strm, libdap::Array *a, std::string indent)
{

    BESDEBUG(W10N_DEBUG_KEY,
        "W10nJsonTransform::transform() - Processing Array. " << " a->type_name(): " << a->type_name() << " a->var()->type_name(): " << a->var()->type_name() << endl);

    switch (a->var()->type()) {
    // Handle the atomic types - that's easy!
    case libdap::dods_byte_c:
        json_simple_type_array<libdap::dods_byte>(strm, a, indent);
        break;

    case libdap::dods_int16_c:
        json_simple_type_array<libdap::dods_int16>(strm, a, indent);
        break;

    case libdap::dods_uint16_c:
        json_simple_type_array<libdap::dods_uint16>(strm, a, indent);
        break;

    case libdap::dods_int32_c:
        json_simple_type_array<libdap::dods_int32>(strm, a, indent);
        break;

    case libdap::dods_uint32_c:
        json_simple_type_array<libdap::dods_uint32>(strm, a, indent);
        break;

    case libdap::dods_float32_c:
        json_simple_type_array<libdap::dods_float32>(strm, a, indent);
        break;

    case libdap::dods_float64_c:
        json_simple_type_array<libdap::dods_float64>(strm, a, indent);
        break;

    case libdap::dods_str_c: {
        json_string_array(strm, a, indent);
        break;
#if 0
        string s = (string) "W10nJsonTransform:  Arrays of String objects not a supported return type.";
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::sendW10nMetaForVariable() - ERROR! " << s << endl);
        throw BESInternalError(s, __FILE__, __LINE__);
        break;
#endif
    }

    case libdap::dods_url_c: {
        json_string_array(strm, a, indent);
        break;

#if 0
        string s = (string) "W10nJsonTransform:  Arrays of URL objects not a supported return type.";
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::sendW10nMetaForVariable() - ERROR! " << s << endl);
        throw BESInternalError(s, __FILE__, __LINE__);
        break;
#endif
    }

    case libdap::dods_structure_c: {
        std::string s = (std::string) "W10nJsonTransform:  Arrays of Structure objects not a supported return type.";
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::sendW10nMetaForVariable() - ERROR! " << s << endl);
        throw BESInternalError(s, __FILE__, __LINE__);
        break;
    }
    case libdap::dods_grid_c: {
        std::string s = (std::string) "W10nJsonTransform:  Arrays of Grid objects not a supported return type.";
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::sendW10nMetaForVariable() - ERROR! " << s << endl);
        throw BESInternalError(s, __FILE__, __LINE__);
        break;
    }

    case libdap::dods_sequence_c: {
        std::string s = (std::string) "W10nJsonTransform:  Arrays of Sequence objects not a supported return type.";
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::sendW10nMetaForVariable() - ERROR! " << s << endl);
        throw BESInternalError(s, __FILE__, __LINE__);
        break;
    }

    case libdap::dods_array_c: {
        std::string s = (std::string) "W10nJsonTransform:  Arrays of Array objects not a supported return type.";
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::sendW10nMetaForVariable() - ERROR! " << s << endl);
        throw BESInternalError(s, __FILE__, __LINE__);
        break;
    }
    case libdap::dods_int8_c:
    case libdap::dods_uint8_c:
    case libdap::dods_int64_c:
    case libdap::dods_uint64_c:
        // case libdap::dods_url4_c:
    case libdap::dods_enum_c:
    case libdap::dods_group_c: {
        std::string s = (std::string) "W10nJsonTransform:  DAP4 types not yet supported.";
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::sendW10nMetaForVariable() - ERROR! " << s << endl);
        throw BESInternalError(s, __FILE__, __LINE__);
        break;
    }

    default: {
        std::string s = (std::string) "W10nJsonTransform:  Unrecognized type.";
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::sendW10nMetaForVariable() - ERROR! " << s << endl);
        throw BESInternalError(s, __FILE__, __LINE__);
        break;
    }

    }
}

