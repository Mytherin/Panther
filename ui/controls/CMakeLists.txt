
add_subdirectory(basic)
add_subdirectory(main)
add_subdirectory(textfield)

add_library(panther_controls OBJECT container.cpp container.h control.cpp control.h controlmanager.cpp controlmanager.h notification.cpp notification.h)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_controls> PARENT_SCOPE)
