// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
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

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <csignal>
#include <sys/wait.h>
#include <sstream>

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <string>
#include <memory>

#include <BESInternalError.h>
#include <BESInternalFatalError.h>

#include "TempFile.h"
#include "BESLog.h"
#include "BESUtil.h"
#include "BESDebug.h"

using namespace std;

#define MODULE "dap"
#define prolog string("TempFile::").append(__func__).append("() - ")

#define STRICT 0

namespace bes {

std::map<std::string, int, std::less<>> TempFile::open_files;
struct sigaction TempFile::cached_sigpipe_handler;

static string build_error_msg(const string &msg, const string &file = "", int line = -1) {
    if (file.empty())
        return {msg + ": " + strerror(errno) + " (errno: " + to_string(errno) + ")"};
    else
        return {msg + ": " + strerror(errno) + " (errno: " + to_string(errno) + "at" + file + ":"
                + to_string(line) + ")\n"};
}

/**
 * We need to make sure that all of the open temporary files get cleaned up if
 * bad things happen. So far, SIGPIPE is the only bad thing we know about
 * at least with respect to the TempFile class.
 * FIXME This code needs to restore the existing SIGPIPE handler. See server/ServerApp.cc. jhrg 8/13/24
 */
void TempFile::sigpipe_handler(int sig) {
    try {
        int saved_errno = errno;
        if (sig == SIGPIPE) {
            for (const auto &mpair: open_files) {
                if (unlink((mpair.first).c_str()) == -1) {
                    ERROR_LOG(build_error_msg("Error unlinking temporary file: " + mpair.first, __FILE__, __LINE__));
                }
            }

            // Files cleaned up? Sweet! Time to bail...
            // replace this SIGPIPE handler with the cached one.
            sigaction(SIGPIPE, &cached_sigpipe_handler, nullptr);

            // Re-raise SIGPIPE
            errno = saved_errno;
            raise(SIGPIPE);
        }
    }
    catch (const std::exception &e) {
        ERROR_LOG(build_error_msg("std::exception: " + string(e.what()), __FILE__, __LINE__));
    }
}

/**
 * @brief Attempts to create the directory identified by dir_name
 *
 * Tries to make a directory for temporary files. If the directory already
 * exists, returns false. If the directory is created, returns true. If the
 * directory is made, then the access mode is 770.
 *
 * @note Does not test that a directory that exists is has the access mode of
 * at least 770.
 *
 * @param dir_name Full path of the directory to be created
 * @return true if the directory was created, false if it already exists
 * @exception BESInternalFatalError if the directory could not be created and does not already exist
 */
bool TempFile::mk_temp_dir(const std::string &dir_name) {

    mode_t mode = S_IRWXU | S_IRWXG;
    BESDEBUG(MODULE, prolog << "mode: " << std::oct << mode << endl);

    if (mkdir(dir_name.c_str(), mode)) {
        if (errno != EEXIST) {
            throw BESInternalFatalError(build_error_msg("Failed to create temp directory: " + dir_name), __FILE__,
                                        __LINE__);
        }
        else {
            BESDEBUG(MODULE, prolog << "The temp directory: " << dir_name << " exists." << endl);
            return false;
#if STRICT
            uid_t uid = getuid();
            gid_t gid = getgid();
            BESDEBUG(MODULE,prolog << "Assuming ownership of " << dir_name << " (uid: " << uid << " gid: "<< gid << ")" << endl);
            if(chown(dir_name.c_str(),uid,gid)){
                stringstream msg;
                msg << prolog  << "ERROR - Failed to assume ownership (uid: "<< uid;
                msg << " gid: " << gid << ") of: " << dir_name;
                msg << " errno: " << errno << " reason: " << strerror(errno);
                throw BESInternalFatalError(msg.str(),__FILE__,__LINE__);
            }
            BESDEBUG(MODULE,prolog << "Changing permissions to mode: " << std::oct << mode << endl);
            if(chmod(dir_name.c_str(),mode)){
                stringstream msg;
                msg << prolog  << "ERROR - Failed to change permissions (mode: " << std::oct << mode;
                msg << ") for: " << dir_name;
                msg << " errno: " << errno << " reason: " << strerror(errno);
                throw BESInternalFatalError(msg.str(),__FILE__,__LINE__);
            }
#endif
        }
    }
    else {
        BESDEBUG(MODULE, prolog << "The temp directory: " << dir_name << " was created." << endl);
        return true;
    }
}

/**
 * @brief Create a new temporary file
 *
 * Get a new temporary file using the given directory and temporary file prefix.
 * If the directory does not exist it will be created.
 *
 * @param dir_name The name of the directory in which the temporary file
 * will be created.
 * @param temp_file_prefix A prefix to be used for the temporary file.
 * @return The name of the temporary file.
 */
string TempFile::create(const std::string &dir_name, const std::string &temp_file_prefix) {
    // TODO The lock can be moved down to the section with the shared resource. jhrg 8/13/24
    std::lock_guard<std::recursive_mutex> lock_me(d_tf_lock_mutex);

    BESDEBUG(MODULE, prolog << "dir_name: " << dir_name << endl);

    if (access(dir_name.c_str(), F_OK) == -1) {
        if (errno == ENOENT) {
            mk_temp_dir(dir_name);
        }
        else {
            throw BESInternalFatalError(build_error_msg("Failed to access temp directory: " + dir_name), __FILE__,
                                        __LINE__);
        }
    }

    // Added process id (ala getpid()) to tempfile name to see if that affects
    // our troubles with permission denied errors. This may need to be revisited if this
    // is going to be utilized in a place where multiple threads could end up in this spot.
    //   - ndp 04/03/24
    string target_file = BESUtil::pathConcat(dir_name, temp_file_prefix + "_" + to_string(getpid()) + "_XXXXXX");

    BESDEBUG(MODULE, prolog << "target_file: " << target_file << endl);

    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask(077);
    // The 'hack' &temp_file_name[0] is explicitly supported by the C++ 11 standard.
    d_fd = mkstemp(&target_file[0]);
    umask(original_mode);

    if (d_fd == -1) {
        throw BESInternalError(
                build_error_msg("Failed to open the temporary file using mkstemp(), FileTemplate: " + target_file),
                __FILE__, __LINE__);
    }

    d_fname.assign(target_file);

    // TODO Lock only this section. jhrg 8/13/24
    // Check to see if there are already active TempFile things,
    // we can tell because if open_files->size() is zero then this
    // is the first, and we need to register SIGPIPE handler.
    if (open_files.empty()) {
        struct sigaction act{};
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGPIPE);
        act.sa_flags = 0;

        // FIXME Save the existing signal handler. jhrg 8/13/24
        act.sa_handler = bes::TempFile::sigpipe_handler;

        if (sigaction(SIGPIPE, &act, &cached_sigpipe_handler)) {
            throw BESInternalFatalError(build_error_msg("Could not register a handler to catch SIGPIPE"), __FILE__,
                                        __LINE__);
        }
    }

