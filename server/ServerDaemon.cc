#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdio.h>


#include <fstream>
#include <iostream>

using std::ifstream ;
using std::ofstream ;
using std::cout ;
using std::endl ;
using std::cerr ;
using std::flush;

#include "ServerExitConditions.h"

#define XDAP_LISTENER_LOCATION "XDAP_LISTENER_LOCATION"
#define XDAP_LISTENER "xdods"
#define XDAP_LISTENER_PID "xdods_listener.pid"

int  daemon_init();
int  mount_server(char* *);
int  pr_exit(int status);
void store_listener_id(int pid);
bool load_names();

const char *NameProgram;
extern "C" { extern int errno; }

// This two variables are set by load_names
char server_name[2048];
char file_for_listener[2048];


char* *arguments=0; 

int main(int argc, char *argv[])
{
  NameProgram=argv[0];

  // Set the name of the listener and the file for the listenet pid
  if (!load_names())
    return 1;
  
  if(!access(file_for_listener, F_OK))
    {
      ifstream temp(file_for_listener);
      cout<<NameProgram<<": there seems to be a DODS daemon running at ";
      char buf[500];
      temp.getline(buf,500);
      cout<<buf<<endl;
      temp.close();
      return 1;
    }

      

  arguments= new char* [argc+1];
 
  // Set arguments[0] to the name of the listener
  arguments[0]=server_name;


  // Marshal the arguments to the listener from the command line arguments to the daemon
  for (int i=1; i<argc; i++)
    arguments[i]=argv[i];
  arguments[argc]=NULL;

  daemon_init();

  int restart=mount_server(arguments);
  if (restart==2)
      {
	  cout<<NameProgram
	      <<": server can not mount at first try (core dump). Please correct problems on the process manager "
	      <<server_name
	      <<endl;
	  return 0;
      }
  while (restart)
    {
      sleep(5);
      cout<<NameProgram<<": attempting to restart the server..."<<endl;
      restart=mount_server(arguments);
    }
  cout<<NameProgram<<": daemon normal shutdown"<<endl;
  delete [] arguments; arguments=0;
  
  if(!access(file_for_listener, F_OK))
    remove (file_for_listener);

  return 0;
}

int daemon_init()
{
  pid_t pid;
  if ((pid=fork())<0)
    return (-1);
  else if (pid!=0)
    exit(0);
  setsid();
  return 0;
}

int mount_server(char* *arguments)
{
  const char* perror_string=0;
  pid_t pid;
  int status;
  if ((pid=fork())<0)
    {
      cerr<<NameProgram<<": fork error ";
      perror_string=strerror(errno);
      if (perror_string)
	cerr<<perror_string;
      cerr<<endl;
      return 1;
    }
  else if (pid==0) /* child process */
    {
      cout<<NameProgram<<": attempting to mount listener "<<server_name<<"..."<<endl;
      execvp (arguments[0],arguments);
      cerr<<NameProgram<<": mounting listener, subprocess failed because; ";
      perror_string=strerror(errno);
      if (perror_string)
	  cerr<<perror_string;
      cerr<<endl;
      exit(1);
    }
  store_listener_id(pid);
  if ( (pid=waitpid(pid,&status,0))<0 ) /* parent process */
    { 
      cerr<<NameProgram<<": waitpid error ";
      perror_string=strerror(errno);
      if (perror_string)
	cerr<<perror_string;
      cerr<<endl;
      return 1;
    }
  int child_status=pr_exit(status);
  return child_status;
}

int pr_exit(int status)
{
  if (WIFEXITED(status))
    {
      int status_to_be_returned=SERVER_EXIT_UNDEFINED_STATE;
      switch ( WEXITSTATUS(status) )
	{
	  
	case SERVER_EXIT_NORMAL_SHUTDOWN:
	  status_to_be_returned=0;
	  break;
	case SERVER_EXIT_FATAL_CAN_NOT_START:
	  cerr<<NameProgram<<": server can not start, exited with status "<<WEXITSTATUS(status)<<"\n";
	  cerr<<"Please check all error messages and proceed to adjust server installation so server can start.\n";
	  status_to_be_returned=0;
	  break;
	case SERVER_EXIT_ABNORMAL_TERMINATION:
	  cerr<<NameProgram<<": abnormal server termination, exited with status "<<WEXITSTATUS(status)<<"\n";
	  status_to_be_returned=1;
	  break;
	case SERVER_EXIT_RESTART:
	  cout<<NameProgram<<": server has been requested to re-start."<<endl;
	  status_to_be_returned=1;
	  break;
	default:
	  status_to_be_returned=1;
	  break;
	}
	
	return status_to_be_returned;
    }
  else if (WIFSIGNALED(status))
    {
      cerr<<NameProgram<<": abnormal server termination, signaled with signal number "<<WTERMSIG(status)<<"\n";
#ifdef WCOREDUMP
      if (WCOREDUMP(status)) 
	  {
	      cerr<<NameProgram<<": server dumped core.\n";
	      return 2;
	  }
#endif
      return 1;
    }
  else if (WIFSTOPPED(status))
    {
      cerr<<NameProgram<<": abnormal server termination, stopped with signal number "<<WSTOPSIG(status)<<"\n";
      return 1;
    }
  return 0;
}

void store_listener_id(int pid)
{
  const char* perror_string=0;
  ofstream f (file_for_listener);
  if(!f)
    {
      cerr<<NameProgram<<": file open error ";
      perror_string=strerror(errno);
      if (perror_string)
	cerr<<perror_string;
      cerr<<endl;
    }
  f<<"PID: "<<pid<<" UID: "<<getuid()<<endl;
  f.close();
  cout<<NameProgram<<": listener id is at "<<file_for_listener<<"..."<<endl;
}

bool load_names()
{
  char *xdap_root=0;
  xdap_root=getenv(XDAP_LISTENER_LOCATION);
  if(xdap_root)
    {
      cout<<NameProgram<<": using environment variable "<<xdap_root<<endl;
      strcpy(server_name,xdap_root);
      strcat(server_name,"/");
      strcat(server_name,XDAP_LISTENER);
      strcpy(file_for_listener,xdap_root);
      strcat(file_for_listener,"/");
      strcat(file_for_listener,XDAP_LISTENER_PID);
    }
  else
    {
      // XDAP_LISTENER_LOCATION is not set, attemp to obtain current
      // working directory
      char t_buf[1024];
      if(getcwd(t_buf, sizeof(t_buf)))
	{
	  cout<<NameProgram<<": using current working directory "<<t_buf<<endl;
	  strcpy(server_name,t_buf);
	  strcat(server_name,"/");
	  strcat(server_name,XDAP_LISTENER);
	  strcpy(file_for_listener,t_buf);
	  strcat(file_for_listener,"/");
	  strcat(file_for_listener,XDAP_LISTENER_PID);
	}
    }
  if (strcmp(server_name,"")!=0)
    {
      cout<<NameProgram<<": checking for existance of listener "<<server_name<<"..."<<flush;
      if (access(server_name, F_OK)!=0)
	{
	  cout<<" failed"<<endl;
	  cerr<<NameProgram<<": can not start. Please set environment variable "
	      <<XDAP_LISTENER_LOCATION<<" to the location of your listener "<<server_name<<endl;
	  return false;
	}
      cout<<" OK"<<endl;
      return true;
    }
  cout<<NameProgram<<": failed to obtain a working directory!"<<endl;
  return false;
}
