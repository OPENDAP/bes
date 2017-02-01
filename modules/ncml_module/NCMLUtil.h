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
#ifndef __NCML_MODULE_NCML_UTIL_H__
#define __NCML_MODULE_NCML_UTIL_H__

/** static class NCMLUtil overview
 *
 * This is a repository for generic, reusable functions that many locations
 * in the module might need.
 *
 * The class design is meant to try and minimize outside dependencies
 * beyond libdap, but already we have some BES dependencies.
 *
 * @TODO Refactor out the purely libdap util functions into a
 * LibdapUtil class so that this class can be reused within
 * the agg_util module without introducing BES or ncml_module
 * dependencies.
 */

namespace libdap {
// FDecls
class BaseType;
class Constructor;
class DDS;
class DAS;
class AttrTable;
}

class BESDapResponse;

#include <string>
#include <vector>

// If there isn't one defined yet, define a safe delete
// to clear ptr after delete to avoid problems.
#ifndef SAFE_DELETE
#define SAFE_DELETE(a) { delete (a); (a) = 0; }
#endif // SAFE_DELETE

namespace ncml_module {
/**
 *  Static class of utility functions
 */
class NCMLUtil {
    NCMLUtil()
    {
    }
public:
    ~NCMLUtil()
    {
    }

    /** Delimiter set for tokenizing whitespace separated data.  Currently " \t" */
    static const std::string WHITESPACE;

    /**
     * Split str into tokens using the characters in delimiters as split boundaries.
     * Return the number of tokens appended to tokens.
     */
    static int tokenize(const std::string& str, std::vector<std::string>& tokens,
        const std::string& delimiters = " \t");

    /** Split str into a vector with one char in str per token slot. */
    static int tokenizeChars(const std::string& str, std::vector<std::string>& tokens);

    /** Does the string contain only ASCII 7-bit characters
     * according to isascii()?
     */
    static bool isAscii(const std::string& str);

    /** Is all the string whitespace as defined by chars in WHITESPACE ?  */
    static bool isAllWhitespace(const std::string& str);

    /** Trim off any number of any character in trimChars from the left side of str in place.
     */
    static void trimLeft(std::string& str, const std::string& trimChars = WHITESPACE);

    /** Trim off any number of any character in trimChars from the right side of str in place
     */
    static void trimRight(std::string& str, const std::string& trimChars = WHITESPACE);

    /** Trim from both left and right.
     */
    static void trim(std::string& str, const std::string& trimChars = WHITESPACE)
    {
        trimLeft(str, trimChars);
        trimRight(str, trimChars);
    }

    /** Call trim on each string in tokens.
     * tokens is mutated to contain the trimmed strings.
     */
    static void trimAll(std::vector<std::string>& tokens, const std::string& trimChars = WHITESPACE);

    /** Convert the string to an unsigned int into oVal.
     *  Return success, else oVal is invalid on return.
     * @param stringVal val to parse
     * @param oVal location to place parsed result
     * @return success at parse
     */
    static bool toUnsignedInt(const std::string& stringVal, unsigned int& oVal);

    /** Given we have a valid attribute tree inside of the DDS, recreate it in the DAS.
     @param das the das to clear and populate
     @param dds_const the source dds
     */
    static void populateDASFromDDS(libdap::DAS* das, const libdap::DDS& dds_const);

    /** Make a deep copy of the global attributes and variables within dds_in
     * into *dds_out.
     * Doesn't affect any other top-level data.
     * @param dds_out place to copy global attribute table and variables into
     * @param dds_in source DDS
     */
    static void copyVariablesAndAttributesInto(libdap::DDS* dds_out, const libdap::DDS& dds_in);

    /**
     * Return the DDS* for the given response object. It is assumed to be either a
     * BESDDSResponse or BESDataDDSResponse.
     * @param reponse either a  BESDDSResponse or BESDataDDSResponse to extract DDS from.
     * @return the DDS* contained in the response object, or NULL if incorrect response type.
     */
    static libdap::DDS* getDDSFromEitherResponse(BESDapResponse* response);

    static void hackGlobalAttributesForDAP2(libdap::AttrTable &global_attributes,
        const std::string &global_container_name);

    /** Currently BaseType::set_name only sets in BaseType.  Unfortunately, the DDS transmission
     * for Vector subclasses uses the name of the template BaseType* describing the variable,
     * which is not set by set_name.  This is a workaround until Vector overrides BaseType::set_name
     * to also set the name of the template _var if there is one.
     */
    static void setVariableNameProperly(libdap::BaseType* pVar, const std::string& name);

};
// class NCMLUtil
}// namespace ncml_module

#endif /* __NCML_MODULE_NCML_UTIL_H__ */
