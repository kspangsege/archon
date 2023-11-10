add_executable(DisplaySnooper
  display_snooper.cpp
)

set_target_properties(DisplaySnooper PROPERTIES
  OUTPUT_NAME "archon-display-snooper"
)

target_link_libraries(DisplaySnooper PUBLIC
  Core
  Log
  Cli
  Display
)


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


add_executable(MultipleWindows
  multiple_windows.cpp
)

set_target_properties(MultipleWindows PROPERTIES
  OUTPUT_NAME "archon-multiple-windows"
)

target_link_libraries(MultipleWindows PUBLIC
  Core
  Log
  Cli
  Display
)
