// dods_module.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "DODSInitList.h"
#include "DODSInitOrder.h"
#include "DODSResponseNames.h"
#include "TheResponseHandlerList.h"

#include "DASResponseHandler.h"
#include "DDSResponseHandler.h"
#include "DataResponseHandler.h"
#include "DDXResponseHandler.h"

#include "HelpResponseHandler.h"
#include "ProcIdResponseHandler.h"
#include "VersionResponseHandler.h"
#include "ContainersResponseHandler.h"
#include "DefinitionsResponseHandler.h"
#include "KeysResponseHandler.h"

#include "StatusResponseHandler.h"

#include "DefineResponseHandler.h"

#include "SetResponseHandler.h"

#include "DeleteResponseHandler.h"

#include "TheDODSReturnManager.h"
#include "DODSBasicTransmitter.h"
#include "DODSBasicHttpTransmitter.h"

#include "TheDODSLog.h"

static bool
DODSModuleInit(int, char**) {
    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "Initializing DODS:" << endl;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << DAS_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( DAS_RESPONSE, DASResponseHandler::DASResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << DDS_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( DDS_RESPONSE, DDSResponseHandler::DDSResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << DDX_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( DDX_RESPONSE, DDXResponseHandler::DDXResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << DATA_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( DATA_RESPONSE, DataResponseHandler::DataResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << HELP_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( HELP_RESPONSE, HelpResponseHandler::HelpResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << PROCESS_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( PROCESS_RESPONSE, ProcIdResponseHandler::ProcIdResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << CONTAINERS_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( CONTAINERS_RESPONSE, ContainersResponseHandler::ContainersResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << DEFINITIONS_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( DEFINITIONS_RESPONSE, DefinitionsResponseHandler::DefinitionsResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << KEYS_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( KEYS_RESPONSE, KeysResponseHandler::KeysResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << VERS_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( VERS_RESPONSE, VersionResponseHandler::VersionResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << STATUS_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( STATUS_RESPONSE, StatusResponseHandler::StatusResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << DEFINE_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( DEFINE_RESPONSE, DefineResponseHandler::DefineResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << SET_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( SET_RESPONSE, SetResponseHandler::SetResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << DELETE_RESPONSE << " response handler" << endl;
    TheResponseHandlerList->add_handler( DELETE_RESPONSE, DeleteResponseHandler::DeleteResponseBuilder ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << BASIC_TRANSMITTER << " transmitter" << endl;
    TheDODSReturnManager->add_transmitter( BASIC_TRANSMITTER, new DODSBasicTransmitter ) ;

    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "    adding " << HTTP_TRANSMITTER << " transmitter" << endl;
    TheDODSReturnManager->add_transmitter( HTTP_TRANSMITTER, new DODSBasicHttpTransmitter ) ;

    return true;
}

static bool
DODSModuleTerm(void) {
    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "Removing DODS Response Handlers" << endl;
    TheResponseHandlerList->remove_handler( DAS_RESPONSE ) ;
    TheResponseHandlerList->remove_handler( DDS_RESPONSE ) ;
    TheResponseHandlerList->remove_handler( DDX_RESPONSE ) ;
    TheResponseHandlerList->remove_handler( DATA_RESPONSE ) ;
    TheResponseHandlerList->remove_handler( HELP_RESPONSE ) ;
    TheResponseHandlerList->remove_handler( VERS_RESPONSE ) ;
    return true;
}

FUNINITQUIT(DODSModuleInit, DODSModuleTerm, DODSMODULE_INIT);

// $Log: dods_module.cc,v $
// Revision 1.7  2005/04/19 17:53:34  pwest
// added keys handler for show keys command
//
// Revision 1.6  2005/03/17 19:26:22  pwest
// added delete command to delete a specific container, a specific definition, or all definitions
//
// Revision 1.5  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.4  2005/02/09 19:46:55  pwest
// added basic transmitter and http transmitter to return manager
//
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
