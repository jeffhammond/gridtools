#include <gridtools.h>
#include <common/halo_descriptor.h>

#ifdef CUDA_EXAMPLE
#include <boundary-conditions/apply_gpu.h>
#else
#include <boundary-conditions/apply.h>
#endif

#include <boundary-conditions/zero.h>
#include <boundary-conditions/value.h>
#include <boundary-conditions/copy.h>

using gridtools::direction;
using gridtools::sign;
using gridtools::minus;
using gridtools::zero;
using gridtools::plus;

#ifdef CUDA_EXAMPLE
#include <stencil-composition/backend_cuda.h>
#else
#include <stencil-composition/backend_naive.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include <boost/utility/enable_if.hpp>

#ifdef CUDA_EXAMPLE
#define BACKEND backend<gridtools::_impl::Cuda, gridtools::_impl::Naive>
#else
#ifdef BACKEND_BLOCK
#define BACKEND backend<, gridtools::_impl::Host, , gridtools::_impl::Block>
#else
#define BACKEND backend<gridtools::_impl::Host, gridtools::_impl::Naive >
#endif
#endif


struct bc_basic {

    // relative coordinates
    template <typename Direction, typename DataField0>
    GT_FUNCTION
    void operator()(Direction,
                    DataField0 & data_field0,
                    int i, int j, int k) const {
        data_field0(i,j,k) = i+j+k;
    }
};

#define SET_TO_ZERO                                     \
    template <typename Direction, typename DataField0>  \
    void operator()(Direction,                          \
                    DataField0 & data_field0,           \
                    int i, int j, int k) const {        \
        data_field0(i,j,k) = 0;                         \
    }


template <sign X>
struct is_minus {
    static const bool value = (X == minus);
};

template <typename T, typename U>
struct is_one_of {
    static const bool value = T::value || U::value;
};

struct bc_two {

    template <typename Direction, typename DataField0>
    GT_FUNCTION
    void operator()(Direction,
                    DataField0 & data_field0,
                    int i, int j, int k) const {
        data_field0(i,j,k) = 0;
    }

    template <sign I, sign J, sign K, typename DataField0>
    GT_FUNCTION
    void operator()(direction<I,J,K>,
                    DataField0 & data_field0,
                    int i, int j, int k,
                    typename boost::enable_if<is_one_of<is_minus<J>, is_minus<K> > >::type *dummy = 0) const {
        data_field0(i,j,k) = (i+j+k+1);
    }

    // THE CODE ABOVE IS A REPLACEMENT OF THE FOLLOWING 4 DIFFERENT SPECIALIZATIONS
    // IT IS UGLY BUT CAN SAVE QUITE A BIT OF CODE

    // template <sign I, sign K, typename DataField0>
    // void operator()(direction<I,minus,K>,
    //                 DataField0 & data_field0,
    //                 int i, int j, int k) const {
    //     data_field0(i,j,k) = i+j+k+1;
    // }

    // template <sign J, sign K, typename DataField0>
    // void operator()(direction<minus,J,K>,
    //                 DataField0 & data_field0,
    //                 int i, int j, int k) const {
    //     data_field0(i,j,k) = i+j+k+1;
    // }

    // template <sign I, typename DataField0>
    // void operator()(direction<I,minus,minus>,
    //                 DataField0 & data_field0,
    //                 int i, int j, int k) const {
    //     data_field0(i,j,k) = i+j+k+1;
    // }

    // template <typename DataField0>
    // void operator()(direction<minus,minus,minus>,
    //                 DataField0 & data_field0,
    //                 int i, int j, int k) const {
    //     data_field0(i,j,k) = i+j+k+1;
    // }
};

struct minus_predicate {
    template <typename Direction>
    bool operator()(Direction) const {
        return true;
    }

    template <sign I, sign J>
    bool operator()(direction<I,J,minus>) const {
        return false;
    }

    template <sign I, sign K>
    bool operator()(direction<I,minus,K>) const {
        return false;
    }

    template <sign J, sign K>
    bool operator()(direction<minus,J,K>) const {
        return false;
    }


    template <sign I>
    bool operator()(direction<I,minus,minus>) const {
        return false;
    }

    template <sign I>
    bool operator()(direction<minus,I,minus>) const {
        return false;
    }

    template <sign I>
    bool operator()(direction<minus,minus,I>) const {
        return false;
    }

