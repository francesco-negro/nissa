#ifndef _GEOMETRY_VIR_HPP
#define _GEOMETRY_VIR_HPP

#ifndef EXTERN_GEOMETRY_VIR
 #define EXTERN_GEOMETRY_VIR extern
#endif

#include "base/grid.hpp"
#include "base/vectors.hpp"
#include "geometry/geometry_eo.hpp"
#include "geometry/geometry_lx.hpp"

#include "new_types/su3.hpp"
#include "new_types/float_128.hpp"

namespace nissa
{
#if SIMD_TYPE == SIMD_EMU
  const int simd_width=256;
#endif
  const int nvranks_max=simd_width/(8*sizeof(float));
  
  EXTERN_GEOMETRY_VIR int use_vranks;
  EXTERN_GEOMETRY_VIR int vloc_min_vol;
  EXTERN_GEOMETRY_VIR coords vloc_min_size;
  
  //! structure to hold a virtual geometry
  template <class base_type> struct vranks_geom_t
  {
    //! number of elements inside a simd
    static const int nvranks=simd_width/(8*sizeof(base_type));
    
    //! initialization flag
    int inited;
    //! hold which directions are parallelized
    coords vparal_dir;
    //! coordinateds of the elements
    coords vrank_coord[nvranks];
    //! number of parallelized directions
    int nvparal_dir;
    //! virtual rank local volume
    int vloc_vol;
    //! volume of the even or odd sublattices
    int vloc_volh;
    //! size of the virtual ranks grid
    coords vloc_size;
    //! number of virtual ranks per direction
    coords nvranks_per_dir;
    //! offset between corresponding elements in vrank 0
    int vrank_loclx_offset[nvranks];
    //! virtual rank hosting a local lattice element
    int *vrank_of_loclx;
    //! virtual local element corresponding to vrank 0 equivalent of loclx
    int *vloc_of_loclx;
    //! loclx corresponding to virtual vrank 0 element
    int *loclx_of_vloc;
    //! virtual local element (e/o) corresponding to vrank 0 equivalent of loclx
    int *veos_of_loclx;
    //! loclx corresponding to vrank 0 element
    int *loclx_of_veos[2];
    //! virtual local element (e/o) corresponding to vrank 0 equivalent of loceo
    int *veos_of_loceo[2];
    //! loclx corresponding to vrank 0 element
    int *loceo_of_veos[2];
    
    //! return the rank of the loceo
    int vrank_of_loceo(int par,int eo)
    {return vrank_of_loclx[loclx_of_loceo[par][eo]];}
    
    //! return the vn=0 equivalent
    int vn0lx_of_loclx(int loclx)
    {return loclx_of_vloc[vloc_of_loclx[loclx]];}
    
    //! init the geometry
    void init(void (*fill)(vranks_geom_t *geo))
    {
      if(!inited)
      {
  	inited=true;
	master_printf("Initializing vgeom for type of %d bits, nvranks=%d\n",(int)sizeof(base_type)*8,nvranks);
	
  	//CRASH if eo-geom not inited
  	if(!eo_geom_inited)
  	  {
  	    if(!use_eo_geom) CRASH("eo geometry must be enabled in order to use vir one");
  	    CRASH("initialize eo_geometry before vir one");
  	  }
	
	//set volume of the virtual node
	vloc_vol=loc_vol/nvranks;
	vloc_volh=vloc_vol/2;
	
	//allocate
	vrank_of_loclx=nissa_malloc("vrank_of_loclx",loc_vol,int);
	vloc_of_loclx=nissa_malloc("vloc_of_loclx",loc_vol,int);
	veos_of_loclx=nissa_malloc("veos_of_loclx",loc_vol,int);
	loclx_of_vloc=nissa_malloc("loclx_of_vloc",vloc_vol,int);
	for(int par=0;par<2;par++)
	  {
	    loclx_of_veos[par]=nissa_malloc("loceo_of_vloc",vloc_volh,int);
	    loceo_of_veos[par]=nissa_malloc("loceo_of_veos",vloc_volh,int);
	    veos_of_loceo[par]=nissa_malloc("veo_of_loceo",loc_volh,int);
	  }
	
	//fix the number of minimal node
	partitioning_t vranks_partitioning(loc_vol,nvranks);
	master_printf("Grouping loc_vol %d in %d vranks obtained %u possible combos\n",loc_vol,nvranks,vranks_partitioning.ncombo);
	coords VRPD;
	coords min_loc_size,fix_nvranks;
	for(int mu=0;mu<NDIM;mu++)
	  {
	    min_loc_size[mu]=2;
	    fix_nvranks[mu]=0;
	  }
	master_printf("need to find the optimal here ANNNNNNA\n");
	while(vranks_partitioning.find_next_valid_partition(VRPD,loc_size,min_loc_size,fix_nvranks))
	  {
	    //set the vir local size
	    //coords VLS;
	    //for(int mu=0;mu<NDIM;mu++) VLS[mu]=nvloc_max_per_dir[mu]/VPD[mu];

	    //if it is the minimal surface (or first valid combo) copy it and compute the border size
	    //if(rel_surf<min_rel_surf||min_rel_surf<0)
	      {
		//min_rel_surf=rel_surf;
		
		for(int mu=0;mu<NDIM;mu++)
		  {
		    nvranks_per_dir[mu]=VRPD[mu];
		    vloc_size[mu]=loc_size[mu]/VRPD[mu];
		  }
		
		//something_found=1;
	      }
	  }
	
	//assign vrank to all loclx
	for(int loclx=0;loclx<loc_vol;loclx++)
	  {
	    coords vrank_coords;  //< coordinates of the virtual rank
	    //coords vloclx_coords; //< coordinates inside the virtual rank
	    for(int mu=0;mu<NDIM;mu++)
	      {
		vrank_coords[mu]=loc_coord_of_loclx[loclx][mu]/vloc_size[mu];
		//vloclx_coords[mu]=loc_coord_of_loclx[loclx][mu]-vrank_coords[mu]*vloc_size[mu];
	      }
	      vrank_of_loclx[loclx]=lx_of_coord(vrank_coords,nvranks_per_dir);
	  }
	
	//offset between lx sites relative to a single vector
	for(int vrank=0;vrank<nvranks;vrank++)
	  {
	    coords c;
	    for(int mu=0;mu<NDIM;mu++) c[mu]=vrank_coord[vrank][mu]*vloc_size[mu];
	    vrank_loclx_offset[vrank]=loclx_of_coord(c);
	  }
	
	//check whether each direction dir is virtually parallelized
	for(int mu=0;mu<NDIM;mu++)
	  {
	    vparal_dir[mu]=(nvranks_per_dir[mu]>1);
	    nvparal_dir+=vparal_dir[mu];
	  }
	
	//fill all indices
	fill(this);
	
	//compute the local virtual size
	for(int mu=0;mu<NDIM;mu++) vloc_size[mu]=loc_size[mu]/nvranks_per_dir[mu];
	
	//create the virtual grid
	for(int i=0;i<nvranks;i++) coord_of_lx(vrank_coord[i],i,nvranks_per_dir);
	
	//print information on virtual local volume
	master_printf("Virtual local volume\t%d",vloc_size[0]);
	for(int mu=1;mu<NDIM;mu++) master_printf("x%d",vloc_size[mu]);
	master_printf(" = %d\n",vloc_vol);
	master_printf("List of virtually parallelized dirs:\t");
	for(int mu=0;mu<NDIM;mu++) if(vparal_dir[mu]) master_printf("%d ",mu);
	if(nvparal_dir==0) master_printf("(none)");
	master_printf("\n");
	
      }
    }
    
    //! unset
    ~vranks_geom_t()
    {
      if(inited)
      {
  	inited=false;
	
	nissa_free(vrank_of_loclx);
	
   	nissa_free(vloc_of_loclx);
   	nissa_free(loclx_of_vloc);
	
   	nissa_free(veos_of_loclx);
	
  	for(int par=0;par<2;par++)
  	  {
	    //viroe_or_vireo_hopping_matrix_output_pos[par].free();
	    
	    nissa_free(loclx_of_veos[par]);
	    
  	    nissa_free(veos_of_loceo[par]);
  	    nissa_free(loceo_of_veos[par]);
	  }
      }
    }
    
  };
  
