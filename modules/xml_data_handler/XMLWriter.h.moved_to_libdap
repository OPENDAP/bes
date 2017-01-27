/*
 * XMLWriter.h
 *
 *  Created on: Jul 28, 2010
 *      Author: jimg
 */

#ifndef XMLWRITER_H_
#define XMLWRITER_H_

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include <string>

#include <InternalErr.h>

using namespace std;

class XMLWriter {
private:
    // Various xml writer stuff
    xmlTextWriterPtr d_writer;
    xmlBufferPtr d_doc_buf;
    bool d_started;
    bool d_ended;

    string d_doc;

    void m_cleanup() ;

public:
    XMLWriter();
    virtual ~XMLWriter();

    xmlTextWriterPtr get_writer() { return d_writer; }
    const char *get_doc();
};

#endif /* XMLWRITER_H_ */
