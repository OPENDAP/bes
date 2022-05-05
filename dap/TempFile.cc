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

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h> // for wait
#include <sstream>      // std::stringstream

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <vector>
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

namespace bes {

std::once_flag TempFile::d_init_once;
std::unique_ptr< std::map<std::string, int> > TempFile::open_files;
struct sigaction TempFile::cached_sigpipe_handler;


/**
 * We need to make sure that all of the open temporary files get cleaned up if
 * bad things happen. So far, SIGPIPE is the only bad thing we know about
 * at least with respect to the TempFile class.
 */
void TempFile::sigpipe_handler(int sig)
{
    if (sig == SIGPIPE) {
        for(const auto &mpair: *open_files){
            if (unlink((mpair.first).c_str()) == -1)
                ERROR_LOG(string("Error unlinking temporary file: '").append(mpair.first).append("': ").append(strerror(errno)).append("\n"));
        }
        // Files cleaned up? Sweet! Time to bail...
        sigaction(SIGPIPE, &cached_sigpipe_handler, 0);
        // signal(SIGPIPE, SIG_DFL);
        raise(SIGPIPE);
    }
}


/**
 * @brief Attempts to create the directory identified by dir_name, throws an exception if it fails.
 * @param dir_name
 */
void TempFile::mk_temp_dir(const std::string &dir_name) {

    mode_t mode = S_IRWXU | S_IRWXG;
    stringstream ss;
    ss << prolog << "mode: " <<  std::oct << mode << endl;
    BESDEBUG(MODULE, ss.str());

    if(mkdir(dir_name.c_str(), mode)){
        if(errno != EEXIST){
            stringstream msg;
            msg << prolog  << "ERROR - Failed to create temp directory: " << dir_name;
            msg << " errno: " << errno << " reason: " << strerror(errno);
            throw BESInternalFatalError(msg.str(),__FILE__,__LINE__);
        }
        else {
            BESDEBUG(MODULE,prolog << "The temp directory: " << dir_name << " exists." << endl);
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
        }
    }
    else {
        BESDEBUG(MODULE,prolog << "The temp directory: " << dir_name << " was created." << endl);
    }
}

/**
 * @brief Initialize static class members, should only be called once using std::call_once()
 */
void TempFile::init() {
    open_files = unique_ptr<std::map<string, int>>(new std::map<string, int>());
}

/**
 * @brief Get a new temporary file
 *
 * Get a new temporary file using the given template. The template must give
 * the fully qualified path for the temporary file and must end in one or more
 * Xs (but six are usually used) with no characters following.
 *
 * @note If you pass in a bad template, behavior of this class is undefined.
 *
 * @param dir_name The nae of the directory in which the temporary file
 * will be created.
 * @param path_template Template passed to mkstemp() to build the temporary
 * filename.
 * @param keep_temps Keep the temporary files.
 */
TempFile::TempFile(const std::string &dir_name, const std::string &file_template, bool keep_temps)
    : d_keep_temps(keep_temps)
{
    std::call_once(d_init_once,TempFile::init);

    BESDEBUG(MODULE, prolog << "dir_name: " << dir_name << endl);
    mk_temp_dir(dir_name);

    BESDEBUG(MODULE, prolog << "file_template: " << file_template << endl);

    string target_file = BESUtil::pathConcat(dir_name,file_template);
    BESDEBUG(MODULE, prolog << "target_file: " << target_file << endl);

    char tmp_name[target_file.length() + 1];
    std::string::size_type len = target_file.copy(tmp_name, target_file.length());
    tmp_name[len] = '\0';

    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask(077);
    d_fd = mkstemp(tmp_name);
    umask(original_mode);

    if (d_fd == -1) {
        stringstream msg;
        msg << "Failed to open the temporary file using mkstemp(). errno: " << errno;
        msg << " message: " << strerror(errno) <<  " FileTemplate: " + target_file;
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    d_fname.assign(tmp_name);


    std::lock_guard<std::recursive_mutex> lock_me(d_tf_lock_mutex);

    // Check to see if there are already active TempFile things,
    // we can tell because if open_files->size() is zero then this
    // is the first and we need to register SIGPIPE handler
    if (open_files->empty()) {
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGPIPE);
        act.sa_flags = 0;

        act.sa_handler = bes::TempFile::sigpipe_handler;

        if (sigaction(SIGPIPE, &act, &cached_sigpipe_handler)) {
            throw BESInternalFatalError("Could not register a handler to catch SIGPIPE.", __FILE__, __LINE__);
        }
    }
    open_files->insert(std::pair<string, int>(d_fname, d_fd));


}

/**
 * @brief Free the temporary file
 *
 * Close the open descriptor and delete (unlink) the file name.
 */
TempFile::~TempFile()
{
    try {
        if (close(d_fd) == -1) {
            ERROR_LOG(string("Error closing temporary file: '").append(d_fname).append("': ").append(strerror(errno)).append("\n"));
        }
        if (!d_keep_temps) {
            if (unlink(d_fname.c_str()) == -1) {
                ERROR_LOG(string("Error unlinking temporary file: '").append(d_fname).append("': ").append(strerror(errno)).append("\n"));
            }
        }
    }
    catch (BESError &e) {
        // This  protects against BESLog (i.e., ERROR) throwing an exception.
        // If BESLog has failed, we cannot log the error, punt and write to stderr.
        cerr << "Could not close temporary file '" << d_fname << "' due to an error in BESlog (" << e.get_verbose_message() << ").";
    }
    catch (...) {
        cerr << "Could not close temporary file '" << d_fname << "' due to an error in BESlog.";
    }


    std::lock_guard<std::recursive_mutex> lock_me(d_tf_lock_mutex);
    open_files->erase(d_fname);
    if (open_files->empty()) {
        if (sigaction(SIGPIPE, &cached_sigpipe_handler, nullptr)) {
            stringstream msg;
            msg << "Could not de-register the SIGPIPE handler function cached_sigpipe_handler(). ";
            msg << " errno: " << errno << " message: " << strerror(errno);
            ERROR_LOG(msg.str());
        }
    }
}

} // namespace bes

