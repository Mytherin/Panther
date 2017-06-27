add_library(panther_testing OBJECT replaymanager.cpp replaymanager.h)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_testing> PARENT_SCOPE)
