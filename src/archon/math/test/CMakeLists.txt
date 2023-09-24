add_library(MathTest OBJECT
  test_vector.cpp
  test_matrix.cpp
  test_quaternion.cpp
  test_rotation.cpp
)

target_link_libraries(MathTest
  Core
  Check
  Math
)
