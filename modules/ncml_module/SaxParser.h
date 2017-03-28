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
#ifndef __NCML_MODULE__SAX_PARSER_H__
#define __NCML_MODULE__SAX_PARSER_H__

#include <string>

namespace ncml_module {
// FDecls
class XMLAttributeMap;
class XMLNamespaceMap;

/**
 * @brief Interface class for the wrapper between libxml C SAX parser and our NCMLParser.
 *
 * Also contains definition for AttrMap, which is how the attrs will be returned to the parser.
 * The user should also be careful about making copies of any returned const reference objects (string or AttrMap) as
 * they are only valid in memory for the scope of the handler calls!
 *
 * @author mjohnson <m.johnson@opendap.org>
 */
class SaxParser {
protected:
    SaxParser(); // Interface class

public:
    virtual ~SaxParser()
    {
    }

    virtual void onStartDocument() = 0;
    virtual void onEndDocument() = 0;

    /** @deprecated We are preferring onStartElementWithNamespace() now
     *  Called at the start of the element with the given name and attribute dictionary
     *  The args are only valid for the duration of the call, so copy if necessary to keep.
     * @param name name of the element
     * @param attrs a map of any attributes -> values.  Volatile for this call.
     * @see onStartElementWithNamespace()
     */
    virtual void onStartElement(const std::string& name, const XMLAttributeMap& attrs) = 0;

    /** @deprecated We are preferring onEndElementWithNamespace() now.
     *   Called at the end of the element with the given name.
     *  The args are only valid for the duration of the call, so copy if necessary to keep.
     */
    virtual void onEndElement(const std::string& name) = 0;

    /**
     * SAX2 start element call with gets namespace information.
     * @param localname the localname of the element
     * @param prefix the namespace prefix of the element, or "" if none.
     * @param uri the uri for the namespace of the element.
     * @param attributes  table of the attributes (excluding namespace attributes
     *                    prefixed with xmlns)
     * @param namespace  table of all the namespaces specification on this element
     * */

    virtual void onStartElementWithNamespace(const std::string& localname, const std::string& prefix,
        const std::string& uri, const XMLAttributeMap& attributes, const XMLNamespaceMap& namespaces) = 0;

    /** SAX2 End element with namespace information.
     * @param localname  the localname of the element
     * @param prefix the namespace prefix or "" on the element
     * @param uri  the uri (or "") associated with the namespace of the element.
     */
    virtual void onEndElementWithNamespace(const std::string& localname, const std::string& prefix,
        const std::string& uri) = 0;

    /** Called when characters are encountered within an element.
     * content is only valid for the call duration.
     * Note: this will return all whitespace in the document as well, which makes it messy to use.
     */
    virtual void onCharacters(const std::string& content) = 0;

    /** A recoverable parse error occured. */
    virtual void onParseWarning(std::string msg) = 0;

    /** An unrecoverable parse error occurred */
    virtual void onParseError(std::string msg) = 0;

    /** Before any of the callbacks are issued, this function is called to let the implementing
     * parser know what line number in the parse the next callback is being issued from to allow
     * for more informative error messages.
     * (Default impl is to ignore it now).
     */
    virtual void setParseLineNumber(int /* line */)
    {
    }

};
// class SaxParser

}// namespace ncml_module

#endif /* __NCML_MODULE__SAX_PARSER_H__ */
