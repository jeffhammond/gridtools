/*
GridTools Libraries

Copyright (c) 2016, GridTools Consortium
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For information: http://eth-cscs.github.io/gridtools/
*/
#pragma once
#include "../../common/generic_metafunctions/variadic_to_vector.hpp"
#include "../../common/generic_metafunctions/accumulate.hpp"
/**
   @file metafunctions used in the cache_storage class
*/

namespace gridtools {

    namespace _impl {

        template < typename Layout, typename Plus, typename Minus, typename Tiles, typename Storage >
        struct compute_meta_storage;

        /**
           @class computing the correct storage_info type for the cache storage
           \tparam Layout memory layout of the cache storage
           \tparam Plus the positive extents in all directions
           \tparam Plus the negative extents in all directions

           The extents and block size are used to compute the dimension of the cache storage, which is
           all we need.
         */
        template < typename Layout,
            typename P1,
            typename P2,
            typename... Plus,
            typename M1,
            typename M2,
            typename... Minus,
            typename T1,
            typename T2,
            typename... Tiles,
            typename Storage >
        struct compute_meta_storage< Layout,
            variadic_to_vector< P1, P2, Plus... >,
            variadic_to_vector< M1, M2, Minus... >,
            variadic_to_vector< T1, T2, Tiles... >,
            Storage > {

            typedef meta_storage_cache< Layout,
                P1::value - M1::value + T1::value,
                P2::value - M2::value + T2::value, // first 2 dimensions are special (the block)
                ((Plus::value - Minus::value) > 0 ? (Tiles::value - Minus::value + Plus::value) : 1)...,
                Storage::field_dimensions,
                1 > type;
        };

        template < typename T >
        struct generate_layout_map;

        /**@class automatically generates the layout map for the cache storage. By default
           i and j have the smallest stride. The largest stride is in the field dimension. This reduces bank conflicts.
         */
        template < uint_t... Id >
        struct generate_layout_map< gt_integer_sequence< uint_t, Id... > > {
#ifdef CUDA8
            typedef layout_map< (sizeof...(Id)-Id - 1)... > type;
#else
            typedef typename boost::mpl::if_c<
                sizeof...(Id) == 2,
                layout_map< 1, 0 >,
                typename boost::mpl::if_c< sizeof...(Id) == 3,
                    layout_map< 2, 1, 0 >,
                    typename boost::mpl::if_c< sizeof...(Id) == 4,
                                               layout_map< 3, 2, 1, 0 >,
                                               typename boost::mpl::if_c< sizeof...(Id) == 5,
                                                   layout_map< 4, 3, 2, 1, 0 >,
                                                   boost::mpl::false_ >::type >::type >::type >::type type;
#endif
        };

#ifndef CUDA8
        template < typename Minus, typename Plus, typename Tiles, typename Storage >
        struct compute_size;

        template < typename... Minus, typename... Plus, typename... Tiles, typename Storage >
        struct compute_size< variadic_to_vector< Minus... >,
            variadic_to_vector< Plus... >,
            variadic_to_vector< Tiles... >,
            Storage > {
            static constexpr auto value =
                accumulate(multiplies(), (Plus::value + Tiles::value - Minus::value)...) * Storage::field_dimensions;
        };
#endif

    } // namespace _impl
} // namespace gridtools