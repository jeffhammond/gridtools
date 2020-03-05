/*
 * GridTools
 *
 * Copyright (c) 2014-2019, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * Integration test for testing expressions inside a computation.
 * The test setup is not very nice but it was designed that way to minimize compilation time, i.e. to test everything
 * within one make_computation call.
 */
#include <gtest/gtest.h>

#include <gridtools/stencil_composition/backend/naive.hpp>
#include <gridtools/stencil_composition/cartesian.hpp>
#include <gridtools/stencil_composition/global_parameter.hpp>
#include <gridtools/stencil_composition/positional.hpp>

#define GT_FLOAT_TYPE double
#define GT_STORAGE_X86

#include <cartesian_fixture.hpp>

using namespace gridtools;
using namespace cartesian;

struct test_functor {
    using val2 = in_accessor<0>;
    using val3 = in_accessor<1>;
    using out = inout_accessor<2>;
    using i_pos = in_accessor<3>;
    using param_list = make_param_list<val2, val3, out, i_pos>;

    template <class Eval>
    GT_FUNCTION static void apply(Eval &&eval) {
        using namespace expressions;

        constexpr dimension<1> i;
        constexpr dimension<2> j;
        constexpr dimension<3> k;

        // starts the cascade
        int index = 0;
        if (false)
            assert(false);

#define EXPRESSION_TEST(expr, expected)      \
    else if (eval(i_pos()) == index++) {     \
        eval(out()) = eval(expr) - expected; \
    }

        /*
         * Put expression test here in the form
         * EXPRESSION_TEST( <expr> ) where <expr> is the expression to test.
         * Then put the result below. The order of EXPRESSION_TESTs and EXPRESSION_TEST_RESULTs has to be preserved
         */
        EXPRESSION_TEST(val3() * val2(), 6)
        EXPRESSION_TEST(val3() + val2(), 5)
        EXPRESSION_TEST(val3() - val2(), 1)
        EXPRESSION_TEST(val3() / val2(), 1.5)

        EXPRESSION_TEST(val3(i, j, k) * val2(i, j, k), 6)
        EXPRESSION_TEST(val3(i, j, k) + val2(i, j, k), 5)
        EXPRESSION_TEST(val3(i, j, k) - val2(i, j, k), 1)
        EXPRESSION_TEST(val3(i, j, k) / val2(i, j, k), 1.5)

        EXPRESSION_TEST(val3() * 3., 9)
        EXPRESSION_TEST(3. * val3(), 9)
        EXPRESSION_TEST(val3() * 3, 9) // accessor<double> mult int
        EXPRESSION_TEST(3 * val3(), 9) // int mult accessor<double>

        EXPRESSION_TEST(val3() + 3., 6)
        EXPRESSION_TEST(3. + val3(), 6)
        EXPRESSION_TEST(val3() + 3, 6) // accessor<double> plus int
        EXPRESSION_TEST(3 + val3(), 6) // int plus accessor<double>

        EXPRESSION_TEST(val3() - 2., 1)
        EXPRESSION_TEST(3. - val2(), 1)
        EXPRESSION_TEST(val3() - 2, 1) // accessor<double> minus int
        EXPRESSION_TEST(3 - val2(), 1) // int minus accessor<double>

        EXPRESSION_TEST(val3() / 2., 1.5)
        EXPRESSION_TEST(3. / val2(), 1.5)
        EXPRESSION_TEST(val3() / 2, 1.5) // accessor<double> div int
        EXPRESSION_TEST(3 / val2(), 1.5) // int div accessor<double>

        EXPRESSION_TEST(-val2(), -2)
        EXPRESSION_TEST(+val2(), 2)

        EXPRESSION_TEST(val3() + 2. * val2(), 7)

        EXPRESSION_TEST(pow<2>(val3()), 9)
#undef EXPRESSION_TEST
        else eval(out()) = 0;
    }
};

struct test_expressions : computation_fixture<> {
    test_expressions() : computation_fixture<>(100, 1, 1) {}
};

TEST_F(test_expressions, integration_test) {
    auto out = make_storage();
    run_single_stage(test_functor(),
        naive::backend(),
        make_grid(),
        make_global_parameter<float_type>(2),
        make_global_parameter<float_type>(3),
        out,
        positional<dim::i>());
    verify(0, out);
}