// BESContainerStorageFile.cc

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

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>

using std::endl;
using std::ifstream;
using std::list;
using std::ostream;
using std::string;
using std::stringstream;

#include "BESContainerStorageFile.h"
#include "BESFileContainer.h"
#include "BESInfo.h"
#include "BESInternalError.h"
#include "BESServiceRegistry.h"
#include "BESSyntaxUserError.h"
#include "TheBESKeys.h"

/** @brief pull container information from the specified file
 *
 * Constructs a BESContainerStorageFile from a file specified by
 * a key in the bes configuration file. The key is constructed using the
 * name of this persistent store.
 *
 * BES.Container.Persistence.File.&lt;name&gt;
 *
 * where &lt;name&gt; is the name of this persistent store.
 *
 * The containers are then read into memory. The format of the file is as
 * follows.
 *
 * &lt;symbolic_name&gt; &lt;real_name&gt; &lt;data type&gt;
 *
 * where the symbolic name is the symbolic name of the container, the
 * &lt;real_name&gt; represents the physical location of the data, such as
 * a file, and the &lt;data type&gt; is the type of data being represented,
 * such as netcdf, cedar, etc...
 *
 * One container per line, cannot span multiple lines
 *
 * @param n name of this persistent store
 * @throws BESInternalError if the file cannot be opened or
 * if there is an error in reading in the container information.
 * @see BESContainerStorage
 * @see BESFileContainer
 * @see BESInternalError
 */
BESContainerStorageFile::BESContainerStorageFile(const string &n) : BESContainerStorage(n) {
    // TODO: Need to store the kind of container each line represents. Does
    // it represent a file? A database entry? What? For now, they all
    // represent a BESFileContainer.

    string key = "BES.Container.Persistence.File." + n;
    bool found = false;
    TheBESKeys::TheKeys()->get_value(key, _file, found);
    if (_file.empty()) {
        string s = key + " not defined in BES configuration file";
        throw BESSyntaxUserError(s, __FILE__, __LINE__);
    }

    ifstream persistence_file(_file.c_str());
    int myerrno = errno;
    if (!persistence_file) {
        char *err = strerror(myerrno);
        string s = "Unable to open persistence file " + _file + ": ";
        if (err)
            s += err;
        else
            s += "Unknown error";

        throw BESInternalError(s, __FILE__, __LINE__);
    }

    try {
        char cline[80];

        while (!persistence_file.eof()) {
            stringstream strm;
            persistence_file.getline(cline, 80);
            if (!persistence_file.eof()) {
                strm << cline;
                auto *c = new BESContainerStorageFile::container;
                strm >> c->_symbolic_name;
                strm >> c->_real_name;
                strm >> c->_container_type;
                string dummy;
                strm >> dummy;
                if (c->_symbolic_name.empty() || c->_real_name.empty() || c->_container_type.empty()) {
                    delete c;
                    persistence_file.close();
                    string s = "Incomplete container persistence line in file " + _file;
                    throw BESInternalError(s, __FILE__, __LINE__);
                }
                if (!dummy.empty()) {
                    persistence_file.close();
                    delete c;
                    string s = "Too many fields in persistence file " + _file;
                    throw BESInternalError(s, __FILE__, __LINE__);
                }
                _container_list[c->_symbolic_name] = c;
            }
        }
        persistence_file.close();
    } catch (...) {
        persistence_file.close();
        throw;
    }
}

BESContainerStorageFile::~BESContainerStorageFile() {
    BESContainerStorageFile::Container_citer i = _container_list.begin();
    BESContainerStorageFile::Container_citer ie = _container_list.end();
    for (; i != ie; ++i) {
        BESContainerStorageFile::container *c = (*i).second;
        delete c;
    }
}

/** @brief looks for the specified container in the list of containers loaded
 * from the file.
 *
 * If a match is made with the specified symbolic name then a BESFileContainer
 * instance is created using the information found (real name and
 * container type). If not found then nullptr is returned.
 *
 * @note It is the caller's responsibility to delete this raw pointer. This would
 * be a place where a shared_ptr or weak_ptr could be used. jhrg 4/15/25
 *
 * @param sym_name name of the container to look for
 * @return a new BESFileContainer if the sym_name is found in the file, else nullptr.
 * @see BESFileContainer
 */
