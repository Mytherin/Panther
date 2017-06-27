add_library(panther_controls_main OBJECT findtext.cpp findtext.h goto.cpp goto.h projectexplorer.cpp projectexplorer.h searchbox.cpp searchbox.h statusbar.cpp statusbar.h statusnotification.cpp statusnotification.h toolbar.cpp toolbar.h)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_controls_main> PARENT_SCOPE)
