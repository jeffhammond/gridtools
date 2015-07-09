#pragma once

#include <gridtools.hpp>

namespace horizontal_diffusion {

#ifdef CUDA_EXAMPLE
typedef gridtools::backend<gridtools::enumtype::Cuda, gridtools::enumtype::Block > hd_backend;
#else
#ifdef BACKEND_BLOCK
typedef gridtools::backend<gridtools::enumtype::Host, gridtools::enumtype::Block > hd_backend;
#else
typedef gridtools::backend<gridtools::enumtype::Host, gridtools::enumtype::Naive > hd_backend;
#endif
#endif

using gridtools::uint_t;
using gridtools::int_t;

class repository
{
public:
#ifdef __CUDACC__
    typedef gridtools::layout_map<2,1,0> layout_ijk;//stride 1 on i
    typedef gridtools::layout_map<1,0,-1> layout_ij;
#else
    typedef gridtools::layout_map<0,1,2> layout_ijk;//stride 1 on k
    typedef gridtools::layout_map<0,1,-1> layout_ij;
#endif

    typedef gridtools::layout_map<-1,-1,-1> layout_scalar;

    typedef hd_backend::storage_type<gridtools::float_type, layout_ijk >::type storage_type;
    typedef hd_backend::storage_type<gridtools::float_type, layout_ij >::type ij_storage_type;

    typedef hd_backend::storage_type<gridtools::float_type, layout_scalar >::type scalar_storage_type;
    typedef hd_backend::temporary_storage_type<gridtools::float_type, layout_ijk >::type tmp_storage_type;
    typedef hd_backend::temporary_storage_type<gridtools::float_type, layout_scalar>::type tmp_scalar_storage_type;

    repository(const uint_t idim, const uint_t jdim, const uint_t kdim, const int halo_size) :
        in_(idim, jdim, kdim, -1., "in"),
        out_(idim, jdim, kdim, -1., "out"),
        out_ref_(idim, jdim, kdim, -1., "in_ref"),
        coeff_(idim, jdim, kdim, -1., "coeff"),
        halo_size_(halo_size),
        idim_(idim), jdim_(jdim), kdim_(kdim)
    {}

    void init_fields()
    {
        const double PI = std::atan(1.)*4.;
        init_field_to_value(out_, 0.0);
        init_field_to_value(out_ref_, 0.0);

        const uint_t i_begin = 0;
        const uint_t i_end=  idim_;
        const uint_t j_begin = 0;
        const uint_t j_end=  jdim_;
        const uint_t k_begin = 0;
        const uint_t k_end=  kdim_;

        double dx = 1./(double)(i_end-i_begin);
        double dy = 1./(double)(j_end-j_begin);
        double dz = 1./(double)(k_end-k_begin);

        for( int j=j_begin; j<j_end; j++ ){
            for( int i=i_begin; i<i_end; i++ ){
                double x = dx*(double)(i-i_begin);
                double y = dy*(double)(j-j_begin);
                for( int k=k_begin; k<k_end; k++ )
                {
                    double z = dz*(double)(k-k_begin);
                    // in values between 5 and 9
                    in_(i,j,k) = 5. + 4*(2.+cos(PI*(x+1.5*y)) + sin(2*PI*(x+1.5*y)))/4.;
                    // coefficient values
                    coeff_(i,j,k) = 3e-6*(-1 + 2*(2.+cos(PI*(x+y)) + cos(PI*y*z))/4.+ 0.05*(0.5-35.5/50.));
                }
            }
        }

    }

    template<typename TStorage_type, typename TValue_type>
    void init_field_to_value(TStorage_type& field, TValue_type value)
    {

        const bool has_dim0 = TStorage_type::basic_type::layout::template at<0>() != -1;
        const bool has_dim1 = TStorage_type::basic_type::layout::template at<1>() != -1;
        const bool has_dim2 = TStorage_type::basic_type::layout::template at<2>() != -1;
        for(int k=0; k < (has_dim2 ? kdim_ : 1); ++k)
        {
            for (int i = 0; i < (has_dim0 ? idim_ : 1); ++i)
            {
                for (int j = 0; j < (has_dim1 ? jdim_ : 1); ++j)
                {
                    field(i,j,k) = value;
                }
            }
        }
    }

    void generate_reference()
    {
        ij_storage_type lap(idim_, jdim_, 1, -1., "lap");
        ij_storage_type flx(idim_, jdim_, 1, -1., "flx");
        ij_storage_type fly(idim_, jdim_, 1, -1., "fly");

        init_field_to_value(lap, 0.0);

        //kbody
        for(int k=0; k < kdim_; ++k)
        {
            for (int i = halo_size_-1; i < idim_-halo_size_+1; ++i)
            {
                for (int j = halo_size_-1; j < jdim_-halo_size_+1; ++j)
                {
                    lap(i,j,0) = (gridtools::float_type)4*in_(i,j,k) - (in_(i+1,j,k) + in_(i,j+1,k) + in_(i-1, j, k) + in_(i,j-1,k));
                }

            }
            for (int i = halo_size_-1; i < idim_-halo_size_; ++i)
            {
                for (int j = halo_size_; j < jdim_-halo_size_; ++j)
                {
                    flx(i,j,0) = lap(i+1,j,0) - lap(i,j,0);
                    if (flx(i,j,0)*(in_(i+1,j,k)-in_(i,j,k)) > 0)
                        flx(i,j,0) = 0.;
                }
            }
            for (int i = halo_size_; i < idim_-halo_size_; ++i)
            {
                for (int j = halo_size_-1; j < jdim_-halo_size_; ++j)
                {
                    fly(i,j,0) = lap(i,j+1,0) - lap(i,j,0);
                    if (fly(i,j,0)*(in_(i,j+1,k)-in_(i,j,k)) > 0)
                        fly(i,j,0) = 0.;
                }
            }
            for (int i = halo_size_; i < idim_-halo_size_; ++i)
            {
                for (int j = halo_size_; j < jdim_-halo_size_; ++j)
                {
                    out_ref_(i,j,k) = in_(i,j,k) - coeff_(i,j,k) *
                            (flx(i,j,0) - flx(i-1,j,0) + fly(i,j,0) - fly(i,j-1,0));
                }
            }
        }
    }
    void update_cpu()
    {
#ifdef CUDA_EXAMPLE
        out_.data().update_cpu();
#endif
    }

    storage_type& in() {return in_;}
    storage_type& out() {return out_;}
    storage_type& out_ref() {return out_ref_;}
    storage_type& coeff() {return coeff_;}

private:
    storage_type in_, out_, out_ref_, coeff_;
    const int halo_size_;
    const int idim_, jdim_, kdim_;
};

}