BESContainer *BESContainerStorageFile::look_for(const string &sym_name) {
    BESFileContainer *ret_container = nullptr;
    BESContainerStorageFile::Container_citer i;
    i = _container_list.find(sym_name);
    if (i != _container_list.end()) {
        BESContainerStorageFile::container *c = (*i).second;
        ret_container = new BESFileContainer(c->_symbolic_name, c->_real_name, c->_container_type);
    }

    return ret_container;
}

/** @brief adds a container with the provided information
 *
 * This method adds a container to the persistence store with the
 * specified information. This functionality is not currently supported for
 * file persistence.
 *
 * @param sym_name symbolic name for the container
 * @param real_name real name for the container
 * @param type type of data represented by this container
 */
void BESContainerStorageFile::add_container(const string &, const string &, const string &) {
    string err = "Unable to add a container to a file, not yet implemented";
    throw BESInternalError(err, __FILE__, __LINE__);
}

void BESContainerStorageFile::add_container(BESContainer *) {
    string err = "Unable to add a container to a file, not yet implemented";
    throw BESInternalError(err, __FILE__, __LINE__);
}

/** @brief removes a container with the given symbolic name
 *
 * This method removes a container to the persistence store with the
 * given symbolic name. It deletes the container. The container is NOT
 * removed from the file from which it was loaded, however.
 *
 * @param s_name symbolic name for the container
 * @return true if successfully removes container, false otherwise
 */
bool BESContainerStorageFile::del_container(const string &s_name) {
    bool ret = false;
    BESContainerStorageFile::Container_iter i;
    i = _container_list.find(s_name);
    if (i != _container_list.end()) {
        BESContainerStorageFile::container *c = (*i).second;
        _container_list.erase(i);
        if (c) {
            delete c;
        }
        ret = true;
    }
    return ret;
}

/** @brief removes all containers
 *
 * This method removes all containers from the persistent store. The
 * container is NOT removed from the file from which it was loaded, however.
 *
 * @return true if successfully removes all containers, false otherwise
 */
bool BESContainerStorageFile::del_containers() {
    while (_container_list.size() != 0) {
        Container_iter ci = _container_list.begin();
        BESContainerStorageFile::container *c = (*ci).second;
        _container_list.erase(ci);
        if (c) {
            delete c;
        }
    }
    return true;
}

/** @brief determine if the given container is data and what servies
 * are available for it
 *
 * @param inQuestion the container in question
 * @param provides an output parameter for storing the list of
 * services provided for this container
 */
bool BESContainerStorageFile::isData(const string &inQuestion, list<string> &provides) {
    bool isit = false;
    BESContainer *c = look_for(inQuestion);
    if (c) {
        isit = true;
        string node_type = c->get_container_type();
        BESServiceRegistry::TheRegistry()->services_handled(node_type, provides);
        delete c;
        c = nullptr; // added jhrg 1.4.12
    }
    return isit;
}

/** @brief show information for each container in this persistent store
 *
 * For each container in this persistent store, add infomation about each of
 * those containers. The information added to the information object
 * includes a line for each container within this persistent store which
 * includes the symbolic name, the real name, and the data type,
 * separated by commas.
 *
 * In the case of this persistent store all of the containers loaded from
 * the file specified by the key
 * BES.Container.Persistence.File.&lt;store_name&gt;
 * is added to the information object.
 *
 * @param info object to store the container and persistent store information into
 * @see BESInfo
 */
void BESContainerStorageFile::show_containers(BESInfo &info) {
    BESContainerStorageFile::Container_citer i;
    i = _container_list.begin();
    for (i = _container_list.begin(); i != _container_list.end(); i++) {
        BESContainerStorageFile::container *c = (*i).second;
        string sym = c->_symbolic_name;
        string real = c->_real_name;
        string type = c->_container_type;
        show_container(sym, real, type, info);
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * the containers in this storage
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESContainerStorageFile::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESContainerStorageFile::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "name: " << get_name() << endl;
    strm << BESIndent::LMarg << "file: " << _file << endl;
    if (_container_list.size()) {
        strm << BESIndent::LMarg << "containers:" << endl;
        BESIndent::Indent();
        BESContainerStorageFile::Container_citer i = _container_list.begin();
        BESContainerStorageFile::Container_citer ie = _container_list.end();
        for (i = _container_list.begin(); i != ie; i++) {
            BESContainerStorageFile::container *c = (*i).second;
            strm << BESIndent::LMarg << c->_symbolic_name;
            strm << ", " << c->_real_name;
            strm << ", " << c->_container_type;
            strm << endl;
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "    containers: none" << endl;
    }
    BESIndent::UnIndent();
}
