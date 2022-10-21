add_executable(ImageViewer
  image_viewer.cpp
)

set_target_properties(ImageViewer PROPERTIES
  OUTPUT_NAME "archon-image-viewer"
)

target_link_libraries(ImageViewer PUBLIC
  Core
  Log
  Cli
  Image
  Display
)
