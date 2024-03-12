// CmdTranslation.cc

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

#include <iostream>
#include <list>
#include <map>

#include <libxml/parser.h>

using std::cerr;
using std::cout;
using std::list;
using std::map;
using std::endl;
using std::ostream ;
using std::string;

#include "CmdTranslation.h"
#include "BESTokenizer.h"
#include "BESSyntaxUserError.h"

#define MY_ENCODING "ISO-8859-1"

map<string, CmdTranslation::p_cmd_translator> CmdTranslation::_translations;
bool CmdTranslation::_is_show = false;

int CmdTranslation::initialize(int, char**)
{
    _translations["show"] = CmdTranslation::translate_show;
    _translations["show.catalog"] = CmdTranslation::translate_catalog;
    _translations["show.info"] = CmdTranslation::translate_catalog;
    _translations["show.error"] = CmdTranslation::translate_show_error;
    _translations["set"] = CmdTranslation::translate_set;
    _translations["set.context"] = CmdTranslation::translate_context;
    _translations["set.container"] = CmdTranslation::translate_container;
    _translations["define"] = CmdTranslation::translate_define;
    _translations["delete"] = CmdTranslation::translate_delete;
    _translations["get"] = CmdTranslation::translate_get;
    return 0;
}

int CmdTranslation::terminate(void)
{
    return 0;
}

void CmdTranslation::add_translation(const string &name, p_cmd_translator func)
{
    CmdTranslation::_translations[name] = func;
}

void CmdTranslation::remove_translation(const string &name)
{
    map<string, p_cmd_translator>::iterator i = CmdTranslation::_translations.find(name);
    if (i != CmdTranslation::_translations.end()) {
        CmdTranslation::_translations.erase(i);
    }
}

string CmdTranslation::translate(const string &commands)
{
    BESTokenizer t;
    try {
        t.tokenize(commands.c_str());

        string token = t.get_first_token();
        if (token.empty()) {
            return "";
        }
    }
    catch (BESSyntaxUserError &e) {
        cerr << "failed to build tokenizer for translation" << endl;
        cerr << e.get_message() << endl;
        return "";
    }

    LIBXML_TEST_VERSION;

    int rc;
    xmlTextWriterPtr writer = 0;
    xmlBufferPtr buf = 0;
    // Unused xmlChar *tmp = 0;

    /* Create a new XML buffer, to which the XML document will be
     * written */
    buf = xmlBufferCreate();
    if (buf == NULL) {
        cerr << "testXmlwriterMemory: Error creating the xml buffer" << endl;
        return "";
    }

    /* Create a new XmlWriter for memory, with no compression.
     * Remark: there is no compression for this kind of xmlTextWriter */
    writer = xmlNewTextWriterMemory(buf, 0);
    if (writer == NULL) {
        cerr << "testXmlwriterMemory: Error creating the xml writer" << endl;
        return "";
    }

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. MY_ENCODING defined at top of this file*/
    rc = xmlTextWriterStartDocument(writer, NULL, MY_ENCODING, NULL);
    if (rc < 0) {
        cerr << "testXmlwriterMemory: Error at xmlTextWriterStartDocument" << endl;
        xmlFreeTextWriter(writer);
        return "";
    }

    /* Start an element named "request". Since thist is the first
     * element, this will be the root element of the document. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "request");
    if (rc < 0) {
        cerr << "testXmlwriterMemory: Error at xmlTextWriterStartElement" << endl;
        xmlFreeTextWriter(writer);
        return "";
    }

    /* Add the request id attribute */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "reqID",
    BAD_CAST "some_unique_value");
    if (rc < 0) {
        cerr << "failed to add the request id attribute" << endl;
        return "";
    }

    bool status = do_translate(t, writer);
    if (!status) {
        xmlFreeTextWriter(writer);
        return "";
    }

    // this should end the request element
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        cerr << "failed to close request element" << endl;
        xmlFreeTextWriter(writer);
        return "";
    }

    rc = xmlTextWriterEndDocument(writer);
    if (rc < 0) {
        cerr << "failed to end the document" << endl;
        return "";
    }

    xmlFreeTextWriter(writer);

    // get the xml document as a string and return
    string doc;
    if (!buf->content) {
        cerr << "failed to retrieve document as string" << endl;
    }
    else {
        doc = (char *) buf->content;
    }

    xmlBufferFree(buf);

    xmlCleanupParser();

    return doc;
}

