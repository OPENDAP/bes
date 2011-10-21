/*
 * BESXMLWriter.h
 *
 *  Created on: Jul 28, 2010
 *      Author: jimg
 */

#ifndef XMLWRITER_H_
#define XMLWRITER_H_

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include <string>

using namespace std;

class BESXMLWriter {
private:
    // Various xml writer stuff
    xmlTextWriterPtr d_writer;
    xmlBufferPtr d_doc_buf;
    bool d_started;
    bool d_ended;
    string d_ns_uri;

    string d_doc;

    void m_cleanup() ;

public:
    BESXMLWriter();
    virtual ~BESXMLWriter();

    xmlTextWriterPtr get_writer() { return d_writer; }
    // string get_ns_uri() const { return d_ns_uri; }
    const char *get_doc();
};

#endif /* XMLWRITER_H_ */
