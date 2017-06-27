add_library(panther_files OBJECT directory.cpp directory.h file.cpp filemanager.cpp filemanager.h mmap.h searchindex.cpp searchindex.h)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:panther_files> PARENT_SCOPE)
