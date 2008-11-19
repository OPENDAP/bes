// BESCatalogUtils.h

#ifndef S_BESCatalogUtils_h
#define S_BESCatalogUtils_h 1

#include <map>
#include <list>
#include <string>

using std::map ;
using std::list ;
using std::string ;

#include "BESObj.h"
#include "BESUtil.h"

class BESCatalogUtils : public BESObj
{
private:
    static map<string, BESCatalogUtils *> _instances ;

    string			_root_dir ;
    list<string>		_exclude ;
    list<string>		_include ;
    bool			_follow_syms ;

public:
    struct type_reg
    {
	string type ;
	string reg ;
    } ;

private:
    list< type_reg >		_match_list ;

    				BESCatalogUtils() {}
public:
    				BESCatalogUtils( const string &name ) ;
    virtual			~BESCatalogUtils() {}
    const string &		get_root_dir() const { return _root_dir ; }
    bool			follow_sym_links() const { return _follow_syms ; }
    virtual bool		include( const string &inQuestion ) const ;
    virtual bool		exclude( const string &inQuestion ) const ;

    typedef list< type_reg >::const_iterator match_citer ;
    BESCatalogUtils::match_citer match_list_begin() const ;
    BESCatalogUtils::match_citer match_list_end() const ;

    virtual void		dump( ostream &strm ) const ;

    static const BESCatalogUtils *Utils( const string &name ) ;
} ;

#endif // S_BESCatalogUtils_h

