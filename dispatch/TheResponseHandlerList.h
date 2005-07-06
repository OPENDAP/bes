// TheResponseHandlerList.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef E_TheResponseHandlerList_h
#define E_TheResponseHandlerList_h 1

#include "DODSResponseHandlerList.h"

/** @brief The global response handler list used by this server.
 *
 * TheResponseHandlerList is the global DODSResponseHandlerList object used to
 * store registered response handlers. The object is built during global
 * initialization. The order of initialization is RESPONSEHANDLERLIST_INIT.
 *
 * @see DODSResponseHandlerList
 * @see DODSResponseHandler
 * @see DODSResponseObject
 * @see DODSGlobalIQ
 */
extern DODSResponseHandlerList *TheResponseHandlerList;

#endif // E_TheResponseHandlerList_h

// $Log: TheResponseHandlerList.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
