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
#include <algorithm>

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
class BESPluginFactory: public BESObj
{
private:
	std::map<std::string, BESPlugin<C> *> d_children;

    /** The Copy constructor is not supported. In the current implementation,
     this class uses pointers to BESPlugin<C> held in an instance of \b
     std::map. I'm not sure how \b std::map will handle the pointers when
     it is destroyed (if it will call their destructor or not), so I'm
     making this private. 07/18/02 jhrg It won't! 11/05/02 jhrg */

    // I just removed these impls entirely. jhrg 5/13/15
    BESPluginFactory(const BESPluginFactory &); // throw (BESInternalError)
#if 0
    {
        throw BESInternalError("Unimplemented method.", __FILE__, __LINE__);
    }
#endif

    /** The assignment operator is not supported.
     @see BESPluginFactory(const BESPluginFactory &pf)
     */
    const BESPluginFactory &operator=(const BESPluginFactory &); // throw (BESInternalError)
#if 0
    {
        throw BESInternalError("Unimplemented method.", __FILE__, __LINE__);
    }
#endif

    struct DeletePlugins: public std::unary_function<std::pair<std::string, BESPlugin<C> *>, void>
    {

        void operator()(std::pair<std::string, BESPlugin<C> *> elem)
        {
            delete elem.second;
        }
    };

public:
    /** Create a BESPluginFactory and set up a single entry. configure other
     entries using the add_mapping() method.

     @param name Use \b name to get an instance of the child defined in
     \b library_name.
     @param library_name The name of the library which contains the child
     class implementation.
     @see add_mapping.
     */
    BESPluginFactory(const std::string &name, const std::string &library_name)
    {
        add_mapping(name, library_name);
    }

    /** Create an empty BESPluginFactory.
     */
    BESPluginFactory()
    {
    }

    virtual ~BESPluginFactory()
    {
        for_each(d_children.begin(), d_children.end(), DeletePlugins());
    }

    /** Add a mapping of \b name to \b library_name to the BESPluginFactory.
     @param name The child object's name.
     @param library_name The name of the library which holds its
     implementation.
     */
    void add_mapping(const std::string &name, const std::string &library_name)
    {
        BESPlugin<C> *child_class = new BESPlugin<C>(library_name);
        d_children.insert(std::make_pair(name, child_class));
    }

    /** Use the BESPlugingFactory to get an instance of the class
     \b C matched to \b name. Once the name \b name has been bound to a
     SO library \b library_name, this method can be used to get an
     instance of the object whose implementation is in the SO file
     using only the name \b name.

     @param name The name registered with the implementation of a child of
     the class C using either the PlugFactory ctor or the add_mapping
     method.

     @exception NoSuchObject thrown if name has not been registered.

     @exception NoSuchLibrary thrown if the library matched to \b name
     cannot be found.
     */
    C *get(const std::string &name) throw (NoSuchObject, NoSuchLibrary)
    {
        BESPlugin<C> *child_implementation = d_children[name];
        if (!child_implementation) throw NoSuchObject(std::string("No class is bound to ") + name, __FILE__, __LINE__);
        return child_implementation->instantiate();
    }

    virtual void dump(std::ostream &strm) const
    {
        strm << "BESPluginFactory::dump - (" << (void *) this << ")" << std::endl;
        /*
         typedef map<string, BESPlugin<C> *>::const_iterator Plugin_citer ;
         BESPluginFactory::Plugin_citer i = d_children.begin() ;
         BESPluginFactory::Plugin_citer ie = d_children.end() ;
         for( ; i != ie; i++ )
         {
         strm << i.second ;
         }
         */
    }
};

#endif //plugin_h
