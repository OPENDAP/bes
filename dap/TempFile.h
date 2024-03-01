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

#ifndef DAP_TEMPFILE_H_
#define DAP_TEMPFILE_H_

#include <csignal>
#include <string>
#include <map>
#include <mutex>

namespace bes {

/**
 * @brief Get a new temporary file
 *
 * Get a new temporary file that will be closed and deleted when
 * the instance is deleted (i.e., goes out of scope). The intent of this
 * class is to build temporary files that will be closed/deleted regardless
 * of how the caller exits - regularly or via an exception.
 */
class TempFile {

    friend class TemporaryFileTest;

    // Lifecycle controls
    static struct sigaction cached_sigpipe_handler;
    mutable std::recursive_mutex d_tf_lock_mutex;

    // Holds the static list of all open files
    static std::map<std::string, int, std::less<>> open_files;

    // Instance variables
    int d_fd = -1;
    std::string d_fname;
    bool d_keep_temps = false;

public:
    // Odd, but even with TemporaryFileTest declared as a friend, the tests won't
    // compile unless this is declared public.
    static void sigpipe_handler(int signal);

    TempFile() = default;
    explicit TempFile(bool keep_temps) : d_keep_temps(keep_temps) {}

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    TempFile(TempFile&&) = delete;
    TempFile& operator=(TempFile&&) = delete;

    ~TempFile();

    bool mk_temp_dir(const std::string &dir_name = "/tmp/hyrax_tmp");

    std::string create(const std::string &dir_name = "/tmp/hyrax_tmp", const std::string &path_template = "opendap");

    /** @return The temporary file's file descriptor */
    int get_fd() const { return d_fd; }

    /** @return The temporary file's name */
    std::string get_name() const { return d_fname; }
};

} // namespace bes

#endif /* DAP_TEMPFILE_H_ */
