// BESXMLInfo.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <sstream>
#include <map>

using std::ostringstream;
using std::endl;
using std::map;
using std::string;
using std::ostream;

#include "BESXMLInfo.h"
#include "BESUtil.h"
#include "BESDataNames.h"

#define MY_ENCODING "ISO-8859-1"
#define BES_SCHEMA "http://xml.opendap.org/ns/bes/1.0#"

/** @brief constructs an informational response object as an xml document
 *
 * @see BESInfo
 * @see BESResponseObject
 */
BESXMLInfo::BESXMLInfo() :
    BESInfo(), _writer(0), _doc_buf(0), _started(false), _ended(false)
{
}

BESXMLInfo::~BESXMLInfo()
{
    cleanup();
}

void BESXMLInfo::cleanup()
{
    // make sure the buffer and writer are all cleaned up
    if (_writer) {
        xmlFreeTextWriter(_writer);
        _writer = 0;
        _doc_buf = 0;
    }

    if (_doc_buf) {
        xmlBufferFree(_doc_buf);
        _doc_buf = 0;
    }

    // this always seems to be causing a memory fault
    // xmlCleanupParser();

    _started = false;
    _ended = false;
    if (_strm) {
        ((ostringstream *) _strm)->str("");
    }
}

/** @brief begin the informational response
 *
 * This will add the response name as well as the <response> tag to
 * the informational response object
 *
 * @param response_name name of the response this information represents
 * @param dhi information about the request and response
 */
void BESXMLInfo::begin_response(const string &response_name, BESDataHandlerInterface &dhi)
{
    map<string, string, std::less<>> empty_attrs;
    begin_response(response_name,  &empty_attrs, dhi);

}
/** @brief begin the informational response
 *
 * This will add the response name as well as the <response> tag to
 * the informational response object
 *
 * @param response_name name of the response this information represents
 * @param dhi information about the request and response
 */