    bool operator()(direction<minus,minus,minus>) const {
        return false;
    }

};

bool basic() {

    int d1 = 5;
    int d2 = 5;
    int d3 = 5;

    typedef gridtools::BACKEND::storage_type<int, gridtools::layout_map<0,1,2> >::type storage_type;

    // Definition of the actual data fields that are used for input/output
    storage_type in(d1,d2,d3,-1, std::string("in"));

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                in(i,j,k) = 0;
            }
        }
    }

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", in(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    gridtools::array<gridtools::halo_descriptor, 3> halos;
    halos[0] = gridtools::halo_descriptor(1,1,1,d1-2,d1);
    halos[1] = gridtools::halo_descriptor(1,1,1,d2-2,d2);
    halos[2] = gridtools::halo_descriptor(1,1,1,d3-2,d3);

#ifdef CUDA_EXAMPLE
    in.clone_to_gpu();
    out.clone_to_gpu();
    in.h2d_update();
    out.h2d_update();

    gridtools::boundary_apply_gpu<bc_basic>(halos, bc_basic()).apply(in);

    in.d2h_update();
#else
    gridtools::boundary_apply<bc_basic>(halos, bc_basic()).apply(in);
#endif

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", in(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    bool result = true;

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<1; ++k) {
                if (in(i,j,k) != i+j+k) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=d3-1; k<d3; ++k) {
                if (in(i,j,k) != i+j+k) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<1; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != i+j+k) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=d2-1; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != i+j+k) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != i+j+k) {
                    result = false;
                }
            }
        }
    }

    for (int i=d1-1; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != i+j+k) {
                    result = false;
                }
            }
        }
    }

    for (int i=1; i<d1-1; ++i) {
        for (int j=1; j<d2-1; ++j) {
            for (int k=1; k<d3-1; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    return result;

}

bool predicate() {

    int d1 = 5;
    int d2 = 5;
    int d3 = 5;

    typedef gridtools::BACKEND::storage_type<int, gridtools::layout_map<0,1,2> >::type storage_type;

    // Definition of the actual data fields that are used for input/output
    storage_type in(d1,d2,d3,-1, std::string("in"));


    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                in(i,j,k) = 0;
            }
        }
    }

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", in(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    gridtools::array<gridtools::halo_descriptor, 3> halos;
    halos[0] = gridtools::halo_descriptor(1,1,1,d1-2,d1);
    halos[1] = gridtools::halo_descriptor(1,1,1,d2-2,d2);
    halos[2] = gridtools::halo_descriptor(1,1,1,d3-2,d3);

#ifdef CUDA_EXAMPLE
    in.clone_to_gpu();
    out.clone_to_gpu();
    in.h2d_update();
    out.h2d_update();

    gridtools::boundary_apply_gpu<bc_basic, minus_predicate>(halos, bc_basic(), minus_predicate()).apply(in);

    in.d2h_update();
#else
    gridtools::boundary_apply<bc_basic, minus_predicate>(halos, bc_basic(), minus_predicate()).apply(in);
#endif

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", in(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    bool result = true;

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<1; ++k) {
                if (in(i,j,k) != 0) {
#ifndef NDEBUG
                    printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                    result = false;
                }
            }
        }
    }

    for (int i=1; i<d1; ++i) {
        for (int j=1; j<d2; ++j) {
            for (int k=d3-1; k<d3; ++k) {
                if (in(i,j,k) != i+j+k) {
#ifndef NDEBUG
                    printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<1; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 0) {
#ifndef NDEBUG
                    printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                    result = false;
                }
            }
        }
    }

    for (int i=1; i<d1; ++i) {
        for (int j=d2-1; j<d2; ++j) {
            for (int k=1; k<d3; ++k) {
                if (in(i,j,k) != i+j+k) {
#ifndef NDEBUG
                    printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 0) {
#ifndef NDEBUG
                    printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                    result = false;
                }
            }
        }
    }

    for (int i=d1-1; i<d1; ++i) {
        for (int j=1; j<d2; ++j) {
            for (int k=1; k<d3; ++k) {
                if (in(i,j,k) != i+j+k) {
#ifndef NDEBUG
                    printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                    result = false;
                }
            }
        }
    }

    for (int i=1; i<d1-1; ++i) {
        for (int j=1; j<d2-1; ++j) {
            for (int k=1; k<d3-1; ++k) {
                if (in(i,j,k) != 0) {
#ifndef NDEBUG
                    printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                    result = false;
                }
            }
        }
    }

    return result;

}

