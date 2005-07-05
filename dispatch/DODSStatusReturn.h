// DODSStatusReturn.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSStatusReturn_h_
#define DODSStatusReturn_h_ 1

#define DODS_EXECUTED_OK 0
#define DODS_TERMINATE_IMMEDIATE 1
#define DODS_REQUEST_INCORRECT 2
#define DODS_MEMORY_EXCEPTION 3
#define DODS_MYSQL_CONNECTION_FAILURE 4
#define DODS_MYSQL_BAD_QUERY 5
#define DODS_PARSER_ERROR 6
#define DODS_CONTAINER_PERSISTENCE_ERROR 7
#define DODS_INITIALIZATION_FILE_PROBLEM 8
#define DODS_LOG_FILE_PROBLEM 9
#define DODS_DATA_HANDLER_FAILURE 10
#define DODS_AUTHENTICATE_EXCEPTION 11
#define DODS_AGGREGATION_EXCEPTION 12
#define OPeNDAP_FAILED_TO_EXECUTE_COMMIT_COMMAND 13


#endif // DODSStatusReturn_h_

// $Log: DODSStatusReturn.h,v $
// Revision 1.4  2005/03/15 19:57:45  pwest
// new return status
//
// Revision 1.3  2004/12/15 17:39:03  pwest
// Added doxygen comments
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
