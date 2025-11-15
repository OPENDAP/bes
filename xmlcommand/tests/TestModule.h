// DapModule.h

// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//                                                                            
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.

#ifndef I_TestModule_H
#define I_TestModule_H 1

#include <string>
#include <ostream>

#include "BESAbstractModule.h"

/**
 * @brief Add a new, non-default, catalog to the BES.
 *
 * This module is used soley for testing. It adds a second
 * catalog, using a name set in the .cc implmentation file
 * that must match the value used in the bes.conf file.
 */
class TestModule: public BESAbstractModule {
public:
	TestModule() { }
	virtual ~TestModule() { }

	virtual void initialize(const std::string &modname);
	virtual void terminate(const std::string &modname);

	void dump(std::ostream &strm) const override;
};

#endif // I_TestModule_H
