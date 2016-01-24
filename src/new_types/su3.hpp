#ifndef _SU3_HPP
#define _SU3_HPP

#include "complex.hpp"
#include "dirac.hpp"
#include "spin.hpp"
#include "float_128.hpp"

namespace nissa
{
  typedef complex color[NCOL];
  
  typedef complex color2[2];
  typedef color2 su2[2];
  
  typedef color spincolor[4];
  typedef color halfspincolor[2];
  typedef spin colorspin[NCOL];
  
  typedef colorspin spincolorspin[4];
  typedef spincolorspin colorspincolorspin[NCOL];
  
  typedef spinspin colorspinspin[NCOL];
  
  typedef color su3[NCOL];
  typedef su3 quad_su3[NDIM];
  typedef su3 oct_su3[2*NDIM];
  
  typedef colorspinspin su3spinspin[NCOL];
  
  typedef su3 as2t_su3[NDIM*(NDIM+1)/2];
  typedef su3 opt_as2t_su3[4];
  
  typedef single_complex single_color[NCOL];
  typedef single_color single_su3[NCOL];
  typedef single_color single_halfspincolor[2];
  typedef single_color single_spincolor[4];
  typedef single_su3 single_quad_su3[4];
  
  typedef bi_complex bi_color[NCOL];
  typedef bi_color bi_su3[NCOL];
  typedef bi_su3 bi_oct_su3[8];
  typedef bi_color bi_spincolor[4];
  typedef bi_color bi_halfspincolor[2];
  typedef bi_complex bi_halfspin[2];
  typedef bi_su3 bi_opt_as2t_su3[4];
  
  typedef bi_single_complex bi_single_color[NCOL];
  typedef bi_single_color bi_single_su3[NCOL];
  typedef bi_single_color bi_single_halfspincolor[2];
  typedef bi_single_color bi_single_spincolor[4];
  typedef bi_single_su3 bi_single_oct_su3[8];
}

#endif
