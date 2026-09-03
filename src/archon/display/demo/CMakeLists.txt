add_executable(DisplaySnooper
  display_snooper.cpp
)

set_target_properties(DisplaySnooper PROPERTIES
  OUTPUT_NAME "archon-display-snooper"
)

target_link_libraries(DisplaySnooper
  PRIVATE Core
  PRIVATE Log
  PRIVATE Cli
  PRIVATE Math
  PRIVATE Util
  PRIVATE Image
  PRIVATE Display
)


add_executable(ImageViewer
  image_viewer.cpp
)

set_target_properties(ImageViewer PROPERTIES
  OUTPUT_NAME "archon-image-viewer"
)

target_link_libraries(ImageViewer
  PRIVATE Core
  PRIVATE Log
  PRIVATE Cli
  PRIVATE Util
  PRIVATE Image
  PRIVATE Display
)
