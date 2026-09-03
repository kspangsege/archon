add_library(Math
  archon/math/quaternion.cpp
)

set_target_properties(Math PROPERTIES OUTPUT_NAME "archon-math")


target_sources(Math PUBLIC FILE_SET HEADERS FILES
  archon/math/type_traits.hpp
  archon/math/vector_base.hpp
  archon/math/vector.hpp
  archon/math/matrix_base.hpp
  archon/math/matrix.hpp
  archon/math/quaternion.hpp
  archon/math/rotation.hpp
)


target_link_libraries(Math
  PUBLIC Core
)


install(TARGETS Math FILE_SET HEADERS)


if(ARCHON_INCLUDE_TEST_SUITE)
  add_subdirectory(archon/math/test)
endif()
