add_library(panther_utils OBJECT logger.cpp logger.h scheduler.cpp scheduler.h thread.cpp thread.h utils.cpp utils.h)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_utils> PARENT_SCOPE)
