// TheDefineList.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "TheDefineList.h"
#include "DODSInitList.h"
#include "DODSInitOrder.h"

DODSDefineList *TheDefineList = 0;

static bool
buildDefineList(int, char**) {
    TheDefineList = new DODSDefineList ;
    return true ;
}

static bool
destroyDefineList(void) {
    if(TheDefineList)
    {
	delete (DODSDefineList *)TheDefineList;
    }
    TheDefineList = 0 ;
    return true;
}

FUNINITQUIT(buildDefineList, destroyDefineList, DEFINELIST_INIT);

// $Log: TheDefineList.cc,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