bool CmdTranslation::do_translate(BESTokenizer &t, xmlTextWriterPtr writer)
{
    string token = t.get_current_token();
    CmdTranslation::p_cmd_translator p = _translations[token];
    if (!p) {
        cerr << endl << "Invalid command " << token << endl << endl;
        return false;
    }

    try {
        bool status = p(t, writer);
        if (!status) {
            return status;
        }
    }
    catch (BESSyntaxUserError &e) {
        cerr << e.get_message() << endl;
        return false;
    }

    // if this throws an exception then there are no more tokens. Catch it
    // and ignore the exception. This means we're done.
    try {
        token = t.get_next_token();
    }
    catch (BESSyntaxUserError &e) {
        token.clear();
    }

    if (token.empty()) {
        // we are done.
        return true;
    }

    // more translation to do, so call do_translate again. It will grab the
    // current token which we just got.
    return do_translate(t, writer);
}

bool CmdTranslation::translate_show(BESTokenizer &t, xmlTextWriterPtr writer)
{
    CmdTranslation::set_show(true);

    string show_what = t.get_next_token();
    if (show_what.empty()) {
        t.parse_error("show command must be followed by target");
    }

    string new_cmd = "show." + show_what;
    CmdTranslation::p_cmd_translator p = _translations[new_cmd];
    if (p) {
        return p(t, writer);
    }

    string semi = t.get_next_token();
    if (semi != ";") {
        string err = (string) "show " + show_what + " commands must end with a semicolon";
        t.parse_error(err);
    }
    show_what[0] = toupper(show_what[0]);
    string tag = "show" + show_what;

    // start the show element
    int rc = xmlTextWriterStartElement(writer, BAD_CAST tag.c_str());
    if (rc < 0) {
        cerr << "failed to start " << tag << " element" << endl;
        return false;
    }

    // end the show element
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        cerr << "failed to close " << tag << " element" << endl;
        return false;
    }

    return true;
}

bool CmdTranslation::translate_show_error(BESTokenizer &t, xmlTextWriterPtr writer)
{
    string show_what = t.get_current_token();
    if (show_what.empty() || show_what != "error") {
        t.parse_error("show command must be error");
    }

    string etype = t.get_next_token();
    if (etype == ";") {
        string err = (string) "show " + show_what + " command must include the error type to show";
        t.parse_error(err);
    }

    string semi = t.get_next_token();
    if (semi != ";") {
        string err = (string) "show " + show_what + " commands must end with a semicolon";
        t.parse_error(err);
    }
    show_what[0] = toupper(show_what[0]);
    string tag = "show" + show_what;

    // start the show element
    int rc = xmlTextWriterStartElement(writer, BAD_CAST tag.c_str());
    if (rc < 0) {
        cerr << "failed to start " << tag << " element" << endl;
        return false;
    }

    /* Add the error type attribute */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type",
    BAD_CAST etype.c_str());
    if (rc < 0) {
        cerr << "failed to add the get type attribute" << endl;
        return false;
    }

    // end the show element
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        cerr << "failed to close " << tag << " element" << endl;
        return false;
    }

    return true;
}

bool CmdTranslation::translate_catalog(BESTokenizer &t, xmlTextWriterPtr writer)
{
    // show catalog|info [in catalog] [for node]
    // <showCatalog node="" />
    string show_what = t.get_current_token();
    if (show_what.empty() || (show_what != "info" && show_what != "catalog")) {
        t.parse_error("show command must be info or catalog");
    }

    show_what[0] = toupper(show_what[0]);
    string tag = "show" + show_what;

    string token = t.get_next_token();
    string node;
    if (token == "for") {
        node = t.get_next_token();
        if (node == ";") {
            t.parse_error("show catalog command expecting node");
        }
        node = t.remove_quotes(node);
        token = t.get_next_token();
    }
    if (token != ";") {
        t.parse_error("show command must be terminated by a semicolon");
    }

    // start the show element
    int rc = xmlTextWriterStartElement(writer, BAD_CAST tag.c_str());
    if (rc < 0) {
        cerr << "failed to start " << tag << " element" << endl;
        return false;
    }

    /* Add the catalog node */
    if (!node.empty()) {
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "node",
        BAD_CAST node.c_str());
        if (rc < 0) {
            cerr << "failed to add the catalog node attribute" << endl;
            return false;
        }
    }

    // end the show element
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        cerr << "failed to close " << tag << " element" << endl;
        return false;
    }

    return true;
}

