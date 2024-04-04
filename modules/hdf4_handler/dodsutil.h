// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
// 
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

/////////////////////////////////////////////////////////////////////////////
// Copyright 1996, by the California Institute of Technology.
// ALL RIGHTS RESERVED. United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the
// Office of Technology Transfer at the California Institute of
// Technology. This software may be subject to U.S. export control
// laws and regulations. By accepting this software, the user
// agrees to comply with all applicable U.S. export laws and
// regulations. User has the responsibility to obtain export
// licenses, or other export authority as may be required before
// exporting such information to foreign countries or providing
// access to foreign persons.

// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//


#include <string>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "HDFStructure.h"
#include "HDFArray.h"

class Stat {
  public:
    explicit Stat(const char *filename) {
	_badstat = (stat(filename, &_sbuf) != 0);
	// Stat(string(filename)); Stat is destroyed immediately. jhrg
    }

    explicit Stat(const string & filename):_filename(filename) {
        _badstat = (stat(filename.c_str(), &_sbuf) != 0);
    }

    // File mode [see mknod(2)]
    mode_t mode() const {
        return _sbuf.st_mode;
    }
    // Inode number  
    ino_t ino() const {
        return _sbuf.st_ino;
    }
    // ID of device containing a directory entry for this file  
    dev_t dev() const {
        return _sbuf.st_dev;
    }
    // ID of device -- this entry is defined only for char special or block 
    // special files 
    dev_t rdev() const {
        return _sbuf.st_rdev;
    }
    // Number of links  
    nlink_t nlink() const {
        return _sbuf.st_nlink;
    }
    // User ID of the file's owner 
    uid_t uid() const {
        return _sbuf.st_uid;
    }
    // Group ID of the file's group  
    gid_t gid() const {
        return _sbuf.st_gid;
    }
    // File size in bytes  
    off_t size() const {
        return _sbuf.st_size;
    }
    // Time of last access (Times measured in seconds since 
    // 00:00:00 UTC, Jan. 1, 1970)
    time_t atime() const {
        return _sbuf.st_atime;
    }
    // Time of last data modification 
    time_t mtime() const {
        return _sbuf.st_mtime;
    }
    // Time of last status change  
    time_t ctime() const {
        return _sbuf.st_ctime;
    }
    // Preferred I/O block size  
    long blksize() const {
        return _sbuf.st_blksize;
    }
    // Number st_blksize blocks allocated  
    long blocks() const {
        return _sbuf.st_blocks;
    }
    // flag indicating constructor was not successful 
    bool bad() const {
        return _badstat;
    }
    // convenience mfunction: return filename 
    const char *filename() const {
        return _filename.c_str();
    }
    // convenience operator: return badstat 
    bool operator!() const {
        return bad();
  } 
  private:
    string _filename;          // name of file
    struct stat _sbuf;          // buffer to hold stat() results
    bool _badstat;              // indicates whether stat() was successful
};

// return the last component of a full pathname
inline string basename(const string & path)
{
    // If the filename has a # in it, it's probably been decompressed
    // to <cachedir>/the#path#of#the#file#<filename> and all we want is
    // <filename>.  rph 06/14/01.

    if (path.find("#") != string::npos)
        return path.substr(path.find_last_of("#") + 1);
    else
        return path.substr(path.find_last_of("/") + 1);
}

inline string first_part_of_full_path(const string &path) {

    string ret_value ="";
    size_t first_fs_pos = 0;
    if(path.front()=='/') 
       first_fs_pos = path.find('/',1);
    else
       first_fs_pos = path.find('/',0);
      
    if (first_fs_pos == string::npos && path !="/")
        ret_value = path;
    else 
        ret_value = path.substr(0,first_fs_pos);

    return ret_value;
}