void BESXMLInfo::begin_response(const string &response_name, map<string, string, std::less<>> *attrs, BESDataHandlerInterface &dhi)
{
    BESInfo::begin_response(response_name, attrs, dhi);

    _response_name = response_name;

#if 1
    LIBXML_TEST_VERSION
#endif

    int rc = 0;

    /* Create a new XML buffer, to which the XML document will be
     * written */
    _doc_buf = xmlBufferCreate();
    if (_doc_buf == NULL) {
        cleanup();
        string err = (string) "Error creating the xml buffer for response " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    /* Create a new XmlWriter for memory, with no compression.
     * Remark: there is no compression for this kind of xmlTextWriter */
    _writer = xmlNewTextWriterMemory(_doc_buf, 0);
    if (_writer == NULL) {
        cleanup();
        string err = (string) "Error creating the xml writer for response " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    rc = xmlTextWriterSetIndent(_writer, 4);
    if (rc < 0) {
        cleanup();
        string err = (string) "Error starting indentation for response document " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    rc = xmlTextWriterSetIndentString( _writer, BAD_CAST "    " );
    if (rc < 0) {
        cleanup();
        string err = (string) "Error setting indentation for response document " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    _started = true;

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. MY_ENCODING defined at top of this file*/
    rc = xmlTextWriterStartDocument(_writer, NULL, MY_ENCODING, NULL);
    if (rc < 0) {
        cleanup();
        string err = (string) "Error starting xml response document for " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    /* Start an element named "response". Since this is the first element,
     * this will be the root element of the document */
    rc = xmlTextWriterStartElementNS(_writer, NULL, BAD_CAST "response", BAD_CAST BES_SCHEMA);
    if (rc < 0) {
        cleanup();
        string err = (string) "Error starting the response element for response " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    /* Add the request id attribute */
    string reqid = dhi.data[REQUEST_ID_KEY];
    if (!reqid.empty()) {
        rc = xmlTextWriterWriteAttribute( _writer, reinterpret_cast<xmlChar *>(REQUEST_ID_KEY),
            BAD_CAST reqid.c_str() );
        if (rc < 0) {
            cleanup();
            string err = (string) "Error adding attribute " + REQUEST_ID_KEY + " for response " + _response_name;
            throw BESInternalError(err, __FILE__, __LINE__);
        }
    }

    /* Add the request uuid attribute */
    string req_uuid = dhi.data[REQUEST_UUID_KEY];
    if (!req_uuid.empty()) {
        rc = xmlTextWriterWriteAttribute( _writer, BAD_CAST REQUEST_UUID_KEY,
            BAD_CAST req_uuid.c_str() );
        if (rc < 0) {
            cleanup();
            string err = (string) "Error adding attribute " + REQUEST_UUID_KEY + " for response " + _response_name;
            throw BESInternalError(err, __FILE__, __LINE__);
        }
    }

    /* Start an element for the specific response. */
    rc = xmlTextWriterStartElement( _writer, BAD_CAST _response_name.c_str() );
    if (rc < 0) {
        cleanup();
        string err = (string) "Error creating root element for response " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    map<string, string>::iterator it;
    for ( it = attrs->begin(); it != attrs->end(); it++ )
    {
        rc = xmlTextWriterWriteAttribute( _writer, BAD_CAST it->first.c_str(), BAD_CAST it->second.c_str());
        if (rc < 0) {
            cleanup();
            string err = (string) "Error creating root element for response " + _response_name;
            throw BESInternalError(err, __FILE__, __LINE__);
        }
    }

 }

/** @brief end the response
 *
 * Add the terminating tags for the response and for the response name. If
 * there are still tags that have not been closed then an exception is
 * thrown.
 *
 */
void BESXMLInfo::end_response()
{
    BESInfo::end_response();

    int rc = 0;

    // this should end the response element
    rc = xmlTextWriterEndElement(_writer);
    if (rc < 0) {
        cleanup();
        string err = (string) "Error ending response element for response " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    // this should end the specific response element, like showVersion
    rc = xmlTextWriterEndElement(_writer);
    if (rc < 0) {
        cleanup();
        string err = (string) "Error ending specific response element " + "for response " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    rc = xmlTextWriterEndDocument(_writer);
    if (rc < 0) {
        cleanup();
        string err = (string) "Error ending the response document for response " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    // must call this before getting the buffer content
    xmlFreeTextWriter(_writer);
    _writer = 0;

    // get the xml document as a string and return
    if (!_doc_buf->content) {
        cleanup();
        string err = (string) "Error retrieving response document as string " + "for response " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    else {
        _doc = (char *) _doc_buf->content;
    }

    _ended = true;

    cleanup();
}

/** @brief add tagged information to the informational response
 *
 * @param tag_name name of the tag to be added to the response
 * @param tag_data information describing the tag
 * @param attrs map of attributes to add to the tag
 */
void BESXMLInfo::add_tag(const string &tag_name, const string &tag_data, map<string, string, std::less<>> *attrs)
{
    /* Start an element named tag_name. */
    int rc = xmlTextWriterStartElement( _writer, BAD_CAST tag_name.c_str() );
    if (rc < 0) {
        cleanup();
        string err = (string) "Error starting element " + tag_name + " for response " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    if (attrs) {
        map<string, string>::const_iterator i = attrs->begin();
        map<string, string>::const_iterator e = attrs->end();
        for (; i != e; i++) {
            string name = (*i).first;
            string val = (*i).second;

            // FIXME: is there one with no value?
            /* Add the attributes */
            rc = xmlTextWriterWriteAttribute( _writer, BAD_CAST name.c_str(),
                BAD_CAST val.c_str() );
            if (rc < 0) {
                cleanup();
                string err = (string) "Error adding attribute " + name + " for response " + _response_name;
                throw BESInternalError(err, __FILE__, __LINE__);
            }
        }
    }

    /* Write the value of the element */
    if (!tag_data.empty()) {
        rc = xmlTextWriterWriteString( _writer, BAD_CAST tag_data.c_str() );
        if (rc < 0) {
            cleanup();
            string err = (string) "Error writing the value for element " + tag_name + " for response " + _response_name;
            throw BESInternalError(err, __FILE__, __LINE__);
        }
    }

    // this should end the tag_name element
    rc = xmlTextWriterEndElement(_writer);
    if (rc < 0) {
        cleanup();
        string err = (string) "Error ending element " + tag_name + " for response " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }
}

/** @brief begin a tagged part of the information, information to follow
 *
 * @param tag_name name of the tag to begin
 * @param attrs map of attributes to begin the tag with
 */
void BESXMLInfo::begin_tag(const string &tag_name, map<string, string, std::less<>> *attrs)
{
    begin_tag(tag_name, "", "", attrs);
}

/** @brief begin a tagged part of the information, information to follow
 *
 * @param tag_name name of the tag to begin
 * @param ns namespace name to include in the tag
 * @param uri namespace uri
 * @param attrs map of attributes to begin the tag with
 */
void BESXMLInfo::begin_tag(const string &tag_name,
                           const string &ns,
                           const string &uri,
                           map<string, string, std::less<>> *attrs = nullptr)
{
    BESInfo::begin_tag(tag_name);

    /* Start an element named tag_name. */
    int rc = 0;
    if (ns.empty() && uri.empty()) {
        rc = xmlTextWriterStartElement( _writer, BAD_CAST tag_name.c_str());
        if (rc < 0) {
            cleanup();
            string err = (string) "Error starting element " + tag_name + " for response " + _response_name;
            throw BESInternalError(err, __FILE__, __LINE__);
        }
    }
    else {
        const char *cns = nullptr;
        if (!ns.empty()) cns = ns.c_str();
        rc = xmlTextWriterStartElementNS( _writer,
            BAD_CAST cns,
            BAD_CAST tag_name.c_str(),
            BAD_CAST uri.c_str() );
        if (rc < 0) {
            cleanup();
            string err = (string) "Error starting element " + tag_name + " for response " + _response_name;
            throw BESInternalError(err, __FILE__, __LINE__);
        }
    }

    if (attrs) {
        map<string, string>::const_iterator i = attrs->begin();
        map<string, string>::const_iterator e = attrs->end();
        for (; i != e; i++) {
            string name = (*i).first;
            string val = (*i).second;

            /* Add the attributes */
            rc = xmlTextWriterWriteAttribute( _writer, BAD_CAST name.c_str(),
                BAD_CAST val.c_str() );
            if (rc < 0) {
                cleanup();
                string err = (string) "Error adding attribute " + name + " for response " + _response_name;
                throw BESInternalError(err, __FILE__, __LINE__);
            }
        }
    }
}

/** @brief end a tagged part of the informational response
 *
 * If the named tag is not the current tag then an error is thrown.
 *
 * @param tag_name name of the tag to end
 */
void BESXMLInfo::end_tag(const string &tag_name)
{
    BESInfo::end_tag(tag_name);

    int rc = 0;

    string s = ((ostringstream *) _strm)->str();
    if (!s.empty()) {
        /* Write the value of the element */
        rc = xmlTextWriterWriteString( _writer, BAD_CAST s.c_str() );
        if (rc < 0) {
            cleanup();
            string err = (string) "Error writing the value for element " + tag_name + " for response " + _response_name;
            throw BESInternalError(err, __FILE__, __LINE__);
        }

        ((ostringstream *) _strm)->str("");
    }

    // this should end the tag_name element
    rc = xmlTextWriterEndElement(_writer);
    if (rc < 0) {
        cleanup();
        string err = (string) "Error ending element " + tag_name + " for response " + _response_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }
}

/** @brief add a space to the informational response
 *
 * @param num_spaces the number of spaces to add to the information
 */
void BESXMLInfo::add_space(unsigned long num_spaces)
{
    string to_add;
    for (unsigned long i = 0; i < num_spaces; i++) {
        to_add += " ";
    }
    BESInfo::add_data(to_add);
}

/** @brief add a line break to the information
 *
 * @param num_breaks the number of line breaks to add to the information
 */
void BESXMLInfo::add_break(unsigned long num_breaks)
{
    string to_add;
    for (unsigned long i = 0; i < num_breaks; i++) {
        to_add += "\n";
    }
    BESInfo::add_data(to_add);
}

void BESXMLInfo::add_data(const string &s)
{
#if 0
    BESInfo::add_data(s);
#else
    int rc = xmlTextWriterWriteString( _writer, BAD_CAST s.c_str() );
    if (rc < 0) {
        cleanup();
        throw BESInternalError(string("Error writing String data for response ") + _response_name, __FILE__, __LINE__);
    }
#endif
}

/** @brief add data from a file to the informational object
 *
 * This method simply adds a .XML to the end of the key and passes the
 * request on up to the BESInfo parent class.
 *
 * @param key Key from the initialization file specifying the file to be
 * @param name A description of what is the information being loaded
 */
void BESXMLInfo::add_data_from_file(const string &key, const string &name)
{
    // just add the html file with the <html ... wrapper around it
    // <html xmlns="http://www.w3.org/1999/xhtml">
    begin_tag("html", "", "http://www.w3.org/1999/xhtml");

    string newkey = key + ".HTML";
    BESInfo::add_data_from_file(newkey, name);

    end_tag("html");
}

/** @brief transmit the text information as text
 *
 * use the send_text method on the transmitter to transmit the information
 * back to the client.
 *
 * @param transmitter The type of transmitter to use to transmit the info
 * @param dhi information to help with the transmission
 */
void BESXMLInfo::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
{
    if (_started && !_ended) {
        end_response();
    }
    transmitter->send_text(*this, dhi);
}

/** @brief print the information from this informational object to the
 * specified stream
 *
 * @param strm output to this stream
 */
void BESXMLInfo::print(ostream &strm)
{
    if (_started && !_ended) {
        end_response();
    }
    strm << _doc;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this XML informational object.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESXMLInfo::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESXMLInfo::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESInfo::dump(strm);
    BESIndent::UnIndent();
}

BESInfo *
BESXMLInfo::BuildXMLInfo(const string &/*info_type*/)
{
    return new BESXMLInfo();
}