bool CmdTranslation::translate_set(BESTokenizer &t, xmlTextWriterPtr writer)
{
    string set_what = t.get_next_token();
    if (set_what.empty()) {
        t.parse_error("set command must be followed by target");
    }

    string new_cmd = "set." + set_what;
    CmdTranslation::p_cmd_translator p = _translations[new_cmd];
    if (!p) {
        cerr << "no such command: set " << set_what << endl;
        return false;
    }

    return p(t, writer);
}

bool CmdTranslation::translate_context(BESTokenizer &t, xmlTextWriterPtr writer)
{
    // set context blee to blah ;
    // <setContext name="dap_format">dap2</setContext>
    string name = t.get_next_token();
    if (name == ";") {
        t.parse_error("missing context name");
    }
    string to = t.get_next_token();
    if (to != "to") {
        t.parse_error("missing word \"to\" in set context");
    }
    string value = t.get_next_token();
    if (value == ";") {
        t.parse_error("missing context value");
    }
    string semi = t.get_next_token();
    if (semi != ";") {
        t.parse_error("set context command must end with semicolon");
    }

    // start the setContext element
    int rc = xmlTextWriterStartElement(writer, BAD_CAST "setContext");
    if (rc < 0) {
        cerr << "failed to start setContext element" << endl;
        return false;
    }

    /* Add the context name attribute */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
    BAD_CAST name.c_str());
    if (rc < 0) {
        cerr << "failed to add the context name attribute" << endl;
        return false;
    }

    /* Write the value of the context */
    rc = xmlTextWriterWriteString(writer, BAD_CAST value.c_str());
    if (rc < 0) {
        cerr << "failed to write the value of the context" << endl;
        return false;
    }

    // end the setContext element
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        cerr << "failed to close setContext element" << endl;
        return false;
    }

    return true;
}

bool CmdTranslation::translate_container(BESTokenizer &t, xmlTextWriterPtr writer)
{
    // set container in space values name,value,type;
    // <setContainer name="c" space="catalog">/data/fnoc1.nc</setContainer>
    string token = t.get_next_token();
    string space;
    if (token == "in") {
        space = t.get_next_token();
        if (space == "values" || space == ";") {
            t.parse_error("expecting name of container storage");
        }
        token = t.get_next_token();
    }
    if (token != "values") {
        t.parse_error("missing values for set container");
    }

    string name = t.get_next_token();
    if (name == ";" || name == ",") {
        t.parse_error("expecting name of the container");
    }

    token = t.get_next_token();
    if (token != ",") {
        t.parse_error("missing comma in set container after name");
    }

    string value = t.get_next_token();
    if (value == "," || value == ";") {
        t.parse_error("expecting location of the container");
    }

    token = t.get_next_token();
    string type;
    if (token == ",") {
        type = t.get_next_token();
        if (type == ";") {
            t.parse_error("expecting container type");
        }
        token = t.get_next_token();
    }

    if (token != ";") {
        t.parse_error("set container command must end with semicolon");
    }

    // start the setContainer element
    int rc = xmlTextWriterStartElement(writer, BAD_CAST "setContainer");
    if (rc < 0) {
        cerr << "failed to start setContext element" << endl;
        return false;
    }

    /* Add the container name attribute */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
    BAD_CAST name.c_str());
    if (rc < 0) {
        cerr << "failed to add the context name attribute" << endl;
        return false;
    }

    if (!space.empty()) {
        /* Add the container space attribute */
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "space",
        BAD_CAST space.c_str());
        if (rc < 0) {
            cerr << "failed to add the container space attribute" << endl;
            return false;
        }
    }

    if (!type.empty()) {
        /* Add the container space attribute */
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type",
        BAD_CAST type.c_str());
        if (rc < 0) {
            cerr << "failed to add the container type attribute" << endl;
            return false;
        }
    }

    /* Write the value of the container */
    rc = xmlTextWriterWriteString(writer, BAD_CAST value.c_str());
    if (rc < 0) {
        cerr << "failed to write the location of the container" << endl;
        return false;
    }

    // end the setContainer element
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        cerr << "failed to close setContext element" << endl;
        return false;
    }

    return true;
}

