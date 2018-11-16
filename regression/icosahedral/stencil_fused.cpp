/*
  GridTools Libraries

  Copyright (c) 2017, ETH Zurich and MeteoSwiss
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

#include <gtest/gtest.h>

#include <gridtools/common/binops.hpp>
#include <gridtools/stencil-composition/stencil-composition.hpp>
#include <gridtools/tools/regression_fixture.hpp>

#include "neighbours_of.hpp"

using namespace gridtools;

template <uint_t>
struct test_on_edges_functor {
    using in = in_accessor<0, enumtype::edges, extent<0, 1, 0, 1>>;
    using out = inout_accessor<1, enumtype::cells>;
    using arg_list = boost::mpl::vector<in, out>;

    template <typename Evaluation>
    GT_FUNCTION static void Do(Evaluation eval) {
        eval(out{}) = eval(on_edges(binop::sum{}, float_type{}, in{}));
    }
};

template <uint_t>
struct test_on_cells_functor {
    using in = in_accessor<0, enumtype::cells, extent<-1, 1, -1, 1>>;
    using out = inout_accessor<1, enumtype::cells>;
    using arg_list = boost::mpl::vector<in, out>;

    template <typename Evaluation>
    GT_FUNCTION static void Do(Evaluation eval) {
        eval(out{}) = eval(on_cells(binop::sum{}, float_type{}, in{}));
    }
};

using stencil_fused = regression_fixture<2>;

TEST_F(stencil_fused, test) {
    arg<0, edges> p_in;
    arg<1, cells> p_out;
    tmp_arg<0, cells> p_tmp;

    auto in = [](int_t i, int_t c, int_t j, int_t k) { return i + c + j + k; };

    auto tmp = [&](int_t i, int_t c, int_t j, int_t k) {
        float_type res{};
        for (auto &&item : neighbours_of<cells, edges>(i, c, j, k))
            res += item.call(in);
        return res;
    };

    auto ref = [&](int_t i, int_t c, int_t j, int_t k) {
        float_type res{};
        for (auto &&item : neighbours_of<cells, cells>(i, c, j, k))
            res += item.call(tmp);
        return res;
    };

    auto out = make_storage<cells>();

    make_computation(p_in = make_storage<edges>(in),
        p_out = out,
        make_multistage(enumtype::execute<enumtype::forward>(),
            make_stage<test_on_edges_functor, topology_t, cells>(p_in, p_tmp),
            make_stage<test_on_cells_functor, topology_t, cells>(p_tmp, p_out)))
        .run();

    verify(make_storage<cells>(ref), out);
}