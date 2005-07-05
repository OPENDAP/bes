// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.

#ifndef PPTClientServerSessionProtocol_h_
#define PPTClientServerSessionProtocol_h_ 1


// Generic socket message when the mapper fails to find the proper protocol string

#define PPT_PROTOCOL_UNDEFINED "PPT_PROTOCOL_UNDEFINED"

// From client to server

//
#define PPTCLIENT_TESTING_CONNECTION "PPTCLIENT_TESTING_CONNECTION"
#define PPTCLIENT_COMPLETE_DATA_TRANSMITION "PPTCLIENT_COMPLETE_DATA_TRANSMITION"
#define PPTCLIENT_EXIT_NOW "PPTCLIENT_EXIT_NOW"

// From server to client

#define PPTSERVER_CONNECTION_OK "PPTSERVER_CONNECTION_OK"
#define PPTSERVER_COMPLETE_DATA_TRANSMITION "PPTSERVER_COMPLETE_DATA_TRANSMITION"
#define PPTSERVER_EXIT_NOW "PPTSERVER_EXIT_NOW"

#endif // PPTClientServerSessionProtocol_h_