bool CmdTranslation::translate_define(BESTokenizer &t, xmlTextWriterPtr writer)
{
    // define <def_name> [in <storage_name>] as <container_list> [where // <container_x>.constraint="<constraint>",<container_x>.attributes="<attribute_list>"] // [aggregate by "<aggregation_command>"];

    // <define name="definition_name" space="store_name">
    //	<container name="container_name">
    //	    <constraint>legal_constraint</constraint>
    //	    <attributes>attribute_list</attributes>
    //	</container>
    //	<aggregate handler="someHandler" cmd="someCommand" />
    // </define>
    string name = t.get_next_token();
    string space;
    string token = t.get_next_token();
    if (token == "in") {
        space = t.get_next_token();
        token = t.get_next_token();
    }

    if (token != "as") {
        t.parse_error("Looking for keyword as in define command");
    }

    list<string> containers;
    map<string, string> clist;
    bool done = false;
    while (!done) {
        token = t.get_next_token();
        containers.push_back(token);
        clist[token] = token;
        token = t.get_next_token();
        if (token != ",") {
            done = true;
        }
    }

    // constraints and attributes
    map<string, string> constraints;
    string default_constraint;
    map<string, string> attrs;
    if (token == "with") {
        token = t.get_next_token();
        unsigned int type;
        while (token != "aggregate" && token != ";") {
            // see if we have a default constraint for all containers
            if (token == "constraint") {
                default_constraint = t.remove_quotes(t.get_next_token());
            }
            else {
                string c = t.parse_container_name(token, type);
                if (clist[c] != c) {
                    t.parse_error("constraint container does not exist");
                }
                if (type == 1) {
                    // constraint
                    constraints[c] = t.remove_quotes(t.get_next_token());
                }
                else if (type == 2) {
                    // attributed
                    attrs[c] = t.remove_quotes(t.get_next_token());
                }
                else {
                    t.parse_error("unknown constraint type");
                }
                token = t.get_next_token();
                if (token == ",") {
                    token = t.get_next_token();
                }
            }
        }
    }

    string agg_handler;
    string agg_cmd;
    if (token == "aggregate") {
        token = t.get_next_token();
        if (token == "by") {
            agg_cmd = t.remove_quotes(t.get_next_token());
            token = t.get_next_token();
            if (token != "using") {
                t.parse_error("aggregation expecting keyword \"using\"");
            }
            agg_handler = t.get_next_token();
        }
        else if (token == "using") {
            agg_handler = t.get_next_token();
            token = t.get_next_token();
            if (token != "by") {
                t.parse_error("aggregation expecting keyword \"by\"");
            }
            agg_cmd = t.remove_quotes(t.get_next_token());
        }
        else {
            t.parse_error("aggregation expecting keyword \"by\" or \"using\"");
        }

        token = t.get_next_token();
    }

    if (token != ";") {
        t.parse_error("define command must end with semicolon");
    }

    // start the define element
    int rc = xmlTextWriterStartElement(writer, BAD_CAST "define");
    if (rc < 0) {
        cerr << "failed to start setContext element" << endl;
        return false;
    }

    /* Add the definition name attribute */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
    BAD_CAST name.c_str());
    if (rc < 0) {
        cerr << "failed to add the context name attribute" << endl;
        return false;
    }

    if (!space.empty()) {
        /* Add the definition space attribute */
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "space",
        BAD_CAST space.c_str());
        if (rc < 0) {
            cerr << "failed to add the container space attribute" << endl;
            return false;
        }
    }

    // write the default constraint if we have one
    if (!default_constraint.empty()) {
        // start the constraint element
        int rc = xmlTextWriterStartElement(writer, BAD_CAST "constraint");
        if (rc < 0) {
            cerr << "failed to start container constraint element" << endl;
            return false;
        }

        /* Write the value of the constraint */
        rc = xmlTextWriterWriteString(writer, BAD_CAST default_constraint.c_str());
        if (rc < 0) {
            cerr << "failed to write constraint for container" << endl;
            return false;
        }

        // end the container constraint element
        rc = xmlTextWriterEndElement(writer);
        if (rc < 0) {
            cerr << "failed to close constraint element" << endl;
            return false;
        }
    }

    list<string>::iterator i = containers.begin();
    list<string>::iterator e = containers.end();
    for (; i != e; i++) {
        // start the container element
        int rc = xmlTextWriterStartElement(writer, BAD_CAST "container");
        if (rc < 0) {
            cerr << "failed to start container element" << endl;
            return false;
        }

        /* Add the container name attribute */
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
        BAD_CAST (*i).c_str());
        if (rc < 0) {
            cerr << "failed to add the context name attribute" << endl;
            return false;
        }

        // add constraints and attributes elements here
        string constraint = constraints[(*i)];
        if (!constraint.empty()) {
            // start the constraint element
            int rc = xmlTextWriterStartElement(writer, BAD_CAST "constraint");
            if (rc < 0) {
                cerr << "failed to start container constraint element" << endl;
                return false;
            }

            /* Write the value of the constraint */
            rc = xmlTextWriterWriteString(writer, BAD_CAST constraint.c_str());
            if (rc < 0) {
                cerr << "failed to write constraint for container" << endl;
                return false;
            }

            // end the container constraint element
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0) {
                cerr << "failed to close constraint element" << endl;
                return false;
            }
        }

        string attr = attrs[(*i)];
        if (!attr.empty()) {
            // start the attribute element
            int rc = xmlTextWriterStartElement(writer, BAD_CAST "attributes");
            if (rc < 0) {
                cerr << "failed to start container attributes element" << endl;
                return false;
            }

            /* Write the value of the constraint */
            rc = xmlTextWriterWriteString(writer, BAD_CAST attr.c_str());
            if (rc < 0) {
                cerr << "failed to write attributes for container" << endl;
                return false;
            }

            // end the container constraint element
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0) {
                cerr << "failed to close attributes element" << endl;
                return false;
            }
        }

        // end the container element
        rc = xmlTextWriterEndElement(writer);
        if (rc < 0) {
            cerr << "failed to close setContext element" << endl;
            return false;
        }
    }

    if (!agg_cmd.empty()) {
        // start the aggregation element
        int rc = xmlTextWriterStartElement(writer, BAD_CAST "aggregate");
        if (rc < 0) {
            cerr << "failed to start aggregate element" << endl;
            return false;
        }

        if (!agg_handler.empty()) {
            /* Add the aggregation handler attribute */
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "handler",
            BAD_CAST agg_handler.c_str());
            if (rc < 0) {
                cerr << "failed to add the context name attribute" << endl;
                return false;
            }
        }

        /* Add the aggregation command attribute */
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "cmd",
        BAD_CAST agg_cmd.c_str());
        if (rc < 0) {
            cerr << "failed to add the context name attribute" << endl;
            return false;
        }

        // end the aggregation element
        rc = xmlTextWriterEndElement(writer);
        if (rc < 0) {
            cerr << "failed to close setContext element" << endl;
            return false;
        }
    }

    // end the define element
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        cerr << "failed to close setContext element" << endl;
        return false;
    }

    return true;
}

