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
#ifndef __NCML_MODULE__XML_HELPERS_H__
#define __NCML_MODULE__XML_HELPERS_H__

/*
 * This file contains a bunch of quick and dirty classes used to pass and store XML
 * attribute tables and namespace tables.
 *
 * class XMLUtil: conversions from xmlChar to string, mostly.
 * class XMLAttribute: holds namespace-augmented into on XML attribute.
 * class XMLAttributeMap: a container of XMLAttribute that allows lookups, etc.
 * class XMLNamespace: holds {prefix, uri} info for a namespace
 * class XMLNamespaceMap: a container of XMLNamespace's, typically from a single element.
 * class XMLNamespaceStack: container which is a stack of XMLNamespaceMap's to
 *                          allow a SAX lexical scoping operation to be done.
 */

#include <exception>
#include <libxml/xmlstring.h>
#include <string>
#include <vector>

namespace ncml_module {

struct XMLUtil {
    static std::string xmlCharToString(const xmlChar* pChars);
    static void xmlCharToString(std::string& stringToFill, const xmlChar* pChars);
    static std::string xmlCharToStringFromIterators(const xmlChar* startPtr, const xmlChar* endPtr);
};

struct XMLAttribute {
    XMLAttribute(const std::string& localName = "", const std::string& value = "", const std::string& prefix = "",
        const std::string& nsURI = "");

    /** Use the SAX2 namespace attribute point list to make this
     * using fromSAX2NamespaceAttributes(chunkOfFivePointers),
     * Layout: {localname, prefix, uri, valueStartPtr, valueEndPtr}
     */
    XMLAttribute(const xmlChar** chunkOfFivePointers);
    XMLAttribute(const XMLAttribute& proto);
    XMLAttribute& operator=(const XMLAttribute& rhs);

    /** Fill in the fields from the SAX2 namespace attributes array.
     * Assume it's the start of a chunk of 5 pointers to {localname, prefix, uri, valueStartIter, valueEndIter}
     */
    void fromSAX2NamespaceAttributes(const xmlChar** chunkOfFivePointers);

    /** get the name with the prefix:localname if prefix not empty else localname */
    std::string getQName() const;

    /**
     * Get the standard string version as found in an element: prefix:localname="value"
     *  localname="value" if no prefix
     */
    std::string getAsXMLString() const;

    /** Return he QName for the given prefix and localname */
    static std::string getQName(const std::string& prefix, const std::string& localname);

    std::string localname;
    std::string prefix;
    std::string nsURI;
    std::string value;
};

class XMLAttributeMap {
public:
    XMLAttributeMap();
    ~XMLAttributeMap();

    typedef std::vector<XMLAttribute>::const_iterator const_iterator;
    typedef std::vector<XMLAttribute>::iterator iterator;

    XMLAttributeMap::const_iterator begin() const;
    XMLAttributeMap::const_iterator end() const;

    bool empty() const;

    /** make empty */
    void clear();

    /** TODO how do we tell if this exists?  Does it replace?  Do we care? */
    void addAttribute(const XMLAttribute& attribute);

    /** If there is an attribute with localname, return its value, else return default. */
    const std::string/*& jhrg 4/16/14*/getValueForLocalNameOrDefault(const std::string& localname,
        const std::string& defVal = "") const;

    /** These return null if the attribute was not found */
    const XMLAttribute* getAttributeByLocalName(const std::string& localname) const;
    const XMLAttribute* getAttributeByQName(const std::string& qname) const;
    const XMLAttribute* getAttributeByQName(const std::string& prefix, const std::string& localname) const;

    /** The classic {prefix:}foo="value" whitespace separated */
    std::string getAllAttributesAsString() const;

private:
    // helpers

    XMLAttributeMap::iterator findByQName(const std::string& qname);

private:
    // data rep
    // We don't expect many, vector is fast to search and contiguous in memory for few items.
    std::vector<XMLAttribute> _attributes;
};

struct XMLNamespace {
    XMLNamespace(const std::string& prefix = "", const std::string& uri = "");
    XMLNamespace(const XMLNamespace& proto);
    XMLNamespace& operator=(const XMLNamespace& rhs);

    /** Assuming the pointer is an array of two strings: {prefix, uri} */
    void fromSAX2Namespace(const xmlChar** namespaces);

    /** Get the namespace as attribute string, ie  "xmlns:prefix=\"uri\"" for serializing */
    std::string getAsAttributeString() const;

    std::string prefix;
    std::string uri;
};

class XMLNamespaceMap {
public:
    XMLNamespaceMap();
    ~XMLNamespaceMap();
    XMLNamespaceMap(const XMLNamespaceMap& proto);
    XMLNamespaceMap& operator=(const XMLNamespaceMap& rhs);

    /** Read them all in from the xmlChar array. */
    void fromSAX2Namespaces(const xmlChar** pNamespaces, int numNamespaces);

    /** Get a big string full of xmlns:prefix="uri" attributes,
     * separated by spaces.
     */
    std::string getAllNamespacesAsAttributeString() const;

    typedef std::vector<XMLNamespace>::const_iterator const_iterator;

    XMLNamespaceMap::const_iterator begin() const;
    XMLNamespaceMap::const_iterator end() const;

    /** Return the iterator to the element or end() if not found. */
    XMLNamespaceMap::const_iterator find(const std::string& prefix) const;

    bool isInMap(const std::string& prefix) const;

    /** If the given prefix is already in the map, ns REPLACES it */
    void addNamespace(const XMLNamespace& ns);

    void clear();
    bool empty() const;

private:
    // helpers

    typedef std::vector<XMLNamespace>::iterator iterator;

    XMLNamespaceMap::iterator findNonConst(const std::string& prefix);

private:
    std::vector<XMLNamespace> _namespaces;
};

class XMLNamespaceStack {
public:
    XMLNamespaceStack();
    ~XMLNamespaceStack();
    XMLNamespaceStack(const XMLNamespaceStack& proto);
    XMLNamespaceStack& operator=(const XMLNamespaceStack& rhs);

    void push(const XMLNamespaceMap& nsMap);
    void pop();
    const XMLNamespaceMap& top() const;

    bool empty() const;
    void clear();

    // Change the direction since we use the vector as a stack.
    typedef std::vector<XMLNamespaceMap>::const_reverse_iterator const_iterator;

    /** Starts from the top (most recently pushed) and iterates to the bottom (first pushed). */
    XMLNamespaceStack::const_iterator begin() const;
    XMLNamespaceStack::const_iterator end() const;

    /**
     * Scanning from the stack top downwards, add the first found
     * new XMLNamespace (in terms of its prefix) into nsFlattened
     * until the stack bottom is reached.  Effectively finds all the
     * namespaces visible on the stack currently.
     * Note: Doesn't clear the nsFlattened, so it can be "seeded"
     * with namespaces that should be shadowed on the stack
     * (ie a local element's namespaces)
     *
     * @param nsFlattened namespace container (could start !empty() which will contain
     *                    the flattened namespaces using lexical scoping on the stack.
     */
    void getFlattenedNamespacesUsingLexicalScoping(XMLNamespaceMap& nsFlattened) const;

private:
    // helpers

    /** put any XMLNamespace (in terms of prefix) not in intoMap that ARE in fromMap into intoMap. */
    static void addMissingNamespaces(XMLNamespaceMap& intoMap, const XMLNamespaceMap& fromMap);

private:
    // data rep
    // Vector for easy scanning.
    std::vector<XMLNamespaceMap> _stack;
};

} // namespace ncml_module
#endif // __NCML_MODULE__XML_HELPERS_H__
