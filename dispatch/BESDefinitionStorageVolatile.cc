// BESDefinitionStorageVolatile.cc

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

#include "BESDefinitionStorageVolatile.h"
#include "BESDefine.h"
#include "BESInfo.h"

using std::endl;
using std::string;
using std::ostream;

BESDefinitionStorageVolatile::~BESDefinitionStorageVolatile()
{
    del_definitions();
}

/** @brief looks for a definition in this volatile store with the
 * given name
 *
 * @param def_name name of the definition to look for
 * @return definition with the given name, NULL if not found
 */
BESDefine *
BESDefinitionStorageVolatile::look_for(const string &def_name)
{
    Define_citer i;
    i = _def_list.find(def_name);
    if (i != _def_list.end()) {
        return (*i).second;
    }
    return NULL;
}

/** @brief adds a given definition to this volatile storage
 *
 * This method adds a definition to the definition store
 *
 * @param def_name name of the definition to add
 * @param d definition to add
 */
bool BESDefinitionStorageVolatile::add_definition(const string &def_name, BESDefine *d)
{
    if (look_for(def_name) == NULL) {
        _def_list[def_name] = d;
        return true;
    }
    return false;
}

/** @brief deletes a defintion with the given name from this volatile store
 *
 * This method deletes a definition from the definition store with the
 * given name.
 *
 * @param def_name name of the defintion to delete
 * @return true if successfully deleted and false otherwise
 */
bool BESDefinitionStorageVolatile::del_definition(const string &def_name)
{
    bool ret = false;
    Define_iter i;
    i = _def_list.find(def_name);
    if (i != _def_list.end()) {
        BESDefine *d = (*i).second;
        _def_list.erase(i);
        delete d;
        ret = true;
    }
    return ret;
}

/** @brief deletes all defintions from the definition store
 *
 * @return true if successfully deleted and false otherwise
 */
bool BESDefinitionStorageVolatile::del_definitions()
{
    while (_def_list.size() != 0) {
        Define_iter di = _def_list.begin();
        BESDefine *d = (*di).second;
        _def_list.erase(di);
        if (d) {
            delete d;
        }
    }
    return true;
}

/** @brief show the definitions stored in this store
 *
 * Add information to the passed information object about each of the
 * definitions stored within this definition store. The information
 * added to the passed information objects includes the name of this
 * persistent store on the first line followed by the information for
 * each definition on the following lines, one per line.
 *
 * @param info information object to store the information in
 */
void BESDefinitionStorageVolatile::show_definitions(BESInfo &info)
{
    std::map<string, string, std::less<>> dprops; // for the definition
    std::map<string, string, std::less<>> cprops; // for the container
    std::map<string, string, std::less<>> aprops; // for aggregation
    Define_citer di = _def_list.begin();
    Define_citer de = _def_list.end();
    for (; di != de; di++) {
        string def_name = (*di).first;
        BESDefine *def = (*di).second;

        dprops.clear();
        dprops["name"] = def_name;
        info.begin_tag("definition", &dprops);

        auto ci = def->first_container();
        auto ce = def->end_container();
        for (; ci != ce; ci++) {
            cprops.clear();

            string sym = (*ci)->get_symbolic_name();
            cprops["name"] = sym;

            // FIXME: need to get rid of the root directory
            string real = (*ci)->get_real_name();

            string type = (*ci)->get_container_type();
            cprops["type"] = type;

            string con = (*ci)->get_constraint();
            if (!con.empty()) {
                cprops["constraint"] = con;
            }

            string attrs = (*ci)->get_attributes();
            if (!attrs.empty()) {
                cprops["attributes"] = attrs;
            }

            info.add_tag("container", real, &cprops);
        }

        if (!def->get_agg_handler().empty()) {
            aprops.clear();
            aprops["handler"] = def->get_agg_handler();
            info.add_tag("aggregation", def->get_agg_cmd(), &aprops);
        }

        info.end_tag("definition");
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with all the definition
 * stored in this instance.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDefinitionStorageVolatile::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESDefinitionStorageVolatile::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "name: " << get_name() << endl;
    if (_def_list.size()) {
        strm << BESIndent::LMarg << "definitions:" << endl;
        BESIndent::Indent();
        Define_citer di = _def_list.begin();
        Define_citer de = _def_list.end();
        for (; di != de; di++) {
            (*di).second->dump(strm);
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "definitions: none" << endl;
    }
    BESIndent::UnIndent();
}

