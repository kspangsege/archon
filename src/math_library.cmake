add_library(Math
  archon/math/quaternion.cpp
)

set_target_properties(Math PROPERTIES OUTPUT_NAME "archon-math")

target_link_libraries(Math PUBLIC
  Core
)

target_sources(Math PUBLIC FILE_SET HEADERS FILES
  archon/math/vector_base.hpp
  archon/math/vector.hpp
  archon/math/matrix_base.hpp
  archon/math/matrix.hpp
  archon/math/quaternion.hpp
  archon/math/rotation.hpp
)

install(TARGETS Math FILE_SET HEADERS)

add_subdirectory(archon/math/test)
