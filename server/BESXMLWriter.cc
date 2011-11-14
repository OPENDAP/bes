/*
 * BESXMLWriter.cpp
 *
 *  Created on: Jul 28, 2010
 *      Author: jimg
 */

#include "BESXMLWriter.h"

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include <BESInternalFatalError.h>

const char *ENCODING = "ISO-8859-1";
// Hack
const char *HAI_NS = "http://xml.opendap.org/ns/bes/admin/1.0#";
const int XML_BUF_SIZE = 2000000;

BESXMLWriter::BESXMLWriter() // : d_ns_uri(HAI_NS)
{
    LIBXML_TEST_VERSION;

    /* Create a new XML buffer, to which the XML document will be
     * written */
    try {
        if (!(d_doc_buf = xmlBufferCreateSize(XML_BUF_SIZE)))
            throw BESInternalFatalError("Error allocating the xml buffer", __FILE__, __LINE__);

        xmlBufferSetAllocationScheme(d_doc_buf, XML_BUFFER_ALLOC_DOUBLEIT);

        /* Create a new XmlWriter for memory, with no compression.
         * Remark: there is no compression for this kind of xmlTextWriter */
        if (!(d_writer = xmlNewTextWriterMemory(d_doc_buf, 0)))
            throw BESInternalFatalError("Error allocating memory for xml writer", __FILE__, __LINE__);

        if (xmlTextWriterSetIndent(d_writer, 4) < 0)
            throw BESInternalFatalError("Error starting indentation for response document ", __FILE__, __LINE__);

        if (xmlTextWriterSetIndentString(d_writer, (const xmlChar*) "    ") < 0)
            throw BESInternalFatalError("Error setting indentation for response document ", __FILE__, __LINE__);

        d_started = true;
        d_ended = false;

        /* Start the document with the xml default for the version,
         * encoding ISO 8859-1 and the default for the standalone
         * declaration. MY_ENCODING defined at top of this file*/
        if (xmlTextWriterStartDocument(d_writer, NULL, ENCODING, NULL) < 0)
            throw BESInternalFatalError("Error starting xml response document", __FILE__, __LINE__);

        /* Start an element named "Dataset". Since this is the first element,
         * this will be the root element of the document */
        if (xmlTextWriterStartElementNS(d_writer, (const xmlChar*) "hai", (const xmlChar*) "BesAdminCmd", (const xmlChar*) HAI_NS) < 0)
            throw BESInternalFatalError("Error starting the response element for response ", __FILE__, __LINE__);
    }
    catch (BESInternalFatalError &e) {
        m_cleanup();
        throw;
    }
}

BESXMLWriter::~BESXMLWriter()
{
    m_cleanup();
}

void BESXMLWriter::m_cleanup()
{
    // make sure the buffer and writer are all cleaned up
    if (d_writer) {
        xmlFreeTextWriter(d_writer);
        d_writer = 0;
        //d_doc_buf = 0;
    }
    if (d_doc_buf) {
        xmlBufferFree(d_doc_buf);
        d_doc_buf = 0;
    }

    d_started = false;
    d_ended = false;
}

const char *BESXMLWriter::get_doc()
{
    if (d_writer && d_started) {
        // this should end the response element
        if (xmlTextWriterEndElement(d_writer) < 0)
            throw BESInternalFatalError("Error ending Dataset element.", __FILE__, __LINE__);

        if (xmlTextWriterEndDocument(d_writer) < 0)
            throw BESInternalFatalError("Error ending the document", __FILE__, __LINE__);

        d_ended = true;

        // must call this before getting the buffer content. Odd, but appears to be true.
        // jhrg
        xmlFreeTextWriter(d_writer);
        d_writer = 0;
    }

    // get the xml document as a string and return
    if (!d_doc_buf->content)
        throw BESInternalFatalError("Error retrieving response document as string", __FILE__, __LINE__);

    return (const char *) d_doc_buf->content;
}
