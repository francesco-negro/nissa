#include <math.h>
#include <string.h>

#include "../new_types/new_types_definitions.h"
#include "../new_types/complex.h"
#include "../new_types/su3.h"
#include "../base/global_variables.h"
#include "../base/communicate.h"
#include "../geometry/geometry_eo.h"
#include "../geometry/geometry_lx.h"
#include "../base/vectors.h"
#include "../base/routines.h"

#ifdef BGP
 #include "../base/bgp_instructions.h"
#endif

//apply a gauge transformation to the conf
void gauge_transform_conf(quad_su3 *uout,su3 *g,quad_su3 *uin)
{
  //communicate borders
  communicate_lx_su3_borders(g);

  //transform
  su3 temp;
  nissa_loc_vol_loop(ivol)
    for(int mu=0;mu<4;mu++)
      {
	unsafe_su3_prod_su3_dag(temp,uin[ivol][mu],g[loclx_neighup[ivol][mu]]);
	unsafe_su3_prod_su3(uout[ivol][mu],g[ivol],temp);
      }
  
  //invaidate borders
  set_borders_invalid(uout);
}

//determine the gauge transformation bringing to temporal gauge with T-1 timeslice diferent from id
void find_temporal_gauge_fixing_matr(su3 *fixm,quad_su3 *u)
{
  int loc_slice_area=loc_size[1]*loc_size[2]*loc_size[3];
  su3 *buf=NULL;
  
  //if the number of ranks in the 0 dir is greater than 1 allocate room for border
  if(nrank_dir[0]>1) buf=nissa_malloc("buf",loc_slice_area,su3);
  
  //if we are on first rank slice put to identity the t=0 slice, otherwise receive it from previous rank slice
  if(rank_coord[0]==0)
    {
      nissa_loc_vol_loop(ivol)
	if(glb_coord_of_loclx[ivol][0]==0)
	  su3_put_to_id(fixm[ivol]);
    }
  else
    if(nrank_dir[0]>1)
      MPI_Recv((void*)fixm,loc_slice_area,MPI_SU3,rank_neighdw[0],252,cart_comm,MPI_STATUS_IGNORE);
  
  //now go ahead along t
  int c[4];
  //loop over spatial slice
  for(c[1]=0;c[1]<loc_size[1];c[1]++)
    for(c[2]=0;c[2]<loc_size[2];c[2]++)
      for(c[3]=0;c[3]<loc_size[3];c[3]++)
	{
	  //bulk
	  for(c[0]=1;c[0]<loc_size[0];c[0]++)
	    {
	      int icurr=loclx_of_coord(c);
	      c[0]--;int iback=loclx_of_coord(c);c[0]++;
	      
	      unsafe_su3_prod_su3(fixm[icurr],fixm[iback],u[iback][0]);
	    }
	  //border
	  if(nrank_dir[0]>1)
	    {
	      c[0]=loc_size[0]-1;int iback=loclx_of_coord(c);
	      c[0]=0;int icurr=loclx_of_coord(c);
	      
	      unsafe_su3_prod_su3(buf[icurr],fixm[iback],u[iback][0]);
	    }
	  
	}
  
  //if we are not on last slice of rank send g to next slice
  if(rank_coord[0]!=(nrank_dir[0]-1) && nrank_dir[0]>1)
    MPI_Send((void*)buf,loc_slice_area,MPI_SU3,rank_neighup[0],252,cart_comm);

  if(nrank_dir[0]>1) nissa_free(buf);
}

//////////////////////////////////////////// landau or coulomb gauges //////////////////////////////////////////////////////////////