bool twosurfaces() {

    int d1 = 5;
    int d2 = 5;
    int d3 = 5;

    typedef gridtools::BACKEND::storage_type<int, gridtools::layout_map<0,1,2> >::type storage_type;

    // Definition of the actual data fields that are used for input/output
    storage_type in(d1,d2,d3,-1, std::string("in"));

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                in(i,j,k) = 1;
            }
        }
    }

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", in(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    gridtools::array<gridtools::halo_descriptor, 3> halos;
    halos[0] = gridtools::halo_descriptor(1,1,1,d1-2,d1);
    halos[1] = gridtools::halo_descriptor(1,1,1,d2-2,d2);
    halos[2] = gridtools::halo_descriptor(1,1,1,d3-2,d3);

#ifdef CUDA_EXAMPLE
    in.clone_to_gpu();
    out.clone_to_gpu();
    in.h2d_update();
    out.h2d_update();

    gridtools::boundary_apply_gpu<bc_two>(halos, bc_two()).apply(in);

    in.d2h_update();
#else
    gridtools::boundary_apply<bc_two>(halos, bc_two()).apply(in);
#endif

#ifndef NDEBUG
        for (int i=0; i<d1; ++i) {
            for (int j=0; j<d2; ++j) {
                for (int k=0; k<d3; ++k) {
                    printf("%d ", in(i,j,k));
                }
                printf("\n");
            }
            printf("\n");
        }
#endif

            bool result = true;

            for (int i=0; i<d1; ++i) {
                for (int j=0; j<d2; ++j) {
                    for (int k=0; k<1; ++k) {
                        if (in(i,j,k) != i+j+k+1) {
                            printf("A %d %d %d %d\n", i,j,k, in(i,j,k));
                            result = false;
                        }
                    }
                }
            }

            for (int i=0; i<d1; ++i) {
                for (int j=1; j<d2; ++j) {
                    for (int k=d3-1; k<d3; ++k) {
                        if (in(i,j,k) != 0) {
#ifndef NDEBUG
                            printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                            result = false;
                        }
                    }
                }
            }

            for (int i=0; i<d1; ++i) {
                for (int j=0; j<1; ++j) {
                    for (int k=0; k<d3; ++k) {
                        if (in(i,j,k) != i+j+k+1) {
#ifndef NDEBUG
                            printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                            result = false;
                        }
                    }
                }
            }

            for (int i=0; i<d1; ++i) {
                for (int j=d2-1; j<d2; ++j) {
                    for (int k=1; k<d3; ++k) {
                        if (in(i,j,k) != 0) {
#ifndef NDEBUG
                            printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                            result = false;
                        }
                    }
                }
            }

            for (int i=0; i<1; ++i) {
                for (int j=1; j<d2; ++j) {
                    for (int k=1; k<d3; ++k) {
                        if (in(i,j,k) != 0) {
#ifndef NDEBUG
                            printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                            result = false;
                        }
                    }
                }
            }

            for (int i=d1-1; i<d1; ++i) {
                for (int j=1; j<d2; ++j) {
                    for (int k=1; k<d3; ++k) {
                        if (in(i,j,k) != 0) {
#ifndef NDEBUG
                            printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                            result = false;
                        }
                    }
                }
            }

            for (int i=1; i<d1-1; ++i) {
                for (int j=1; j<d2-1; ++j) {
                    for (int k=1; k<d3-1; ++k) {
                        if (in(i,j,k) != 1) {
#ifndef NDEBUG
                            printf("%d %d %d %d\n", i,j,k, in(i,j,k));
#endif
                            result = false;
                        }
                    }
                }
            }

            return result;

}

bool usingzero_1() {

    int d1 = 5;
    int d2 = 5;
    int d3 = 5;

    typedef gridtools::BACKEND::storage_type<int, gridtools::layout_map<0,1,2> >::type storage_type;

    // Definition of the actual data fields that are used for input/output
    storage_type in(d1,d2,d3,-1, std::string("in"));

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                in(i,j,k) = -1;
            }
        }
    }

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", in(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    gridtools::array<gridtools::halo_descriptor, 3> halos;
    halos[0] = gridtools::halo_descriptor(1,1,1,d1-2,d1);
    halos[1] = gridtools::halo_descriptor(1,1,1,d2-2,d2);
    halos[2] = gridtools::halo_descriptor(1,1,1,d3-2,d3);

