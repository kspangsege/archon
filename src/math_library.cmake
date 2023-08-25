add_library(Math INTERFACE)

target_sources(Math PUBLIC FILE_SET HEADERS FILES
  archon/math/type_traits.hpp
  archon/math/vec_fwd.hpp
  archon/math/vec_val.hpp
  archon/math/impl/vec_adapt_rep.hpp
  archon/math/vec_adapt.hpp
  archon/math/impl/vec_rep.hpp
  archon/math/vec_var.hpp
  archon/math/vec_ops.hpp
  archon/math/vec_misc.hpp
  archon/math/vec.hpp
  archon/math/mat_fwd.hpp
  archon/math/mat_val.hpp
  archon/math/mat_adapt_fwd.hpp
  archon/math/impl/mat_adapt_rep.hpp
  archon/math/mat_adapt.hpp
  archon/math/impl/mat_rep.hpp
  archon/math/mat_var.hpp
  archon/math/mat_ops.hpp
  archon/math/mat_misc.hpp
  archon/math/mat.hpp
)

install(TARGETS Math FILE_SET HEADERS)

add_subdirectory(archon/math/test)
