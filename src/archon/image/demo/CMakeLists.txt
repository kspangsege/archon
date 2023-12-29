add_executable(CreateTestImage
  create_test_image.cpp
)

set_target_properties(CreateTestImage PROPERTIES
  OUTPUT_NAME "archon-create-test-image"
)

target_link_libraries(CreateTestImage PUBLIC
  Core
  Log
  Cli
  Image
)


add_executable(ConvertImage
  convert_image.cpp
)

set_target_properties(ConvertImage PROPERTIES
  OUTPUT_NAME "archon-convert-image"
)

target_link_libraries(ConvertImage PUBLIC
  Core
  Log
  Cli
  Image
)


add_executable(RectanglePacker
  rectangle_packer.cpp
)

set_target_properties(RectanglePacker PROPERTIES
  OUTPUT_NAME "archon-rectangle-packer"
)

target_link_libraries(RectanglePacker PUBLIC
  Core
  Log
  Cli
  Util
  Image
)


add_executable(ImageCompose
  image_compose.cpp
)

set_target_properties(ImageCompose PROPERTIES
  OUTPUT_NAME "archon-image-compose"
)

target_link_libraries(ImageCompose PUBLIC
  Core
  Log
  Cli
  Image
)


add_executable(ImageFill
  image_fill.cpp
)

set_target_properties(ImageFill PROPERTIES
  OUTPUT_NAME "archon-image-fill"
)

target_link_libraries(ImageFill PUBLIC
  Core
  Log
  Cli
  Util
  Image
)


add_executable(PerlinNoise
  perlin_noise.cpp
)

set_target_properties(PerlinNoise PROPERTIES
  OUTPUT_NAME "archon-perlin-noise"
)

target_link_libraries(PerlinNoise PUBLIC
  Core
  Log
  Cli
  Util
  Image
)
