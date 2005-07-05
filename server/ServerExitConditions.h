#ifndef ServerExitConditions_h_
#define ServerExitConditions_h_ 1

// Server exit conditions

#define SERVER_EXIT_NORMAL_SHUTDOWN 0
#define SERVER_EXIT_FATAL_CAN_NOT_START 1
#define SERVER_EXIT_ABNORMAL_TERMINATION 2
#define SERVER_EXIT_RESTART 3

// This exit condition is ignored by the daemon.
#define SERVER_EXIT_CHILD_SUBPROCESS_NORMAL_TERMINATION 4
// This exit condition is ignore by the daemon.
#define SERVER_EXIT_CHILD_SUBPROCESS_ABNORMAL_TERMINATION 5

#define CHILD_SUBPROCESS_READY 6

// The server itself nevers uses this value, just the daemon initializes with this number
#define SERVER_EXIT_UNDEFINED_STATE 7

#endif // ServerExitConditions_h_
