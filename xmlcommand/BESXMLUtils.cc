// BESXMLUtils.cc

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

#include "BESXMLUtils.h"
#include "BESUtil.h"

using std::vector;
using std::string;
using std::map;

/** @brief error function used by libxml2 to report errors
 *
 * libxml2 has a default error function which simply displays the errors to
 * stderr or stdout, whichever. This function can be used to replace the
 * default function and store the error strings in a vector of strings
 *
 * @param context vector<string> pointer to list of error strings
 * @param msg the error message format string
 * @param ... the arguments to the error message format string
 */
void BESXMLUtils::XMLErrorFunc(void *context, const char *msg, ...)
{
    va_list args;
    va_start( args, msg );
    char mymsg[1024];
    vsnprintf(mymsg, sizeof mymsg, msg, args);
    va_end(args); // Added jhrg 9/17/15
    vector<string> *myerrors = (vector<string> *) context;
    myerrors->push_back(mymsg);
}

/**
 * @brief given an xml node, build the map of properties (xml attributes) for that node
 *
 * Properties (xml attributes) can have multiple values, hence the need for a map of string
 * keys and a list of values.
 *
 * @param node xml node to retrieve properties from
 * @param props map to store the property name and values in
 */
void BESXMLUtils::GetProps(xmlNode *node, map<string, string> &props)
{
    if (!node) {
        return;
    }

    if (node->properties == NULL) {
        return;
    }

    xmlAttr *curr_prop = node->properties;
    while (curr_prop) {
        string prop_name = (char *) curr_prop->name;
        BESUtil::removeLeadingAndTrailingBlanks(prop_name);
        string prop_val;
        xmlNode *curr_val = curr_prop->children;
        if (curr_val && curr_val->content) {
            prop_val = BESUtil::xml2id((char *) curr_val->content);
            BESUtil::removeLeadingAndTrailingBlanks(prop_val);
        }
        props[prop_name] = prop_val;

        curr_prop = curr_prop->next;
    }
}

/** @brief get the name, value if any, and any properties for the specified
 * node
 *
 * @param node the xml node to get the information for
 * @param name value-result parameter: the name of the node
 * @param value v-r parameter: the content of the node, if this is a text node,
 * else the empty string
 * @param props v-r parameter to store any properties of the node
 */
void BESXMLUtils::GetNodeInfo(xmlNode *node, string &name, string &value, map<string, string> &props)
{
    if (node) {
        name = (char *) node->name;
        BESUtil::removeLeadingAndTrailingBlanks(name);
        BESXMLUtils::GetProps(node, props);
        xmlNode *child_node = node->children;
        bool done = false;
        while (child_node && !done) {
            if (child_node->type == XML_TEXT_NODE) {
                if (child_node->content) {
                    value = BESUtil::xml2id((char *)child_node->content);
                    BESUtil::removeLeadingAndTrailingBlanks(value);
                }
                else {
                    value = "";
                }
                done = true;
            }
            child_node = child_node->next;
        }
    }
}

/** @brief get the first element child node for the given node
 *
 * @param node the xml node to get the first element child from
 * @param child_name parameter to store the name of the first child
 * @param child_value parameter to store the value, if any, of the first child
 * @param child_props parameter to store any properties of the first child
 */
xmlNode *
BESXMLUtils::GetFirstChild(xmlNode *node, string &child_name, string &child_value, map<string, string> &child_props)
{
    xmlNode *child_node = NULL;
    if (node) {
        child_node = node->children;
        bool done = false;
        while (child_node && !done) {
            if (child_node->type == XML_ELEMENT_NODE) {
                done = true;
                BESXMLUtils::GetNodeInfo(child_node, child_name, child_value, child_props);
            }
            else {
                child_node = child_node->next;
            }
        }
    }
    return child_node;
}

/** @brief get the next element child node after the given child node
 *
 * @param child_node get the next child after this child
 * @param next_name parameter to store the name of the next child
 * @param next_value parameter to store the value, if any, of the next child
 * @param next_props parameter to store any properties of the next child
 */
xmlNode *
BESXMLUtils::GetNextChild(xmlNode *child_node, string &next_name, string &next_value, map<string, string> &next_props)
{
    if (child_node) {
        child_node = child_node->next;
        bool done = false;
        while (child_node && !done) {
            if (child_node->type == XML_ELEMENT_NODE) {
                done = true;
                BESXMLUtils::GetNodeInfo(child_node, next_name, next_value, next_props);
            }
            else {
                child_node = child_node->next;
            }
        }
    }
    return child_node;
}

/** @brief get the element child node of the given node with the given name
 *
 * @param node the xml node to get the named child node for
 * @param child_name name of the child element node to get
 * @param child_value parameter to store the value, if any, of the named child
 * @param child_props parameter to store any properties of the named child
 */
xmlNode *
BESXMLUtils::GetChild(xmlNode *node, const string &child_name, string &child_value, map<string, string> &child_props)
{
    xmlNode *child_node = NULL;
    if (node) {
        child_node = node->children;
        bool done = false;
        while (child_node && !done) {
            if (child_node->type == XML_ELEMENT_NODE) {
                string name = (char *) child_node->name;
                BESUtil::removeLeadingAndTrailingBlanks(name);
                if (name == child_name) {
                    done = true;
                    BESXMLUtils::GetNodeInfo(child_node, name, child_value, child_props);
                }
                else {
                    child_node = child_node->next;
                }
            }
            else {
                child_node = child_node->next;
            }
        }
    }
    return child_node;
}

