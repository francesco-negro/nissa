#include <math.h>

#include "nissa.h"

#include "../src/propagators/twisted_propagator.h"
#include "../src/diagrams/tadpole.h"
#include "../src/types/types_routines.h"
#include "../src/routines/read_and_write.h"
#include "../src/routines/correlations.h"

spinspin *prop,*tad_prop;
corr16 *corr;
double alpha=0;

//initialize the program
void init_calc(int narg,char **arg)
{
  //Basic mpi initialization
  init_nissa();
  
  if(narg<3) crash("use %s L T [alpha]",arg[0]);
  int T=atoi(arg[1]);
  int L=atoi(arg[2]);
  if(narg>=4) sscanf(arg[3],"%lg",&alpha);
  
  //init the grid
  init_grid(T,L);
  
  //allocate propagators
  prop=nissa_malloc("prop",loc_vol,spinspin);
  tad_prop=nissa_malloc("tad_prop",loc_vol,spinspin);
  corr=nissa_malloc("corr",loc_vol,corr16);
}

//close the program
void close_calc()
{
  nissa_free(prop);
  nissa_free(tad_prop);
  nissa_free(corr);
  
  close_nissa();
}

int main(int narg,char **arg)
{
  init_calc(narg,arg);
  
  //kappa and mass
  double kappa=1.0/8;
  double mass=0.00;
  double quark_theta[4]={1,0,0,0};
  quark_info qu=create_twisted_quark_info(kappa,mass,quark_theta);
  
  //gluon
  double gluon_theta[4]={0,0,0,0};
  gluon_info gl=create_tlSym_gluon_info(alpha,gluon_theta);
  master_printf("alpha=%lg\n",alpha);
  
  /////////////////////////////// propagator and pion computed analytically //////////////////////////
  
  compute_x_space_twisted_propagator_by_fft(prop,qu);
  compute_tadpole_twisted_propagator_in_x_space(tad_prop,qu,gl);
  
  //////////////////////////////// compute correlation and write them on disk ////////////////////////

  compute_all_2pts_qdagq_correlations(corr,prop,tad_prop);
  
  write_corr16("tadpole_corr",corr,64);
  
  close_calc();
  
  return 0;
}