//compute g=\sum_mu U_mu(x)+U^dag_mu(x-mu)
void compute_landau_or_coulomb_delta(su3 g,quad_su3 *conf,int ivol,int nmu)
{
  int b=loclx_neighdw[ivol][0];

#ifdef BGP

  bgp_complex g00,g01,g02,g10,g11,g12,g20,g21,g22;
  bgp_complex c00,c01,c02,c10,c11,c12,c20,c21,c22;
  bgp_complex b00,b01,b02,b10,b11,b12,b20,b21,b22;

  //first dir: reset and sum
  bgp_su3_load(c00,c01,c02,c10,c11,c12,c20,c21,c22, conf[ivol][0]);
  bgp_su3_load(b00,b01,b02,b10,b11,b12,b20,b21,b22, conf[b][0]);
  bgp_su3_summ_su3_dag(g00,g01,g02,g10,g11,g12,g20,g21,g22,
		       c00,c01,c02,c10,c11,c12,c20,c21,c22,
		       b00,b01,b02,b10,b11,b12,b20,b21,b22);
  //remaining dirs
  for(int mu=1;mu<nmu;mu++)
    {
      b=loclx_neighdw[ivol][mu];
      
      bgp_su3_load(c00,c01,c02,c10,c11,c12,c20,c21,c22, conf[ivol][mu]);
      bgp_su3_load(b00,b01,b02,b10,b11,b12,b20,b21,b22, conf[b][mu]);
      bgp_su3_summassign_su3(g00,g01,g02,g10,g11,g12,g20,g21,g22,
			     c00,c01,c02,c10,c11,c12,c20,c21,c22);
      bgp_su3_summassign_su3_dag(g00,g01,g02,g10,g11,g12,g20,g21,g22,
				 b00,b01,b02,b10,b11,b12,b20,b21,b22);
    }

  bgp_su3_save(g, g00,g01,g02,g10,g11,g12,g20,g21,g22);

#else

  //first dir: reset and sum
  for(int ic1=0;ic1<3;ic1++)
    for(int ic2=0;ic2<3;ic2++)
      {
	g[ic1][ic2][0]=conf[ivol][0][ic1][ic2][0]+conf[b][0][ic2][ic1][0];
	g[ic1][ic2][1]=conf[ivol][0][ic1][ic2][1]-conf[b][0][ic2][ic1][1];
      }
  //remaining dirs
  for(int mu=1;mu<nmu;mu++)
    {
      b=loclx_neighdw[ivol][mu];
      
      for(int ic1=0;ic1<3;ic1++)
	for(int ic2=0;ic2<3;ic2++)
	  {
	    g[ic1][ic2][0]+=conf[ivol][mu][ic1][ic2][0]+conf[b][mu][ic2][ic1][0];
	    g[ic1][ic2][1]+=conf[ivol][mu][ic1][ic2][1]-conf[b][mu][ic2][ic1][1];
	  }
    }

#endif

}