  void virsome_remap_to_lx(void *out,void *in,int size_per_site,int nel_per_site,int *idx_out);
  template <class T1,class T2,class base_type> void virsome_remap_to_lx(T1 *out,T2 *in,int *idx_out)
  {
    //static
      virsome_remap_to_lx(out,in,sizeof(base_type),sizeof(T2)/sizeof(base_type),idx_out);}
  
  // void lx_conf_remap_to_virlx(vir_oct_su3 *out,quad_su3 *in);
  // void lx_conf_remap_to_virlx_blocked(vir_su3 *out,quad_su3 *in);
  // void virlx_conf_remap_to_lx(quad_su3 *out,vir_oct_su3 *in);
  // void lx_conf_remap_to_vireo(vir_oct_su3 **out,quad_su3 *in);
  // void lx_conf_remap_to_single_vireo(vir_single_oct_su3 **out,quad_su3 *in);
  // void eo_conf_remap_to_vireo(vir_oct_su3 **out,quad_su3 **in);
  // void eo_conf_remap_to_single_vireo(vir_single_oct_su3 **out,quad_su3 **in);
  
  // void lx_quad_su3_remap_to_virlx(vir_quad_su3 *out,quad_su3 *in);
  // void virlx_quad_su3_remap_to_lx(quad_su3 *out,vir_quad_su3 *in);
  // inline void lx_clover_term_t_remap_to_virlx(vir_clover_term_t *out,clover_term_t *in)
  // {lx_quad_su3_remap_to_virlx(out,in);}
  // inline void virlx_clover_term_t_remap_to_lx(clover_term_t *out,vir_clover_term_t *in)
  // {virlx_quad_su3_remap_to_lx(out,in);}
  
