// OPeNDAPAggFactory.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_OPeNDAPAggFactory_h
#define I_OPeNDAPAggFactory_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

class DODSAggregationServer ;

typedef DODSAggregationServer * (*p_agg_handler)( string name ) ;

/** @brief List of all registered aggregation handlers for this server
 *
 * A OPeNDAPAggFactory allows the developer to add or remove aggregation
 * handlers from the list of handlers available for this server.
 *
 * @see 
 */
class OPeNDAPAggFactory {
private:
    map< string, p_agg_handler > _handler_list ;
public:
				OPeNDAPAggFactory(void) {}
    virtual			~OPeNDAPAggFactory(void) {}

    typedef map< string, p_agg_handler >::const_iterator Handler_citer ;
    typedef map< string, p_agg_handler >::iterator Handler_iter ;

    virtual bool		add_handler( string handler_name,
					   p_agg_handler handler_method ) ;
    virtual bool		remove_handler( string handler_name ) ;
    virtual DODSAggregationServer *find_handler( string handler_name ) ;

    virtual string		get_handler_names() ;
};

#endif // I_OPeNDAPAggFactory_h

// $Log: OPeNDAPAggFactory.h,v $