//horrible, horrifying routine of unknown meaning copied from APE
void exponentiate(su3 g,su3 a)
{
  //Exponentiate. Commented lines are original from APE (more or less)
  //up0=a[0][0]+a[1][1]~
  complex up0;
  complex_summ_conj2(up0,a[0][0],a[1][1]);
  //up1=a[0][1]-a[1][0]~
  complex up1;
  complex_subt_conj2(up1,a[0][1],a[1][0]);
  //icsi=1/sqrt(up0*up0~+up1*up1~)
  double icsi=1/sqrt(squared_complex_norm(up0)+squared_complex_norm(up1));
  //up0=up0*icsi
  complex_prod_double(up0,up0,icsi);
  //appuno=up0~
  complex appuno;
  complex_conj(appuno,up0);
  //up1=up1*icsi
  complex_prod_double(up1,up1,icsi);
  //appdue=-up1
  complex appdue;
  complex_prod_double(appdue,up1,-1);
  //n0=a[0][0]*appuno+a[1][0]*appdue
  complex n0;
  unsafe_complex_prod(n0,a[0][0],appuno);
  complex_summ_the_prod(n0,a[1][0],appdue);
  //n2=a[0][2]*appuno+a[1][2]*appdue
  complex n2;
  unsafe_complex_prod(n2,a[0][2],appuno);
  complex_summ_the_prod(n2,a[1][2],appdue);
  //n3=-a[0][0]*appdue~+a[1][0]*appuno~
  complex n3;
  unsafe_complex_conj2_prod(n3,a[1][0],appuno);
  complex_subt_the_conj2_prod(n3,a[0][0],appdue);
  //n6=a[2][0]
  complex n6={a[2][0][0],a[2][0][1]};
  //n5=-a[0][2]*appdue~+a[1][2]*appuno~
  complex n5;
  unsafe_complex_conj2_prod(n5,a[1][2],appuno);
  complex_subt_the_conj2_prod(n5,a[0][2],appdue);
  //n7=a[2][1]
  complex n7={a[2][1][0],a[2][1][1]};
  //up1=n5-n7~
  complex_subt_conj2(up1,n5,n7);
  //n4=-a[0][1]*appdue~+a[1][1]*appuno~
  complex n4;
  unsafe_complex_conj2_prod(n4,a[1][1],appuno);
  complex_subt_the_conj2_prod(n4,a[0][1],appdue);
  //n8=a[2][2]
  complex n8={a[2][2][0],a[2][2][1]};
  //up0=n8~+n4 
  complex_summ_conj1(up0,n8,n4);
  //icsi=1/sqrt(up0*up0~+up1*up1~)
  icsi=1/sqrt(squared_complex_norm(up0)+squared_complex_norm(up1));
  //up0=up0*icsi
  complex_prod_double(up0,up0,icsi);
  //bppuno=up0~
  complex bppuno;
  complex_conj(bppuno,up0);
  //up1=up1*icsi
  complex_prod_double(up1,up1,icsi);
  //bppdue=-up1
  complex bppdue={-up1[0],-up1[1]};
  //a[0][2]=n2
  a[0][2][0]=n2[0];
  a[0][2][1]=n2[1];
  //a[2][0]=-n3*bppdue~+n6*bppuno~
  unsafe_complex_conj2_prod(a[2][0],n6,bppuno);
  complex_subt_the_conj2_prod(a[2][0],n3,bppdue);
  //up1=a[0][2]-a[2][0]~
  complex_subt_conj2(up1,a[0][2],a[2][0]);
  //a[0][0]=n0
  a[0][0][0]=n0[0];
  a[0][0][1]=n0[1];
  //a[2][2]=-n5*bppdue~+n8*bppuno~
  unsafe_complex_conj2_prod(a[2][2],n8,bppuno);
  complex_subt_the_conj2_prod(a[2][2],n5,bppdue);
  //up0=a[2][2]~+a[0][0]
  complex_summ_conj1(up0,a[2][2],a[0][0]);
  //icsi=1/sqrt(up0*up0~+up1*up1~)
  icsi=1/sqrt(squared_complex_norm(up0)+squared_complex_norm(up1));
  //up0=up0*icsi
  complex_prod_double(up0,up0,icsi);
  //cppuno=up0~
  complex cppuno;
  complex_conj(cppuno,up0);
  //up1=up1*icsi
  complex_prod_double(up1,up1,icsi);
  //cppdue=-up1
  complex cppdue={-up1[0],-up1[1]};
  //e0=appuno
  complex e0={appuno[0],appuno[1]};
  //e1=appdue
  complex e1={appdue[0],appdue[1]};
  //e2=0
  //complex e2={0,0};
  //e3=-bppuno*appdue~
  complex e3={0,0};
  complex_subt_the_conj2_prod(e3,bppuno,appdue);
  //e4=bppuno*appuno~
  complex e4;
  unsafe_complex_conj2_prod(e4,bppuno,appuno);
  //e5=bppdue
  complex e5={bppdue[0],bppdue[1]};
  //e6=bppdue~*appdue~
  complex e6;
  unsafe_complex_conj_conj_prod(e6,bppdue,appdue);
  //e7=-bppdue~*appuno~
  complex e7={0,0};
  complex_subt_the_conj_conj_prod(e7,bppdue,appuno);
  //e8=bppuno~
  complex e8;
  complex_conj(e8,bppuno);
  //g[0][0]=cppuno*e0+cppdue*e6
  unsafe_complex_prod(g[0][0],cppuno,e0);
  complex_summ_the_prod(g[0][0],cppdue,e6);
  //g[0][1]=cppuno*e1+cppdue*e7
  unsafe_complex_prod(g[0][1],cppuno,e1);
  complex_summ_the_prod(g[0][1],cppdue,e7);
  //g[0][2]=cppdue*e8
  unsafe_complex_prod(g[0][2],cppdue,e8);
  //g[1][0]=e3
  g[1][0][0]=e3[0];
  g[1][0][1]=e3[1];
  //g[1][1]=e4
  g[1][1][0]=e4[0];
  g[1][1][1]=e4[1];
  //g[1][2]=e5
  g[1][2][0]=e5[0];
  g[1][2][1]=e5[1];
  //g[2][0]=-cppdue~*e0+cppuno~*e6
  unsafe_complex_conj1_prod(g[2][0],cppuno,e6);
  complex_subt_the_conj1_prod(g[2][0],cppdue,e0);
  //g[2][1]=-cppdue~*e1+cppuno~*e7
  unsafe_complex_conj1_prod(g[2][1],cppuno,e7);
  complex_subt_the_conj1_prod(g[2][1],cppdue,e1);
  //g[2][2]=cppuno~*e8
  unsafe_complex_conj1_prod(g[2][2],cppuno,e8);
}

