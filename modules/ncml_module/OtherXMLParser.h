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
#ifndef __NCML_MODULE__OTHER_XML_PARSER_H__
#define __NCML_MODULE__OTHER_XML_PARSER_H__

#include "SaxParser.h" // super

namespace ncml_module {
class NCMLParser;
}

namespace ncml_module {

/**
 * Class used to handle parsing in an attribute of type=="OtherXML"
 * which basically just has to keep appending the elements and content into a string
 * until the containing <attribute> element is closed.
 * Subclass of SaxParser so the NCMLParser can just hand off calls to it and it can
 * do what it needs to do, as well as give a proper error.
 */
class OtherXMLParser: public SaxParser {
private:
    // Disallow copy and assign
    explicit OtherXMLParser(const OtherXMLParser& proto);
    OtherXMLParser& operator=(const OtherXMLParser& rhs);

public:
    explicit OtherXMLParser(NCMLParser& p);
    virtual ~OtherXMLParser();

    /**
     * Get the current parse depth (how many elements we've opened with onStartElement and not closed yet)
     * It's an int so negative implies an underflow error state.
     */
    int getParseDepth() const;

    /**
     * Get the parsed data as big string that we've been parsing in.
     */
    const std::string& getString() const;

    /** Reset the string and depth so we can start parsing from scratch again */
    void reset();

    // These two calls are invalid for this class since it for parsing subtree
    // They just throw exception.
    virtual void onStartDocument();
    virtual void onEndDocument();

    // This is the main calls, they just keep consing up a string with all the XML in it.
    virtual void onStartElement(const std::string& name, const XMLAttributeMap& attrs);
    virtual void onEndElement(const std::string& name);
    virtual void onCharacters(const std::string& content);

    virtual void onStartElementWithNamespace(const std::string& localname, const std::string& prefix,
        const std::string& uri, const XMLAttributeMap& attributes, const XMLNamespaceMap& namespaces);

    virtual void onEndElementWithNamespace(const std::string& localname, const std::string& prefix,
        const std::string& uri);

    // Implemented to add explanation that the error occured
    // within the OtherXML parse and not the NCMLParser.
    virtual void onParseWarning(std::string msg);
    virtual void onParseError(std::string msg);

private:
    // helpers

    // Add "<prefix:localname " or "<localname " to _otherXML
    void appendOpenStartElementTag(const std::string& localname, const std::string& prefix);

    // For each attribute in attributes, append it to _otherXML
    void appendAttributes(const XMLAttributeMap& attributes);

    // For each namespace in XMLNamespaceMap, append it to  _otherXML
    void appendNamespaces(const XMLNamespaceMap& namespaces);

    // Append ">" since we don't handle self-closing via sax
    void appendCloseStartElementTag();

    // Append a </qname> to _otherXML
    void appendEndElementTag(const string& qname);

    // Increase the depth
    void pushDepth();

    // Decrease the depth checking for underflow and throw internal exception if so.
    void popDepth();

private:
    // Data rep
    NCMLParser& _rParser; // ref to the enclosing parser so we can get to its state if need be.
    int _depth; // how many opened elements not closed yet, used to see if the subtree parse is complete
    std::string _otherXML; // The current string with all the parsed elements and content appended into it
};
}

#endif /* __NCML_MODULE__OTHER_XML_PARSER_H__ */
