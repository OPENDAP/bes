// DODSInitList.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef E_DODSInitList_h
#define E_DODSInitList_h 1

#include "DODSGlobalInit.h"
#include "DODSInitFuns.h"

extern DODSInitializer *DODSGlobalInitList[11];

#define FUNINITQUIT(a,b,c) static DODSGlobalInit dummy(a,b,DODSGlobalInitList[c], c);
#define FUNINIT(a,c) static DODSGlobalInit dummy(a,0,DODSGlobalInitList[c], c);

#endif // E_DODSInitList_h

// $Log: DODSInitList.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