#ifdef CUDA_EXAMPLE
    in.clone_to_gpu();
    out.clone_to_gpu();
    in.h2d_update();
    out.h2d_update();

    gridtools::boundary_apply_gpu<gridtools::zero_boundary>(halos).apply(in);

    in.d2h_update();
#else
    gridtools::boundary_apply<gridtools::zero_boundary>(halos).apply(in);
#endif

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", in(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    bool result = true;

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<1; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=d3-1; k<d3; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<1; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=d2-1; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    for (int i=d1-1; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    for (int i=1; i<d1-1; ++i) {
        for (int j=1; j<d2-1; ++j) {
            for (int k=1; k<d3-1; ++k) {
                if (in(i,j,k) != -1) {
                    result = false;
                }
            }
        }
    }

    return result;

}

bool usingzero_2() {

    int d1 = 5;
    int d2 = 5;
    int d3 = 5;

    typedef gridtools::BACKEND::storage_type<int, gridtools::layout_map<0,1,2> >::type storage_type;

    // Definition of the actual data fields that are used for input/output
    storage_type in(d1,d2,d3,-1, std::string("in"));
    storage_type out(d1,d2,d3,-1, std::string("out"));

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                in(i,j,k) = -1;
                out(i,j,k) = -1;
            }
        }
    }

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", in(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    gridtools::array<gridtools::halo_descriptor, 3> halos;
    halos[0] = gridtools::halo_descriptor(1,1,1,d1-2,d1);
    halos[1] = gridtools::halo_descriptor(1,1,1,d2-2,d2);
    halos[2] = gridtools::halo_descriptor(1,1,1,d3-2,d3);

#ifdef CUDA_EXAMPLE
    in.clone_to_gpu();
    out.clone_to_gpu();
    in.h2d_update();
    out.h2d_update();

    gridtools::boundary_apply_gpu<gridtools::zero_boundary>(halos).apply(in, out);

    in.d2h_update();
#else
    gridtools::boundary_apply<gridtools::zero_boundary>(halos).apply(in, out);
#endif

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", in(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    bool result = true;

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<1; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
                if (out(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=d3-1; k<d3; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
                if (out(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<1; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
                if (out(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=d2-1; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
                if (out(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
                if (out(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    for (int i=d1-1; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 0) {
                    result = false;
                }
                if (out(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    for (int i=1; i<d1-1; ++i) {
        for (int j=1; j<d2-1; ++j) {
            for (int k=1; k<d3-1; ++k) {
                if (in(i,j,k) != -1) {
                    result = false;
                }
                if (out(i,j,k) != -1) {
                    result = false;
                }
            }
        }
    }

    return result;

}


bool usingvalue_2() {

    int d1 = 5;
    int d2 = 5;
    int d3 = 5;

    typedef gridtools::BACKEND::storage_type<int, gridtools::layout_map<0,1,2> >::type storage_type;

    // Definition of the actual data fields that are used for input/output
    storage_type in(d1,d2,d3,-1, std::string("in"));
    storage_type out(d1,d2,d3,-1, std::string("out"));

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                in(i,j,k) = -1;
                out(i,j,k) = -1;
            }
        }
    }

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", in(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    gridtools::array<gridtools::halo_descriptor, 3> halos;
    halos[0] = gridtools::halo_descriptor(1,1,1,d1-2,d1);
    halos[1] = gridtools::halo_descriptor(1,1,1,d2-2,d2);
    halos[2] = gridtools::halo_descriptor(1,1,1,d3-2,d3);

#ifdef CUDA_EXAMPLE
    in.clone_to_gpu();
    out.clone_to_gpu();
    in.h2d_update();
    out.h2d_update();

    gridtools::boundary_apply_gpu<gridtools::value_boundary<int> >(halos, gridtools::value_boundary<int>(101)).apply(in, out);

    in.d2h_update();
#else
    gridtools::boundary_apply<gridtools::value_boundary<int> >(halos, gridtools::value_boundary<int>(101)).apply(in, out);
#endif

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", in(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    bool result = true;

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<1; ++k) {
                if (in(i,j,k) != 101) {
                    result = false;
                }
                if (out(i,j,k) != 101) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=d3-1; k<d3; ++k) {
                if (in(i,j,k) != 101) {
                    result = false;
                }
                if (out(i,j,k) != 101) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<1; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 101) {
                    result = false;
                }
                if (out(i,j,k) != 101) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=d2-1; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 101) {
                    result = false;
                }
                if (out(i,j,k) != 101) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 101) {
                    result = false;
                }
                if (out(i,j,k) != 101) {
                    result = false;
                }
            }
        }
    }

    for (int i=d1-1; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (in(i,j,k) != 101) {
                    result = false;
                }
                if (out(i,j,k) != 101) {
                    result = false;
                }
            }
        }
    }

    for (int i=1; i<d1-1; ++i) {
        for (int j=1; j<d2-1; ++j) {
            for (int k=1; k<d3-1; ++k) {
                if (in(i,j,k) != -1) {
                    result = false;
                }
                if (out(i,j,k) != -1) {
                    result = false;
                }
            }
        }
    }

    return result;

}

