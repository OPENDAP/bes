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

#include "XMLHelpers.h"

using std::string;

namespace ncml_module {

///////////////////////////// XMLUtil Impl

string XMLUtil::xmlCharToString(const xmlChar* theCharsOrNull)
{
    const char* asChars = reinterpret_cast<const char*>(theCharsOrNull);
    return ((asChars) ? (string(asChars)) : (string("")));
}

void XMLUtil::xmlCharToString(string& stringToFill, const xmlChar* pChars)
{
    stringToFill = xmlCharToString(pChars);
}

// Interpret the args as the start and stop iterator of chars.
// But check for empty on it.
string XMLUtil::xmlCharToStringFromIterators(const xmlChar* startIter, const xmlChar* endIter)
{
    // Just be safe, no use for exceptions.
    if (!startIter || !endIter || (startIter > endIter)) {
        return string("");
    }

    // These are interpreted as const char* iterators.
    return string(reinterpret_cast<const char*>(startIter), reinterpret_cast<const char*>(endIter));
}

/////////////////////////////// XMLAttribute Impl ///////////////////////////////
XMLAttribute::XMLAttribute(const string& localNameA, const string& valueA, const string& prefixA, const string& nsURIA) :
    localname(localNameA), prefix(prefixA), nsURI(nsURIA), value(valueA)
{
}

/** Use the SAX2 namespace attribute point list to make it.
 * Layout: {localname, prefix, uri, valueStartPtr, valueEndPtr}
 */
XMLAttribute::XMLAttribute(const xmlChar** chunkOfFivePointers)
{
    fromSAX2NamespaceAttributes(chunkOfFivePointers);
}

XMLAttribute::XMLAttribute(const XMLAttribute& proto) :
    localname(proto.localname), prefix(proto.prefix), nsURI(proto.nsURI), value(proto.value)
{
}

XMLAttribute&
XMLAttribute::operator=(const XMLAttribute& rhs)
{
    if (&rhs == this) {
        return *this;
    }
    localname = rhs.localname;
    prefix = rhs.prefix;
    value = rhs.value;
    nsURI = rhs.nsURI;	// jhrg 3/16/11
    return *this;
}

void XMLAttribute::fromSAX2NamespaceAttributes(const xmlChar** chunkOfFivePointers)
{
    const xmlChar* xmlLocalName = (*chunkOfFivePointers++);
    const xmlChar* xmlPrefix = (*chunkOfFivePointers++);
    const xmlChar* xmlURI = (*chunkOfFivePointers++);
    const xmlChar* xmlValueStart = (*chunkOfFivePointers++);
    // pointer to end of the value since not null terminated.
    const xmlChar* xmlValueEnd = (*chunkOfFivePointers++);

    // makeString calls map null into "".
    localname = XMLUtil::xmlCharToString(xmlLocalName);
    prefix = XMLUtil::xmlCharToString(xmlPrefix);
    nsURI = XMLUtil::xmlCharToString(xmlURI);
    value = XMLUtil::xmlCharToStringFromIterators(xmlValueStart, xmlValueEnd);
}

/** get the name with the prefix:localname if prefix not empty else localname */
string XMLAttribute::getQName() const
{
    return getQName(prefix, localname);
}

/**
 * Get the standard string version as found in an element: prefix:localname="value"
 *  localname="value" if no prefix
 */
string XMLAttribute::getAsXMLString() const
{
    return getQName() + "=\"" + value + "\"";
}

/*static */
string XMLAttribute::getQName(const string& prefix, const string& localname)
{
    if (prefix.empty()) {
        return localname;
    }
    else {
        return prefix + ":" + localname;
    }
}

//////////////////////////// XMLAttributeMap Impl ///////////////////////
XMLAttributeMap::XMLAttributeMap() :
    _attributes()
{
}

XMLAttributeMap::~XMLAttributeMap()
{
}

XMLAttributeMap::const_iterator XMLAttributeMap::begin() const
{
    return _attributes.begin();
}

XMLAttributeMap::const_iterator XMLAttributeMap::end() const
{
    return _attributes.end();
}

bool XMLAttributeMap::empty() const
{
    return _attributes.empty();
}

void XMLAttributeMap::clear()
{
    // won't resize, we might be reusing it so no point.
    _attributes.clear();
}

void XMLAttributeMap::addAttribute(const XMLAttribute& attribute)
{
    XMLAttributeMap::iterator foundIt = findByQName(attribute.getQName());
    // if in there, replace it.
    if (foundIt != _attributes.end()) {
        // replace with a copy of new one
        (*foundIt) = XMLAttribute(attribute);
    }

    // otherwise push on a new one
    _attributes.push_back(attribute);
}

const string /*& returns a reference to a local temp object (the else clause). jhrg 4/16/14*/
XMLAttributeMap::getValueForLocalNameOrDefault(const string& localname, const string& defVal/*=""*/) const
{
    const XMLAttribute* pAttr = getAttributeByLocalName(localname);
    if (pAttr) {
        return pAttr->value;
    }
    else {
        // Reference to a local temporary object. jhrg 4/16/14
        return defVal;
    }
}

const XMLAttribute*
XMLAttributeMap::getAttributeByLocalName(const string& localname) const
{
    const XMLAttribute* pAtt = 0; // if not found
    for (XMLAttributeMap::const_iterator it = begin(); it != end(); ++it) {
        const XMLAttribute& rAttr = *it;
        if (rAttr.localname == localname) {
            pAtt = &rAttr;
            break;
        }
    }
    return pAtt;
}

const XMLAttribute*
XMLAttributeMap::getAttributeByQName(const string& qname) const
{
    const XMLAttribute* pAtt = 0; // if not found
    for (XMLAttributeMap::const_iterator it = begin(); it != end(); ++it) {
        const XMLAttribute& rAttr = *it;
        if (rAttr.getQName() == qname) {
            pAtt = &rAttr;
            break;
        }
    }
    return pAtt;
}

const XMLAttribute*
XMLAttributeMap::getAttributeByQName(const string& prefix, const string& localname) const
{
    return getAttributeByQName(XMLAttribute::getQName(prefix, localname));
}

/** The classic {prefix:}foo="value" whitespace separated */
string XMLAttributeMap::getAllAttributesAsString() const
{
    string result("");
    XMLAttributeMap::const_iterator it;
    for (it = begin(); it != end(); ++it) {
        const XMLAttribute& attr = *it;
        result += (attr.getQName() + "=\"" + attr.value + "\" ");
    }
    return result;
}

XMLAttributeMap::iterator XMLAttributeMap::findByQName(const string& qname)
{
    XMLAttributeMap::iterator it;
    for (it = _attributes.begin(); it != _attributes.end(); ++it) {
        if (it->getQName() == qname) {
            break;
        }
    }
    return it;
}

///////////////////////////////// XMLNamespace Impl ///////////////////////

XMLNamespace::XMLNamespace(const string& prefixArg/*=""*/, const string& uriArg/*=""*/) :
    prefix(prefixArg), uri(uriArg)
{
}

XMLNamespace::XMLNamespace(const XMLNamespace& proto) :
    prefix(proto.prefix), uri(proto.uri)
{
}

XMLNamespace&
XMLNamespace::operator=(const XMLNamespace& rhs)
{
    if (this == &rhs) {
        return *this;
    }

    prefix = rhs.prefix;
    uri = rhs.uri;
    return *this;
}

void XMLNamespace::fromSAX2Namespace(const xmlChar** pNamespace)
{
    prefix = XMLUtil::xmlCharToString(*pNamespace);
    uri = XMLUtil::xmlCharToString(*(pNamespace + 1));
}

/** Get the namespace as attribute string, ie  "xmlns:prefix=\"uri\"" for serializing */
string XMLNamespace::getAsAttributeString() const
{
    string attr("xmlns");
    if (!prefix.empty()) {
        attr += (string(":") + prefix);
    }
    attr += string("=\"");
    attr += uri;
    attr += string("\"");
    return attr;
}

//////////////////////////////////// XMLNamespaceMap impl /////////////////////

XMLNamespaceMap::XMLNamespaceMap() :
    _namespaces()
{
}

XMLNamespaceMap::~XMLNamespaceMap()
{
    _namespaces.clear();
}

XMLNamespaceMap::XMLNamespaceMap(const XMLNamespaceMap& proto) :
    _namespaces(proto._namespaces)
{
}

XMLNamespaceMap&
XMLNamespaceMap::operator=(const XMLNamespaceMap& rhs)
{
    if (this == &rhs) {
        return *this;
    }
    _namespaces = rhs._namespaces;
    return *this;
}

void XMLNamespaceMap::fromSAX2Namespaces(const xmlChar** pNamespaces, int numNamespaces)
{
    clear();
    for (int i = 0; i < numNamespaces; ++i) {
        XMLNamespace ns;
        ns.fromSAX2Namespace(pNamespaces);
        pNamespaces += 2; // this array is stride 2
        addNamespace(ns);
    }
}

string XMLNamespaceMap::getAllNamespacesAsAttributeString() const
{
    string allAttrs("");
    for (XMLNamespaceMap::const_iterator it = begin(); it != end(); ++it) {
        const XMLNamespace& ns = *it;
        allAttrs += string(" ") + ns.getAsAttributeString();
    }
    return allAttrs;
}

XMLNamespaceMap::const_iterator XMLNamespaceMap::begin() const
{
    return _namespaces.begin();
}

XMLNamespaceMap::const_iterator XMLNamespaceMap::end() const
{
    return _namespaces.end();
}

XMLNamespaceMap::const_iterator XMLNamespaceMap::find(const string& prefix) const
{
    XMLNamespaceMap::const_iterator foundIt;
    for (foundIt = begin(); foundIt != end(); ++foundIt) {
        if (foundIt->prefix == prefix) {
            break;
        }
    }
    return foundIt;
}

bool XMLNamespaceMap::isInMap(const string& prefix) const
{
    return (find(prefix) != end());
}

void XMLNamespaceMap::addNamespace(const XMLNamespace& ns)
{
    XMLNamespaceMap::iterator foundIt = findNonConst(ns.prefix);
    if (foundIt == _namespaces.end()) // not found, push
        {
        _namespaces.push_back(ns);
    }
    else {
        // overwrite
        (*foundIt) = XMLNamespace(ns);
    }
}

void XMLNamespaceMap::clear()
{
    _namespaces.clear();
}

bool XMLNamespaceMap::empty() const
{
    return _namespaces.empty();
}

XMLNamespaceMap::iterator XMLNamespaceMap::findNonConst(const string& prefix)
{
    XMLNamespaceMap::iterator foundIt;
    for (foundIt = _namespaces.begin(); foundIt != _namespaces.end(); ++foundIt) {
        if (foundIt->prefix == prefix) {
            break;
        }
    }
    return foundIt;
}

//////////////////////////////////// XMLNamespaceStack Impl //////////////////////////

XMLNamespaceStack::XMLNamespaceStack() :
    _stack()
{
}

XMLNamespaceStack::~XMLNamespaceStack()
{
    _stack.clear();
    _stack.resize(0);
}

XMLNamespaceStack::XMLNamespaceStack(const XMLNamespaceStack& proto) :
    _stack(proto._stack)
{
}

XMLNamespaceStack&
XMLNamespaceStack::operator=(const XMLNamespaceStack& rhs)
{
    if (this == &rhs) {
        return *this;
    }
    _stack = rhs._stack;
    return *this;
}

void XMLNamespaceStack::push(const XMLNamespaceMap& nsMap)
{
    _stack.push_back(nsMap);
}

void XMLNamespaceStack::pop()
{
    _stack.pop_back();
}

const XMLNamespaceMap&
XMLNamespaceStack::top() const
{
    return _stack.back();
}

bool XMLNamespaceStack::empty() const
{
    return _stack.empty();
}

void XMLNamespaceStack::clear()
{
    _stack.clear();
}

XMLNamespaceStack::const_iterator XMLNamespaceStack::begin() const
{
    return _stack.rbegin();
}

XMLNamespaceStack::const_iterator XMLNamespaceStack::end() const
{
    return _stack.rend();
}

void XMLNamespaceStack::getFlattenedNamespacesUsingLexicalScoping(XMLNamespaceMap& nsFlattened) const
{
    // Scan the stack in top (lexically innermost) to bottom order, adding in
    // the namespaces we don't have a prefix for.
    for (XMLNamespaceStack::const_iterator it = begin(); it != end(); ++it) {
        addMissingNamespaces(nsFlattened, *it);
    }
}

/* static */
void XMLNamespaceStack::addMissingNamespaces(XMLNamespaceMap& intoMap, const XMLNamespaceMap& fromMap)
{
    for (XMLNamespaceMap::const_iterator it = fromMap.begin(); it != fromMap.end(); ++it) {
        const XMLNamespace& ns = *it;
        // If this namespace is not in the output map, add it
        if (intoMap.find(ns.prefix) == intoMap.end()) {
            intoMap.addNamespace(ns);
        }
        // otherwise, it's been lexically shadowed, so ignore it.
    }
}
}
// namespace ncml_module
