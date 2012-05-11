#include <mpi.h>
#include <stdlib.h>
#include <execinfo.h>
#include <stdarg.h>

#include "global_variables.h"
#include "routines.h"
#include "vectors.h"

void print_backtrace_list()
{
  void *callstack[128];
  int frames=backtrace(callstack,128);
  char **strs=backtrace_symbols(callstack,frames);
  
  master_printf("Backtracing...\n");
  for(int i=0;i<frames;i++) master_printf("%s\n",strs[i]);
  
  free(strs);               
}

//crash
void internal_crash(int line,const char *file,const char *templ,...)
{
  va_list ap;
  va_start(ap,templ);
  
  fflush(stdout);
  fflush(stderr);
  
  if(rank==0)
    {
      char mess[1024];
      vsprintf(mess,templ,ap);
      fprintf(stderr,"ERROR on line %d of file \"%s\", message error: \"%s\".\n",line,file,mess);
      print_backtrace_list();
      MPI_Abort(MPI_COMM_WORLD,1);
    }
    
    va_end(ap);
}

void terminate_sigsegv(int par)
{
  //if(par==11)
    {
      print_all_nissa_vect_content();
      print_backtrace_list();
      crash("Signal %d detected, exiting\n",par);
    }
}

void internal_decript_MPI_error(int line,const char *file,int rc,const char *templ,...)
{
  va_list ap;
  va_start(ap,templ);
  
  if(rank==0)
    {
      char err[1024];
      int len=1024;
      MPI_Error_string(rc,err,&len);
      char mess[1024];
      vsprintf(mess,templ,ap);
      internal_crash(line,file,"%s, raised error: %s",mess,err);
    }
    
    va_end(ap);
}