#include "nissa.hpp"

using namespace nissa;

void in_main(int narg,char **arg)
{
  
}

int main(int narg,char **arg)
{
  init_nissa_threaded(narg,arg,in_main);
  close_nissa();
  
  return 0;
}