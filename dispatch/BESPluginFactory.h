// BESPluginFactory.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
// and James Gallagher <jgallagher@gso.uri.edu>
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
//      jimg        James Gallagher <jgallagher@gso.uri.edu>

#ifndef plugin_factory_h
#define plugin_factory_h

#include <string>
#include <map>
#include <memory>

#include "BESPlugin.h"

#include "BESObj.h"

/** A Factory for objects whose implementations reside in shared objects
 designed to be loaded at run time. This uses the BESPlugin class
 to perform the actual instantiation of those objects; the role of this
 class is to maintain a mapping between the names of the SO libraries
 and the names of the C++ object implementations they hold.

 @see BESPlugin
 */

template<typename C>
class BESPluginFactory : public BESObj {
    std::map<std::string, std::unique_ptr<BESPlugin<C>> > d_children;

public:
    BESPluginFactory() = default;

    /**
     * Make a BESPluginFactory and set up a single entry. configure other entries using the add_mapping() method.
     * @param name Use \b name to get an instance of the child defined in \b library_name.
     * @param library_name The name of the library which contains the child class implementation.
     * @see add_mapping.
     */
    BESPluginFactory(const std::string &name, const std::string &library_name) {
        add_mapping(name, library_name);
    }

    BESPluginFactory(const BESPluginFactory &) = delete;

    ~BESPluginFactory() override = default;

    const BESPluginFactory &operator=(const BESPluginFactory &) = delete;

    /** Add a mapping of \b name to \b library_name to the BESPluginFactory.
     @param name The child object's name.
     @param library_name The name of the library which holds its
     implementation.
     */
    void add_mapping(const std::string &name, const std::string &library_name) {
        d_children.emplace(name, std::make_unique<BESPlugin<C>>(library_name));
    }

    /** Use the BESPluginFactory to get an instance of the class
     \b C matched to \b name. Once the name \b name has been bound to a
     SO library \b library_name, this method can be used to get an
     instance of the object whose implementation is in the SO file
     using only the name \b name.

     @param name The name registered with the implementation of a child of
     the class C using either the PlugFactory ctor or the add_mapping
     method.

     @exception NoSuchObject thrown if name has not been registered.
     */
    C *get(const std::string &name) {
        auto plugin_it = d_children.find(name);
        if (plugin_it == d_children.end()) {
            throw NoSuchObject(std::string("No class is bound to ") + name, __FILE__, __LINE__);
        }
        return plugin_it->second->instantiate();
    }

    void dump(std::ostream &strm) const override {
        strm << "BESPluginFactory::dump - (" << std::ios::hex << this << ")" << std::endl;
    }
};

#endif //plugin_h
