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
// $RCSfile: dodsutil.h,v $ - Miscellaneous classes and routines for DODS HDF server
//
// $Log: dodsutil.h,v $
// Revision 1.3  1998/09/26 04:11:04  jimg
// Moved basename to hdfdesc.cc.
//
// Revision 1.2  1998/09/10 21:32:28  jehamby
// Properly escape high-ASCII strings with octstring() and hexstring()
//
// Revision 1.1  1997/03/10 22:55:02  jimg
// New files for the 2.12 compatible HDF server
//
// Revision 1.1  1996/09/24 22:38:16  todd
// Initial revision
//
//
/////////////////////////////////////////////////////////////////////////////


#include <String.h>
#include <ctype.h>

// change this to the following when g++ supports const_cast
// #define CONST_CAST(TYPE,EXPR) const_cast<TYPE>(EXPR)
#define CONST_CAST(TYPE,EXPR) (TYPE)(EXPR)

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "HDFStructure.h"
#include "HDFArray.h"

class Stat {
public:
    Stat(const char *filename) { Stat(String(filename)); }
    
    Stat(const String& filename) : _filename(filename) {
	_badstat = (stat(filename, &_sbuf) != 0);
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
#ifdef __GNUG__
    const char *filename() const { return _filename.chars(); }
#else
    const char *filename() const { return _filename.data(); }
#endif

    // convenience operator: return badstat
    bool operator!() const { return bad(); }
protected:
    String _filename;		// name of file
    struct stat _sbuf;		// buffer to hold stat() results
    bool _badstat;		// indicates whether stat() was successful
};

#if 0
// Moved to hdfdesc.cc. 9/25/98 jhrg
// return the last component of a full pathname
inline String basename(String path) {
#ifdef __GNUG__
    String tmp = path.after(path.index('/',-1));
    return tmp.after(path.index('#',-1));
#else
    String tmp =  path.substr(path.find_last_of("/")+1);
    return tmp.substr(path.find_last_of("#")+1);
#endif
}
#endif

// globally substitute in for out in string s
inline String& gsub(String& s, const String& in, const String& out) {
    while (s.index(in) >= 0)
	s.at(in) = out;
    return s;
}

String hexstring(unsigned char val);
char unhexstring(String s);
String id2dods(String s);
String dods2id(String s);
char unoctstring(String s);
String octstring(unsigned char val);
String escattr(String s);
String unescattr(String s);
HDFStructure *CastBaseTypeToStructure(BaseType *p);
HDFArray *CastBaseTypeToArray(BaseType *p);
int *CastBaseTypeToInt(BaseType *p);
double *CastBaseTypeToDouble(BaseType *p);

