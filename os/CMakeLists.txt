

if (WIN32)
	add_subdirectory(windows)
endif(WIN32)

if (APPLE)
	add_subdirectory(macos)
endif(APPLE)

add_library(panther_os OBJECT windowfunctions.cpp windowfunctions.h)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_os> PARENT_SCOPE)
