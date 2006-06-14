
// -*- C++ -*-

// (c) COPYRIGHT DAS, llc. 2001
//
// Author: James Gallagher <jgallagher@gso.uri.edu>

#ifndef T_BESPlugin_h
#define T_BESPlugin_h

#include <dlfcn.h>
#include <string>
#include <iostream>

#include "debug.h"
#include "BESPluginException.h"

using std::string;
using std::cerr;
using std::endl;

/** Thrown as an exception when BESPlugin cannot find the named shareable
    library.
*/
class NoSuchLibrary : public BESPluginException {
public:
    NoSuchLibrary(const string &msg) : BESPluginException(msg) {}
};

/** Thrown as an exception when BESPlugin cannot find or run the maker()
    function in a shared library already loaded.
*/
class NoSuchObject : public BESPluginException {
public:
    NoSuchObject(const string &msg) : BESPluginException(msg) {}
};

/** BESPlugin provides a mechanism that can load C++ classes at runtime.
    Classes are compiled and stored in shareable-object libraries. This
    class binds the name of that class (which is used by Plugin's client)
    to the name of the library.

    Each class/library must contain at least one function. The function must
    have the name and type signature `extern "C" M* maker()' and must return
    a pointer to a new instance of the class \b M. Note that \b M is the
    parameter of the Plugin template. Suppose you have a base class \b Base
    and a collection of specializations \b S1, \b S2, ..., \b Sn. You would
    use \b N instances of BESPlugin<Base> to provide access to the
    implementations in those \n N shareable-object libraries. The exectuable
    that loads the libraries must have been compiled and linked with \b Base.

    External symbols defined in the library will be made available to
    subsequently loaded libraries.
    
    @author James Gallagher
*/

template<typename M>
class BESPlugin {
private:
    string d_filename;		// Library filename
    void *d_lib;		// Open library handle

    /** Do not allow empty instances to be created.
    */
    BESPlugin()  throw(BESPluginException) {	
	throw BESPluginException( "Unimplemented method.");
    }

    /** Do not allow clients to use the copy constructor. BESPlugin
        manages the dlopen resource and it's important to not copy that
	pointer (since doing so could result in calling dlclose too many
	times, something that is apt to be bad.
    */
    BESPlugin(const BESPlugin &p) throw(BESPluginException) {
	throw BESPluginException( "Unimplemented method.");
    }

    /** Do not allow clients to use the assignment operator.
	@see BESPlugin(const BESPlugin &p)
    */
    BESPlugin &operator=(const BESPlugin &p) throw(BESPluginException) {
	throw BESPluginException( "Unimplemented method.");
    }

    void *get_lib() throw(NoSuchLibrary) {
	if (!d_lib) {
	    d_lib = dlopen(d_filename.c_str(), RTLD_NOW|RTLD_GLOBAL);
	    if (d_lib == NULL) {
		DBG(cerr << "Error opening library: " << dlerror() << endl);
		throw NoSuchLibrary(string(dlerror()));
	    }
	}

	return d_lib;
    }

public:
    /** Create a new BESPlugin.
	@param filename The name of the shareable object library that holds
	the class' implementation.
    */
    BESPlugin(const string &filename) : d_filename(filename), d_lib(0) {}

    /** The destructor closes the library.
    */
    virtual ~BESPlugin() {
	if (d_lib)
	    dlclose(d_lib);
    }

    /** Instantiate the object. Using the \b maker function found in the
	shreable-object library, create a new instance of class \b M where \b
	M was the template parameter of BESPlugin.

	@return A pointer to the new instance.
    */
    M* instantiate() throw(NoSuchLibrary, NoSuchObject) {
	void *maker = dlsym(get_lib(), "maker");
	if (!maker) {
	    DBG(cerr << "Error running maker: " << dlerror() << endl);
	    throw NoSuchObject(string(dlerror()));
	}
    
	typedef M *(*maker_func_ptr)();
	maker_func_ptr my_maker = *reinterpret_cast<maker_func_ptr*>(&maker);
	M *my_M = (my_maker)();

	return my_M;
    }
};

#endif // T_BESPlugin_h

