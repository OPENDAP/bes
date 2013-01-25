// BESCatalogUtils.h

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef S_BESCatalogUtils_h
#define S_BESCatalogUtils_h 1

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <map>
#include <vector>
#include <list>
#include <string>

using std::map;
using std::vector;
using std::list;
using std::string;

#include "BESObj.h"
#include "BESUtil.h"

class BESInfo;
class BESCatalogEntry;

class BESCatalogUtils: public BESObj {
private:
	static map<string, BESCatalogUtils *> _instances;

	string _name;
	string _root_dir;
	list<string> _exclude;
	list<string> _include;
	bool _follow_syms;

public:
	struct type_reg {
		string type;
		string reg;
	};
private:
	vector<type_reg> _match_list;

	BESCatalogUtils() {
	}

	static void bes_get_stat_info(BESCatalogEntry *entry, struct stat &buf);
public:
	BESCatalogUtils(const string &name);
	virtual ~BESCatalogUtils() {}

	const string & get_root_dir() const {
		return _root_dir;
	}
	bool follow_sym_links() const {
		return _follow_syms;
	}
	virtual bool include(const string &inQuestion) const ;
	virtual bool exclude(const string &inQuestion) const ;

	typedef vector<type_reg>::const_iterator match_citer;
	BESCatalogUtils::match_citer match_list_begin() const ;
	BESCatalogUtils::match_citer match_list_end() const ;

	virtual unsigned int get_entries(DIR *dip, const string &fullnode,
			const string &use_node, const string &coi, BESCatalogEntry *entry,
			bool dirs_only);

	static void display_entry(BESCatalogEntry *entry, BESInfo *info);

	static void bes_add_stat_info(BESCatalogEntry *entry,
			const string &fullnode);

	static bool isData(const string &inQuestion, const string &catalog,
			list<string> &services);

	virtual void dump(ostream &strm) const ;

	static BESCatalogUtils * Utils(const string &name);

	// Added because of reported memory leaks. jhrg 12/24/12
	static void delete_all_catalogs();


};

#endif // S_BESCatalogUtils_h
