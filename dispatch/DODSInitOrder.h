// DODSInitOrder.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef D_DODSInitOrder_h
#define D_DODSInitOrder_h 1

#define DODSKEYS_INIT 1
#define DODSLOG_INIT 2

#define PERSISTENCELIST_INIT 1
#define REQUESTHANDLERLIST_INIT 1
#define RESPONSEHANDLERLIST_INIT 1
#define DODSRETURNMANAGER_INIT 1
#define REPORTERLIST_INIT 1
#define DEFINELIST_INIT 1
#define AGGFACTORY_INIT 1

#define CGIPERSISTENCE_INIT 3
#define USERPERSISTENCE_INIT 3
#define FILEPERSISTENCE_INIT 3
#define MYSQLPERSISTENCE_INIT 3

#define THEDODSAUTHENTICATOR_INIT 3

#define DODSMODULE_INIT 3

#endif // E_DODSInitOrder_h

// $Log: DODSInitOrder.h,v $
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
