// DmrppModule.h

// Copyright (c) 2016 OPeNDAP, Inc. Author: Nathan Potter <npotter@opendap.org>
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

#ifndef I_DmrppModule_H
#define I_DmrppModule_H 1

#include <string>

#include "BESAbstractModule.h"

namespace dmrpp {

class DmrppModule: public BESAbstractModule {
public:
	DmrppModule() { }
	virtual ~DmrppModule() { }
	virtual void initialize(const std::string &modname);
	virtual void terminate(const std::string &modname);

	void dump(std::ostream &strm) const override;
};

} //  namespace dmrpp

#endif // I_DmrppModule_H
