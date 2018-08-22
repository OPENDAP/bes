// BESContainerStorageFile.h

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

#ifndef I_BESContainerStorageFile_h_
#define I_BESContainerStorageFile_h_ 1

#include <string>
#include <map>
#include <list>

using std::string;
using std::map;

#include "BESContainerStorage.h"

/** @brief implementation of BESContainerStorage that represents a
 * way to read container information from a file.
 *
 * This implementation of BESContainerStorage loads container information
 * from a file. The name of the file is determined from the bes
 * configuration file. The key is:
 *
 * BES.Container.Persistence.File.&lt;name&gt;
 *
 * where &lt;name&gt; is the name of this persistent store.
 *
 * The format of the file is:
 *
 * &lt;symbolic_name&gt; &lt;real_name&gt; &lt;data type&gt;
 *
 * where the &lt;symbolic_name&gt; is the symbolic name of the container, the
 * &lt;real_name&gt; represents the physical location of the data, such as a
 * file, and the &lt;data type&gt; is the type of data being represented,
 * such as netcdf, cedar, etc...
 *
 * One container per line, cannot span multiple lines
 *
 * @see BESContainerStorage
 * @see BESFileContainer
 * @see BESKeys
 */
class BESContainerStorageFile: public BESContainerStorage {
private:
    string _file;
    typedef struct _container {
        string _symbolic_name;
        string _real_name;
        string _container_type;
    } container;
    map<string, BESContainerStorageFile::container *> _container_list;
    typedef map<string, BESContainerStorageFile::container *>::const_iterator Container_citer;
    typedef map<string, BESContainerStorageFile::container *>::iterator Container_iter;

public:
    BESContainerStorageFile(const string &n);
    virtual ~BESContainerStorageFile();

    virtual BESContainer * look_for(const string &sym_name);
    virtual void add_container(const string &sym_name, const string &real_name, const string &type);
    virtual bool del_container(const string &s_name);
    virtual bool del_containers();

    virtual bool isData(const string &inQuestion, std::list<string> &provides);

    virtual void show_containers(BESInfo &info);

    virtual void dump(ostream &strm) const;
};

#endif // I_BESContainerStorageFile_h_
