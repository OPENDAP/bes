// http_transmitter.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "DODSInitList.h"
#include "DODSInitOrder.h"
#include "DODSLog.h"
#include "DODSReturnManager.h"
#include "DODSBasicHttpTransmitter.h"

static bool
HTTPTransmitterInit(int, char**) {
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << HTTP_TRANSMITTER << " transmitter" << endl;
    DODSReturnManager::TheManager()->add_transmitter( HTTP_TRANSMITTER, new DODSBasicHttpTransmitter ) ;

    return true;
}

static bool
HTTPTransmitterQuit(void) {
    return true ;
}

FUNINITQUIT( HTTPTransmitterInit, HTTPTransmitterQuit, TRANSMITTER_INIT ) ;

// $Log: http_transmitter.cc,v $
