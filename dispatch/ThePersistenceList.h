// ThePersistenceList.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef E_ThePersistenceList_H
#define E_ThePersistenceList_H 1

#include "DODSContainerPersistenceList.h"

#define PERSISTENCE_VOLATILE "volatile"

extern DODSContainerPersistenceList *ThePersistenceList;

#endif

// $Log: ThePersistenceList.h,v $
// Revision 1.3  2005/03/15 20:01:01  pwest
// using VOLATILE macro instead of hard codes string
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
