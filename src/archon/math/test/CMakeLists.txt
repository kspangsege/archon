add_library(MathTest OBJECT
  test_vec.cpp
  test_vec_adapt.cpp
  test_vec_ops.cpp
  test_mat.cpp
  test_mat_ops.cpp
  test_tensor_experiment.cpp
)

target_link_libraries(MathTest
  Core
  Check
  Math
)