    open_files.insert(std::pair<string, int>(d_fname, d_fd));

    return d_fname;
}

/**
 * @brief Free the temporary file
 *
 * Close the open descriptor and delete (unlink) the file name.
 */
TempFile::~TempFile() {
    try {
        if (d_fd != -1 && close(d_fd) == -1) {
            ERROR_LOG(build_error_msg("Error closing temporary file: " + d_fname, __FILE__, __LINE__));
        }
        if (!d_fname.empty()) {
            std::lock_guard<std::recursive_mutex> lock_me(d_tf_lock_mutex);
            if (!d_keep_temps) {
                if (unlink(d_fname.c_str()) == -1) {
                    ERROR_LOG(build_error_msg("Error unlinking temporary file: " + d_fname, __FILE__, __LINE__));
                }
            }

            open_files.erase(d_fname);

            if (open_files.empty()) {
                // No more files means we can unload the SIGPIPE handler
                // If more files are created at a later time then it will get reloaded.
                // FIXME Restore saved signal handler. jhrg 8/13/24
                if (sigaction(SIGPIPE, &cached_sigpipe_handler, nullptr)) {
                    ERROR_LOG(build_error_msg("Could not remove SIGPIPE handler" , __FILE__, __LINE__));
                }
            }
        }
    }
    catch (BESError const &e) {
        ERROR_LOG(build_error_msg("BESError while closing " + d_fname + ": " + e.get_verbose_message(),
                                  __FILE__, __LINE__));
    }
    catch (std::exception const &e) {
        ERROR_LOG(build_error_msg("C++ exception while closing " + d_fname + ": " + e.what(),  __FILE__, __LINE__));
    }
}

} // namespace bes

