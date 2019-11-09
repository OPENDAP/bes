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
#include "NCMLDebug.h"
#include "NCMLParser.h"
#include "OtherXMLParser.h"
#include "XMLHelpers.h"

namespace ncml_module {

OtherXMLParser::OtherXMLParser(NCMLParser& p) :
    _rParser(p), _depth(0), _otherXML("")
{
}

OtherXMLParser::~OtherXMLParser()
{
    reset(); // for consistency
}

int OtherXMLParser::getParseDepth() const
{
    return _depth;
}

const std::string&
OtherXMLParser::getString() const
{
    return _otherXML;
}

void OtherXMLParser::reset()
{
    _depth = 0;
    _otherXML = "";
}

void OtherXMLParser::onStartDocument()
{
    THROW_NCML_INTERNAL_ERROR("OtherXMLParser::onStartDocument called!  This is a logic bug.");
}

void OtherXMLParser::onEndDocument()
{
    THROW_NCML_INTERNAL_ERROR("OtherXMLParser::onEndDocument called!  This is a logic bug.");
}

void OtherXMLParser::onStartElement(const std::string& name, const XMLAttributeMap& attrs)
{
    appendOpenStartElementTag(name, "");
    appendAttributes(attrs);
    // no namespaces for this set of calls...  presumably in the attributes
    appendCloseStartElementTag();

    pushDepth();
}

void OtherXMLParser::onEndElement(const std::string& name)
{
    appendEndElementTag(name);
    popDepth();
}

void OtherXMLParser::onStartElementWithNamespace(const std::string& localname, const std::string& prefix,
    const std::string& /* uri */, const XMLAttributeMap& attributes, const XMLNamespaceMap& namespaces)
{
    appendOpenStartElementTag(localname, prefix);
    appendAttributes(attributes);

    // if a root element, grab ALL the namespaces from the parents since we will be
    // disconnected from them and will lose the namespaces if not
    if (_depth == 0) {
        BESDEBUG("ncml",
            "Got depth 0 OtherXML element while parsing OtherXML attribute..." << " Pulling all un-shadowed ancestral namespaces into the element with localname=" << localname << std::endl);

        // initialize it with the local node namespaces, since they
        // take precedence over the stack
        XMLNamespaceMap ancestralNamespaces(namespaces);
        // then fill in the rest from the current stack
        _rParser.getXMLNamespaceStack().getFlattenedNamespacesUsingLexicalScoping(ancestralNamespaces);
        appendNamespaces(ancestralNamespaces);
    }
    else // Append the ones local to JUST this element if not root
    {
        appendNamespaces(namespaces);
    }

    appendCloseStartElementTag();

    pushDepth();
}

void OtherXMLParser::onEndElementWithNamespace(const std::string& localname, const std::string& prefix,
    const std::string& /*uri*/)
{
    appendEndElementTag(XMLAttribute::getQName(prefix, localname));
    popDepth();
}

void OtherXMLParser::onCharacters(const std::string& content)
{
    // Really just shove them on there, whitespace and all to maintain formatting I guess.  Does this do that?
    _otherXML.append(content);
}

void OtherXMLParser::onParseWarning(std::string msg)
{
    THROW_NCML_PARSE_ERROR(-1, // the SAX errors have the line in there already
        "OtherXMLParser: got SAX parse warning while parsing OtherXML.  Msg was: " + msg);
}

void OtherXMLParser::onParseError(std::string msg)
{
    THROW_NCML_PARSE_ERROR(-1, "OtherXMLParser: got SAX parse error while parsing OtherXML.  Msg was: " + msg);
}

void OtherXMLParser::appendOpenStartElementTag(const std::string& localname, const std::string& prefix)
{
    // Append this element and all its attributes onto the string
    _otherXML += string("<");
    _otherXML += XMLAttribute::getQName(prefix, localname);
}

void OtherXMLParser::appendAttributes(const XMLAttributeMap& attributes)
{
    for (XMLAttributeMap::const_iterator it = attributes.begin(); it != attributes.end(); ++it) {
        _otherXML += (string(" ") + it->getQName() + "=\"" + it->value + "\"");
    }
}

void OtherXMLParser::appendNamespaces(const XMLNamespaceMap& namespaces)
{
    _otherXML += namespaces.getAllNamespacesAsAttributeString();
}

void OtherXMLParser::appendCloseStartElementTag()
{
    _otherXML += ">";
}

void OtherXMLParser::appendEndElementTag(const string& qname)
{
    _otherXML += (string("</") + qname + ">");
}

void OtherXMLParser::pushDepth()
{
    ++_depth;
}

void OtherXMLParser::popDepth()
{
    --_depth;

    // Check for underflow
    if (_depth < 0) {
        // I am not sure this is internal or user can cause it... making it internal for now...
        THROW_NCML_INTERNAL_ERROR("OtherXMLElement::onEndElement: _depth < 0!  Logic error in parsing OtherXML.");
    }
}

} // namespace ncml_module
