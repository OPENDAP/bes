
// BESModuleApp.C

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

#include "BESModuleApp.h"
#include "BESError.h"
#include "BESPluginFactory.h"
#include "BESAbstractModule.h"
#include "TheBESKeys.h"
#include "BESUtil.h"

using namespace std;

/** @brief Default constructor
 *
 * Initialized the static _the Applicatioon to point to this application
 * object
 */
BESModuleApp::BESModuleApp() :
        BESApp()
{
}

#if 0
/** @brief Default destructor
 *
 * sets the static _theApplication to null. Does not call terminate. It is up
 * to the main method to call the terminate method.
 */
BESModuleApp::~BESModuleApp()
{
}
#endif

/** @brief Load and initialize any BES modules
 *
 * @return 0 if successful and not 0 otherwise
 * @param argC argc value passed to the main function
 * @param argV argv value passed to the main function
 */
int BESModuleApp::initialize(int argC, char **argV)
{
    int retVal = BESApp::initialize(argC, argV);
    if (!retVal) {
        try {
            retVal = loadModules();
        }
        catch( BESError &e ) {
            cerr << "Error during module initialization: " << e.get_message() << endl;
            retVal = 1;
        }
        catch( ... ) {
            cerr << "Error during module initialization: Unknown exception"  << endl;
            retVal = 1;
        }
    }

    return retVal;
}

/** @brief load data handler modules using the initialization file
 */
int BESModuleApp::loadModules()
{
    int retVal = 0;

    bool found = false;
    vector<string> vals;
    TheBESKeys::TheKeys()->get_values("BES.modules", vals, found);
    vector<string>::iterator l = vals.begin();
    vector<string>::iterator le = vals.end();

    // The following code was likely added before we had the 'Include'
    // directive. Now all modules have a line in their .conf file to
	// Include dap.conf and that makes this redundant. However, what
    // was happening was a module named XdapX would match the find()
    // call below and would wind up being loaded _before_ the dap module.
    // That led to all sorts of runtime problems. See ticket 2258. Since
    // we don't need this, and it can cause problems, I'm removing it.
    // jhrg 10/13/14
#if 0
    // This is a kludge. But we want to be sure that the dap
    // modules get loaded first.
    vector<string> ordered_list;
    for (; l != le; l++) {
        string mods = (*l);
        if (mods != "") {
            if (mods.find("dap", 0) != string::npos) {
                ordered_list.insert(ordered_list.begin(), mods);
            }
            else {
                ordered_list.push_back(mods);
            }
        }
    }

    l = ordered_list.begin();
    le = ordered_list.end();
#endif
    for (; l != le; l++) {
        string mods = (*l);
        list<string> mod_list;
        BESUtil::explode(',', mods, mod_list);

        list<string>::iterator i = mod_list.begin();
        list<string>::iterator e = mod_list.end();
        for (; i != e; i++) {
            if (!(*i).empty()) {
                string key = "BES.module." + (*i);
                string so;
                try {
                    TheBESKeys::TheKeys()->get_value(key, so, found);
                }
                catch( BESError &e ) {
                    cerr << e.get_message() << endl;
                    return 1;
                }
                if (so == "") {
                    cerr << "Couldn't find the module for " << (*i) << endl;
                    return 1;
                }
                bes_module new_mod;
                new_mod._module_name = (*i);
                new_mod._module_library = so;
                _module_list.push_back(new_mod);
            }
        }
    }

    list<bes_module>::iterator mi = _module_list.begin();
    list<bes_module>::iterator me = _module_list.end();
    for (; mi != me; mi++) {
        bes_module curr_mod = *mi;
        _moduleFactory.add_mapping(curr_mod._module_name, curr_mod._module_library);
    }

    for (mi = _module_list.begin(); mi != me; mi++) {
        bes_module curr_mod = *mi;
        try {
            string modname = curr_mod._module_name;
            BESAbstractModule *o = _moduleFactory.get(modname);
            o->initialize(modname);
            delete o;
        }
        catch( BESError &e ) {
            cerr << "Caught plugin exception during initialization of " << curr_mod._module_name << " module:" << endl
                    << "    " << e.get_message() << endl;
            retVal = 1;
            break;
        }
        catch( ... ) {
            cerr << "Caught unknown exception during initialization of " << curr_mod._module_name << " module" << endl;
            retVal = 1;
            break;
        }
    }

    return retVal;
}

/** @brief clean up after the application
 *
 * Calls terminate on each of the loaded modules
 *
 * @return 0 if successful and not 0 otherwise
 * @param sig if the application is terminating due to a signal, otherwise 0
 * is passed.
 */
int BESModuleApp::terminate(int sig)
{
    list<bes_module>::iterator i = _module_list.begin();
    list<bes_module>::iterator e = _module_list.end();
    bool done = false;
    try {
        // go in the reverse order that the modules were loaded
        // TODO replace this with a reverse iterator. jhrg 12/21/12
        while (!done) {
            if (e == i)
                done = true;
            else {
                e--;
                bes_module curr_mod = *e;
                string modname = curr_mod._module_name;
                BESAbstractModule *o = _moduleFactory.get(modname);
                if (o) {
                    o->terminate(modname);
                    delete o;
                }
            }
        }
    }
    catch( BESError &e ) {
        cerr << "Caught exception during module termination: " << e.get_message() << endl;
    }
    catch( ... ) {
        cerr << "Caught unknown exception during terminate" << endl;
    }

    return BESApp::terminate(sig);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the name of the
 * application, whether the application is initialized or not and whether the
 * application debugging is turned on.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESModuleApp::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESModuleApp::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_module_list.size()) {
        strm << BESIndent::LMarg << "loaded modules:" << endl;
        BESIndent::Indent();
        list<bes_module>::const_iterator i = _module_list.begin();
        list<bes_module>::const_iterator e = _module_list.end();
        for (; i != e; i++) {
            bes_module curr_mod = *i;
            strm << BESIndent::LMarg << curr_mod._module_name << ": " << curr_mod._module_library << endl;
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "loaded modules: none" << endl;
    }
    BESIndent::UnIndent();
}

