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

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h> // for wait

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <vector>
#include <string>

#include <BESInternalError.h>

#include "TempFile.h"
#include "BESLog.h"

using namespace std;

namespace bes {

std::map<string,int> *TempFile::open_files = new std::map<string, int>;

void TempFile::delete_temp_files(int sig) {
    if (sig == SIGPIPE) {
        std::map<string,int>::iterator it;
        for (it=open_files->begin(); it!=open_files->end(); ++it){
            unlink((it->first).c_str());
        }
        signal(sig, SIG_DFL);
        raise(sig);
    }
}

void TempFile::delete_temp_file(string fname, int fd)
{
    try {
        cerr << __func__ << "() -  Closing '" << fname << "'  fd: " << fd << endl;
        if (!close(fd)){
            ERROR(string("Error closing temporary file: '").append(fname).append("'  msg: ").append(strerror(errno)).append("\n") );
        }
        cerr << __func__ << "() -  Unlinking '" << fname << "'  fd: " << fd << endl;
        if (!unlink(fname.c_str())){
           ERROR(string("Error unlinking temporary file: '").append(fname).append("' msg: ").append(strerror(errno)).append("\n"));
        }
    }
    catch (...) {
        // Do nothing. This just protects against BESLog (i.e., ERROR)
        // throwing an exception
    }
}


/**
 * @brief Free the temporary file
 *
 * Close the open descriptor and delete (unlink) the file name.
 */
TempFile::~TempFile()
{
    cerr << __func__ << "() - BEGIN The end is nigh!" << endl;
    delete_temp_file(d_fname,d_fd);
    cerr << __func__ << "() -  Dropping file '" << d_fname << "'  from open_files list" << endl;
    open_files->erase(d_fname);
    cerr << __func__ << "() - END" << endl;
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
 */
TempFile::TempFile(const std::string &path_template)
{
    char tmp_name[path_template.length()+1];
    std::string::size_type len = path_template.copy(tmp_name, path_template.length());
    tmp_name[len] = '\0';

    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask(077);
    d_fd = mkstemp(tmp_name);
    umask(original_mode);

    if (d_fd == -1) throw BESInternalError("Failed to open the temporary file.", __FILE__, __LINE__);

    d_fname.assign(tmp_name);
    cerr << __func__ << "() - Created '" << d_fname << "' fd: "<< d_fd << endl;
    open_files->insert(std::pair<string,int>(d_fname, d_fd));

}

} // namespace bes