bool CmdTranslation::translate_delete(BESTokenizer &t, xmlTextWriterPtr writer)
{
    // delete container <container_name> [from <storage_name>];
    // delete containers [from <storage_name>]
    // delete definition <definition_name> [from <storage_name>];
    // delete definitions [from <storage_name>];

    // <deleteContainer name="container_name" space="store_name" />
    // <deleteContainers space="store_name" />
    // <deleteDefinition name="definition_name" space="store_name" />
    // <deleteDefinitions space="store_name" />

    string del_what = t.get_next_token();
    string new_cmd = "delete." + del_what;

    CmdTranslation::p_cmd_translator p = _translations[new_cmd];
    if (p) {
        return p(t, writer);
    }

    bool single = true;
    if (del_what == "container" || del_what == "definition") {
        single = true;
    }
    else if (del_what == "containers" || del_what == "definitions") {
        single = false;
    }
    else {
        t.parse_error("unknown delete command");
    }

    del_what[0] = toupper(del_what[0]);
    string tag = "delete" + del_what;

    string name;
    if (single) {
        name = t.get_next_token();
    }

    string space;
    string token = t.get_next_token();
    if (token == "from") {
        space = t.get_next_token();
        token = t.get_next_token();
    }

    if (token != ";") {
        t.parse_error("delete command expected to end with semicolon");
    }

    // start the delete element
    int rc = xmlTextWriterStartElement(writer, BAD_CAST tag.c_str());
    if (rc < 0) {
        cerr << "failed to start aggregate element" << endl;
        return false;
    }

    if (!name.empty()) {
        /* Add the container or definition name attribute */
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
        BAD_CAST name.c_str());
        if (rc < 0) {
            cerr << "failed to add the context name attribute" << endl;
            return false;
        }
    }

    if (!space.empty()) {
        /* Add the container or definition storage space attribute */
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "space",
        BAD_CAST space.c_str());
        if (rc < 0) {
            cerr << "failed to add the context name attribute" << endl;
            return false;
        }
    }

    // end the delete element
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        cerr << "failed to close setContext element" << endl;
        return false;
    }

    return true;
}