bool usingcopy_3() {

    int d1 = 5;
    int d2 = 5;
    int d3 = 5;

    typedef gridtools::BACKEND::storage_type<int, gridtools::layout_map<0,1,2> >::type storage_type;

    // Definition of the actual data fields that are used for input/output
    storage_type src(d1,d2,d3,-1, std::string("src"));
    storage_type one(d1,d2,d3,-1, std::string("one"));
    storage_type two(d1,d2,d3,-1, std::string("two"));

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                src(i,j,k) = i+k+j;
                one(i,j,k) = -1;
                two(i,j,k) = 0;
            }
        }
    }

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", one(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    gridtools::array<gridtools::halo_descriptor, 3> halos;
    halos[0] = gridtools::halo_descriptor(1,1,1,d1-2,d1);
    halos[1] = gridtools::halo_descriptor(1,1,1,d2-2,d2);
    halos[2] = gridtools::halo_descriptor(1,1,1,d3-2,d3);

#ifdef CUDA_EXAMPLE
    in.clone_to_gpu();
    out.clone_to_gpu();
    in.h2d_update();
    out.h2d_update();

    gridtools::boundary_apply_gpu<gridtools::copy_boundary>(halos).apply(one, two, src);

    in.d2h_update();
#else
    gridtools::boundary_apply<gridtools::copy_boundary>(halos).apply(one, two, src);
#endif

#ifndef NDEBUG
    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                printf("%d ", one(i,j,k));
            }
            printf("\n");
        }
        printf("\n");
    }
#endif

    bool result = true;

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<1; ++k) {
                if (one(i,j,k) != i+j+k) {
                    result = false;
                }
                if (two(i,j,k) != i+j+k) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=d3-1; k<d3; ++k) {
                if (one(i,j,k) != i+j+k) {
                    result = false;
                }
                if (two(i,j,k) != i+j+k) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=0; j<1; ++j) {
            for (int k=0; k<d3; ++k) {
                if (one(i,j,k) != i+j+k) {
                    result = false;
                }
                if (two(i,j,k) != i+j+k) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<d1; ++i) {
        for (int j=d2-1; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (one(i,j,k) != i+j+k) {
                    result = false;
                }
                if (two(i,j,k) != i+j+k) {
                    result = false;
                }
            }
        }
    }

    for (int i=0; i<1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (one(i,j,k) != i+j+k) {
                    result = false;
                }
                if (two(i,j,k) != i+j+k) {
                    result = false;
                }
            }
        }
    }

    for (int i=d1-1; i<d1; ++i) {
        for (int j=0; j<d2; ++j) {
            for (int k=0; k<d3; ++k) {
                if (one(i,j,k) != i+j+k) {
                    result = false;
                }
                if (two(i,j,k) != i+j+k) {
                    result = false;
                }
            }
        }
    }

    for (int i=1; i<d1-1; ++i) {
        for (int j=1; j<d2-1; ++j) {
            for (int k=1; k<d3-1; ++k) {
                if (one(i,j,k) != -1) {
                    result = false;
                }
                if (two(i,j,k) != 0) {
                    result = false;
                }
            }
        }
    }

    return result;

}
