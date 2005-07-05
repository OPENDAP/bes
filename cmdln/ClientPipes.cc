// System includes.
#include <unistd.h>
#include <sys/wait.h>

#include "ClientPipes.h"

// DODS includes.
#include "DDS.h"
#include "DAS.h"

#define DEFAULT_PAGER "/usr/bin/more"


// Global variables.
extern char *NameProgram;

// External variables.
extern int errno;


int ClientPipes::das_magic_stdout(DAS &das)
{
  char *pager;
  streambuf *out;
  out=cout.rdbuf();
  ostream os(out);
  pid_t pid1;
  int status1;
  if ((pid1=fork())<0)
    {
      cerr<<NameProgram<<": fork error\n";
      return 1;
    }
  else if (pid1==0) /* This is the child is the main process, we called child 1 */
    {
      int status;
      pid_t pid;
      int fd[2];
      if (pipe(fd)<0)
	{
	  cerr<<NameProgram<<": first child: Can not make pipe because of-> "<<strerror(errno)<<endl;
	  return 1; // if this child can not make pipe himself, good bye cruel world!!!
	}
      if ((pid=fork())<0)
	{
	  cerr<<NameProgram<<": first child: fork error\n";
	  return 1; // if this child can not fork himself, good bye cruel world!!!
	}
      else if (pid>0) /* Now here child 1 has a son child 2. child1 set his STDOUT to pipe to child2 */ 
	{
	  close (fd[0]); // He does not need to read from the pipe!!!
	  if(fd[1]!=STDOUT_FILENO)
	    {
	      if (dup2(fd[1],STDOUT_FILENO)!=STDOUT_FILENO)
		cerr<<"first child: dup2 error on stdout\n";
	      close(fd[1]);
	    }
	  das.print(os);
	  close(STDIN_FILENO);
	  close(STDOUT_FILENO);
	  if ( (pid=waitpid(pid,&status,0))<0 )
	    { 
	      cerr<<NameProgram<<": first child: waitpid error\n";
	      exit(1); // If child1 can not wait then dies...
	    }
	  exit (0); //This child1 is done, goodbye!!!!
	}
      else if (pid==0) /* This is child2, the pager */
	{
	  close (fd[1]);// He does need to write!!!
	  if(fd[0]!=STDIN_FILENO)
	    {
	      if (dup2(fd[0],STDIN_FILENO)!=STDIN_FILENO)
		cerr<<"second child: dup2 error on stdin\n";
	      close(fd[0]);
	    }
	  if((pager=getenv("PAGER"))==NULL)
	      pager=DEFAULT_PAGER;
	  execlp (pager,pager,(char*)0);
	  cerr<<NameProgram<<": second child: Could not execute external process "<<pager<<" because of-> "<<strerror(errno)<<endl;
	  exit(1);// Could not launch pager, good bye...
	}
    }
  if ( (pid1=waitpid(pid1,&status1,0))<0 )
    { 
      cerr<<NameProgram<<": waitpid error\n";
      return 1;
    }
  return 0;
}

int ClientPipes::dds_magic_stdout(DDS *pdds)
{
  char *pager;
  streambuf *out;
  out=cout.rdbuf();
  ostream os(out);
  pid_t pid1;
  int status1;
  if ((pid1=fork())<0)
    {
      cerr<<NameProgram<<": fork error\n";
      return 1;
    }
  else if (pid1==0)
    {
      cout<<NameProgram<<":"<<getpid()<<"staring"<<endl;
      int status;
      pid_t pid;
      int fd[2];
      if (pipe(fd)<0)
	{
	  cerr<<NameProgram<<": first child: Can not make pipe because of-> "<<strerror(errno)<<endl;
	  exit(1);
	}
      if ((pid=fork())<0)
	{
	  cerr<<NameProgram<<": first child: fork error\n";
	  exit(1);
	}
      else if (pid>0)
	{
	  close (fd[0]);
	  int hold_STDOUT_FILENO=STDOUT_FILENO;
	  if(fd[1]!=STDOUT_FILENO)
	    {
	      if (dup2(fd[1],STDOUT_FILENO)!=STDOUT_FILENO)
		cerr<<"first child: dup2 error on stdout\n";
	      close(fd[1]);
	    }
	  // for(Pix primera=pdds->first_var(); primera; pdds->next_var(primera))
// 	    {
// 	      if (primera)
// 		{
// 		  pdds->var(primera)->print_val(os);
// 		}
// 	    }
	  pdds->print();
	  close(STDIN_FILENO);
	  close(STDOUT_FILENO);
	  if ( (pid=waitpid(pid,&status,0))<0 )
	    { 
	      cerr<<NameProgram<<": first child: waitpid error\n";
	      exit(1);
	    }
	  cout<<NameProgram<<":"<<getpid()<<"staring"<<endl;
	  exit (0);
	}
      else if (pid==0)
	{
	  cout<<NameProgram<<":"<<getpid()<<"staring"<<endl;
	  close (fd[1]);
	  if(fd[0]!=STDIN_FILENO)
	    {
	      if (dup2(fd[0],STDIN_FILENO)!=STDIN_FILENO)
		cerr<<"second child: dup2 error on stdin\n";
	      close(fd[0]);
	    }
	  if((pager=getenv("PAGER"))==NULL)
	    pager=DEFAULT_PAGER;
	  execlp (pager,pager,(char*)0);
	  cerr<<NameProgram<<": second child: Could not execute external process "<<pager<<" because of-> "<<strerror(errno)<<endl;
	  exit(1);
	}
    }
  cout<<NameProgram<<":"<<getpid()<<"waiting"<<endl;
  if ( (pid1=waitpid(pid1,&status1,0))<0 )
    { 
      cerr<<NameProgram<<": waitpid error\n";
      return 1;
    }
  return 0;
}


  
  
