// http_transmitter.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "DODSInitList.h"
#include "DODSInitOrder.h"
#include "TheDODSLog.h"
#include "TheDODSReturnManager.h"
#include "DODSBasicHttpTransmitter.h"

static bool
HTTPTransmitterInit(int, char**) {
    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << HTTP_TRANSMITTER << " transmitter" << endl;
    TheDODSReturnManager->add_transmitter( HTTP_TRANSMITTER, new DODSBasicHttpTransmitter ) ;

    return true;
}

static bool
HTTPTransmitterQuit(void) {
    return true ;
}

FUNINITQUIT( HTTPTransmitterInit, HTTPTransmitterQuit, TRANSMITTER_INIT ) ;

// $Log: http_transmitter.cc,v $
