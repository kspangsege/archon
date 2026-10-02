if(ARCHON_DISPLAY_HAVE_OPENGL)
  add_executable(ObjectViewer
    object_viewer.cpp
  )

  set_target_properties(ObjectViewer PROPERTIES
    OUTPUT_NAME "archon-object-viewer"
  )

  target_link_libraries(ObjectViewer
    PRIVATE Core
    PRIVATE Log
    PRIVATE Cli
    PRIVATE Math
    PRIVATE Util
    PRIVATE Image
    PRIVATE Gfx
    PRIVATE Display
    PRIVATE Render
  )
endif()


if(ARCHON_DISPLAY_HAVE_OPENGL)
  add_executable(Blocks
    blocks.cpp
  )

  set_target_properties(Blocks PROPERTIES
    OUTPUT_NAME "archon-blocks"
  )

  target_link_libraries(Blocks
    PRIVATE Core
    PRIVATE Log
    PRIVATE Cli
    PRIVATE Math
    PRIVATE Image
    PRIVATE Gfx
    PRIVATE Display
    PRIVATE Render
  )
endif()
