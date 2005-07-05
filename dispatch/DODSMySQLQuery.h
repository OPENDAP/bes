// DODSMySQLQuery.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSMySQLQuery_h_
#define DODSMySQLQuery_h_ 1

#include "DODSMySQLResult.h"
#include "DODSMySQLQueryException.h"

class DODSMySQLConnect ;

class DODSMySQLQuery
{
    DODSMySQLConnect *_my_connection;
    DODSMySQLResult *_results;
    DODSMySQLQuery(const DODSMySQLQuery &){}   //disable copy constructor
    void empty_query()
    {
	DODSMySQLQueryException qe;
	qe.set_error_description("No results loaded for this query");
	throw qe;
    }
public:
    DODSMySQLQuery(const string &server, const string &user, const string &password, const string &dataset);
    ~DODSMySQLQuery();

    void run_query(const string &query);
    string get_field()
    {
	if(!_results)
	    empty_query(); 
	return _results->get_field();
    }
    bool first_field()
    {
	if(!_results)
	    empty_query(); 
	return _results->first_field();
    }
    bool next_field()
    {
	if(!_results)
	    empty_query(); 
	return _results->next_field();
    }
    bool first_row()
    {
	if(!_results)
	    empty_query(); 
	return _results->first_row();
    }
    bool next_row()
    {
	if(!_results)
	    empty_query(); 
	return _results->next_row();
    }
    int get_nrows()
    {
	if(!_results)
	    empty_query();
	return _results->get_nrows();
    }
    int get_nfields()
    {
	if(!_results)
	    empty_query();
	return _results->get_nfields();
    }
    bool is_empty_set()
    {
	if(!_results)
	    empty_query();
	if( (_results->get_nfields()>0) && (_results->get_nrows()>0) )
	    return false;
	return true;
    }
};

#endif //   DODSMySQLQuery_h_

// $Log: DODSMySQLQuery.h,v $
// Revision 1.3  2005/03/15 20:05:03  pwest
// eliminated warnings of no return for non-void function
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
