#ifndef _CG_INVERT_WCLOVQ2_H
#define _CG_INVERT_WCLOVQ2_H

void inv_WclovQ2_cg(spincolor *sol,spincolor *guess,quad_su3 *conf,double kappa,double csw,as2t_su3 *Pmunu,int niter,int rniter,double residue,spincolor *source);

#endif
