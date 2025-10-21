// DebugFunctions.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#ifndef DEBUGFUNCTIONS_H_
#define DEBUGFUNCTIONS_H_

#include <stdlib.h>     

#include <libdap/BaseType.h>
#include <libdap/DDS.h>
#include <libdap/ServerFunction.h>
#include "BESAbstractModule.h"

namespace debug_function {


class DebugFunctions: public BESAbstractModule {
public:
    DebugFunctions()
    {
    }
    virtual ~DebugFunctions()
    {
    }
    virtual void initialize(const string &modname);
    virtual void terminate(const string &modname);

    void dump(ostream &strm) const override;
};





/*****************************************************************************************
 * 
 * Abort Function (Debug Functions)
 * 
 * This server side function calls abort(). (boom)
 *
 */
void abort_ssf(int argc, libdap::BaseType * argv[], libdap::DDS &dds, libdap::BaseType **btpp);
class AbortFunc: public libdap::ServerFunction {
public:
    AbortFunc();
    virtual ~AbortFunc(){}
};

/*****************************************************************************************
 * 
 * Sleep Function (Debug Functions)
 * 
 * This server side function calls sleep() for the number 
 * of millisecs passed in at argv[0]. (Zzzzzzzzzzzzzzz)
 *
 */
void sleep_ssf(int argc, libdap::BaseType * argv[], libdap::DDS &dds, libdap::BaseType **btpp);
class SleepFunc: public libdap::ServerFunction {
public:
    SleepFunc();
    virtual ~SleepFunc(){}
};


/*****************************************************************************************
 * 
 * SumUntil Function (Debug Functions)
 * 
 * This server side function computes a sum until a the amount of
 * of millisecs passed in at argv[0] has transpired. (++++++)
 *
 */
void sum_until_ssf(int argc, libdap::BaseType * argv[], libdap::DDS &dds, libdap::BaseType **btpp);
class SumUntilFunc: public libdap::ServerFunction {
public:
    SumUntilFunc();
    virtual ~SumUntilFunc(){}

};


/*****************************************************************************************
 * 
 * Error Function (Debug Functions)
 * 
 * This server side function calls calls sleep() for the number 
 * of ms passed in at argv[0]. (Zzzzzzzzzzzzzzz)
 *
 */
void error_ssf(int argc, libdap::BaseType * argv[], libdap::DDS &dds, libdap::BaseType **btpp);
class ErrorFunc: public libdap::ServerFunction {
public:
    ErrorFunc();
    virtual ~ErrorFunc(){}
};

} // namespace debug
#endif /* DEBUGFUNCTIONS_H_ */
