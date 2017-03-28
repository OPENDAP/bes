//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////

#ifndef __NCML_MODULE__NCML_DEBUG__
#define __NCML_MODULE__NCML_DEBUG__

#include "config.h"

#include <sstream>
#include <string>
#include "BESDebug.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"
#include "BESNotFoundError.h"

/*
 * Some basic macros to reduce code clutter, cut & pasting, and to greatly improve readability.
 * I would have made them functions somewhere, but the __FILE__ and __LINE__ are useful.
 * We can also specialize these based on debug vs release builds etc. or new error types later as well.
 */

// I modified these macros so that the assert-like things compile to null statements
// when NDEBUG is defined. The bes configure script's --enable-developer will suppress
// that, otherwise it is defined. 10/16/15 jhrg

// Where my BESDEBUG output goes
#define NCML_MODULE_DBG_CHANNEL "ncml"

// For more verbose stuff, level 2
#define NCML_MODULE_DBG_CHANNEL_2 "ncml:2"

// Shorthand macro for printing debug info that includes the containing fully qualified function name,
// even though it's a nightmare of verbosity in some cases like the usage of STL containers.
// Switch this out if it gets too ugly...
#define NCML_MODULE_FUNCTION_NAME_MACRO __PRETTY_FUNCTION__
// #define NCML_MODULE_FUNCTION_NAME_MACRO __func__

// These are called only when performance no longer matters... 10/16/15 jhrg

// Spew the std::string msg to debug channel then throw BESInternalError.  for those errors that are internal problems, not user/parse errors.
#define THROW_NCML_INTERNAL_ERROR(msg) { \
  std::ostringstream __NCML_PARSE_ERROR_OSS__; \
  __NCML_PARSE_ERROR_OSS__ << std::string("NCMLModule InternalError: ") << "[" << __PRETTY_FUNCTION__ << "]: " << (msg); \
  BESDEBUG(NCML_MODULE_DBG_CHANNEL,  __NCML_PARSE_ERROR_OSS__.str() << endl); \
                                   throw BESInternalError( __NCML_PARSE_ERROR_OSS__.str(), \
                                                          __FILE__, __LINE__); }

// Spew the std::string msg to debug channel then throw a BESSyntaxUserError.  For parse and syntax errors in the NCML.
#define THROW_NCML_PARSE_ERROR(parseLine, msg) { \
      std::ostringstream __NCML_PARSE_ERROR_OSS__; \
      __NCML_PARSE_ERROR_OSS__ << "NCMLModule ParseError: at *.ncml line=" << (parseLine) << ": " \
     << (msg); \
      BESDEBUG(NCML_MODULE_DBG_CHANNEL, \
              __NCML_PARSE_ERROR_OSS__.str() << endl); \
          throw BESSyntaxUserError( __NCML_PARSE_ERROR_OSS__.str(), \
              __FILE__, \
              __LINE__); }


#ifdef NDEBUG
#define BESDEBUG_FUNC(channel, info)
#else
#define BESDEBUG_FUNC(channel, info) BESDEBUG( (channel), "[" << std::string(NCML_MODULE_FUNCTION_NAME_MACRO) << "]: " << info )
#endif

#ifdef NDEBUG
#define NCML_ASSERT(cond)
#else
// My own assert to throw an internal error instead of assert() which calls abort(), which is not so nice to do on a server.
#define NCML_ASSERT(cond)   { if (!(cond)) { THROW_NCML_INTERNAL_ERROR(std::string("ASSERTION FAILED: ") + std::string(#cond)); } }
#endif

#ifdef NDEBUG
#define NCML_ASSERT_MSG(cond, msg)
#else
// An assert that can carry a std::string msg
#define NCML_ASSERT_MSG(cond, msg)  { if (!(cond)) { \
  BESDEBUG(NCML_MODULE_DBG_CHANNEL, __PRETTY_FUNCTION__ << ": " << (msg) << endl); \
  THROW_NCML_INTERNAL_ERROR(std::string("ASSERTION FAILED: condition=( ") + std::string(#cond) + std::string(" ) ") + std::string(msg)); } }
#endif

#ifdef NDEBUG
#define VALID_PTR(ptr)
#else
// Quick macro to check pointers before dereferencing them.
#define VALID_PTR(ptr) NCML_ASSERT_MSG((ptr), std::string("Null pointer:" + std::string(#ptr)));
#endif

#endif // __NCML_MODULE__NCML_DEBUG__
