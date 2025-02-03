// BESContainerStorageVolatile.cc

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

#include "config.h"

#include "BESContainerStorageVolatile.h"
#include "BESFileContainer.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"
#include "BESInfo.h"
#include "TheBESKeys.h"
#include "BESUtil.h"
#include "BESServiceRegistry.h"
#include "BESDebug.h"

// FIXME This is a lie, but it will help with debugging/design for the RemoteResources
// FIXME design/fix. Remove this once that's done. jhrg 8/7/20
#define MODULE "ngap"
#define prolog std::string("BESContainerStorageVolatile::").append(__func__).append("() - ")

using std::endl;
using std::string;
using std::list;
using std::ostream;

/** @brief create an instance of this persistent store with the given name.
 *
 * Creates an instances of BESContainerStorageVolatile with the given name.
 *
 * @note This constructor will fail with an Internal Fatal Error if the BES
 * Keys file cannot be read. That is, the if the singleton TheBESKeys cannot
 * be initialized, the access to the BES keys will throw a BESInternalFatalError.
 *
 * @todo Remove the non-BESCatalog file system stuff in this class and combine it with
 * BESContainerStorageCatalog.
 *
 * @param n name of this persistent store
 * @see BESContainer
 */
BESContainerStorageVolatile::BESContainerStorageVolatile(const string &n) :
    BESContainerStorage(n), _root_dir(""), _follow_sym_links(false)
{
    string key = "BES.Data.RootDirectory";
    bool found = false;
    TheBESKeys::TheKeys()->get_value(key, _root_dir, found);
    if (_root_dir == "") {
        string s = key + " not defined in BES configuration file";
        throw BESSyntaxUserError(s, __FILE__, __LINE__);
    }

    found = false;
    key = (string) "BES.FollowSymLinks";
    string s_str;
    TheBESKeys::TheKeys()->get_value(key, s_str, found);
    s_str = BESUtil::lowercase(s_str);
    if (found && (s_str == "yes" || s_str == "on" || s_str == "true")) {
        _follow_sym_links = true;
    }
}

BESContainerStorageVolatile::~BESContainerStorageVolatile()
{
    del_containers();
}

/** @brief looks for the specified container using the symbolic name passed
 *
 * If a match is made with the symbolic name then the stored container is
 * duplicated and returned to the user. If not, 0 is returned.
 *
 * @param sym_name symbolic name of the container to look for
 * @return a new BESContainer instance using the ptr_duplicate method on
 * BESContainer
 */
BESContainer *
BESContainerStorageVolatile::look_for(const string &sym_name)
{
    BESContainer *ret_container = 0;

    BESContainerStorageVolatile::Container_citer i;
    i = _container_list.find(sym_name);
    if (i != _container_list.end()) {
#if 1
        BESContainer *c = (*i).second;
        ret_container = c->ptr_duplicate();
#else
        ret_container = (*i).second;
#endif
    }

    return ret_container;
}

/** @brief add a file container to the volatile list. The container's
 * realname represents the path to a data file in the file system.
 *
 * If a container other than a BESFileContainer is to be added to the list
 * then a derived class of BESContainerStorageVolatile should call the
 * protected add_container method that takes an already built container.
 * This method adds a container represented by a data file.
 *
 * @param sym_name symbolic name of the container being created
 * @param real_name The real name of the container, as provided by the user/client.
 * This name is relative to the BES Data Root Directory
 * @param type type of data represented by this container
 * @throws BESInternalError if no type is specified
 * @throws BESInternalError if a container with the passed
 * symbolic name already exists.
 */
void BESContainerStorageVolatile::add_container(const string &sym_name, const string &real_name, const string &type)
{
    // The type must be specified so that we can find the request handler
    // that knows how to handle the container.
    // Changed sym_name to real_name to make the message clearer. jhrg 11/14/19
    if (type.empty())
        throw BESInternalError(string("Unable to add container '").append(real_name).append("', the type of data must be specified."), __FILE__, __LINE__);

    // if the container already exists then throw an error
    BESContainerStorageVolatile::Container_citer i = _container_list.find(sym_name);
    if (i != _container_list.end()) {
        throw BESInternalError(string("A container with the name '").append(sym_name).append("' already exists"), __FILE__, __LINE__);
    }

    // make sure that the path to the container exists. If follow_sym_links
    // is false and there is a symbolic link in the path then an error will
    // be thrown. If the path does not exist, an error will be thrown.
    BESUtil::check_path(real_name, _root_dir, _follow_sym_links);

    // add the root directory to the real_name passed
    string fully_qualified_real_name = BESUtil::assemblePath(_root_dir, real_name, false);

    BESDEBUG("container","BESContainerStorageVolatile::add_container() - "
    		<< " _root_dir: " << _root_dir
    		<< " real_name: " << real_name
    		<< " symbolic name: " << sym_name
			<< " fully_qualified_real_name: " << fully_qualified_real_name
			<< " type: " << type
			<< endl);

    // Create the file container with the new information
    BESContainer *c = new BESFileContainer(sym_name, fully_qualified_real_name, type);
    c->set_relative_name(real_name);

    // add it to the container list
    _container_list[sym_name] = c;
}

