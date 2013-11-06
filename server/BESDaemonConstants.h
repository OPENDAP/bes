/*
 * BESDaemonConstants.h
 *
 *  Created on: Jun 13, 2011
 *      Author: jimg
 */

// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//                                                                            
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.
#ifndef BESDAEMONCONSTANTS_H_
#define BESDAEMONCONSTANTS_H_

#define BESLISTENER_STOPPED 0
#define BESLISTENER_RUNNING 4   // 1,2 are abnormal term, restart is 3
#define BESLISTENER_RESTART SERVER_EXIT_RESTART

// This is the file descriptor used for the pipe that enables the beslistener
// to send its status back to the besdaemon telling it that the beslistener
// has, in fact, started. NB: stdout is '1'

#define BESLISTENER_PIPE_FD 1

#endif /* BESDAEMONCONSTANTS_H_ */