  // void evn_or_odd_quad_su3_remap_to_virevn_or_odd(vir_quad_su3 *out,quad_su3 *in,int par);
  // void virevn_or_odd_quad_su3_remap_to_evn_or_odd(quad_su3 *out,vir_quad_su3 *in,int par);
  // inline void evn_or_odd_clover_term_t_remap_to_virevn_or_odd(vir_clover_term_t *out,clover_term_t *in,int par)
  // {evn_or_odd_quad_su3_remap_to_virevn_or_odd(out,in,par);}
  // inline void virevn_or_odd_clover_term_t_remap_to_evn_or_odd(clover_term_t *out,vir_clover_term_t *in,int par)
  // {virevn_or_odd_quad_su3_remap_to_evn_or_odd(out,in,par);}
  
  // void virlx_spincolor_remap_to_lx(spincolor *out,vir_spincolor *in);
  // void virevn_or_odd_spincolor_remap_to_evn_or_odd(spincolor *out,vir_spincolor *in,int par);
  // void virlx_spincolor_128_remap_to_lx(spincolor_128 *out,vir_spincolor_128 *in);
  // void lx_spincolor_remap_to_virlx(vir_spincolor *out,spincolor *in);
  // void lx_spincolor_128_remap_to_virlx(vir_spincolor_128 *out,spincolor_128 *in);
  // void vireo_spincolor_remap_to_lx(spincolor *out,vir_spincolor **in);
  // void evn_or_odd_spincolor_remap_to_virevn_or_odd(vir_spincolor *out,spincolor *in,int par);
  // void virlx_clover_t_term_remap_to_lx(vir_clover_term_t *out,clover_term_t *in);
  
  // void vireo_color_remap_to_lx(color *out,vir_color **in);
  // void virevn_or_odd_color_remap_to_evn_or_odd(color *out,vir_color *in,int par);
  // void virevn_or_odd_single_color_remap_to_evn_or_odd(color *out,vir_single_color *in,int par);
  // void lx_spincolor_remap_to_vireo(vir_spincolor **out,spincolor *in);
  // void lx_color_remap_to_vireo(vir_color **out,color *in);
  // void lx_color_remap_to_single_vireo(vir_single_color **out,color *in);
  // void evn_or_odd_color_remap_to_virevn_or_odd(vir_color *out,color *in,int par);
  // void evn_or_odd_color_remap_to_single_virevn_or_odd(vir_single_color *out,color *in,int par);
  
  // void evn_or_odd_complex_vect_remap_to_virevn_or_odd(vir_complex *out,complex *in,int par,int vl);
  // inline void evn_or_odd_inv_clover_term_t_remap_to_virevn_or_odd(vir_inv_clover_term_t *out,inv_clover_term_t *in,int par)
  // {evn_or_odd_complex_vect_remap_to_virevn_or_odd((vir_complex*)out,(complex*)in,par,sizeof(inv_clover_term_t)/sizeof(complex));}
  
  void set_vranks_geometry();
  void unset_vranks_geometry();
  
  EXTERN_GEOMETRY_VIR vranks_geom_t<double> vlx_double_geom;
}

#undef EXTERN_GEOMETRY_VIR

#endif
