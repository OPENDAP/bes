// BESPlugin.h

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

#ifndef T_BESPlugin_h
#define T_BESPlugin_h

#include <dlfcn.h>
#include <string>
#include <iostream>

#include "BESObj.h"
#include "BESInternalFatalError.h"
#include "BESInternalError.h"
#include "BESDebug.h"

#undef UNPLUG_HANDLERS

/** Thrown as an exception when BESPlugin cannot find the named shareable
 library.
 */
class NoSuchLibrary: public BESInternalFatalError {
public:
    NoSuchLibrary(const std::string &msg, const std::string &file, int line) :
            BESInternalFatalError(msg, file, line)
    {
    }
};

/** Thrown as an exception when BESPlugin cannot find or run the maker()
 function in a shared library already loaded.
 */
class NoSuchObject: public BESInternalFatalError {
public:
    NoSuchObject(const std::string &msg, const std::string &file, int line) :
            BESInternalFatalError(msg, file, line)
    {
    }
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
class BESPlugin: public BESObj {
private:
	std::string d_filename; // Library filename
    void *d_lib = nullptr; // Open library handle

    /// THis is pretty simple caching scheme. However, do we need this?
    /// but it probably does not cost that much.
    void *get_lib() {
        if (!d_lib) {
            d_lib = dlopen(d_filename.c_str(), RTLD_LAZY /*RTLD_NOW*/ | RTLD_GLOBAL);
            BESDEBUG( "bes", "BESPlugin: plug in handler:" << d_filename << ", " << d_lib << std::endl);
            if (d_lib == nullptr) {
                throw NoSuchLibrary(std::string(dlerror()), __FILE__, __LINE__);
            }
        }

        return d_lib;
    }

public:
    BESPlugin() = delete;
    BESPlugin(const BESPlugin &p) = delete;

    /**
     * Create a new BESPlugin.
     * @param filename The name of the sharable object library that holds the class' implementation.
     */
    explicit BESPlugin(const std::string &filename) : d_filename(filename) { }

    ~BESPlugin() override = default;

    BESPlugin &operator=(const BESPlugin &p) = delete;

    /**
     * Instantiate the object. Using the \b maker function found in the shareable-object library,
     * create a new instance of class \b M where \b M was the template parameter of BESPlugin.
     * @return A pointer to the new instance.
     */
    M* instantiate() {
        void *maker = dlsym(get_lib(), "maker");
        if (!maker) {
            throw NoSuchObject(std::string(dlerror()), __FILE__, __LINE__);
        }

        typedef M *(*maker_func_ptr)();
        maker_func_ptr my_maker = *reinterpret_cast<maker_func_ptr*>(&maker);
        M *my_M = my_maker();

        return my_M;
    }

    void dump(std::ostream &strm) const override  {
        strm << "BESPlugin::dump - (" << std::ios::hex << this << ")" << std::endl;
        strm << "    plugin name: " << d_filename << std::endl;
        strm << "    library handle: " << std::ios::hex << d_lib << std::endl;
    }
};

#endif // T_BESPlugin_h