//overrelax the transformation
void overrelax(su3 out,su3 in,double w)
{
  double coef[5]={1,w,w*(w-1)/2,w*(w-1)*(w-2)/6,w*(w-1)*(w-2)*(w-3)/24};
  su3 t[5];

#ifdef BGP
  bgp_complex f00,f01,f02,f10,f11,f12,f20,f21,f22;
  bgp_complex o00,o01,o02,o10,o11,o12,o20,o21,o22;
  bgp_complex t00,t01,t02,t10,t11,t12,t20,t21,t22;
  bgp_complex r0,r1,r2;
  bgp_complex buno;
  
  //prepare multiplicative factor
  bgp_su3_load(f00,f01,f02,f10,f11,f12,f20,f21,f22,in);
  complex uno={1,0};
  bgp_complex_load(buno,uno);
  bgp_subtassign_complex(f00,buno);
  bgp_subtassign_complex(f11,buno);
  bgp_subtassign_complex(f22,buno);
  
  //order 0-1

  bgp_su3_prod_double(o00,o01,o02,o10,o11,o12,o20,o21,o22, f00,f01,f02,f10,f11,f12,f20,f21,f22, coef[1]);
  bgp_summassign_complex(o00,buno);bgp_summassign_complex(o11,buno);bgp_summassign_complex(o22,buno);
  bgp_su3_save(t[1],f00,f01,f02,f10,f11,f12,f20,f21,f22);

  for(int iord=2;iord<5;iord++)
    {
      bgp_su3_load(t00,t01,t02,t10,t11,t12,t20,t21,t22, t[iord-1]);
      
      //t'_0i = t_0j * f_ji ; o_0i+=t'_0i*c
      bgp_color_prod_su3(r0,r1,r2, t00,t01,t02, f00,f01,f02,f10,f11,f12,f20,f21,f22);
      bgp_color_save(t[iord][0], r0,r1,r2);
      bgp_summassign_color_prod_double(o00,o01,o02, r0,r1,r2, coef[iord]);

      //t'_1i = t_1j * f_ji ; o_1i+=t'_1i*c
      bgp_color_prod_su3(r0,r1,r2, t10,t11,t12, f00,f01,f02,f10,f11,f12,f20,f21,f22);
      bgp_color_save(t[iord][1], r0,r1,r2);
      bgp_summassign_color_prod_double(o10,o11,o12, r0,r1,r2, coef[iord]);

      //t'_2i = t_2j * f_ji ; o_2i+=t'_2i*c
      bgp_color_prod_su3(r0,r1,r2, t20,t21,t22, f00,f01,f02,f10,f11,f12,f20,f21,f22);
      bgp_color_save(t[iord][2], r0,r1,r2);
      bgp_summassign_color_prod_double(o20,o21,o22, r0,r1,r2, coef[iord]);
    }
  bgp_su3_save(out, o00,o01,o02,o10,o11,o12,o20,o21,o22);
#else
  su3 f;
  su3_summ_real(f,in,-1);   //subtract 1 from in

  //ord 0
  su3_put_to_id(out);       //output init
  
  //ord 1
  su3_copy(t[1],f);
  su3_summ_the_prod_double(out,t[1],coef[1]);

  //ord 2-4
  for(int iord=2;iord<5;iord++)
    {
      unsafe_su3_prod_su3(t[iord],t[iord-1],f);
      su3_summ_the_prod_double(out,t[iord],coef[iord]);
    }
#endif
}

