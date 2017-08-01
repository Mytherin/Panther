add_library(panther_languages OBJECT c.cpp c.h findresults.cpp findresults.h keywords.cpp keywords.h xml.cpp xml.h syntect.cpp syntect.h)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_languages> PARENT_SCOPE)
