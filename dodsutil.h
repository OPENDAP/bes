/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1996, California Institute of Technology.  
// ALL RIGHTS RESERVED.   U.S. Government Sponsorship acknowledged. 
//
// Please read the full copyright notice in the file COPYRIGH 
// in this directory.
//
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
    Stat(const char *filename) { Stat(string(filename)); }
    
    Stat(const string& filename) : _filename(filename) {
	_badstat = (stat(filename.c_str(), &_sbuf) != 0);
    }
    
    // File mode [see mknod(2)]
    mode_t mode() const {return _sbuf.st_mode; }
    
    // Inode number 
    ino_t ino() const { return _sbuf.st_ino; }   
    
    // ID of device containing a directory entry for this file 
    dev_t dev() const { return _sbuf.st_dev; }   
    
    // ID of device -- this entry is defined only for char special or block 
    // special files 
    dev_t rdev() const { return _sbuf.st_rdev; }
    
    // Number of links 
    nlink_t nlink() const { return _sbuf.st_nlink; }
    
    // User ID of the file's owner
    uid_t uid() const { return _sbuf.st_uid; }
    
    // Group ID of the file's group 
    gid_t gid() const { return _sbuf.st_gid; }	
    
    // File size in bytes 
    off_t size() const { return _sbuf.st_size; } 

    // Time of last access (Times measured in seconds since 
    // 00:00:00 UTC, Jan. 1, 1970)
    time_t atime() const { return _sbuf.st_atime; }  

    // Time of last data modification
    time_t mtime() const { return _sbuf.st_mtime; } 

    // Time of last status change 
    time_t ctime() const { return _sbuf.st_ctime; }
                            
    // Preferred I/O block size 
    long blksize() const { return _sbuf.st_blksize; } 

    // Number st_blksize blocks allocated 
    long blocks() const { return _sbuf.st_blocks; }
				
    // flag indicating constructor was not successful
    bool bad() const { return _badstat; }

    // convenience mfunction: return filename
    const char *filename() const { return _filename.c_str(); }

    // convenience operator: return badstat
    bool operator!() const { return bad(); }
protected:
    string _filename;		// name of file
    struct stat _sbuf;		// buffer to hold stat() results
    bool _badstat;		// indicates whether stat() was successful
};

// return the last component of a full pathname
inline string basename(const string &path) {
   
    // If the filename has a # in it, it's probably been decompressed
    // to <cachedir>/the#path#of#the#file#<filename> and all we want is
    // <filename>.  rph 06/14/01.

    if(path.find("#") != string::npos) 
        return path.substr(path.find_last_of("#")+1);
    else
        return path.substr(path.find_last_of("/")+1);
}

// $Log: dodsutil.h,v $
// Revision 1.7  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.5.4.4  2002/04/12 00:07:04  jimg
// I removed old code that was wrapped in #if 0 ... #endif guards.
//
// Revision 1.5.4.3  2002/04/12 00:03:14  jimg
// Fixed casts that appear throughout the code. I changed most/all of the
// casts to the new-style syntax. I also removed casts that we're not needed.
//
// Revision 1.5.4.2  2002/04/11 20:48:49  jimg
// Removed prototypes for functions defined in dodsutil.cc; those we're not
// being used and that whole file was removed.
//
// Revision 1.5.4.1  2001/06/14 17:27:00  rich
// Modified basename() so it would remove the garbage from the beginning of
// the names of decompressed files
//
// Revision 1.5  2000/03/31 16:56:06  jimg
// Merged with release 3.1.4
//
// Revision 1.4.8.1  2000/03/20 22:26:07  jimg
// Removed prototypes for functions which were duplicates of stuff in the dap
// source file escaping.cc.
//
// Revision 1.4  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3.6.1  1999/05/06 00:27:24  jimg
// Jakes String --> string changes
//
// Revision 1.1  1997/03/10 22:55:02  jimg
// New files for the 2.12 compatible HDF server
//
// Revision 1.1  1996/09/24 22:38:16  todd
// Initial revision


