
// -*- C++ -*-

// (c) COPYRIGHT DAS, llc. 2002
//
// Author: James Gallagher <jgallagher@gso.uri.edu>

#ifndef plugin_factory_h
#define plugin_factory_h

#include <string>
#include <map>
#include <algorithm>

#include "BESPlugin.h"
#include "debug.h"

using std::string;
using std::map;
using std::pair;
using std::unary_function;

/** A Factory for objects whose implementations reside in shared objects
    designed to be loaded at run time. This uses the BESPlugin class
    to perform the actual instantiation of those objects; the role of this
    class is to maintain a mapping between the names of the SO libraries
    and the names of the C++ object implementations they hold.

    @see BESPlugin
*/

template<typename C>
class BESPluginFactory {
private:
    map<string, BESPlugin<C> *> d_children;

    /** The Copy constructor is not supported. In the current implementation,
	this class uses pointers to BESPlugin<C> held in an instance of \b
	std::map. I'm not sure how \b std::map will handle the pointers when
	it is destroyed (if it will call their destructor or not), so I'm
	making this private. 07/18/02 jhrg It won't! 11/05/02 jhrg */

    BESPluginFactory(const BESPluginFactory &pf)
	throw(BESPluginException)
    {
	throw BESPluginException( "Unimplemented method.");
    }

    /** The assignment operator is not supported. 
	@see BESPluginFactory(const BESPluginFactory &pf)
    */
    const BESPluginFactory &operator=(const BESPluginFactory &rhs)
	throw (BESPluginException)
    {
	throw BESPluginException( "Unimplemented method.");
    }

    struct DeletePlugins 
	: public unary_function<pair<string, BESPlugin<C> *>, void>
    {

	void operator()(pair<string, BESPlugin<C> *> elem) {
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
    BESPluginFactory(const string &name, const string &library_name)
    {
	add_mapping(name, library_name);
    }

    /** Create an empty BESPluginFactory.
    */
    BESPluginFactory() { }

    virtual ~BESPluginFactory()
    {
	for_each(d_children.begin(), d_children.end(), DeletePlugins());
    }

    /** Add a mapping of \b name to \b library_name to the BESPluginFactory.
	@param name The child object's name.
	@param library_name The name of the library which holds its
	implementation.
    */
    void add_mapping(const string &name, const string &library_name)
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
    C *get(const string &name) throw(NoSuchObject, NoSuchLibrary)
    {
	BESPlugin<C> *child_implementation = d_children[name];
	if (!child_implementation)
	    throw NoSuchObject(string("No class is bound to ") + name, __FILE__, __LINE__ );
	return child_implementation->instantiate();
    }
};

#endif //plugin_h