/** @brief add the passed container to the list of containers in volatile
 * storage
 *
 * This method adds the passed container to the list of volatile containers.
 * The passed container is owned by the list if added and should not be
 * deleted by the caller.
 *
 * If a container with the symbolic name of the passed container is already
 * in the list then an exception is thrown.
 *
 * @param c container to add to the list
 * @throws BESContainerStorageExcpetion if the passed container is null
 * @throws BESContainerStorageExcpetion if no type is specified in the
 * passed container
 * @throws BESContainerStorageExcpetion if a container with the passed
 * symbolic name already exists.
 */
void BESContainerStorageVolatile::add_container(BESContainer *c)
{
    if (!c) {
        throw BESInternalError("Unable to add container, container passed is null", __FILE__, __LINE__);
    }
    if (c->get_container_type().empty()) {
        throw BESInternalError("Unable to add container, the type of data must be specified.", __FILE__, __LINE__);
    }

    string sym_name = c->get_symbolic_name();

    BESContainerStorageVolatile::Container_citer i = _container_list.find(sym_name);
    if (i != _container_list.end()) {
        throw BESInternalError(string("A container with the name '").append(sym_name).append("' already exists"), __FILE__, __LINE__);
    }

    _container_list[sym_name] = c;
}

/** @brief removes a container with the given symbolic name from the list
 * and deletes it.
 *
 * @param s_name symbolic name for the container
 * @return true if successfully removed and false otherwise
 */
bool BESContainerStorageVolatile::del_container(const string &s_name)
{
    BESDEBUG(MODULE, prolog << "BEGIN: " << s_name <<  endl);
    bool ret = false;
    BESContainerStorageVolatile::Container_iter i = _container_list.find(s_name);
    if (i != _container_list.end()) {
        BESContainer *c = (*i).second;
        _container_list.erase(i);
        if (c) {
            BESDEBUG(MODULE, prolog << "delete the container: "<< (void *) c <<  endl);
            delete c;
        }
        ret = true;
    }
    BESDEBUG(MODULE, prolog << "END" <<  endl);
    return ret;
}

/** @brief removes all container
 *
 * This method removes all containers from the persistent store. It does
 * not delete the real data behind the container.
 *
 * @return true if successfully removed and false otherwise
 */
bool BESContainerStorageVolatile::del_containers()
{
    BESDEBUG(MODULE, prolog << "BEGIN" <<  endl);
    while (_container_list.size() != 0) {
        Container_iter ci = _container_list.begin();
        BESContainer *c = (*ci).second;
        _container_list.erase(ci);
        if (c) {
            BESDEBUG(MODULE, prolog << "delete the container: "<< (void *) c <<  endl);
            delete c;
        }
    }
    BESDEBUG(MODULE, prolog << "END" <<  endl);
    return true;
}

/** @brief determine if the given container is data and what servies
 * are available for it
 *
 * @param inQuestion the container in question
 * @param provides an output parameter for storing the list of
 * services provided for this container
 */
bool BESContainerStorageVolatile::isData(const string &inQuestion, list<string> &provides)
{
    bool isit = false;
    BESContainer *c = look_for(inQuestion);
    if (c) {
        isit = true;
        string node_type = c->get_container_type();
        BESServiceRegistry::TheRegistry()->services_handled(node_type, provides);
    }
    return isit;
}

/** @brief show information for each container in this persistent store
 *
 * For each container in this persistent store, add information about each of
 * those containers. The information added to the information object
 * includes a line for each container within this persistent store which 
 * includes the symbolic name, the real name, and the data type, 
 * separated by commas.
 *
 * In the case of this persistent store information from each container
 * added to the volatile list is added to the information object.
 *
 * @param info object to store the container and persistent store information
 * @see BESInfo
 */
void BESContainerStorageVolatile::show_containers(BESInfo &info)
{
    info.add_tag("name", get_name());
    string::size_type root_len = _root_dir.size();
    BESContainerStorageVolatile::Container_iter i = _container_list.begin();
    BESContainerStorageVolatile::Container_iter e = _container_list.end();
    for (; i != e; i++) {
        BESContainer *c = (*i).second;
        string sym = c->get_symbolic_name();
        string real = c->get_real_name();
        if (real.size() > root_len) {
            if (real.compare(0, root_len, _root_dir) == 0) {
                real = real.substr(root_len, real.size() - root_len);
            }
        }
        string type = c->get_container_type();
        show_container(sym, real, type, info);
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * the containers stored in this volatile list.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESContainerStorageVolatile::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESContainerStorageVolatile::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "name: " << get_name() << endl;
    if (_container_list.size()) {
        strm << BESIndent::LMarg << "containers:" << endl;
        BESIndent::Indent();
        BESContainerStorageVolatile::Container_citer i = _container_list.begin();
        BESContainerStorageVolatile::Container_citer ie = _container_list.end();
        for (; i != ie; i++) {
            BESContainer *c = (*i).second;
            c->dump(strm);
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "containers: none" << endl;
    }
    BESIndent::UnIndent();
}

