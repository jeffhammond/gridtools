#pragma once

#include "intermediate_metafunctions.hpp"
#include "stencil-composition/esf.hpp"
#include "stencil-composition/make_esf.hpp"
#include "stencil-composition/make_stencils.hpp"
#include "stencil-composition/make_computation.hpp"
#include "stencil-composition/stencil.hpp"
#include "stencil-composition/axis.hpp"
#include "stencil-composition/grid.hpp"
#include "stencil-composition/grid_traits.hpp"

#ifndef STRUCTURED_GRIDS
#include "stencil-composition/icosahedral_grids/grid.hpp"
#endif