//find the transformation bringing to the landau or coulomb gauge the point ivol
void find_local_landau_or_coulomb_gauge_fixing_transformation(su3 g,quad_su3 *conf,int ivol,int nmu)
{
  su3 h;
  
  //compute the local summ of U_\mu(x)+U^\dagger_\mu(x-\mu)find local transformation matrix on point ivol
  compute_landau_or_coulomb_delta(h,conf,ivol,nmu);
  //exponentiate
  exponentiate(g,h);
  //overrelax and unitarize
  overrelax(h,g,1.72);
  //unitarize
  su3_unitarize_orthonormalizing(g,h);
}

//apply the passed transformation to the point
void local_gauge_transform(quad_su3 *conf,su3 g,int ivol)
{
#ifdef BGP
  bgp_complex g00,g01,g02,g10,g11,g12,g20,g21,g22;
  bgp_complex o00,o01,o02,o10,o11,o12,o20,o21,o22;
  bgp_complex i00,i01,i02,i10,i11,i12,i20,i21,i22;
  bgp_su3_load(g00,g01,g02,g10,g11,g12,g20,g21,g22,g);
#endif

  // for each dir...
  for(int mu=0;mu<4;mu++)
    {
      int b=loclx_neighdw[ivol][mu];
      
      //perform local gauge transform
#ifdef BGP
      bgp_su3_load(i00,i01,i02,i10,i11,i12,i20,i21,i22,conf[ivol][mu]);
      bgp_su3_prod_su3(o00,o01,o02,o10,o11,o12,o20,o21,o22,
		       g00,g01,g02,g10,g11,g12,g20,g21,g22, 
		       i00,i01,i02,i10,i11,i12,i20,i21,i22);
      bgp_su3_save(conf[ivol][mu], o00,o01,o02,o10,o11,o12,o20,o21,o22);
      
      bgp_su3_load(i00,i01,i02,i10,i11,i12,i20,i21,i22,conf[b][mu]);
      bgp_su3_prod_su3_dag(o00,o01,o02,o10,o11,o12,o20,o21,o22,
			   i00,i01,i02,i10,i11,i12,i20,i21,i22,
			   g00,g01,g02,g10,g11,g12,g20,g21,g22);
      bgp_su3_save(conf[b][mu], o00,o01,o02,o10,o11,o12,o20,o21,o22);
#else
      safe_su3_prod_su3(conf[ivol][mu],g,conf[ivol][mu]);
      safe_su3_prod_su3_dag(conf[b][mu],conf[b][mu],g);
#endif
    }
}

