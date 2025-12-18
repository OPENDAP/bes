// BESDebug.h

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

/** @brief top level BES object to house generic methods
 */

#ifndef I_BESDebug_h
#define I_BESDebug_h 1

#include <iostream>
#include <map>
#include <mutex>
#include <string>

// Helper function for writing debug log lines
std::string get_debug_log_line_prefix();

static std::mutex bes_debug_log_mutex;

/// When NDEBUG is defined, BESDEBUG and BESISDEBUG are no-ops.
#ifdef NDEBUG
#define BESDEBUG(x, y)
#else
/** @brief macro used to send debug information to the debug stream
 *
 * The BESDEBUG macro is used by developers to display debug information
 * if the specified debug context is set to true.
 *
 * example:
 *
 * BESDEBUG( "bes", "function entered with values " << val1 << " and "
 *                  << val2 << endl ) ;
 *
 * @param x the debug context to check
 * @param y information to send to the output stream
 */
#define BESDEBUG(x, y)                                                                                                 \
    do {                                                                                                               \
        if (BESDebug::IsSet(x))                                                                                        \
            *(BESDebug::GetStrm()) << get_debug_log_line_prefix() << "[" << x << "] " << y;                            \
    } while (0)
#endif // NDEBUG

#ifdef NDEBUG
#define BESISDEBUG(x) (false)
#else
/** @brief macro used to determine if the specified debug context is set
 *
 * If there is a lot of debugging information, use this macro to determine if
 * the debug context is set.
 *
 * example:
 *
 * if( BESISDEBUG( "bes" ) )
 * {
 *     for( int i = 0; i < _list_size; i++ )
 *     {
 *         BESDEBUG( "bes", " _list[" << i << "] = " << _list[i] << endl ) ;
 *     }
 * }
 *
 * @note This is used within the BES framework only for the BESStopWatch
 * setup calls and then only sparingly.
 *
 * @param x bes debug to check
 */
#define BESISDEBUG(x) BESDebug::IsSet(x)
#endif

class BESDebug {
private:
    // The time to make 10'000'000 calls to IsSet with a std::map was 3 763 862 us and
    // to make the same calls to IsSet with a std::unordered_map was 2 675 492 us
    // jhrg 4/12/23
    // typedef std::unordered_map<std::string, bool> DebugMap;
    using DebugMap = std::map<std::string, bool>;

    static DebugMap _debug_map;
    static std::ostream *_debug_strm;
    static bool _debug_strm_created;

public:
    static const DebugMap &debug_map() { return _debug_map; }

    // Moved to the .cc file to avoid <algorithm> in a header file. jhrg 4/14/23
    static void Set(const std::string &flagName, bool value);

    /** @brief register the specified debug flag
     *
     * Allows developers to register a debug flag for when the Help method
     * is called. It's OK to register a context more than once (subsequent
     * calls to Register() have no effect. If the pseudo-context 'all' has
     * been registered, the context is set to true (messages will be printed),
     * otherwise it is set to false.
     *
     * @param flagName debug context to register
     */
    static void Register(const std::string &flagName) {
        auto i = _debug_map.find(flagName);
        if (i == _debug_map.end()) {
            auto a = _debug_map.find("all");
            if (a == _debug_map.end()) {
                _debug_map[flagName] = false;
            } else {
                _debug_map[flagName] = true;
            }
        }
    }

    /** @brief see if the debug context flagName is set to true
     *
     * @param flagName debug context to check if set
     * @return whether the specified flagName is set or not
     */
    static bool IsSet(const std::string &flagName) {
        auto i = _debug_map.find(flagName);
        if (i != _debug_map.end())
            return (*i).second;
        else
            i = _debug_map.find("all");

        if (i != _debug_map.end())
            return (*i).second;
        else
            return false;
    }

    /** @brief return the debug stream
     *
     * Can be a file output stream or cerr
     *
     * @return the current debug stream
     */
    static std::ostream *GetStrm() { return _debug_strm; }

    /** @brief set the debug output stream to the specified stream
     *
     * Static method that sets the debug output stream to the specified std::ostream.
     *
     * If the std::ostream was created (not set to cerr), then the created flag
     * should be set to true.
     *
     * If the current debug stream is set and the _debug_strm_created flag is
     * set to true then delete the current debug stream.
     *
     * set the static _debug_strm_created flag to the passed created flag
     *
     * @param strm set the current debug stream to strm
     * @param created whether the passed debug stream was created
     */
    static void SetStrm(std::ostream *strm, bool created) {
        if (_debug_strm_created && _debug_strm) {
            _debug_strm->flush();
            delete _debug_strm;
            _debug_strm = nullptr;
        } else if (_debug_strm) {
            _debug_strm->flush();
        }
        if (!strm) {
            _debug_strm = &std::cerr;
            _debug_strm_created = false;
        } else {
            _debug_strm = strm;
            _debug_strm_created = created;
        }
    }

    static void SetUp(const std::string &values);
    static void Help(std::ostream &strm);
    static bool IsContextName(const std::string &name);
    static std::string GetOptionsString();
};

#endif // I_BESDebug_h