bool CmdTranslation::translate_get(BESTokenizer &t, xmlTextWriterPtr writer)
{
    // get das|dds|dods|ddx for <definition_name> [return as <return_name>];
    // <get type="das|dds|dods|ddx" definition="def_name" returnAs="returnAs" />
    // get html_form for <definition> using <url>;
    // <get type="das|dds|dods|ddx" definition="def_name" url="url" returnAs="returnAs" />
    string get_what = t.get_next_token();
    string token = t.get_next_token();
    if (token != "for") {
        t.parse_error("get command expecting keyword \"for\"");
    }

    string def_name = t.get_next_token();
    string returnAs;
    string url;
    string starting;
    string bounding;
    token = t.get_next_token();
    bool done = false;
    while (!done) {
        if (token == "return") {
            token = t.get_next_token();
            if (token != "as") {
                t.parse_error("get command expecting keyword \"as\" for return");
            }
            returnAs = t.get_next_token();
            token = t.get_next_token();
        }
        else if (token == "using") {
            url = t.get_next_token();
            token = t.get_next_token();
        }
        else if (token == "contentStartId") {
            starting = t.get_next_token();
            token = t.get_next_token();
        }
        else if (token == "mimeBoundary") {
            bounding = t.get_next_token();
            token = t.get_next_token();
        }
        else if (token == ";") {
            done = true;
        }
        else {
            t.parse_error("unexpected token in get command");
        }
    }

    // start the get element
    int rc = xmlTextWriterStartElement(writer, BAD_CAST "get");
    if (rc < 0) {
        cerr << "failed to start aggregate element" << endl;
        return false;
    }

    /* Add the get type attribute */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "type",
    BAD_CAST get_what.c_str());
    if (rc < 0) {
        cerr << "failed to add the get type attribute" << endl;
        return false;
    }

    /* Add the get definition attribute */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "definition",
    BAD_CAST def_name.c_str());
    if (rc < 0) {
        cerr << "failed to add the get definition attribute" << endl;
        return false;
    }

    if (!url.empty()) {
        /* Add the get type attribute */
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "url",
        BAD_CAST url.c_str());
        if (rc < 0) {
            cerr << "failed to add the url attribute" << endl;
            return false;
        }
    }

    if (!returnAs.empty()) {
        /* Add the get type attribute */
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "returnAs",
        BAD_CAST returnAs.c_str());
        if (rc < 0) {
            cerr << "failed to add the returnAs attribute" << endl;
            return false;
        }
    }

    if (!starting.empty()) {
        // start the constraint element
        int rc = xmlTextWriterStartElement(writer, BAD_CAST "contentStartId");
        if (rc < 0) {
            cerr << "failed to start contentStartId element" << endl;
            return false;
        }

        /* Write the value of the contentStartId */
        rc = xmlTextWriterWriteString(writer, BAD_CAST starting.c_str());
        if (rc < 0) {
            cerr << "failed to write contentStartId for get request" << endl;
            return false;
        }

        // end the contentStartId constraint element
        rc = xmlTextWriterEndElement(writer);
        if (rc < 0) {
            cerr << "failed to close constraint element" << endl;
            return false;
        }
    }

    if (!bounding.empty()) {
        // start the mimeBoundary element
        int rc = xmlTextWriterStartElement(writer, BAD_CAST "mimeBoundary");
        if (rc < 0) {
            cerr << "failed to start mimeBoundary element" << endl;
            return false;
        }

        /* Write the value of the constraint */
        rc = xmlTextWriterWriteString(writer, BAD_CAST bounding.c_str());
        if (rc < 0) {
            cerr << "failed to write mimeBoundary for get request" << endl;
            return false;
        }

        // end the mimeBoundary constraint element
        rc = xmlTextWriterEndElement(writer);
        if (rc < 0) {
            cerr << "failed to close mimeBoundary element" << endl;
            return false;
        }
    }

    // end the get element
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        cerr << "failed to close get element" << endl;
        return false;
    }

    return true;
}

void CmdTranslation::dump(ostream &strm)
{
    strm << BESIndent::LMarg << "CmdTranslation::dump" << endl;
    BESIndent::Indent();
    if (_translations.empty()) {
        strm << BESIndent::LMarg << "NO translations registered" << endl;
    }
    else {
        strm << BESIndent::LMarg << "translations registered" << endl;
        BESIndent::Indent();
        map<string, p_cmd_translator>::iterator i = _translations.begin();
        map<string, p_cmd_translator>::iterator e = _translations.end();
        for (; i != e; i++) {
            strm << BESIndent::LMarg << (*i).first << endl;
        }
        BESIndent::UnIndent();
    }
    BESIndent::UnIndent();
}

