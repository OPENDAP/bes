// BESMemoryGlobalArea.cc

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

#include "config.h"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <sys/resource.h>

using std::cerr;
using std::endl;

#include "BESMemoryGlobalArea.h"
#include "BESInternalFatalError.h"
#include "BESDebug.h"
#include "BESLog.h"
#include "TheBESKeys.h"

int BESMemoryGlobalArea::_counter = 0;
unsigned long BESMemoryGlobalArea::_size = 0;
void* BESMemoryGlobalArea::_buffer = 0;

BESMemoryGlobalArea::BESMemoryGlobalArea()
{
    limit.rlim_cur = 0;
    limit.rlim_max = 0;

    if (_counter++ == 0) {
        try {
            bool fnd = false;
            string key = "BES.Memory.GlobalArea.";

            string eps;
            TheBESKeys::TheKeys()->get_value(key + "EmergencyPoolSize", eps, fnd);

            string mhs;
            TheBESKeys::TheKeys()->get_value(key + "MaximumHeapSize", mhs, fnd);

            string verbose;
            TheBESKeys::TheKeys()->get_value(key + "Verbose", verbose, fnd);

            string control_heap;
            TheBESKeys::TheKeys()->get_value(key + "ControlHeap", control_heap, fnd);

            if ((eps == "") || (mhs == "") || (verbose == "") || (control_heap == "")) {
                string line = "cannot determine memory keys.";
                line += (string) "Please set values for" + " BES.Memory.GlobalArea.EmergencyPoolSize,"
                    + " BES.Memory.GlobalArea.MaxiumumHeapSize," + " BES.Memory.GlobalArea.Verbose, and"
                    + " BES.Memory.GlobalArea.ControlHeap" + " in the BES configuration file.";
                throw BESInternalFatalError(line, __FILE__, __LINE__);
            }
            else {
                if (verbose == "no") BESLog::TheLog()->suspend();

                unsigned int emergency = atol(eps.c_str());

                if (control_heap == "yes") {
                    unsigned int max = atol(mhs.c_str());

                    LOG("Initialize emergency heap size to " << (unsigned long)emergency << " and heap size to " << (unsigned long)(max + 1) << " MB" << endl);
                    if (emergency > max) {
                        string s = string("BES: ") + "unable to start since the emergency "
                            + "pool is larger than the maximum size of " + "the heap.\n";
                        LOG(s);
                        throw BESInternalFatalError(s, __FILE__, __LINE__);
                    }
                    log_limits("before setting limits: ");
                    limit.rlim_cur = megabytes(max + 1);
                    limit.rlim_max = megabytes(max + 1);
                    if (setrlimit( RLIMIT_DATA, &limit) < 0) {
                        string s = string("BES: ") + "Could not set limit for the heap " + "because " + strerror(errno)
                            + "\n";
                        if ( errno == EPERM) {
                            s = s + "Attempting to increase the soft/hard " + "limit above the current hard limit, "
                                + "must be superuser\n";
                        }
                        LOG(s);
                        throw BESInternalFatalError(s, __FILE__, __LINE__);
                    }
                    log_limits("after setting limits: ");
                    _buffer = 0;
                    _buffer = malloc(megabytes(max));
                    if (!_buffer) {
                        string s = string("BES: ") + "cannot get heap of size " + mhs + " to start running";
                        LOG(s);
                        throw BESInternalFatalError(s, __FILE__, __LINE__);
                    }
                    free(_buffer);
                }
                else {
                    if (emergency > 10) {
                        string s = "Emergency pool is larger than 10 Megabytes";
                        throw BESInternalFatalError(s, __FILE__, __LINE__);
                    }
                }

                _size = megabytes(emergency);
                _buffer = 0;
                _buffer = malloc(_size);
                if (!_buffer) {
                    string s = (string) "BES: cannot expand heap to " + eps + " to start running";
                    LOG(s << endl);
                    throw BESInternalFatalError(s, __FILE__, __LINE__);
                }
            }
        }
        catch (BESError &ex) {
            cerr << "BES: unable to start properly because " << ex.get_message() << endl;
            exit(1);
        }
        catch (...) {
            cerr << "BES: unable to start: undefined exception happened\n";
            exit(1);
        }
    }

    BESLog::TheLog()->resume();
}

BESMemoryGlobalArea::~BESMemoryGlobalArea()
{
    if (--_counter == 0) {
        if (_buffer) free(_buffer);
        _buffer = 0;
    }
}

inline void BESMemoryGlobalArea::log_limits(const string &msg)
{
    if (getrlimit( RLIMIT_DATA, &limit) < 0) {
        LOG(msg << "Could not get limits because " << strerror( errno) << endl);
        _counter--;
        throw BESInternalFatalError(strerror( errno), __FILE__, __LINE__);
    }
    if (limit.rlim_cur == RLIM_INFINITY)
        LOG(msg << "BES heap size soft limit is infinite" << endl);
    else
        LOG(msg << "BES heap size soft limit is " << (long int) limit.rlim_cur << " bytes ("
            << (long int) (limit.rlim_cur) / (MEGABYTE) << " MB - may be rounded up)" << endl);
    if (limit.rlim_max == RLIM_INFINITY)
        LOG(msg << "BES heap size hard limit is infinite" << endl);
    else
        LOG("BES heap size hard limit is " << (long int) limit.rlim_max << " bytes ("
            << (long int) (limit.rlim_max) / (MEGABYTE) << " MB - may be rounded up)" << endl);
}

void BESMemoryGlobalArea::release_memory()
{
    if (_buffer) {
        free(_buffer);
        _buffer = 0;
    }
}

bool BESMemoryGlobalArea::reclaim_memory()
{
    if (!_buffer) _buffer = malloc(_size);
    if (_buffer)
        return true;
    else
        return false;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this memory area
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESMemoryGlobalArea::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESMemoryGlobalArea::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "area set? " << _counter << endl;
    strm << BESIndent::LMarg << "emergency buffer: " << (void *) _buffer << endl;
    strm << BESIndent::LMarg << "buffer size: " << _size << endl;
    strm << BESIndent::LMarg << "rlimit current: " << limit.rlim_cur << endl;
    strm << BESIndent::LMarg << "rlimit max: " << limit.rlim_max << endl;
    BESIndent::UnIndent();
}

