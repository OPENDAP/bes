// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#ifndef DAP_BESHANDLERUTIL_H_
#define DAP_BESHANDLERUTIL_H_

#include <unistd.h>

#include <vector>
#include <string>

namespace bes {

const std::string default_path_template = "TMP_DIR/opendapXXXXXX";

/**
 * @brief Get a new temporary file
 *
 * Get a new temporary file that will be closed and deleted when
 * the instance is deleted (i.e., goes out of scope). The intent of this
 * class is to build temporary files that will be closed/deleted regardless
 * of how the caller exits - regularly or via an exception.
 */
class TemporaryFile {
private:
    int d_fd;
    std::vector<char> d_name;

public:
    /**
     * @brief Build a temporary file using a default template.
     *
     * The temporary file will be in TMP_DIR (likely /tmp) and will have
     * a name like 'opendapXXXXXX' where the Xs are numbers or letters.
     */
    TemporaryFile() : d_fd(0){
        TemporaryFile(default_path_template);
    }

    TemporaryFile(const std::string &path_template);

    ~TemporaryFile();

#if 0
    /**
     * @brief Free the temporary file
     *
     * Close the open descriptor and delete (unlink) the file name.
     */
    ~TemporaryFile() {
        close(d_fd);
        unlink(d_name.data());
    }
#endif
    /** @return The temporary file's file descriptor */
    int get_fd() const { return d_fd; }

    /** @return The temporary file's name */
    std::string get_name() const { return d_name.data(); }
};

} // namespace bes

#endif /* DAP_BESHANDLERUTIL_H_ */
