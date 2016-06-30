/*
   Copyright 2016 GridTools Consortium

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#pragma once

#ifdef STRUCTURED_GRIDS
#include "stencil-composition/structured_grids/backend_host/iterate_domain_host.hpp"
#else
#include "stencil-composition/icosahedral_grids/backend_host/iterate_domain_host.hpp"
#endif

#include "../iterate_domain_fwd.hpp"

namespace gridtools {
    template < template < class > class IterateDomainBase, typename IterateDomainArguments >
    struct iterate_domain_backend_id< iterate_domain_host< IterateDomainBase, IterateDomainArguments > > {
        typedef enumtype::enum_type< enumtype::platform, enumtype::Host > type;
    };
}
