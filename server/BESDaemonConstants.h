/*
 * BESDaemonConstants.h
 *
 *  Created on: Jun 13, 2011
 *      Author: jimg
 */

#ifndef BESDAEMONCONSTANTS_H_
#define BESDAEMONCONSTANTS_H_

#define BESLISTENER_STOPPED 0
#define BESLISTENER_RUNNING 4   // 1,2 are abnormal term, restart is 3
#define BESLISTENER_RESTART SERVER_EXIT_RESTART

// This is the file descriptor used for the pipe that enables the beslistener
// to send its status back to the besdaemon telling it that the beslistener
// has, in fact, started.

#define BESLISTENER_PIPE_FD 4

#endif /* BESDAEMONCONSTANTS_H_ */
