#ifndef FOURIER_TRANSFORM
#define FOURIER_TRANSFORM

#include <math.h>

#include "../new_types/new_types_definitions.h"
#include "../new_types/su3.h"
#include "../base/global_variables.h"
#include "../base/macros.h"
#include "../base/debug.h"
#include "../base/vectors.h"

//produce the table of the momentum
void Momentum(int **iP,double *bc,double *P2,double *SinP2,double **P,double **SinP,double *SinP4,int nmom)
{
  int lP[4],imom=0;

  for(lP[0]=iP[0][0];lP[0]<=iP[0][1];lP[0]++)
    for(lP[1]=iP[1][0];lP[1]<=iP[1][1];lP[1]++)
      for(lP[2]=iP[2][0];lP[2]<=iP[2][1];lP[2]++)
	for(lP[3]=iP[3][0];lP[3]<=iP[3][1];lP[3]++)
	  {
	    P2[imom]=SinP2[imom]=SinP4[imom]=0;
	    for(int idir=0;idir<4;idir++)
	      {
		P[idir][imom]=2*M_PI*(lP[idir]+bc[idir]*0.5)/glb_size[idir];
		P2[imom]+=pow(P[idir][imom],2);
		SinP[idir][imom]=sin(P[idir][imom]);
		SinP2[imom]+=pow(SinP[idir][imom],2);
		SinP4[imom]+=pow(SinP[idir][imom],4);
	      }
	    imom++;
	  }

  if(imom!=nmom) crash("imom != nmom");
}

//perform the fourier transform momentum per momentum
void spincolor_FT(spincolor *S,spincolor *FT,double *theta,int **iP,int nmom)
{
  double *P2=nissa_malloc("P2",nmom,double);
  double *SinP2=nissa_malloc("SinP2",nmom,double);
  double *SinP4=nissa_malloc("SinP4",nmom,double);
  
  double *P[4];
  double *SinP[4];

  for(int idir=0;idir<4;idir++)
    {
      P[idir]=nissa_malloc("P[idir]",nmom,double);
      SinP[idir]=nissa_malloc("SinP[idir]",nmom,double);
    }
  
  Momentum(iP,theta,P2,SinP2,P,SinP,SinP4,nmom); 

  //local FT
  spincolor *FT_loc=nissa_malloc("FT_loc",nmom,spincolor);
  for(int imom=0;imom<nmom;imom++) spincolor_put_to_zero(FT_loc[imom]); 
	
  double lP[4];
  for(int imom=0;imom<nmom;imom++)
    {	
      for(int idir=0;idir<4;idir++) lP[idir]=P[idir][imom];

      nissa_loc_vol_loop(ivol)
	{
	  double arg=0;
	  for(int idir=0;idir<4;idir++) arg+=lP[idir]*glb_coord_of_loclx[ivol][idir];
	  complex eipx={cos(arg),-sin(arg)};

	  spincolor_summ_the_prod_complex(FT_loc[imom],S[ivol],eipx);			
	}
    }

  MPI_Reduce(FT_loc,FT,24*nmom,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
  
  for(int idir=0;idir<4;idir++)
    {
      nissa_free(P[idir]);
      nissa_free(SinP[idir]);
    }
  nissa_free(P2);nissa_free(SinP2);nissa_free(SinP4);
}

#endif