//compute delta for the quality of landau or coulomb gauges
void compute_landau_or_coulomb_quality_delta(su3 g,quad_su3 *conf,int ivol,int nmu)
{
  //reset g
  memset(g,0,sizeof(su3));
  
  //Calculate \sum_mu(U_mu(x-mu)-U_mu(x-mu)^dag-U_mu(x)+U^dag_mu(x))
  //and subtract the trace. It is computed just as the TA (traceless anti-herm)
  //this has 8 independent element (A+F+I)=0
  // ( 0,A) ( B,C) (D,E)
  // (-B,C) ( 0,F) (G,H)
  // (-D,E) (-G,H) (0,I)
  for(int mu=0;mu<nmu;mu++)
    {
      int b=loclx_neighdw[ivol][mu];
      
      //off-diagonal real parts
      g[0][1][0]+=conf[b][mu][0][1][0]-conf[b][mu][1][0][0]-conf[ivol][mu][0][1][0]+conf[ivol][mu][1][0][0]; //B
      g[0][2][0]+=conf[b][mu][0][2][0]-conf[b][mu][2][0][0]-conf[ivol][mu][0][2][0]+conf[ivol][mu][2][0][0]; //D
      g[1][2][0]+=conf[b][mu][1][2][0]-conf[b][mu][2][1][0]-conf[ivol][mu][1][2][0]+conf[ivol][mu][2][1][0]; //G
      
      //off diagonal imag parts
      g[0][1][1]+=conf[b][mu][0][1][1]+conf[b][mu][1][0][1]-conf[ivol][mu][0][1][1]-conf[ivol][mu][1][0][1]; //C
      g[0][2][1]+=conf[b][mu][0][2][1]+conf[b][mu][2][0][1]-conf[ivol][mu][0][2][1]-conf[ivol][mu][2][0][1]; //E
      g[1][2][1]+=conf[b][mu][1][2][1]+conf[b][mu][2][1][1]-conf[ivol][mu][1][2][1]-conf[ivol][mu][2][1][1]; //H
      
      //digonal imag parts
      g[0][0][1]+=conf[b][mu][0][0][1]+conf[b][mu][0][0][1]-conf[ivol][mu][0][0][1]-conf[ivol][mu][0][0][1]; //A
      g[1][1][1]+=conf[b][mu][1][1][1]+conf[b][mu][1][1][1]-conf[ivol][mu][1][1][1]-conf[ivol][mu][1][1][1]; //F
      g[2][2][1]+=conf[b][mu][2][2][1]+conf[b][mu][2][2][1]-conf[ivol][mu][2][2][1]-conf[ivol][mu][2][2][1]; //I
    }
  
  //compute the trace
  double T3=(g[0][0][1]+g[1][1][1]+g[2][2][1])/3;
  
  //subtract 1/3 of the trace from each element, so to make traceless the matrix
  g[0][0][1]-=T3;
  g[1][1][1]-=T3;
  g[2][2][1]-=T3;
  
  //fill the other parts
  
  //off-diagonal real parts
  g[1][0][0]=-g[0][1][0];
  g[2][0][0]=-g[0][2][0];
  g[2][1][0]=-g[1][2][0];
  
  //off diagonal imag parts
  g[1][0][1]=g[0][1][1];
  g[2][0][1]=g[0][2][1];
  g[2][1][1]=g[1][2][1];
}

