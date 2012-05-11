// Template to invert in quadruple precision using c.g.
// The solution is obtained in a mixed precision approach.
// macro to be defined:
//  -cg_128_invert
//  -cg_128_parameters_proto
//  -cg_128_inner_solver
//  -cg_128_inner_parameters_call
//  -size_of_bulk, size_of_bord
//  -basetype, basetype_128
//  -ndoubles_per_site

#include <string.h>

void cg_128_invert(basetype *sol,basetype *guess,cg_128_parameters_proto,int niter,double external_solver_residue,basetype *external_source)
{
  //Allocate the solution in 128 bit and initialize it. If guess passed copy it.
  basetype_128 *sol_128=nissa_malloc("sol_128",size_of_bulk+size_of_bord,basetype_128);
  memset(sol_128,0,size_of_bulk*sizeof(basetype_128));
  if(guess!=NULL) quadruple_vector_summassign_double_vector((float_128*)sol_128,(double*)guess,size_of_bulk*ndoubles_per_site);
  set_borders_invalid(sol_128);
  
  //compute and print source norm
  double source_norm=double_vector_glb_scalar_prod((double*)external_source,(double*)external_source,size_of_bulk*ndoubles_per_site);
  verbosity_lv2_master_printf("Source norm: %lg\n",source_norm);
  
  //internal inverter source
  basetype *internal_source=nissa_malloc("internal_source",size_of_bulk+size_of_bord,basetype);
  
  //residue in 128 bit
  basetype_128 *residue_128=nissa_malloc("residue_128",size_of_bulk+size_of_bord,basetype_128);
  
  //temporary vector for inverter
  basetype_128 *temp_128=nissa_malloc("temp",size_of_bulk+size_of_bord,basetype_128); 
  
  //invert until residue is lower than required
  double previous_residue=1,current_residue;
  int esci=0;
  do
    {
      // 1) compute D*sol in quadruple precision
      apply_operator_128(residue_128,cg_operator_128_parameters,sol_128);
      
      // 2) compute the new internal_source = external_source - OP * sol_128, and residue module
      quadruple_vector_subt_from_double_vector((float_128*)residue_128,(double*)external_source,(float_128*)residue_128,size_of_bulk*ndoubles_per_site);
      double_vector_from_quadruple_vector((double*)internal_source,(float_128*)residue_128,size_of_bulk*ndoubles_per_site);
      current_residue=double_conv_quadruple_vector_glb_scalar_prod((float_128*)residue_128,(float_128*)residue_128,size_of_bulk*ndoubles_per_site)/source_norm;
      verbosity_lv2_master_printf("\nExternal loop relative residue: %lg\n\n",current_residue);
      
      // 3) calibrate inner solver stopping condition
      double inner_solver_residue=max_double(1.e-20,external_solver_residue/current_residue);
      
      // 3) if residue not reached, compute the new approximated solution
      if(current_residue>=external_solver_residue)
	{
	  //compute partial sol
	  cg_128_inner_solver(sol,NULL,cg_128_inner_parameters_call,100000,1,inner_solver_residue,internal_source);
	  
	  //add the approximated solution to the total one
	  quadruple_vector_summassign_double_vector((float_128*)sol_128,(double*)sol,size_of_bulk*ndoubles_per_site);
	}
      if(!(current_residue<previous_residue))
	  {
	      esci=1;
	      master_printf("Not converging\n");
	  }
      previous_residue=current_residue;
    }
  while(current_residue>=external_solver_residue && !esci);
  
  verbosity_lv1_master_printf("\nFinal residue: %lg\n\n",current_residue);
  
  //copy the solution in 128 bit to the 64 bit
  double_vector_from_quadruple_vector((double*)sol,(float_128*)sol_128,size_of_bulk*ndoubles_per_site);
  
  nissa_free(residue_128);
  nissa_free(temp_128);
  nissa_free(sol_128);
  nissa_free(internal_source);
}

#undef basetype
#undef basetype_128
#undef ndoubles_per_site
#undef size_of_bulk
#undef size_of_bord

#undef apply_operator_128
#undef cg_operator_128_parameters
#undef cg_128_invert
#undef cg_128_parameters_proto
#undef cg_128_inner_parameters_call
#undef cg_128_inner_solver
