// DODSResponseNames.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef D_DODSResponseNames_H
#define D_DODSResponseNames_H 1

/** @brief macros representing the default response objects handled
 *
 * These include
 * <pre>
 * set
 * define
 * send
 * get
 *     das
 *     dds
 *     ddx
 *     dods
 * show
 *     help
 *     process
 *     version
 *     containers
 *     definitions
 *     keys
 *     status
 * delete
 * </pre>
 */

#define SET_RESPONSE "set"
#define SETCONTAINER "container"

#define DEFINE_RESPONSE "define"

#define SEND_RESPONSE "send"

#define GET_RESPONSE "get"
#define DAS_RESPONSE "das"
#define DDS_RESPONSE "dds"
#define DDX_RESPONSE "ddx"
#define DATA_RESPONSE "dods"

#define SHOW_RESPONSE "show"
#define HELP_RESPONSE "help"
#define PROCESS_RESPONSE "process"
#define VERS_RESPONSE "version"
#define CONTAINERS_RESPONSE "containers"
#define DEFINITIONS_RESPONSE "definitions"
#define KEYS_RESPONSE "keys"
#define STATUS_RESPONSE "status"

#define DELETE_RESPONSE "delete"
#define DELETE_CONTAINER "container"
#define DELETE_DEFINITION "definition"
#define DELETE_DEFINITIONS "definitions"

#endif // E_DODSResponseNames_H

// $Log: DODSResponseNames.h,v $
// Revision 1.6  2005/04/19 17:59:47  pwest
// added keys response name for show keys command
//
// Revision 1.5  2005/03/17 19:26:22  pwest
// added delete command to delete a specific container, a specific definition, or all definitions
//
// Revision 1.4  2005/03/15 19:55:36  pwest
// show containers and show definitions
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