//compute the quality of the landau or coulomb gauge fixing
double compute_landau_or_coulomb_gauge_fixing_quality(quad_su3 *conf,int nmu)
{
  communicate_lx_quad_su3_borders(conf);

  double loc_omega=0;
  nissa_loc_vol_loop(ivol)
    {
      su3 delta;
      compute_landau_or_coulomb_quality_delta(delta,conf,ivol,nmu);
      
      //compute the trace of the square and summ it to omega
      for(int i=0;i<18;i++) loc_omega+=((double*)delta)[i]*((double*)delta)[i];
    }
  
  //global reduction
  double glb_omega;
  MPI_Allreduce(&loc_omega,&glb_omega,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  
  return glb_omega/glb_vol/3;
}

//find the transformation bringing to the landau or coulomb gauge
void find_landau_or_coulomb_gauge_fixing_matr(su3 *fixm,quad_su3 *conf,double required_precision,int nmu)
{
  //allocate working conf
  quad_su3 *w_conf=nissa_malloc("Working conf",loc_vol+bord_vol,quad_su3);
  
  //set eo geometry, used to switch between different parity sites
  if(nissa_eo_geom_inited==0) set_eo_geometry();
  
  //count even and odd elements on each external inferior and internal
  //superior border so to allocate room for buffers
  int bpar_dw_size[4][2]={{0,0},{0,0},{0,0},{0,0}};
  int bpar_up_size[4][2]={{0,0},{0,0},{0,0},{0,0}};
  nissa_loc_vol_loop(ivol)
    for(int mu=0;mu<4;mu++)
      {
	int b=loclx_neighdw[ivol][mu];
	int f=loclx_neighup[ivol][mu];
	if(b>=loc_vol) bpar_dw_size[mu][loclx_parity[b]]++;
	if(f>=loc_vol) bpar_up_size[mu][loclx_parity[ivol]]++;
      }
  
  //allocate buffer for sending borders
  su3 *buf_up[4][2];
  su3 *buf_dw[4][2];
  for(int mu=0;mu<4;mu++)
    if(paral_dir[mu])
      for(int par=0;par<2;par++)
	{
	  buf_dw[mu][par]=nissa_malloc("Buf_dw",bpar_dw_size[mu][par],su3);
	  buf_up[mu][par]=nissa_malloc("Buf_up",bpar_up_size[mu][par],su3);
	}
  
  //reset fixing transformation to unity
  nissa_loc_vol_loop(ivol) su3_put_to_id(fixm[ivol]);
  
  //fix iteratively up to reaching required precision, performing a
  //macro-loop in which the fixing is effectively applied to the original
  //conf: this is done in order to avoid accumulated rounding errors
  int iter=0,macro_iter=0;
  double true_precision;
  do
    {
      //copy the gauge configuration on working fixing it with current transformation
      gauge_transform_conf(w_conf,fixm,conf);
      
      //compute initial precision
      true_precision=compute_landau_or_coulomb_gauge_fixing_quality(w_conf,nmu);
      
      if(macro_iter==0) master_printf("Iter 0 [macro: 0], quality: %lg (%lg req)\n",true_precision,required_precision);
      else master_printf("Finished macro iter %d, true quality: %lg (%lg req)\n",macro_iter-1,true_precision,required_precision);
      macro_iter++;
      
      //loop fixing iteratively the working conf
      double precision=true_precision;
      double communic_time=0,computer_time=0;
      while(true_precision>=required_precision && precision>=required_precision)
	{
	  //alternate even and odd
	  for(int par=0;par<2;par++)
	    {
	      double initial_time=take_time();
	      
	      int isend_dw[4]={0,0,0,0};
	      int isend_up[4]={0,0,0,0};
	      
	      nissa_loc_vol_loop(ivol)
		if(loclx_parity[ivol]==par)
		  {
		    su3 g;
		    
		    // 1) find the local transformation bringing to the landau or coulomb gauge
		    find_local_landau_or_coulomb_gauge_fixing_transformation(g,w_conf,ivol,nmu);
		    
		    // 2) locally transform gauge
		    local_gauge_transform(w_conf,g,ivol);
		    
		    // 3) save gauge transformation
		    safe_su3_prod_su3(fixm[ivol],g,fixm[ivol]);
		    
		    // 4) lower external border must be sync.ed with upper internal border of lower node
		    //  upper internal border with same parity must be sent using buf_up[mu][par]
		    //    ""     ""      ""   ""   opp.    "     "  "  recv using buf_up[mu][!par]
		    //  lower external   ""   ""   same    "     "  "  recv using buf_dw[mu][!par]
		    //    ""     ""      ""   ""   opp.    "     "  "  sent using buf_dw[mu][par]
		    for(int mu=0;mu<4;mu++)
		      {
			int f=loclx_neighup[ivol][mu];
			int b=loclx_neighdw[ivol][mu];
			if(f>=loc_vol) su3_copy(buf_up[mu][par][isend_up[mu]++],w_conf[ivol][mu]);
			if(b>=loc_vol) su3_copy(buf_dw[mu][!par][isend_dw[mu]++],w_conf[b][mu]); //indeed parity is of b
		      }
		  }
	      
	      double intermediate_time=take_time();
	      
	      //communicate the borders
	      int nrequest=0;
	      MPI_Request request[16];
	      MPI_Status status[16];
	      for(int mu=0;mu<4;mu++)
		if(paral_dir[mu])
		  {
		    MPI_Isend((void*)buf_up[mu][ par],bpar_up_size[mu][ par],MPI_SU3,rank_neighup[mu],87+4+mu,cart_comm,&request[nrequest++]);
		    MPI_Irecv((void*)buf_up[mu][!par],bpar_up_size[mu][!par],MPI_SU3,rank_neighup[mu],87+  mu,cart_comm,&request[nrequest++]);
		    MPI_Irecv((void*)buf_dw[mu][ par],bpar_dw_size[mu][ par],MPI_SU3,rank_neighdw[mu],87+4+mu,cart_comm,&request[nrequest++]);
		    MPI_Isend((void*)buf_dw[mu][!par],bpar_dw_size[mu][!par],MPI_SU3,rank_neighdw[mu],87+  mu,cart_comm,&request[nrequest++]);
		  }
	      if(nrequest>0) MPI_Waitall(nrequest,request,status);
	      
	      //now unpack the receive borders
	      int irecv_dw[4]={0,0,0,0};
	      int irecv_up[4]={0,0,0,0};
	      nissa_loc_vol_loop(ivol)
		if(loclx_parity[ivol]!=par)
		  for(int mu=0;mu<4;mu++)
		    {
		      int f=loclx_neighup[ivol][mu];
		      int b=loclx_neighdw[ivol][mu];
		      if(f>=loc_vol) su3_copy(w_conf[ivol][mu],buf_up[mu][!par][irecv_up[mu]++]);
		      if(b>=loc_vol) su3_copy(w_conf[b][mu],buf_dw[mu][par][irecv_dw[mu]++]); //indeed parity is of b
		    }
	      
	      double final_time=take_time();
	      
	      computer_time+=intermediate_time-initial_time;
	      communic_time+=final_time-intermediate_time;
	    }
	  
	  set_borders_invalid(w_conf);
	  
	  //compute precision
	  if(iter%10==0 && iter!=0)
	    {
	      precision=compute_landau_or_coulomb_gauge_fixing_quality(w_conf,nmu);	  
	      master_printf("Iter %d [macro: %d], quality: %16.16lg (%lg req), compute time %lg sec, comm time %lg sec\n",
			    iter,macro_iter-1,precision,required_precision,computer_time,communic_time);
	      communic_time=computer_time=0;
	    }
	  
	  iter++;
	}
      
      //normalize the transformation
      nissa_loc_vol_loop(ivol) su3_unitarize_explicitly_inverting(fixm[ivol],fixm[ivol]);
      
      //go to the beginning and check the quality of the macro iteration
      set_borders_invalid(fixm);
    }
  while(true_precision>=required_precision);
  
  master_printf("Final quality: %16.16lg\n",true_precision);
  
  //free buffers
  for(int mu=0;mu<4;mu++)
    if(paral_dir[mu])
      for(int par=0;par<2;par++)
	{
	  nissa_free(buf_dw[mu][par]);
	  nissa_free(buf_up[mu][par]);
	}
  
  nissa_free(w_conf);
}

//perform the landau or coulomb gauge fixing
void landau_or_coulomb_gauge_fix(quad_su3 *conf_out,quad_su3 *conf_in,double precision,int nmu)
{
  //allocate fixing matrix
  su3 *fixm=nissa_malloc("fixm",loc_vol+bord_vol,su3);
  
  //find fixing matrix
  find_landau_or_coulomb_gauge_fixing_matr(fixm,conf_in,precision,nmu);
  
  //apply the transformation
  gauge_transform_conf(conf_out,fixm,conf_in);
  
  //free fixing matrix
  nissa_free(fixm);
}

//wrappers
void landau_gauge_fix(quad_su3 *conf_out,quad_su3 *conf_in,double precision)
{landau_or_coulomb_gauge_fix(conf_out,conf_in,precision,4);}
void coulomb_gauge_fix(quad_su3 *conf_out,quad_su3 *conf_in,double precision)
{landau_or_coulomb_gauge_fix(conf_out,conf_in,precision,3);}