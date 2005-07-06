// TheRequestHandlerList.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef E_TheRequestHandlerList_h
#define E_TheRequestHandlerList_h 1

#include "DODSRequestHandlerList.h"

/** @brief The global request handler list used by this server.
 *
 * TheRequestHandlerList is the global DODSRequestHandlerList object used to
 * store registered request handlers. The object is built during global
 * initialization. The order of initialization is REQUESTHANDLERLIST_INIT.
 *
 * @see DODSRequestHandlerList
 * @see DODSRequestHandler
 * @see DODSGlobalIQ
 */
extern DODSRequestHandlerList *TheRequestHandlerList;

#endif // E_TheRequestHandlerList_h

// $Log: TheRequestHandlerList.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
