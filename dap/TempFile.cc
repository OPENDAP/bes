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

std::map<string, int> *TempFile::open_files = new std::map<string, int>;
struct sigaction TempFile::cached_sigpipe_handler;

/**
 * We need to make sure that all of the open temporary files get cleaned up if
 * bad things happen. So far, SIGPIPE is the only bad thing we know about
 * at least with respect to the TempFile class.
 */
void TempFile::sigpipe_handler(int sig)
{
    if (sig == SIGPIPE) {
        std::map<string, int>::iterator it;
        for (it = open_files->begin(); it != open_files->end(); ++it) {
            if (unlink((it->first).c_str()) == -1)
                ERROR_LOG(string("Error unlinking temporary file: '").append(it->first).append("': ").append(strerror(errno)).append("\n"));
        }
        // Files cleaned up? Sweet! Time to bail...
        sigaction(SIGPIPE, &cached_sigpipe_handler, 0);
        // signal(SIGPIPE, SIG_DFL);
        raise(SIGPIPE);
    }
}

string mkdir_error_msg(int error_value){
    string s;
    switch(error_value){
        case EACCES:
            s = "The parent directory does not allow write permission to "
                "the process, or one of the directories in pathname did not "
                "allow search permission.";
            break;

        case EDQUOT:
            s = "The user's quota of disk blocks or inodes on the "
                "filesystem has been exhausted.";
            break;

        case EEXIST:
            s = "The pathname already exists (not necessarily as a directory). "
                "This includes the case where pathname is a symbolic link, "
                "dangling or not.";
            break;

        case EFAULT:
            s = "The pathname points outside your accessible address space.";
            break;

        case EINVAL:
            s = "The final component (\"basename\") of the new directory's "
                "pathname is invalid (e.g., it contains characters not "
                "permitted by the underlying filesystem).";
            break;

        case ELOOP:
            s = "Too many symbolic links were encountered in resolving pathname.";
            break;

        case EMLINK:
            s = "The number of links to the parent directory would exceed LINK_MAX.";
            break;

        case ENAMETOOLONG:
            s = "The pathname was too long.";
            break;

        case ENOENT:
            s = "A directory component in pathname does not exist or is a dangling symbolic link.";
            break;

        case ENOMEM:
            s = "Insufficient kernel memory was available.";
            break;

        case ENOSPC:
            s = "The device containing pathname has no room for the new directory. Or, "
                "The new directory cannot be created because the user's disk quota is exhausted.";
            break;

        case ENOTDIR:
            s = "A component used as a directory in pathname is not, in fact, a directory.";
            break;

        case EPERM:
            s = "The filesystem containing pathname does not support the creation of directories.";
            break;

        case EROFS:
            s = "The pathname refers to a file on a read-only filesystem.";
            break;

        default:
            s = "Unknown value of errno found after failed mkdir() call.";
            break;
    }
    return s;
}

void TempFile::mk_temp_dir(const std::string &dir_name){
    char tmp_name[dir_name.length() + 1];
    std::string::size_type len = dir_name.copy(tmp_name, dir_name.length());
    tmp_name[len] = '\0';

    mode_t mode = umask(007);
    if(mkdir(tmp_name, mode)){
        if(errno != EEXIST){
            stringstream msg;
            msg << prolog  << "ERROR - Failed to create temp directory: " << dir_name;
            msg << " errno: " << errno << " reason: " << mkdir_error_msg(errno);
            throw BESInternalFatalError(msg.str(),__FILE__,__LINE__);
        }
        else {
            BESDEBUG(MODULE,prolog << "The temp directory: " << dir_name << " exists.");
        }
    }
    else {
        BESDEBUG(MODULE,prolog << "The temp directory: " << dir_name << " was created.");
    }
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
 * @param path_template Template passed to mkstemp() to build the temporary
 * file pathname.
 * @param keep_temps Keep the temporary files.
 */
TempFile::TempFile(const std::string &dir_name, const std::string &file_template, bool keep_temps)
    : d_keep_temps(keep_temps)
{
    mk_temp_dir(dir_name);
    string target_file = BESUtil::pathConcat(dir_name,file_template);

    char tmp_name[target_file.length() + 1];
    std::string::size_type len = target_file.copy(tmp_name, target_file.length());
    tmp_name[len] = '\0';

    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask(077);
    d_fd = mkstemp(tmp_name);
    umask(original_mode);

    if (d_fd == -1) throw BESInternalError("Failed to open the temporary file.", __FILE__, __LINE__);

    d_fname.assign(tmp_name);

    // only register the SIGPIPE handler once. First time, size() is zero.
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

    open_files->erase(d_fname);

    if (open_files->empty()) {
        if (sigaction(SIGPIPE, &cached_sigpipe_handler, nullptr)) {
            ERROR_LOG(string("Could not de-register the SIGPIPE handler function cached_sigpipe_handler(). ").append("(").append(strerror(errno)).append(")"));
        }
    }
}

} // namespace bes

